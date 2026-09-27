/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <QApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QSet>

#include <rcl/time.h>
#include <rclcpp/serialized_message.hpp>
#include <rclcpp/time.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/serialized_bag_message.hpp>
#include <rosbag2_storage/storage_filter.hpp>

#include "rqt_multiplot/ProgressChangeEvent.hpp"

#include "rqt_multiplot/BagOpen.hpp"
#include "rqt_multiplot/BagReader.hpp"
#include "rqt_multiplot/RosContext.hpp"

namespace rqt_multiplot {

namespace {

constexpr double kProgressStep = 0.01;

struct OrderedBag {
  QString path;
  int64_t durationNs = 0;
};

QStringList uniqueBagPaths(const QStringList& paths) {
  QStringList unique;
  QSet<QString> seen;
  for (const QString& path : paths) {
    if (path.isEmpty() || seen.contains(path)) {
      continue;
    }
    seen.insert(path);
    unique.append(path);
  }
  return unique;
}

std::vector<OrderedBag> orderBagsByStartTime(const QStringList& paths) {
  struct Probe {
    QString path;
    bool hasStart = false;
    int64_t startNs = 0;
    int64_t durationNs = 0;
  };

  std::vector<Probe> probes;
  probes.reserve(static_cast<size_t>(paths.size()));
  for (const QString& path : paths) {
    Probe probe;
    probe.path = path;
    try {
      rosbag2_cpp::Reader reader;
      openBag(reader, path.toStdString());
      const auto metadata = reader.get_metadata();
      probe.hasStart = true;
      probe.startNs = metadata.starting_time.time_since_epoch().count();
      probe.durationNs = std::max<int64_t>(metadata.duration.count(), 0);
    } catch (const std::exception&) {
    }
    probes.push_back(probe);
  }

  std::vector<size_t> sortable;
  for (size_t index = 0; index < probes.size(); ++index) {
    if (probes[index].hasStart) {
      sortable.push_back(index);
    }
  }
  std::sort(sortable.begin(), sortable.end(), [&probes](size_t left, size_t right) {
    if (probes[left].startNs != probes[right].startNs) {
      return probes[left].startNs < probes[right].startNs;
    }
    return left < right;
  });

  std::vector<Probe> ordered(probes.size());
  size_t nextSortable = 0;
  for (size_t index = 0; index < probes.size(); ++index) {
    if (probes[index].hasStart) {
      ordered[index] = probes[sortable[nextSortable]];
      ++nextSortable;
    } else {
      ordered[index] = probes[index];
    }
  }

  std::vector<OrderedBag> bags;
  bags.reserve(ordered.size());
  for (const Probe& probe : ordered) {
    bags.push_back(OrderedBag{probe.path, probe.durationNs});
  }
  return bags;
}

void postProgress(QObject* target, double progress, double& lastPosted, bool force) {
  progress = std::clamp(progress, 0.0, 1.0);
  if (!force && (progress - lastPosted) < kProgressStep) {
    return;
  }
  lastPosted = progress;
  QApplication::postEvent(target, new ProgressChangeEvent(progress));
}

double overallProgress(int64_t finishedDurationNs, int64_t totalDurationNs, int completedFiles, int fileCount, double fraction,
                       int64_t fileDurationNs) {
  if (totalDurationNs > 0) {
    const double elapsed = static_cast<double>(finishedDurationNs) + (fraction * static_cast<double>(fileDurationNs));
    return elapsed / static_cast<double>(totalDurationNs);
  }
  if (fileCount > 0) {
    return (static_cast<double>(completedFiles) + fraction) / static_cast<double>(fileCount);
  }
  return 1.0;
}

}  // namespace

BagReader::BagReader(QObject* parent) : MessageBroker(parent), impl_(this) {
  connect(&impl_, SIGNAL(started()), this, SLOT(threadStarted()));
  connect(&impl_, SIGNAL(finished()), this, SLOT(threadFinished()));
}

BagReader::~BagReader() {
  impl_.quit();
  impl_.wait();
}

BagReader::Impl::Impl(QObject* parent) : QThread(parent) {}

BagReader::Impl::~Impl() {
  terminate();
  wait();
}

QString BagReader::getFileName() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.fileName_;
}

QStringList BagReader::getFileNames() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.fileNames_;
}

int BagReader::getFileCount() const {
  QMutexLocker lock(&impl_.mutex_);
  return static_cast<int>(impl_.fileNames_.size());
}

int BagReader::getCurrentFileNumber() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.currentFileNumber_;
}

QString BagReader::readingLabel() const {
  QMutexLocker lock(&impl_.mutex_);
  if (impl_.fileNames_.size() > 1) {
    const int number = std::max(impl_.currentFileNumber_, 1);
    return QString("Reading %1 of %2 bags...").arg(number).arg(impl_.fileNames_.size());
  }
  return QString("Reading bag from [file://%1]...").arg(impl_.fileName_);
}

QString BagReader::getError() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.error_;
}

bool BagReader::isReading() const {
  return impl_.isRunning();
}

void BagReader::read(const QStringList& fileNames) {
  impl_.wait();

  const QStringList unique = uniqueBagPaths(fileNames);
  if (unique.isEmpty()) {
    return;
  }

  {
    QMutexLocker lock(&impl_.mutex_);
    impl_.fileNames_ = unique;
    impl_.fileName_ = unique.first();
    impl_.currentFileNumber_ = 1;
    impl_.error_.clear();
  }

  impl_.start();
}

void BagReader::wait() {
  impl_.wait();
}

