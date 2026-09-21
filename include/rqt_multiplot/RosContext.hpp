/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <rclcpp/rclcpp.hpp>
#include <ros_babel_fish/babel_fish.hpp>

namespace rqt_multiplot {

class RosContext {
 public:
  static void setNode(rclcpp::Node::SharedPtr node);
  static rclcpp::Node::SharedPtr node();
  static ros_babel_fish::BabelFish& fish();

 private:
  static rclcpp::Node::SharedPtr node_;
  static ros_babel_fish::BabelFish::SharedPtr fish_;
};

}  // namespace rqt_multiplot
