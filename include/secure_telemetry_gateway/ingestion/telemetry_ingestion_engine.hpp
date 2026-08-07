#ifndef SECURE_TELEMETRY_GATEWAY_INGESTION_TELEMETRY_INGESTION_ENGINE_HPP_
#define SECURE_TELEMETRY_GATEWAY_INGESTION_TELEMETRY_INGESTION_ENGINE_HPP_

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "secure_telemetry_gateway/models/telemetry_data.hpp"
#include "secure_telemetry_gateway/containers/concurrent_queue.hpp"

namespace secure_telemetry_gateway {
namespace ingestion {

enum class IngestionStatus {
  SUCCESS = 0,
  INVALID_JSON_SYNTAX,
  MISSING_REQUIRED_FIELDS,
  TIMESTAMP_OUT_OF_BOUNDS,
  PAYLOAD_EXCEEDS_SIZE_LIMIT,
  QUEUE_FULL
};

class TelemetryIngestionEngine {
public:
  TelemetryIngestionEngine() = default;
  ~TelemetryIngestionEngine() = default;

  IngestionStatus parse_and_validate(const std::string& raw_json, models::TelemetryData& out_data);

  IngestionStatus process_payload(
      const std::string& raw_json, 
      containers::ConcurrentQueue<models::TelemetryData>& target_queue);

private:
  bool validate_field_bounds(const models::TelemetryData& data) const;
};

} // namespace ingestion
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_INGESTION_TELEMETRY_INGESTION_ENGINE_HPP_