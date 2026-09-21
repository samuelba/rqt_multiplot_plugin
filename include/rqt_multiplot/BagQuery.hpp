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

#include <rclcpp/serialized_message.hpp>
#include <rclcpp/time.hpp>

#include "rqt_multiplot/Message.hpp"

namespace rqt_multiplot {

class BagQuery : public QObject {
  Q_OBJECT
 public:
  friend class BagReader;

  explicit BagQuery(QObject* parent = nullptr);
  ~BagQuery() override;

  bool event(QEvent* event) override;

 signals:
  void messageRead(const QString& topic, const Message& message);
  void aboutToBeDestroyed();

 private:
  void callback(const QString& topic, const QString& type, const rclcpp::SerializedMessage& serialized, const rclcpp::Time& time);

  void disconnectNotify(const QMetaMethod& signal) override;
};

}  // namespace rqt_multiplot
