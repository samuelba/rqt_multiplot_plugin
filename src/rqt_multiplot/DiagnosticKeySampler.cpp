/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/DiagnosticKeySampler.hpp"

#include <exception>
#include <utility>

#include <QMutexLocker>

#include <rclcpp/serialized_message.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_filter.hpp>

#include "rqt_multiplot/BagOpen.hpp"
#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/MessageSubscriberRegistry.hpp"
#include "rqt_multiplot/RosContext.hpp"

namespace rqt_multiplot {

namespace {

QVector<DiagnosticKeyRef> keysOfMessage(const ros_babel_fish::Message& message) {
  QVector<DiagnosticKeyRef> keys;
  for (const DiagnosticStatusKey& entry : diagnosticStatusKeys(message)) {
    keys.append({QString::fromStdString(entry.name), QString::fromStdString(entry.hardwareId), QString::fromStdString(entry.key)});
  }
  return keys;
}

}  // namespace

DiagnosticKeySampler::DiagnosticKeySampler(QObject* parent) : QObject(parent), impl_(this), registry_(new MessageSubscriberRegistry(this)) {
  connect(&impl_, &QThread::finished, this, &DiagnosticKeySampler::threadFinished);
}

DiagnosticKeySampler::~DiagnosticKeySampler() {
  stopLive();
  impl_.wait();
}

DiagnosticKeySampler::Impl::Impl(QObject* parent) : QThread(parent) {}

DiagnosticKeySampler::Impl::~Impl() {
  wait();
}

void DiagnosticKeySampler::sampleLive(const QString& topic) {
  stopLive();
  if (registry_->subscribe(topic, this, SLOT(subscriberMessageReceived(const QString&, const Message&)),
                           MessageSubscriberRegistry::PropertyMap(), Qt::AutoConnection)) {
    liveTopic_ = topic;
  } else {
    emit samplingFailed(tr("Failed to subscribe to %1").arg(topic));
  }
}

bool DiagnosticKeySampler::isSamplingLive() const {
  return !liveTopic_.isEmpty();
}

void DiagnosticKeySampler::sampleBag(const QStringList& fileNames, const QString& topic, const QString& type) {
  impl_.wait();
  {
    QMutexLocker lock(&impl_.mutex_);
    impl_.fileNames_ = fileNames;
    impl_.topic_ = topic;
    impl_.type_ = type;
    impl_.keys_.clear();
    impl_.error_.clear();
  }
  impl_.start();
}

void DiagnosticKeySampler::wait() {
  impl_.wait();
}

const QVector<DiagnosticKeyRef>& DiagnosticKeySampler::getKeys() const {
  return keys_;
}

void DiagnosticKeySampler::stopLive() {
  if (!liveTopic_.isEmpty()) {
    registry_->unsubscribe(liveTopic_, this, nullptr);
    liveTopic_.clear();
  }
}

void DiagnosticKeySampler::mergeKeys(const QVector<DiagnosticKeyRef>& incoming) {
  QVector<DiagnosticKeyRef> merged = mergeDiagnosticKeys(keys_, incoming);
  if (merged.count() != keys_.count()) {
    keys_ = std::move(merged);
    emit keysChanged(keys_);
  }
}

void DiagnosticKeySampler::Impl::run() {
  QStringList fileNames;
  QString topic;
  QString type;
  {
    QMutexLocker lock(&mutex_);
    fileNames = fileNames_;
    topic = topic_;
    type = type_;
  }

  QVector<DiagnosticKeyRef> keys;
  QString error;
  int scanned = 0;
  for (const QString& fileName : fileNames) {
    try {
      rosbag2_cpp::Reader reader;
      openBag(reader, fileName.toStdString());
      registerMessageDefinitions(reader, RosContext::typeSupportProvider());
      rosbag2_storage::StorageFilter filter;
      filter.topics = {topic.toStdString()};
      reader.set_filter(filter);
      while (reader.has_next() && (scanned < kMaxDiagnosticMessagesScanned)) {
        const auto bagMessage = reader.read_next();
        const rclcpp::SerializedMessage serialized(*bagMessage->serialized_data);
        keys = mergeDiagnosticKeys(keys, keysOfMessage(*deserializeMessage(type.toStdString(), serialized)));
        ++scanned;
      }
    } catch (const std::exception& exception) {
      error = QString::fromStdString(exception.what());
    }
  }
  if ((scanned == 0) && error.isEmpty()) {
    error = QStringLiteral("No messages on %1").arg(topic);
  }

  QMutexLocker lock(&mutex_);
  keys_ = keys;
  error_ = (scanned == 0) ? error : QString();
}

void DiagnosticKeySampler::subscriberMessageReceived(const QString& /*topic*/, const Message& message) {
  if (liveTopic_.isEmpty() || message.isEmpty()) {
    return;
  }
  mergeKeys(keysOfMessage(*message.getCompound()));
}

void DiagnosticKeySampler::threadFinished() {
  QVector<DiagnosticKeyRef> keys;
  QString error;
  {
    QMutexLocker lock(&impl_.mutex_);
    keys = impl_.keys_;
    error = impl_.error_;
  }
  if (!error.isEmpty()) {
    emit samplingFailed(error);
    return;
  }
  keys_ = keys;
  emit keysChanged(keys_);
}

}  // namespace rqt_multiplot
