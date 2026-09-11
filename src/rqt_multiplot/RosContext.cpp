/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/RosContext.h"

namespace rqt_multiplot {

rclcpp::Node::SharedPtr RosContext::node_;
ros_babel_fish::BabelFish::SharedPtr RosContext::fish_;

void RosContext::setNode(rclcpp::Node::SharedPtr node) {
  node_ = std::move(node);
  if (!fish_) {
    fish_ = ros_babel_fish::BabelFish::make_shared();
  }
}

rclcpp::Node::SharedPtr RosContext::node() {
  return node_;
}

ros_babel_fish::BabelFish& RosContext::fish() {
  if (!fish_) {
    fish_ = ros_babel_fish::BabelFish::make_shared();
  }
  return *fish_;
}

}  // namespace rqt_multiplot
