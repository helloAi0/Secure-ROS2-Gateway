#include "secure_telemetry_gateway/ingestion/telemetry_ingestion_engine.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"
#include "secure_telemetry_gateway/utils/config_manager.hpp"

namespace secure_telemetry_gateway {
namespace ingestion {

using json = nlohmann::json;

IngestionStatus TelemetryIngestionEngine::parse_and_validate(
    const std::string& raw_json, 
    models::TelemetryData& out_data) {

  // Read maximum payload byte limit from ConfigManager (fallback 4096 bytes)
  const auto max_payload_bytes = utils::ConfigManager::instance().get<size_t>(
      "gateway.telemetry.max_payload_bytes", 4096);

  if (raw_json.size() > max_payload_bytes) {
    LOG_WARN("IngestionEngine: Incoming payload size ({} bytes) exceeds threshold ({} bytes)",
             raw_json.size(), max_payload_bytes);
    return IngestionStatus::PAYLOAD_EXCEEDS_SIZE_LIMIT;
  }

  json parsed_json;
  try {
    parsed_json = json::parse(raw_json);
  } catch (const json::parse_error& ex) {
    LOG_ERROR("IngestionEngine: Failed to parse JSON string: {}", ex.what());
    return IngestionStatus::INVALID_JSON_SYNTAX;
  }

  // Schema validation: Verify key telemetry fields exist in JSON map
  if (!parsed_json.contains("robot_id") || 
      !parsed_json.contains("sensor_type") || 
      !parsed_json.contains("timestamp_ns") ||
      !parsed_json.contains("sequence_id")) {
    LOG_WARN("IngestionEngine: Incoming JSON missing mandatory header fields");
    return IngestionStatus::MISSING_REQUIRED_FIELDS;
  }

  try {
    // Extract fields safely into TelemetryData structure
    out_data.robot_id = parsed_json.at("robot_id").get<std::string>();
    out_data.sensor_type = parsed_json.at("sensor_type").get<std::string>();
    out_data.timestamp_ns = parsed_json.at("timestamp_ns").get<uint64_t>();
    out_data.sequence_id = parsed_json.at("sequence_id").get<uint64_t>();

    // Extract payload dictionary
    if (parsed_json.contains("payload_values") && parsed_json["payload_values"].is_object()) {
      out_data.payload_values.clear();
      for (auto& [key, val] : parsed_json["payload_values"].items()) {
        if (val.is_number()) {
          out_data.payload_values[key] = val.get<double>();
        }
      }
    }

  } catch (const json::exception& ex) {
    LOG_ERROR("IngestionEngine: JSON data type extraction mismatch: {}", ex.what());
    return IngestionStatus::MISSING_REQUIRED_FIELDS;
  }

  // Domain-specific range and sanity checks
  if (!validate_field_bounds(out_data)) {
    return IngestionStatus::TIMESTAMP_OUT_OF_BOUNDS;
  }

  return IngestionStatus::SUCCESS;
}

IngestionStatus TelemetryIngestionEngine::process_payload(
    const std::string& raw_json, 
    containers::ConcurrentQueue<models::TelemetryData>& target_queue) {

  models::TelemetryData data;
  IngestionStatus status = parse_and_validate(raw_json, data);

  if (status != IngestionStatus::SUCCESS) {
    return status;
  }

  // Attempt to push item into concurrent queue
  if (!target_queue.push(std::move(data))) {
    LOG_WARN("IngestionEngine: Target queue reached max capacity, dropped telemetry frame #{}",
             data.sequence_id);
    return IngestionStatus::QUEUE_FULL;
  }

  LOG_TRACE("IngestionEngine: Successfully ingested frame #{} from robot '{}'", 
            data.sequence_id, data.robot_id);
  return IngestionStatus::SUCCESS;
}

bool TelemetryIngestionEngine::validate_field_bounds(const models::TelemetryData& data) const {
  if (data.robot_id.empty() || data.sensor_type.empty()) {
    LOG_WARN("IngestionEngine: Empty robot_id or sensor_type string provided");
    return false;
  }

  // Sanity check: Ensure timestamp is non-zero
  if (data.timestamp_ns == 0) {
    LOG_WARN("IngestionEngine: Invalid timestamp (0 ns) detected for frame #{}", data.sequence_id);
    return false;
  }

  return true;
}

} // namespace ingestion
} // namespace secure_telemetry_gateway