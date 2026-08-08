#include <rclcpp/rclcpp.hpp>
#include "secure_telemetry_gateway/ros/gateway_node.hpp"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<secure_telemetry_gateway::ros::GatewayNode>();
  
  RCLCPP_INFO(node->get_logger(), "Secure Telemetry Gateway Node is now running and listening...");
  
  // spin() keeps the node alive indefinitely to process incoming callbacks
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}