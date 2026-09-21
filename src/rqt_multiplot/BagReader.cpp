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

#include <QApplication>
#include <QDebug>
#include <QMutexLocker>

#include <rcl/time.h>
#include <rclcpp/serialized_message.hpp>
#include <rclcpp/time.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/serialized_bag_message.hpp>

#include "rqt_multiplot/ProgressChangeEvent.hpp"

#include "rqt_multiplot/BagOpen.hpp"
#include "rqt_multiplot/BagReader.hpp"

namespace rqt_multiplot {

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
  return impl_.fileName_;
}

QString BagReader::getError() const {
  return impl_.error_;
}

bool BagReader::isReading() const {
  return impl_.isRunning();
}

void BagReader::read(const QString& fileName) {
  impl_.wait();

  impl_.fileName_ = fileName;
  impl_.error_.clear();

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
  if (queries_.isEmpty()) {
    return;
  }

  try {
    rosbag2_cpp::Reader reader;
    openBag(reader, fileName_.toStdString());

    QMap<QString, QString> topicTypes;
    for (const auto& topic : reader.get_all_topics_and_types()) {
      topicTypes.insert(QString::fromStdString(topic.name), QString::fromStdString(topic.type));
    }

    const auto& metadata = reader.get_metadata();
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

      double progress = 1.0;
      if (durationNs > 0) {
        progress = static_cast<double>(bagMessage->recv_timestamp - startNs) / static_cast<double>(durationNs);
      }

      auto* progressChangeEvent = new ProgressChangeEvent(progress);
      QApplication::postEvent(parent(), progressChangeEvent);
    }
  } catch (const std::exception& exception) {
    error_ = QString::fromStdString(exception.what());
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
