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

#include <chrono>
#include <utility>

#include <QApplication>

#include "rqt_multiplot/MessageEvent.hpp"
#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/RosContext.hpp"

#include "rqt_multiplot/MessageSubscriber.hpp"

namespace {

// Rolling passes the callback group through SubscriptionOptions. Older releases still take it as its own argument.
template <typename Callback>
auto createTopicSubscription(ros_babel_fish::BabelFish& fish, rclcpp::Node& node, const std::string& topic, const rclcpp::QoS& qos,
                             Callback&& callback, std::chrono::nanoseconds timeout, int /*preferNewApi*/)
    -> decltype(fish.create_subscription(node, topic, qos, std::forward<Callback>(callback), rclcpp::SubscriptionOptions{}, timeout)) {
  return fish.create_subscription(node, topic, qos, std::forward<Callback>(callback), rclcpp::SubscriptionOptions{}, timeout);
}

template <typename Callback>
ros_babel_fish::BabelFishSubscription::SharedPtr createTopicSubscription(ros_babel_fish::BabelFish& fish, rclcpp::Node& node,
                                                                         const std::string& topic, const rclcpp::QoS& qos,
                                                                         Callback&& callback, std::chrono::nanoseconds timeout,
                                                                         long /*preferLegacyApi*/) {
  return fish.create_subscription(node, topic, qos, std::forward<Callback>(callback), nullptr, {}, timeout);
}

template <typename Callback>
ros_babel_fish::BabelFishSubscription::SharedPtr createTopicSubscription(ros_babel_fish::BabelFish& fish, rclcpp::Node& node,
                                                                         const std::string& topic, const rclcpp::QoS& qos,
                                                                         Callback&& callback, std::chrono::nanoseconds timeout) {
  return createTopicSubscription(fish, node, topic, qos, std::forward<Callback>(callback), timeout, 0);
}

template <typename Callback>
auto createTypedTopicSubscription(ros_babel_fish::BabelFish& fish, rclcpp::Node& node, const std::string& topic, const std::string& type,
                                  const rclcpp::QoS& qos, Callback&& callback, int /*preferNewApi*/)
    -> decltype(fish.create_subscription(node, topic, type, qos, std::forward<Callback>(callback), rclcpp::SubscriptionOptions{})) {
  return fish.create_subscription(node, topic, type, qos, std::forward<Callback>(callback), rclcpp::SubscriptionOptions{});
}

template <typename Callback>
ros_babel_fish::BabelFishSubscription::SharedPtr createTypedTopicSubscription(ros_babel_fish::BabelFish& fish, rclcpp::Node& node,
                                                                              const std::string& topic, const std::string& type,
                                                                              const rclcpp::QoS& qos, Callback&& callback,
                                                                              long /*preferLegacyApi*/) {
  return fish.create_subscription(node, topic, type, qos, std::forward<Callback>(callback), nullptr, {});
}

template <typename Callback>
ros_babel_fish::BabelFishSubscription::SharedPtr createTypedTopicSubscription(ros_babel_fish::BabelFish& fish, rclcpp::Node& node,
                                                                              const std::string& topic, const std::string& type,
                                                                              const rclcpp::QoS& qos, Callback&& callback) {
  return createTypedTopicSubscription(fish, node, topic, type, qos, std::forward<Callback>(callback), 0);
}

constexpr int kSubscribeRetryIntervalMs = 1000;

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
    resubscribe();
  }
}

void MessageSubscriber::setMessageType(const QString& type) {
  if (type != messageType_) {
    messageType_ = type;
    hasReportedError_ = false;
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

  auto onMessage = [this](const ros_babel_fish::CompoundMessage& compound) { callback(compound); };
  try {
    if (messageType_.isEmpty()) {
      subscriber_ = createTopicSubscription(RosContext::fish(), *node, topic_.toStdString(), rclcpp::QoS(queueSize_), onMessage,
                                            std::chrono::nanoseconds(0));
    } else {
      subscriber_ = createTypedTopicSubscription(RosContext::fish(), *node, topic_.toStdString(),
                                                 normalizeTypeName(messageType_.toStdString()), rclcpp::QoS(queueSize_), onMessage);
    }
  } catch (const std::exception& ex) {
    subscriber_.reset();
    if (!hasReportedError_) {
      hasReportedError_ = true;
      qWarning("MessageSubscriber: cannot subscribe to [%s] with type [%s]: %s. Source the workspace that provides the message type.",
               qPrintable(topic_), qPrintable(messageType_), ex.what());
    }
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

void MessageSubscriber::callback(const ros_babel_fish::CompoundMessage& compound) {
  Message message;
  auto node = RosContext::node();
  message.setReceiptTime(node ? node->now() : rclcpp::Clock(RCL_ROS_TIME).now());
  message.setCompound(ros_babel_fish::CompoundMessage::make_shared(compound.clone()));

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
