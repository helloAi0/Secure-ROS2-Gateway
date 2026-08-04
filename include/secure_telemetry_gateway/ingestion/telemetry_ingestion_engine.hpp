#ifndef SECURE_TELEMETRY_GATEWAY_INGESTION_TELEMETRY_INGESTION_ENGINE_HPP_
#define SECURE_TELEMETRY_GATEWAY_INGESTION_TELEMETRY_INGESTION_ENGINE_HPP_

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "secure_telemetry_gateway/models/telemetry_data.hpp"
#include "secure_telemetry_gateway/containers/concurrent_queue.hpp"

namespace secure_telemetry_gateway {
namespace ingestion {

/**
 * @brief Telemetry Ingestion Result Codes
 */
enum class IngestionStatus {
  SUCCESS = 0,
  INVALID_JSON_SYNTAX,
  MISSING_REQUIRED_FIELDS,
  TIMESTAMP_OUT_OF_BOUNDS,
  PAYLOAD_EXCEEDS_SIZE_LIMIT,
  QUEUE_FULL
};

/**
 * @brief High-speed telemetry ingestion and payload validation engine.
 */
class TelemetryIngestionEngine {
public:
  TelemetryIngestionEngine() = default;
  ~TelemetryIngestionEngine() = default;

  /**
   * @brief Deserializes and validates a raw JSON telemetry string into a TelemetryData struct.
   * @param raw_json Raw input payload buffer.
   * @param out_data Output telemetry data struct populated on success.
   * @return IngestionStatus Status indicating success or specific validation failure.
   */
  IngestionStatus parse_and_validate(const std::string& raw_json, models::TelemetryData& out_data);

  /**
   * @brief Processes a raw JSON payload and pushes valid entries directly to an MPMC queue.
   * @param raw_json Raw input string payload.
   * @param target_queue Target concurrent queue where valid telemetry items are pushed.
   * @return IngestionStatus Status of the ingestion operation.
   */
  IngestionStatus process_payload(
      const std::string& raw_json, 
      containers::ConcurrentQueue<models::TelemetryData>& target_queue);

private:
  /**
   * @brief Enforces domain-specific numerical and sanity checks on raw telemetry fields.
   */
  bool validate_field_bounds(const models::TelemetryData& data) const;
};

} // namespace ingestion
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_INGESTION_TELEMETRY_INGESTION_ENGINE_HPP_