/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/TopicSampleLoader.hpp"

#include <QMutexLocker>

#include <rclcpp/serialized_message.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_filter.hpp>

#include "rqt_multiplot/BagOpen.hpp"
#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/MessageSubscriberRegistry.hpp"
#include "rqt_multiplot/RosContext.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"

namespace rqt_multiplot {

TopicSampleLoader::TopicSampleLoader(QObject* parent) : QObject(parent), impl_(this), registry_(new MessageSubscriberRegistry(this)) {
  connect(&impl_, &QThread::finished, this, &TopicSampleLoader::threadFinished);
}

TopicSampleLoader::~TopicSampleLoader() {
  stopLive();
  impl_.wait();
}

TopicSampleLoader::Impl::Impl(QObject* parent) : QThread(parent) {}

TopicSampleLoader::Impl::~Impl() {
  wait();
}

void TopicSampleLoader::sampleLive(const QString& topic) {
  stopLive();
  if (registry_->subscribe(topic, this, SLOT(subscriberMessageReceived(const QString&, const Message&)),
                           MessageSubscriberRegistry::PropertyMap(), Qt::AutoConnection)) {
    liveTopic_ = topic;
  } else {
    emit samplingFailed(tr("Failed to subscribe to %1").arg(topic));
  }
}

void TopicSampleLoader::sampleBag(const QString& fileName, const QString& topic, const QString& type) {
  impl_.wait();
  {
    QMutexLocker lock(&impl_.mutex_);
    impl_.fileName_ = fileName;
    impl_.topic_ = topic;
    impl_.type_ = type;
    impl_.lengths_.clear();
    impl_.error_.clear();
  }
  impl_.start();
}

void TopicSampleLoader::wait() {
  impl_.wait();
}

void TopicSampleLoader::stopLive() {
  if (!liveTopic_.isEmpty()) {
    registry_->unsubscribe(liveTopic_, this, nullptr);
    liveTopic_.clear();
  }
}

void TopicSampleLoader::Impl::run() {
  QString fileName;
  QString topic;
  QString type;
  {
    QMutexLocker lock(&mutex_);
    fileName = fileName_;
    topic = topic_;
    type = type_;
  }

  QHash<QString, int> lengths;
  QString error;
  try {
    rosbag2_cpp::Reader reader;
    openBag(reader, fileName.toStdString());
    registerMessageDefinitions(reader, RosContext::typeSupportProvider());
    rosbag2_storage::StorageFilter filter;
    filter.topics = {topic.toStdString()};
    reader.set_filter(filter);
    if (reader.has_next()) {
      const auto bagMessage = reader.read_next();
      const rclcpp::SerializedMessage serialized(*bagMessage->serialized_data);
      lengths = collectArrayLengths(*deserializeMessage(type.toStdString(), serialized), kMaxArrayElementsShown);
    } else {
      error = QStringLiteral("No messages on %1").arg(topic);
    }
  } catch (const std::exception& exception) {
    error = QString::fromStdString(exception.what());
  }

  QMutexLocker lock(&mutex_);
  lengths_ = lengths;
  error_ = error;
}

void TopicSampleLoader::subscriberMessageReceived(const QString& /*topic*/, const Message& message) {
  if (liveTopic_.isEmpty()) {
    return;
  }
  stopLive();

  if (message.isEmpty()) {
    emit samplingFailed(tr("Received an empty message"));
    return;
  }
  emit sampled(collectArrayLengths(*message.getCompound(), kMaxArrayElementsShown));
}

void TopicSampleLoader::threadFinished() {
  QHash<QString, int> lengths;
  QString error;
  {
    QMutexLocker lock(&impl_.mutex_);
    lengths = impl_.lengths_;
    error = impl_.error_;
  }
  if (error.isEmpty()) {
    emit sampled(lengths);
  } else {
    emit samplingFailed(error);
  }
}

}  // namespace rqt_multiplot
