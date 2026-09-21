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
#include "rqt_multiplot/RosContext.hpp"

#include "rqt_multiplot/MessageSubscriber.hpp"

namespace {

// Rolling passes the callback group through SubscriptionOptions. Older releases still take it as its own argument.
template <typename Callback>
auto createTopicSubscription(ros_babel_fish::BabelFish& fish, rclcpp::Node& node, const std::string& topic, const rclcpp::QoS& qos,
                             Callback&& callback, std::chrono::nanoseconds timeout,
                             int /*preferNewApi*/) -> decltype(fish.create_subscription(node, topic, qos, std::forward<Callback>(callback),
                                                                                        rclcpp::SubscriptionOptions{}, timeout)) {
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

}  // namespace

namespace rqt_multiplot {

MessageSubscriber::MessageSubscriber(QObject* parent) : QObject(parent), queueSize_(100) {}

MessageSubscriber::~MessageSubscriber() {
  unsubscribe();
}

const QString& MessageSubscriber::getTopic() const {
  return topic_;
}

void MessageSubscriber::setTopic(const QString& topic) {
  if (topic != topic_) {
    topic_ = topic;

    if (subscriber_) {
      unsubscribe();
      subscribe();
    }
  }
}

void MessageSubscriber::setQueueSize(size_t queueSize) {
  if (queueSize != queueSize_) {
    queueSize_ = queueSize;

    if (subscriber_) {
      unsubscribe();
      subscribe();
    }
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

  subscriber_ = createTopicSubscription(
      RosContext::fish(), *node, topic_.toStdString(), rclcpp::QoS(queueSize_),
      [this](const ros_babel_fish::CompoundMessage& compound) { callback(compound); }, std::chrono::nanoseconds(0));

  if (subscriber_) {
    emit subscribed(topic_);
  }
}

void MessageSubscriber::unsubscribe() {
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
  if (receivers(QMetaObject::normalizedSignature(SIGNAL(messageReceived(const QString&, const Message&)))) == 0) {
    if (subscriber_) {
      unsubscribe();
    }

    emit aboutToBeDestroyed();

    deleteLater();
  }
}

}  // namespace rqt_multiplot
