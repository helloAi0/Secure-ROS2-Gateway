#include <rclcpp/rclcpp.hpp>
#include "secure_telemetry_gateway/ros/gateway_node.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"

int main(int argc, char** argv) {
  // Initialize ROS 2 C++ client library
  rclcpp::init(argc, argv);

  // Initialize Logger
  secure_telemetry_gateway::utils::Logger::init();
  LOG_INFO("==================================================");
  LOG_INFO(" Starting Secure Telemetry Gateway Node");
  LOG_INFO("==================================================");

  // Instantiate Node
  auto node = std::make_shared<secure_telemetry_gateway::ros::GatewayNode>();

  // Simulate synthetic telemetry payloads arriving from network socket
  std::string mock_battery_payload = R"({
    "robot_id": "robot_alpha",
    "sensor_type": "battery",
    "timestamp_ns": 1700000000000000000,
    "sequence_id": 101,
    "payload": {
      "voltage": 24.8,
      "percentage": 0.88
    }
  })";

  std::string mock_temp_payload = R"({
    "robot_id": "robot_alpha",
    "sensor_type": "temperature",
    "timestamp_ns": 1700000001000000000,
    "sequence_id": 102,
    "payload": {
      "temperature": 42.5
    }
  })";

  // Feed mock frames to gateway pipeline
  node->process_incoming_raw_payload(mock_battery_payload);
  node->process_incoming_raw_payload(mock_temp_payload);

  // Spin node executor
  rclcpp::spin(node);

  // Shutdown ROS 2
  rclcpp::shutdown();
  return 0;
}