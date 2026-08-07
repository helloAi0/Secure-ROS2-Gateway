#include "secure_telemetry_gateway/ingestion/telemetry_ingestion_engine.hpp"
#include <chrono>

namespace secure_telemetry_gateway {
namespace ingestion {

IngestionStatus TelemetryIngestionEngine::parse_and_validate(
    const std::string& raw_json, 
    models::TelemetryData& out_data) 
{
  try {
    auto parsed = nlohmann::json::parse(raw_json);

    // Support both client_id and robot_id key formats
    if (parsed.contains("client_id")) {
      out_data.robot_id = parsed["client_id"].get<std::string>();
    } else if (parsed.contains("robot_id")) {
      out_data.robot_id = parsed["robot_id"].get<std::string>();
    } else {
      out_data.robot_id = "unknown_robot";
    }

    // Set sequence_id if available
    if (parsed.contains("sequence_id")) {
      out_data.sequence_id = parsed["sequence_id"].get<uint64_t>();
    } else {
      out_data.sequence_id = 0;
    }

    // Set timestamp
    if (parsed.contains("timestamp_ns")) {
      out_data.timestamp_ns = parsed["timestamp_ns"].get<uint64_t>();
    } else {
      out_data.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // Extract numerical payload values
    out_data.payload_values.clear();
    for (auto& [key, value] : parsed.items()) {
      if (value.is_number()) {
        out_data.payload_values[key] = value.get<double>();
      }
    }

    // Identify sensor category
    if (parsed.contains("battery")) {
      out_data.sensor_type = "battery";
    } else if (parsed.contains("temperature")) {
      out_data.sensor_type = "temperature";
    } else if (parsed.contains("velocity_x")) {
      out_data.sensor_type = "velocity";
    } else {
      out_data.sensor_type = "generic";
    }

    return IngestionStatus::SUCCESS;
  } catch (const nlohmann::json::parse_error&) {
    return IngestionStatus::INVALID_JSON_SYNTAX;
  }
}

IngestionStatus TelemetryIngestionEngine::process_payload(
    const std::string& raw_json, 
    containers::ConcurrentQueue<models::TelemetryData>& target_queue) 
{
  models::TelemetryData data;
  IngestionStatus status = parse_and_validate(raw_json, data);
  if (status == IngestionStatus::SUCCESS) {
    if (!target_queue.push(data)) {
      return IngestionStatus::QUEUE_FULL;
    }
  }
  return status;
}

bool TelemetryIngestionEngine::validate_field_bounds(const models::TelemetryData& /*data*/) const {
  return true;
}

} // namespace ingestion
} // namespace secure_telemetry_gateway