/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/BagTopicLoader.hpp"

#include <string>

#include <QMutexLocker>
#include <QSet>

#include <rclcpp/logging.hpp>
#include <rosbag2_cpp/reader.hpp>

#include "rqt_multiplot/BagOpen.hpp"
#include "rqt_multiplot/RosContext.hpp"

namespace rqt_multiplot {

BagTopicLoader::BagTopicLoader(QObject* parent) : QObject(parent), impl_(this) {
  connect(&impl_, &QThread::finished, this, &BagTopicLoader::threadFinished);
}

BagTopicLoader::~BagTopicLoader() {
  impl_.wait();
}

BagTopicLoader::Impl::Impl(QObject* parent) : QThread(parent) {}

BagTopicLoader::Impl::~Impl() {
  wait();
}

QString BagTopicLoader::getFileName() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.fileNames_.value(0);
}

QStringList BagTopicLoader::getFileNames() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.fileNames_;
}

QMap<QString, QString> BagTopicLoader::getTopics() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.topics_;
}

QString BagTopicLoader::getError() const {
  QMutexLocker lock(&impl_.mutex_);
  return impl_.error_;
}

bool BagTopicLoader::isLoading() const {
  return impl_.isRunning();
}

void BagTopicLoader::load(const QString& fileName) {
  load(QStringList{fileName});
}

void BagTopicLoader::load(const QStringList& fileNames) {
  impl_.wait();

  QStringList unique;
  QSet<QString> seen;
  for (const QString& fileName : fileNames) {
    if (fileName.isEmpty() || seen.contains(fileName)) {
      continue;
    }
    seen.insert(fileName);
    unique.append(fileName);
  }

  {
    QMutexLocker lock(&impl_.mutex_);
    impl_.fileNames_ = unique;
  }
  impl_.start();
}

void BagTopicLoader::wait() {
  impl_.wait();
}

void BagTopicLoader::Impl::run() {
  QStringList fileNames;
  {
    QMutexLocker lock(&mutex_);
    topics_.clear();
    error_.clear();
    fileNames = fileNames_;
  }

  QMap<QString, QString> topics;
  QStringList errors;
  for (const QString& fileName : fileNames) {
    try {
      rosbag2_cpp::Reader reader;
      openBag(reader, fileName.toStdString());
      registerMessageDefinitions(reader, RosContext::typeSupportProvider());
      for (const auto& topic : reader.get_all_topics_and_types()) {
        const QString name = QString::fromStdString(topic.name);
        const QString type = QString::fromStdString(topic.type);
        const auto existing = topics.constFind(name);
        if (existing == topics.constEnd()) {
          topics.insert(name, type);
        } else if (existing.value() != type) {
          const std::string topicName = name.toStdString();
          const std::string topicType = type.toStdString();
          const std::string bagPath = fileName.toStdString();
          const std::string keptType = existing.value().toStdString();
          RCLCPP_WARN(rclcpp::get_logger("rqt_multiplot"), "Topic [%s] type [%s] in [%s] differs from [%s]; keeping the first",
                      topicName.c_str(), topicType.c_str(), bagPath.c_str(), keptType.c_str());
        }
      }
    } catch (const std::exception& exception) {
      errors.append(QString("%1: %2").arg(fileName, QString::fromStdString(exception.what())));
    }
  }

  if (!errors.isEmpty() && !topics.isEmpty()) {
    const std::string skipped = errors.join(QStringLiteral("; ")).toStdString();
    RCLCPP_WARN(rclcpp::get_logger("rqt_multiplot"), "Skipped bag topics: %s", skipped.c_str());
  }

  QMutexLocker lock(&mutex_);
  if (!errors.isEmpty() && topics.isEmpty()) {
    error_ = errors.join(QStringLiteral("\n"));
    return;
  }
  topics_ = topics;
}

void BagTopicLoader::threadFinished() {
  const QString error = getError();
  if (error.isEmpty()) {
    emit loadingFinished();
  } else {
    emit loadingFailed(error);
  }
}

}  // namespace rqt_multiplot
