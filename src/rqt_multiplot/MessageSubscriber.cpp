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

#include <string>

#include <QApplication>

#include "rqt_multiplot/MessageEvent.hpp"
#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/RosContext.hpp"

#include "rqt_multiplot/MessageSubscriber.hpp"

namespace {

constexpr int kSubscribeRetryIntervalMs = 1000;

std::string discoverTopicType(rclcpp::Node& node, const std::string& resolvedTopic) {
  const auto topics = node.get_topic_names_and_types();
  const auto it = topics.find(resolvedTopic);
  if (it == topics.end() || it->second.empty()) {
    return {};
  }
  return it->second.front();
}

}  // namespace

namespace rqt_multiplot {

MessageSubscriber::MessageSubscriber(QObject* parent)
    : QObject(parent), queueSize_(100), retryTimer_(new QTimer(this)), hasReportedError_(false) {
  retryTimer_->setSingleShot(true);
  retryTimer_->setInterval(kSubscribeRetryIntervalMs);
  connect(retryTimer_, SIGNAL(timeout()), this, SLOT(retryTimerTimeout()));
}

MessageSubscriber::~MessageSubscriber() {
  unsubscribe();
}

const QString& MessageSubscriber::getTopic() const {
  return topic_;
}

void MessageSubscriber::setTopic(const QString& topic) {
  if (topic != topic_) {
    topic_ = topic;
    hasReportedError_ = false;
    hasReportedDeserializeError_ = false;
    resubscribe();
  }
}

void MessageSubscriber::setMessageType(const QString& type) {
  if (type != messageType_) {
    messageType_ = type;
    hasReportedError_ = false;
    hasReportedDeserializeError_ = false;
    resubscribe();
  }
}

const QString& MessageSubscriber::getMessageType() const {
  return messageType_;
}

void MessageSubscriber::setQueueSize(size_t queueSize) {
  if (queueSize != queueSize_) {
    queueSize_ = queueSize;
    resubscribe();
  }
}

size_t MessageSubscriber::getQueueSize() const {
  return queueSize_;
}

size_t MessageSubscriber::getNumPublishers() const {
  return subscriber_ ? subscriber_->get_publisher_count() : 0;
}

bool MessageSubscriber::isValid() const {
  return static_cast<bool>(subscriber_);
}

bool MessageSubscriber::event(QEvent* event) {
  if (event->type() == MessageEvent::Type) {
    auto* messageEvent = dynamic_cast<MessageEvent*>(event);

    emit messageReceived(messageEvent->getTopic(), messageEvent->getMessage());

    return true;
  }

  return QObject::event(event);
}

void MessageSubscriber::subscribe() {
  auto node = RosContext::node();
  if (!node || topic_.isEmpty()) {
    return;
  }

  try {
    const auto topics = node->get_node_topics_interface();
    const std::string topic = topics->resolve_topic_name(topic_.toStdString());
    const std::string type = messageType_.isEmpty() ? discoverTopicType(*node, topic) : normalizeTypeName(messageType_.toStdString());
    if (!type.empty()) {
      auto typeSupport = RosContext::fish().get_message_type_support(type);
      auto onMessage = [this, typeSupport](const rclcpp::SerializedMessage& serialized) { callback(*typeSupport, serialized); };
      auto subscription = std::make_shared<SerializedSubscription>(node->get_node_base_interface().get(), typeSupport, topic,
                                                                   rclcpp::QoS(queueSize_), onMessage);
      topics->add_subscription(subscription, nullptr);
      subscriber_ = subscription;
    }
  } catch (const std::exception& ex) {
    subscriber_.reset();
    if (!hasReportedError_) {
      hasReportedError_ = true;
      qWarning("MessageSubscriber: cannot subscribe to [%s] with type [%s]: %s. Retrying until a publisher provides the type.",
               qPrintable(topic_), qPrintable(messageType_), ex.what());
    }
    retryTimer_->start();
    return;
  }

  if (subscriber_) {
    emit subscribed(topic_);
  } else {
    retryTimer_->start();
  }
}

void MessageSubscriber::resubscribe() {
  if (subscriber_ || retryTimer_->isActive() || hasReceivers()) {
    unsubscribe();
    subscribe();
  }
}

bool MessageSubscriber::hasReceivers() const {
  return receivers(QMetaObject::normalizedSignature(SIGNAL(messageReceived(const QString&, const Message&)))) > 0;
}

void MessageSubscriber::retryTimerTimeout() {
  if (!subscriber_ && hasReceivers()) {
    subscribe();
  }
}

void MessageSubscriber::unsubscribe() {
  retryTimer_->stop();

  if (subscriber_) {
    subscriber_.reset();

    QApplication::removePostedEvents(this, MessageEvent::Type);

    emit unsubscribed(topic_);
  }
}

void MessageSubscriber::callback(const ros_babel_fish::MessageTypeSupport& typeSupport, const rclcpp::SerializedMessage& serialized) {
  Message message;
  auto node = RosContext::node();
  message.setReceiptTime(node ? node->now() : rclcpp::Clock(RCL_ROS_TIME).now());
  try {
    message.setCompound(deserializeMessage(typeSupport, serialized));
  } catch (const std::exception& ex) {
    if (!hasReportedDeserializeError_.exchange(true)) {
      qWarning("MessageSubscriber: cannot deserialize message on [%s]: %s", qPrintable(topic_), ex.what());
    }
    return;
  }
  message.setSerializedSize(serialized.size());

  auto* messageEvent = new MessageEvent(topic_, message);

  QApplication::postEvent(this, messageEvent);
}

void MessageSubscriber::connectNotify(const QMetaMethod& signal) {
  if (signal == QMetaMethod::fromSignal(&MessageSubscriber::messageReceived) && !subscriber_) {
    subscribe();
  }
}

void MessageSubscriber::disconnectNotify(const QMetaMethod& /*signal*/) {
  if (!hasReceivers()) {
    if (subscriber_) {
      unsubscribe();
    }

    emit aboutToBeDestroyed();

    deleteLater();
  }
}

}  // namespace rqt_multiplot
