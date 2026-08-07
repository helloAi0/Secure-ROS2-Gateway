#include <rclcpp/rclcpp.hpp>
#include "secure_telemetry_gateway/ros/gateway_node.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<secure_telemetry_gateway::ros::GatewayNode>();

  RCLCPP_INFO(node->get_logger(), "Starting Secure Telemetry Gateway Standalone Test...");

  // Simulate an incoming encrypted payload structure for testing
  std::string client_id = "amr_robot_01";
  std::vector<uint8_t> dummy_ciphertext = {0x01, 0x02, 0x03, 0x04};
  std::vector<uint8_t> dummy_iv(12, 0x00);
  std::vector<uint8_t> dummy_tag(16, 0x00);

  // Test the 4-argument secure payload processor pipeline
  node->process_incoming_raw_payload(client_id, dummy_ciphertext, dummy_iv, dummy_tag);

  rclcpp::spin_some(node);
  rclcpp::shutdown();
  return 0;
}