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

#include "rqt_multiplot/Message.hpp"

namespace rqt_multiplot {

Message::Message() = default;

Message::Message(const Message& src) = default;

Message::~Message() = default;

void Message::setReceiptTime(const rclcpp::Time& receiptTime) {
  receiptTime_ = receiptTime;
}

const rclcpp::Time& Message::getReceiptTime() const {
  return receiptTime_;
}

void Message::setCompound(ros_babel_fish::CompoundMessage::SharedPtr compound) {
  compound_ = std::move(compound);
}

ros_babel_fish::CompoundMessage::SharedPtr Message::getCompound() const {
  return compound_;
}

bool Message::isEmpty() const {
  return !compound_ || !compound_->isValid();
}

}  // namespace rqt_multiplot
