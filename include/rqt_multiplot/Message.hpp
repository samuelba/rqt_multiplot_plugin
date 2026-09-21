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

#include <rclcpp/time.hpp>
#include <ros_babel_fish/messages/compound_message.hpp>

namespace rqt_multiplot {

class Message {
 public:
  Message();
  Message(const Message& src);
  ~Message();

  void setReceiptTime(const rclcpp::Time& receiptTime);
  const rclcpp::Time& getReceiptTime() const;
  void setCompound(ros_babel_fish::CompoundMessage::SharedPtr compound);
  ros_babel_fish::CompoundMessage::SharedPtr getCompound() const;
  bool isEmpty() const;

 private:
  rclcpp::Time receiptTime_{0, 0, RCL_ROS_TIME};
  ros_babel_fish::CompoundMessage::SharedPtr compound_;
};

}  // namespace rqt_multiplot
