/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <ros_babel_fish/babel_fish.hpp>

#include "rqt_multiplot/runtime_types/RuntimeTypeSupportProvider.hpp"

namespace rqt_multiplot {

class RosContext {
 public:
  static void setNode(rclcpp::Node::SharedPtr node);
  static rclcpp::Node::SharedPtr node();
  static ros_babel_fish::BabelFish& fish();
  static runtime_types::RuntimeTypeSupportProvider& typeSupportProvider();

 private:
  static rclcpp::Node::SharedPtr node_;
  static ros_babel_fish::BabelFish::SharedPtr fish_;
  static std::shared_ptr<runtime_types::RuntimeTypeSupportProvider> typeSupportProvider_;
};

}  // namespace rqt_multiplot
