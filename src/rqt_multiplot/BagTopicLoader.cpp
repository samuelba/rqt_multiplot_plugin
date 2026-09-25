/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/BagTopicLoader.hpp"

#include <QMutexLocker>

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
  return impl_.fileName_;
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
  impl_.wait();
  {
    QMutexLocker lock(&impl_.mutex_);
    impl_.fileName_ = fileName;
  }
  impl_.start();
}

void BagTopicLoader::wait() {
  impl_.wait();
}

void BagTopicLoader::Impl::run() {
  QMutexLocker lock(&mutex_);
  topics_.clear();
  error_.clear();

  try {
    rosbag2_cpp::Reader reader;
    openBag(reader, fileName_.toStdString());
    registerMessageDefinitions(reader, RosContext::typeSupportProvider());
    for (const auto& topic : reader.get_all_topics_and_types()) {
      topics_.insert(QString::fromStdString(topic.name), QString::fromStdString(topic.type));
    }
  } catch (const std::exception& exception) {
    topics_.clear();
    error_ = QString::fromStdString(exception.what());
  }
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
