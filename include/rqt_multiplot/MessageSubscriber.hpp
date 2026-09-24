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

#pragma once

#include <QMetaMethod>
#include <QObject>
#include <QString>
#include <QTimer>

#include <ros_babel_fish/detail/babel_fish_subscription.hpp>
#include <ros_babel_fish/messages/compound_message.hpp>

#include "rqt_multiplot/Message.hpp"

namespace rqt_multiplot {

class MessageSubscriber : public QObject {
  Q_OBJECT
 public:
  enum Property { QueueSize, MessageType };

  explicit MessageSubscriber(QObject* parent = nullptr);
  ~MessageSubscriber() override;

  void setTopic(const QString& topic);
  const QString& getTopic() const;
  void setMessageType(const QString& type);
  const QString& getMessageType() const;
  void setQueueSize(size_t queueSize);
  size_t getQueueSize() const;
  size_t getNumPublishers() const;
  bool isValid() const;

  bool event(QEvent* event) override;

 signals:
  void subscribed(const QString& topic);
  void messageReceived(const QString& topic, const Message& message);
  void unsubscribed(const QString& topic);
  void aboutToBeDestroyed();

 private slots:
  void retryTimerTimeout();

 private:
  QString topic_;
  QString messageType_;
  size_t queueSize_;
  QTimer* retryTimer_;
  bool hasReportedError_;

  ros_babel_fish::BabelFishSubscription::SharedPtr subscriber_;

  void subscribe();
  void unsubscribe();
  void resubscribe();
  bool hasReceivers() const;

  void callback(const ros_babel_fish::CompoundMessage& compound);

  void connectNotify(const QMetaMethod& signal) override;
  void disconnectNotify(const QMetaMethod& signal) override;
};

}  // namespace rqt_multiplot
