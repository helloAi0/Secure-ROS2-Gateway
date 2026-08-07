#include "secure_telemetry_gateway/ros/gateway_node.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"

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

  // Initialize SQLite3 Storage Engine
  storage_engine_ = std::make_unique<storage::StorageEngine>(db_path);
  if (!storage_engine_->init()) {
    LOG_ERROR("GatewayNode: Failed to initialize Storage Engine at path: {}", db_path);
  }

  // Create DDS Publishers using SensorDataQoS (Best Effort, Volatile)
  auto qos = rclcpp::SensorDataQoS();

  battery_pub_ = this->create_publisher<sensor_msgs::msg::BatteryState>("~/telemetry/battery", qos);
  temp_pub_ = this->create_publisher<sensor_msgs::msg::Temperature>("~/telemetry/temperature", qos);
  twist_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("~/telemetry/velocity", qos);

  // Generate 256-bit symmetric encryption key and 96-bit IV
  secret_key_ = security::SecurityEngine::generate_secure_random_bytes(security::AES_256_KEY_SIZE);
  iv_ = security::SecurityEngine::generate_secure_random_bytes(security::GCM_IV_SIZE);
}

bool GatewayNode::process_incoming_raw_payload(const std::string& raw_payload) {
  // 1. Ingest and parse raw JSON using parse_and_validate
  models::TelemetryData data;
  auto status = ingestion_engine_.parse_and_validate(raw_payload, data);
  if (status != ingestion::IngestionStatus::SUCCESS) {
    LOG_WARN("GatewayNode: Dropping invalid JSON telemetry frame.");
    return false;
  }

  // 2. Rate limiting check
  double max_pps = this->get_parameter("max_rate_pps").as_double();
  double burst = this->get_parameter("burst_capacity").as_double();

  if (!security_engine_.check_rate_limit(data.robot_id, max_pps, burst)) {
    LOG_WARN("GatewayNode: Packet rate limit exceeded for robot '{}'. Packet dropped.", data.robot_id);
    return false;
  }

  // 3. Optional AES-256-GCM encryption verification demo
  if (this->get_parameter("enable_encryption").as_bool()) {
    std::vector<uint8_t> plaintext(raw_payload.begin(), raw_payload.end());
    std::vector<uint8_t> ciphertext, tag, decrypted;

    if (security_engine_.encrypt_aes_gcm(plaintext, secret_key_, iv_, ciphertext, tag)) {
      if (!security_engine_.decrypt_aes_gcm(ciphertext, tag, secret_key_, iv_, decrypted)) {
        LOG_ERROR("GatewayNode: Security verification failed for payload.");
        return false;
      }
    }
  }

  // 4. Push frame into bounded MPMC queue
  if (!telemetry_queue_.push(data)) {
    LOG_WARN("GatewayNode: MPMC queue full. Telemetry frame discarded.");
    return false;
  }

  return true;
}

void GatewayNode::worker_thread_loop() {
  LOG_INFO("Gateway worker thread running...");

  while (running_ && rclcpp::ok()) {
    auto data_opt = telemetry_queue_.pop();

    if (data_opt.has_value()) {
      const auto& data = data_opt.value();

      // 1. Persist to local SQLite storage
      if (storage_engine_) {
        storage_engine_->insert_telemetry(data);
      }

      // 2. Publish frame to ROS 2 DDS Network
      publish_ros_messages(data);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  LOG_INFO("Gateway worker thread exiting.");
}

void GatewayNode::publish_ros_messages(const models::TelemetryData& data) {
  auto now = this->now();

  // Convert and publish Battery Telemetry
  if (data.sensor_type == "battery") {
    auto battery_msg = sensor_msgs::msg::BatteryState();
    battery_msg.header.stamp = now;
    battery_msg.header.frame_id = data.robot_id;

    if (data.payload_values.count("voltage")) {
      battery_msg.voltage = static_cast<float>(data.payload_values.at("voltage"));
    }
    if (data.payload_values.count("percentage")) {
      battery_msg.percentage = static_cast<float>(data.payload_values.at("percentage"));
    }

    battery_pub_->publish(battery_msg);
  }
  // Convert and publish Temperature Telemetry
  else if (data.sensor_type == "temperature") {
    auto temp_msg = sensor_msgs::msg::Temperature();
    temp_msg.header.stamp = now;
    temp_msg.header.frame_id = data.robot_id;

    if (data.payload_values.count("temperature")) {
      temp_msg.temperature = data.payload_values.at("temperature");
    }

    temp_pub_->publish(temp_msg);
  }
  // Convert and publish Velocity Commands / Telemetry
  else if (data.sensor_type == "imu" || data.sensor_type == "velocity") {
    auto twist_msg = geometry_msgs::msg::TwistStamped();
    twist_msg.header.stamp = now;
    twist_msg.header.frame_id = data.robot_id;

    if (data.payload_values.count("linear_v")) {
      twist_msg.twist.linear.x = data.payload_values.at("linear_v");
    }
    if (data.payload_values.count("angular_v")) {
      twist_msg.twist.angular.z = data.payload_values.at("angular_v");
    }

    twist_pub_->publish(twist_msg);
  }
}

} // namespace ros
} // namespace secure_telemetry_gateway