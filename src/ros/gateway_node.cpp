#include "secure_telemetry_gateway/ros/gateway_node.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"
#include <nlohmann/json.hpp>

namespace secure_telemetry_gateway {
namespace ros {

GatewayNode::GatewayNode(const rclcpp::NodeOptions& options)
    : Node("secure_telemetry_gateway_node", options),
      telemetry_queue_(1000) {

  LOG_INFO("Initializing Secure Telemetry Gateway ROS 2 Node...");

  declare_node_parameters();
  init_subsystems();

  running_ = true;
  worker_thread_ = std::thread(&GatewayNode::worker_thread_loop, this);

  LOG_INFO("Gateway ROS 2 Node initialized and worker thread started.");
}

GatewayNode::~GatewayNode() {
  LOG_INFO("Shutting down Gateway ROS 2 Node...");
  running_ = false;

  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }

  if (storage_engine_) {
    storage_engine_->close();
  }

  LOG_INFO("Gateway ROS 2 Node shutdown complete.");
}

void GatewayNode::declare_node_parameters() {
  this->declare_parameter<std::string>("db_path", "data/telemetry_history.db");
  this->declare_parameter<double>("max_rate_pps", 500.0);
  this->declare_parameter<double>("burst_capacity", 1000.0);
  this->declare_parameter<bool>("enable_encryption", true);
}

void GatewayNode::init_subsystems() {
  std::string db_path = this->get_parameter("db_path").as_string();

  storage_engine_ = std::make_unique<storage::StorageEngine>(db_path);
  if (!storage_engine_->init()) {
    LOG_ERROR("GatewayNode: Failed to initialize Storage Engine at path: {}", db_path);
  }

  auto qos = rclcpp::SensorDataQoS();

  // Subscriber to receive encrypted data from the Mock Publisher
  raw_sub_ = this->create_subscription<std_msgs::msg::String>(
      "~/telemetry_raw", 10,
      std::bind(&GatewayNode::raw_telemetry_callback, this, std::placeholders::_1));

  battery_pub_ = this->create_publisher<sensor_msgs::msg::BatteryState>("~/telemetry/battery", qos);
  temp_pub_ = this->create_publisher<sensor_msgs::msg::Temperature>("~/telemetry/temperature", qos);
  twist_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("~/telemetry/velocity", qos);
}

void GatewayNode::raw_telemetry_callback(const std_msgs::msg::String::SharedPtr msg) {
  try {
    auto j = nlohmann::json::parse(msg->data);
    std::string sender_id = j["client_id"];
    std::vector<uint8_t> payload = j["ciphertext"];
    std::vector<uint8_t> iv = j["iv"];
    std::vector<uint8_t> tag = j["tag"];
    
    process_incoming_raw_payload(sender_id, payload, iv, tag);
  } catch (const std::exception& e) {
    LOG_ERROR("GatewayNode: Failed to parse incoming raw telemetry JSON: {}", e.what());
  }
}

bool GatewayNode::process_incoming_raw_payload(
    const std::string& sender_id,
    const std::vector<uint8_t>& encrypted_payload,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& tag) 
{
  if (!security_engine_.check_rate_limit(sender_id, 10.0, 20.0)) {
    LOG_WARN("GatewayNode: Rate limit exceeded for '{}'. Dropping packet.", sender_id);
    return false;
  }

  static const std::vector<uint8_t> PRE_SHARED_KEY = {
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
  };

  std::vector<uint8_t> plaintext_bytes;
  if (!security_engine_.decrypt_aes_gcm(encrypted_payload, tag, PRE_SHARED_KEY, iv, plaintext_bytes)) {
    LOG_ERROR("GatewayNode: Decryption failed for sender '{}'", sender_id);
    return false;
  }

  std::string decrypted_json(plaintext_bytes.begin(), plaintext_bytes.end());
  models::TelemetryData telemetry_data;
  ingestion::IngestionStatus status = ingestion_engine_.parse_and_validate(decrypted_json, telemetry_data);

  if (status != ingestion::IngestionStatus::SUCCESS) {
    LOG_ERROR("GatewayNode: Failed to parse decrypted payload. Code: {}", static_cast<int>(status));
    return false;
  }

  telemetry_queue_.push(telemetry_data);
  return true;
}

void GatewayNode::worker_thread_loop() {
  LOG_INFO("Gateway worker thread running...");

  while (running_ && rclcpp::ok()) {
    auto data_opt = telemetry_queue_.pop();

    if (data_opt.has_value()) {
      const auto& data = data_opt.value();

      if (storage_engine_) {
        storage_engine_->insert_telemetry(data);
      }

      publish_ros_messages(data);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  LOG_INFO("Gateway worker thread exiting.");
}

void GatewayNode::publish_ros_messages(const models::TelemetryData& data) {
  auto now = this->now();

  if (data.sensor_type == "battery") {
    auto battery_msg = sensor_msgs::msg::BatteryState();
    battery_msg.header.stamp = now;
    battery_msg.header.frame_id = data.robot_id;

    if (data.payload_values.count("battery")) {
      battery_msg.percentage = static_cast<float>(data.payload_values.at("battery"));
    }
    battery_pub_->publish(battery_msg);
  }
  else if (data.sensor_type == "temperature") {
    auto temp_msg = sensor_msgs::msg::Temperature();
    temp_msg.header.stamp = now;
    temp_msg.header.frame_id = data.robot_id;

    if (data.payload_values.count("temperature")) {
      temp_msg.temperature = data.payload_values.at("temperature");
    }
    temp_pub_->publish(temp_msg);
  }
  else if (data.sensor_type == "velocity") {
    auto twist_msg = geometry_msgs::msg::TwistStamped();
    twist_msg.header.stamp = now;
    twist_msg.header.frame_id = data.robot_id;

    if (data.payload_values.count("velocity_x")) {
      twist_msg.twist.linear.x = data.payload_values.at("velocity_x");
    }
    twist_pub_->publish(twist_msg);
  }
}

} // namespace ros
} // namespace secure_telemetry_gateway