#include "secure_telemetry_gateway/security/security_engine.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <algorithm>

namespace secure_telemetry_gateway {
namespace security {

bool SecurityEngine::encrypt_aes_gcm(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    std::vector<uint8_t>& out_ciphertext,
    std::vector<uint8_t>& out_tag) {

  if (key.size() != AES_256_KEY_SIZE || iv.size() != GCM_IV_SIZE) {
    LOG_ERROR("SecurityEngine: Invalid Key size ({} B) or IV size ({} B) for AES-256-GCM",
              key.size(), iv.size());
    return false;
  }

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    LOG_ERROR("SecurityEngine: Failed to allocate OpenSSL EVP_CIPHER_CTX");
    return false;
  }

  bool success = false;
  int len = 0;
  int ciphertext_len = 0;

  out_ciphertext.resize(plaintext.size());
  out_tag.resize(GCM_TAG_SIZE);

  do {
    // 1. Initialize cipher context with AES-256-GCM
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) break;

    // 2. Set IV length
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1) break;

    // 3. Initialize Key and IV
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) break;

    // 4. Perform encryption pass
    if (EVP_EncryptUpdate(ctx, out_ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) break;
    ciphertext_len = len;

    // 5. Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, out_ciphertext.data() + len, &len) != 1) break;
    ciphertext_len += len;

    // 6. Extract 128-bit authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(GCM_TAG_SIZE), out_tag.data()) != 1) break;

    out_ciphertext.resize(static_cast<size_t>(ciphertext_len));
    success = true;

  } while (false);

  if (!success) {
    LOG_ERROR("SecurityEngine: OpenSSL AES-256-GCM encryption failed");
  }

  EVP_CIPHER_CTX_free(ctx);
  return success;
}

bool SecurityEngine::decrypt_aes_gcm(
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& tag,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    std::vector<uint8_t>& out_plaintext) {

  if (key.size() != AES_256_KEY_SIZE || iv.size() != GCM_IV_SIZE || tag.size() != GCM_TAG_SIZE) {
    LOG_ERROR("SecurityEngine: Decryption parameter size error (Key, IV, or Tag)");
    return false;
  }

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    LOG_ERROR("SecurityEngine: Failed to allocate OpenSSL EVP_CIPHER_CTX");
    return false;
  }

  bool success = false;
  int len = 0;
  int plaintext_len = 0;

  out_plaintext.resize(ciphertext.size());

  do {
    // 1. Initialize cipher context
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) break;

    // 2. Set IV length
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1) break;

    // 3. Initialize Key and IV
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) break;

    // 4. Perform decryption pass
    if (EVP_DecryptUpdate(ctx, out_plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) break;
    plaintext_len = len;

    // 5. Set expected tag value for verification
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), const_cast<uint8_t*>(tag.data())) != 1) break;

    // 6. Finalize decryption AND verify tag authenticity
    int ret = EVP_DecryptFinal_ex(ctx, out_plaintext.data() + len, &len);
    if (ret > 0) {
      plaintext_len += len;
      out_plaintext.resize(static_cast<size_t>(plaintext_len));
      success = true;
    } else {
      LOG_WARN("SecurityEngine: Decryption tag validation failed! Payload tampered or corrupted.");
    }

  } while (false);

  EVP_CIPHER_CTX_free(ctx);
  return success;
}

bool SecurityEngine::check_rate_limit(
    const std::string& client_id,
    double max_rate_pps,
    double burst_capacity) {

  std::lock_guard<std::mutex> lock(rate_limit_mutex_);

  auto now = std::chrono::steady_clock::now();

  // If this is a new client, initialize bucket with full burst capacity
  auto it = buckets_.find(client_id);
  if (it == buckets_.end()) {
    TokenBucket new_bucket;
    new_bucket.tokens = burst_capacity;
    new_bucket.last_update = now;
    buckets_[client_id] = new_bucket;
  }

  auto& bucket = buckets_[client_id];

  // Calculate elapsed time in seconds
  std::chrono::duration<double> elapsed = now - bucket.last_update;
  bucket.last_update = now;

  // Add new tokens accrued since last update
  bucket.tokens += elapsed.count() * max_rate_pps;

  // Cap tokens at burst capacity limit
  if (bucket.tokens > burst_capacity) {
    bucket.tokens = burst_capacity;
  }

  // Consume 1.0 token if available
  if (bucket.tokens >= 1.0) {
    bucket.tokens -= 1.0;
    return true; // Packet permitted
  }

  LOG_WARN("SecurityEngine: Ingress rate limit exceeded for client '{}'", client_id);
  return false; // Rate limit exceeded, packet dropped
}

std::vector<uint8_t> SecurityEngine::generate_secure_random_bytes(size_t num_bytes) {
  std::vector<uint8_t> buffer(num_bytes);
  if (RAND_bytes(buffer.data(), static_cast<int>(num_bytes)) != 1) {
    LOG_ERROR("SecurityEngine: Failed to generate cryptographically secure random bytes");
  }
  return buffer;
}

} // namespace security
} // namespace secure_telemetry_gateway