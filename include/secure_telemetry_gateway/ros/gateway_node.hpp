#ifndef SECURE_TELEMETRY_GATEWAY_ROS_GATEWAY_NODE_HPP_
#define SECURE_TELEMETRY_GATEWAY_ROS_GATEWAY_NODE_HPP_

#include <memory>
#include <thread>
#include <atomic>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <sensor_msgs/msg/temperature.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>

#include "secure_telemetry_gateway/containers/concurrent_queue.hpp"
#include "secure_telemetry_gateway/models/telemetry_data.hpp"
#include "secure_telemetry_gateway/ingestion/telemetry_ingestion_engine.hpp"
#include "secure_telemetry_gateway/security/security_engine.hpp"
#include "secure_telemetry_gateway/storage/storage_engine.hpp"

namespace secure_telemetry_gateway {
namespace ros {

/**
 * @brief Production-grade ROS 2 Node bridging edge telemetry ingestion to DDS topics.
 */
class GatewayNode : public rclcpp::Node {
public:
  explicit GatewayNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
  ~GatewayNode() override;

  /**
   * @brief Ingests raw telemetry string payload from network interface or socket.
   * @param raw_payload Ingested JSON message payload.
   * @return true if successfully parsed, validated, and enqueued.
   */
  bool process_incoming_raw_payload(const std::string& raw_payload);

private:
  void declare_node_parameters();
  void init_subsystems();
  void worker_thread_loop();
  void publish_ros_messages(const models::TelemetryData& data);

  // Core Gateway Subsystems
  ingestion::TelemetryIngestionEngine ingestion_engine_;
  security::SecurityEngine security_engine_;
  std::unique_ptr<storage::StorageEngine> storage_engine_;
  containers::ConcurrentQueue<models::TelemetryData> telemetry_queue_;

  // ROS 2 DDS Publishers
  rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr temp_pub_;
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr twist_pub_;

  // Thread Management
  std::thread worker_thread_;
  std::atomic<bool> running_{false};

  // Encryption Keys
  std::vector<uint8_t> secret_key_;
  std::vector<uint8_t> iv_;
};

} // namespace ros
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_ROS_GATEWAY_NODE_HPP_