bool BagReader::subscribe(const QString& topic, QObject* receiver, const char* method, const PropertyMap& /*properties*/,
                          Qt::ConnectionType type) {
  QMutexLocker lock(&impl_.mutex_);

  QMap<QString, BagQuery*>::iterator it = impl_.queries_.find(topic);

  if (it == impl_.queries_.end()) {
    it = impl_.queries_.insert(topic, new BagQuery(this));

    connect(it.value(), SIGNAL(aboutToBeDestroyed()), this, SLOT(queryAboutToBeDestroyed()));
  }

  return receiver->connect(it.value(), SIGNAL(messageRead(const QString&, const Message&)), method, type) != nullptr;
}

bool BagReader::unsubscribe(const QString& topic, QObject* receiver, const char* method) {
  QMutexLocker lock(&impl_.mutex_);

  QMap<QString, BagQuery*>::iterator it = impl_.queries_.find(topic);

  if (it != impl_.queries_.end()) {
    return it.value()->disconnect(SIGNAL(messageRead(const QString&, const Message&)), receiver, method);
  }
  return false;
}

bool BagReader::event(QEvent* event) {
  if (event->type() == ProgressChangeEvent::Type) {
    auto* progressChangeEvent = dynamic_cast<ProgressChangeEvent*>(event);

    emit readingProgressChanged(progressChangeEvent->getProgress());

    return true;
  }

  return QObject::event(event);
}

void BagReader::Impl::run() {
  std::vector<std::string> topics;
  QStringList paths;
  {
    QMutexLocker lock(&mutex_);
    if (queries_.isEmpty()) {
      return;
    }
    for (auto it = queries_.cbegin(); it != queries_.cend(); ++it) {
      topics.push_back(it.key().toStdString());
    }
    paths = fileNames_;
  }

  const std::vector<OrderedBag> bags = orderBagsByStartTime(paths);
  {
    QMutexLocker lock(&mutex_);
    fileNames_.clear();
    for (const OrderedBag& bag : bags) {
      fileNames_.append(bag.path);
    }
  }

  int64_t totalDurationNs = 0;
  for (const OrderedBag& bag : bags) {
    totalDurationNs += bag.durationNs;
  }

  QStringList errors;
  int64_t finishedDurationNs = 0;
  int completedFiles = 0;
  double lastPostedProgress = -1.0;
  const int fileCount = static_cast<int>(bags.size());

  for (int index = 0; index < fileCount; ++index) {
    const OrderedBag& bag = bags[static_cast<size_t>(index)];
    {
      QMutexLocker lock(&mutex_);
      fileName_ = bag.path;
      currentFileNumber_ = index + 1;
    }

    postProgress(parent(), overallProgress(finishedDurationNs, totalDurationNs, completedFiles, fileCount, 0.0, bag.durationNs),
                 lastPostedProgress, true);

    try {
      rosbag2_cpp::Reader reader;
      openBag(reader, bag.path.toStdString());
      registerMessageDefinitions(reader, RosContext::typeSupportProvider());

      rosbag2_storage::StorageFilter filter;
      filter.topics = topics;
      reader.set_filter(filter);

      QMap<QString, QString> topicTypes;
      for (const auto& topic : reader.get_all_topics_and_types()) {
        topicTypes.insert(QString::fromStdString(topic.name), QString::fromStdString(topic.type));
      }

      const auto metadata = reader.get_metadata();
      const auto startNs = metadata.starting_time.time_since_epoch().count();
      const auto durationNs = metadata.duration.count();

      while (reader.has_next()) {
        auto bagMessage = reader.read_next();

        {
          QMutexLocker lock(&mutex_);
          QMap<QString, BagQuery*>::const_iterator it = queries_.find(QString::fromStdString(bagMessage->topic_name));
          if (it != queries_.end()) {
            rclcpp::SerializedMessage serialized(*bagMessage->serialized_data);
            const auto typeIt = topicTypes.find(QString::fromStdString(bagMessage->topic_name));
            if (typeIt != topicTypes.end()) {
              it.value()->callback(QString::fromStdString(bagMessage->topic_name), typeIt.value(), serialized,
                                   rclcpp::Time(bagMessage->recv_timestamp, RCL_ROS_TIME));
            }
          }
        }

        double fraction = 1.0;
        if (durationNs > 0) {
          fraction = std::clamp(static_cast<double>(bagMessage->recv_timestamp - startNs) / static_cast<double>(durationNs), 0.0, 1.0);
        }
        const bool isLastMessage = !reader.has_next();
        postProgress(parent(), overallProgress(finishedDurationNs, totalDurationNs, completedFiles, fileCount, fraction, bag.durationNs),
                     lastPostedProgress, isLastMessage);
      }
    } catch (const std::exception& exception) {
      errors.append(QString("%1: %2").arg(bag.path, QString::fromStdString(exception.what())));
    }

    finishedDurationNs += bag.durationNs;
    ++completedFiles;
  }

  postProgress(parent(), 1.0, lastPostedProgress, true);

  if (!errors.isEmpty()) {
    QMutexLocker lock(&mutex_);
    error_ = errors.join(QStringLiteral("\n"));
  }
}

void BagReader::threadStarted() {
  emit readingStarted();
}

void BagReader::threadFinished() {
  if (impl_.error_.isEmpty()) {
    qInfo() << "Read bag from [file://" << impl_.fileName_ << "]";

    emit readingFinished();
  } else {
    qWarning() << "Failed to read bag from [file://" << impl_.fileName_ << "]:" << impl_.error_;

    emit readingFailed(impl_.error_);
  }
}

void BagReader::queryAboutToBeDestroyed() {
  for (QMap<QString, BagQuery*>::iterator it = impl_.queries_.begin(); it != impl_.queries_.end(); ++it) {
    if (it.value() == dynamic_cast<BagQuery*>(sender())) {
      impl_.queries_.erase(it);
      break;
    }
  }
}

}  // namespace rqt_multiplot
