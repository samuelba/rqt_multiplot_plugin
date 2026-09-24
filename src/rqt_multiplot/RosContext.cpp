/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/RosContext.hpp"

namespace rqt_multiplot {

rclcpp::Node::SharedPtr RosContext::node_;
ros_babel_fish::BabelFish::SharedPtr RosContext::fish_;
std::shared_ptr<runtime_types::RuntimeTypeSupportProvider> RosContext::typeSupportProvider_;

void RosContext::setNode(rclcpp::Node::SharedPtr node) {
  node_ = std::move(node);
  fish();
}

rclcpp::Node::SharedPtr RosContext::node() {
  return node_;
}

ros_babel_fish::BabelFish& RosContext::fish() {
  if (!fish_) {
    typeSupportProvider();
    fish_ = std::make_shared<ros_babel_fish::BabelFish>(std::vector<ros_babel_fish::TypeSupportProvider::SharedPtr>{typeSupportProvider_});
  }
  return *fish_;
}

runtime_types::RuntimeTypeSupportProvider& RosContext::typeSupportProvider() {
  if (!typeSupportProvider_) {
    typeSupportProvider_ = std::make_shared<runtime_types::RuntimeTypeSupportProvider>([] { return RosContext::node(); });
  }
  return *typeSupportProvider_;
}

}  // namespace rqt_multiplot
