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

#include <rqt_multiplot/MessageEvent.h>
#include <rqt_multiplot/MessageFieldAccess.h>

#include "rqt_multiplot/BagQuery.h"

namespace rqt_multiplot {

BagQuery::BagQuery(QObject* parent) : QObject(parent) {}

BagQuery::~BagQuery() = default;

bool BagQuery::event(QEvent* event) {
  if (event->type() == MessageEvent::Type) {
    auto* messageEvent = dynamic_cast<MessageEvent*>(event);

    emit messageRead(messageEvent->getTopic(), messageEvent->getMessage());

    return true;
  }

  return QObject::event(event);
}

void BagQuery::callback(const QString& topic, const QString& type, const rclcpp::SerializedMessage& serialized, const rclcpp::Time& time) {
  Message message;
  message.setReceiptTime(time);
  message.setCompound(deserializeMessage(type.toStdString(), serialized));

  auto* messageEvent = new MessageEvent(topic, message);

  QApplication::postEvent(this, messageEvent);
}

void BagQuery::disconnectNotify(const QMetaMethod& /*signal*/) {
  if (receivers(QMetaObject::normalizedSignature(SIGNAL(messageRead(const QString&, const Message&)))) == 0) {
    emit aboutToBeDestroyed();

    deleteLater();
  }
}

}  // namespace rqt_multiplot
