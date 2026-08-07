#ifndef SECURE_TELEMETRY_GATEWAY_SECURITY_SECURITY_ENGINE_HPP_
#define SECURE_TELEMETRY_GATEWAY_SECURITY_SECURITY_ENGINE_HPP_

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstdint>

namespace secure_telemetry_gateway {
namespace security {

constexpr size_t AES_256_KEY_SIZE = 32; // 256 bits
constexpr size_t GCM_IV_SIZE = 12;      // 96 bits (Standard for GCM)
constexpr size_t GCM_TAG_SIZE = 16;     // 128 bits authentication tag

/**
 * @brief Token Bucket structure tracking rate limits per client/robot.
 */
struct TokenBucket {
  double tokens{0.0};
  std::chrono::steady_clock::time_point last_update{std::chrono::steady_clock::now()};
};

/**
 * @brief Cryptographic and Security Subsystem providing AES-256-GCM encryption and Rate Limiting.
 */
class SecurityEngine {
public:
  SecurityEngine() = default;
  ~SecurityEngine() = default;

  // Prevent copying
  SecurityEngine(const SecurityEngine&) = delete;
  SecurityEngine& operator=(const SecurityEngine&) = delete;

  /**
   * @brief Encrypts plaintext bytes using AES-256-GCM.
   * @param plaintext Raw binary data to encrypt.
   * @param key 256-bit secret key (32 bytes).
   * @param iv 96-bit Initialization Vector / Nonce (12 bytes).
   * @param out_ciphertext Output encrypted ciphertext buffer.
   * @param out_tag Output 128-bit authentication tag buffer.
   * @return true on successful encryption, false on failure.
   */
  bool encrypt_aes_gcm(
      const std::vector<uint8_t>& plaintext,
      const std::vector<uint8_t>& key,
      const std::vector<uint8_t>& iv,
      std::vector<uint8_t>& out_ciphertext,
      std::vector<uint8_t>& out_tag);

  /**
   * @brief Decrypts AES-256-GCM ciphertext and verifies payload authenticity tag.
   * @param ciphertext Encrypted input buffer.
   * @param tag 128-bit authentication tag.
   * @param key 256-bit secret key.
   * @param iv 96-bit Initialization Vector / Nonce.
   * @param out_plaintext Output decrypted plaintext buffer.
   * @return true if decryption AND tag verification succeed, false if tampered or invalid.
   */
  bool decrypt_aes_gcm(
      const std::vector<uint8_t>& ciphertext,
      const std::vector<uint8_t>& tag,
      const std::vector<uint8_t>& key,
      const std::vector<uint8_t>& iv,
      std::vector<uint8_t>& out_plaintext);

  /**
   * @brief Evaluates whether a client/robot has exceeded allowed ingress packet rates.
   * @param client_id Unique identifier for the client or robot.
   * @param max_rate_pps Maximum allowed packets per second.
   * @param burst_capacity Maximum token bucket burst capacity.
   * @return true if packet is allowed, false if rate limit is exceeded.
   */
  bool check_rate_limit(
      const std::string& client_id,
      double max_rate_pps = 500.0,
      double burst_capacity = 1000.0);

  /**
   * @brief Utility: Generates cryptographically secure random bytes for keys or IVs.
   * @param num_bytes Size of random byte sequence to generate.
   * @return std::vector<uint8_t> Random byte array.
   */
  static std::vector<uint8_t> generate_secure_random_bytes(size_t num_bytes);

private:
  mutable std::mutex rate_limit_mutex_;
  std::unordered_map<std::string, TokenBucket> buckets_;
};

} // namespace security
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_SECURITY_SECURITY_ENGINE_HPP_