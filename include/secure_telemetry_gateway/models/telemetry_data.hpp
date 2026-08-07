#ifndef SECURE_TELEMETRY_GATEWAY_MODELS_TELEMETRY_DATA_HPP_
#define SECURE_TELEMETRY_GATEWAY_MODELS_TELEMETRY_DATA_HPP_

#include <string>
#include <unordered_map>
#include <cstdint>

namespace secure_telemetry_gateway {
namespace models {

struct TelemetryData {
  std::string robot_id;
  std::string sensor_type;
  uint64_t timestamp_ns{0};
  uint64_t sequence_id{0}; // <-- Restored missing field
  std::unordered_map<std::string, double> payload_values;
};

} // namespace models
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_MODELS_TELEMETRY_DATA_HPP_