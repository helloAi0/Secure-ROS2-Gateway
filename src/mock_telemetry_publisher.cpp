#include <rclcpp/rclcpp.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>
#include <string>

#include "secure_telemetry_gateway/security/security_engine.hpp"

using namespace std::chrono_literals;

class MockTelemetryPublisher : public rclcpp::Node {
public:
  MockTelemetryPublisher() : Node("mock_telemetry_publisher"), counter_(0) {
    RCLCPP_INFO(this->get_logger(), "Starting Mock Telemetry Publisher...");

    timer_ = this->create_wall_timer(
        500ms, std::bind(&MockTelemetryPublisher::publish_telemetry_sample, this));
  }

private:
  void publish_telemetry_sample() {
    counter_++;

    // 1. Construct JSON telemetry payload
    nlohmann::json payload;
    payload["client_id"] = "amr_robot_01";
    payload["sequence_id"] = counter_;
    payload["timestamp_ns"] = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Alternating simulated sensor types
    if (counter_ % 3 == 0) {
      payload["battery"] = 98.5 - (counter_ * 0.1);
    } else if (counter_ % 3 == 1) {
      payload["temperature"] = 42.3 + (counter_ * 0.05);
    } else {
      payload["velocity_x"] = 0.75;
      payload["velocity_y"] = 0.00;
    }

    std::string json_str = payload.dump();
    std::vector<uint8_t> plaintext(json_str.begin(), json_str.end());

    // 2. Encryption Keys
    static const std::vector<uint8_t> PRE_SHARED_KEY = {
      0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
      0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
      0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
      0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
    };

    // Instantiate SecurityEngine
    secure_telemetry_gateway::security::SecurityEngine security_engine;

    std::vector<uint8_t> iv = security_engine.generate_secure_random_bytes(12);
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;

    // 3. Encrypt payload
    if (security_engine.encrypt_aes_gcm(
            plaintext, PRE_SHARED_KEY, iv, ciphertext, tag)) 
    {
      RCLCPP_INFO(this->get_logger(), 
                  "Successfully generated encrypted telemetry packet #%lu (size: %zu bytes)", 
                  counter_, ciphertext.size());
    } else {
      RCLCPP_ERROR(this->get_logger(), "Failed to encrypt mock payload!");
    }
  }

  rclcpp::TimerBase::SharedPtr timer_;
  uint64_t counter_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MockTelemetryPublisher>());
  rclcpp::shutdown();
  return 0;
}