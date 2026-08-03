#ifndef SECURE_TELEMETRY_GATEWAY_TELEMETRY_DATA_HPP_
#define SECURE_TELEMETRY_GATEWAY_TELEMETRY_DATA_HPP_

#include <cstdint>
#include <string>
#include <chrono>
#include <array>
#include <optional>

namespace secure_telemetry_gateway {
namespace telemetry {

/**
 * @brief Represents the high-level operational state of the autonomous robot.
 * Explicitly backed by uint8_t to guarantee 1-byte memory footprint.
 */
enum class RobotState : uint8_t {
  UNINITIALIZED = 0,
  DISARMED      = 1,
  STANDBY       = 2,
  AUTONOMOUS    = 3,
  MANUAL_REMOTE = 4,
  EMERGENCY_STOP = 5,
  FAULT         = 6
};

/**
 * @brief Converts RobotState enum to string representation for logging/debugging.
 */
inline std::string to_string(RobotState state) {
  switch (state) {
    case RobotState::UNINITIALIZED:  return "UNINITIALIZED";
    case RobotState::DISARMED:       return "DISARMED";
    case RobotState::STANDBY:        return "STANDBY";
    case RobotState::AUTONOMOUS:     return "AUTONOMOUS";
    case RobotState::MANUAL_REMOTE:  return "MANUAL_REMOTE";
    case RobotState::EMERGENCY_STOP: return "EMERGENCY_STOP";
    case RobotState::FAULT:          return "FAULT";
    default:                         return "UNKNOWN";
  }
}

/**
 * @brief Sensor diagnostics and health metrics collected from onboard compute units.
 * Struct members are ordered from largest byte size to smallest to minimize memory padding.
 */
struct HardwareMetrics {
  double cpu_utilization_pct{0.0};   // 8 bytes (0.0 to 100.0)
  double ram_utilization_pct{0.0};   // 8 bytes (0.0 to 100.0)
  double core_temperature_c{0.0};    // 8 bytes (Celsius)
  double battery_voltage_v{0.0};     // 8 bytes (Volts)
  float battery_percentage{0.0f};    // 4 bytes (0.0f to 100.0f)
  uint8_t active_fault_codes{0};     // 1 byte bitmask of fault flags
};

/**
 * @brief Kinematic pose and velocity state vectors in 3D Space (ISO 8855 / ROS REP 103).
 */
struct KinematicData {
  std::array<double, 3> position_xyz{0.0, 0.0, 0.0};     // [x, y, z] in meters
  std::array<double, 3> linear_velocity{0.0, 0.0, 0.0};   // [vx, vy, vz] in m/s
  std::array<double, 3> angular_velocity{0.0, 0.0, 0.0};  // [wx, wy, wz] in rad/s
  std::array<double, 4> orientation_q{1.0, 0.0, 0.0, 0.0}; // Quaternion [w, x, y, z]
};

/**
 * @brief Complete Unified Telemetry Frame.
 * Designed for zero dynamic allocations in critical path.
 */
struct TelemetryPacket {
  uint64_t sequence_number{0};       // 8 bytes: Monotonically increasing sequence ID
  uint64_t timestamp_us{0};          // 8 bytes: Microseconds since Unix Epoch
  
  std::string robot_id;              // Dynamic string identifier (e.g. "amr_fleet_04")
  RobotState current_state{RobotState::UNINITIALIZED}; // 1 byte
  
  HardwareMetrics hardware;          // Hardware health data
  KinematicData kinematics;          // Motion state vector
  
  std::optional<std::string> status_message; // Optional fault or diagnostic alert string

  /**
   * @brief Helper to generate current microsecond Unix timestamp.
   */
  static uint64_t now_microseconds() noexcept {
    using namespace std::chrono;
    return static_cast<uint64_t>(
      duration_cast<microseconds>(system_clock::now().time_since_epoch()).count()
    );
  }

  /**
   * @brief Helper to check if packet contains an active emergency or fault state.
   */
  [[nodiscard]] bool is_critical() const noexcept {
    return current_state == RobotState::EMERGENCY_STOP ||
           current_state == RobotState::FAULT ||
           hardware.battery_percentage < 10.0f;
  }
};

} // namespace telemetry
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_TELEMETRY_DATA_HPP_