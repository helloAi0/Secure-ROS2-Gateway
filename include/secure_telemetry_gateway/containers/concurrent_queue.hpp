#ifndef SECURE_TELEMETRY_GATEWAY_CONTAINERS_CONCURRENT_QUEUE_HPP_
#define SECURE_TELEMETRY_GATEWAY_CONTAINERS_CONCURRENT_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility>

namespace secure_telemetry_gateway {
namespace containers {

/**
 * @brief Thread-safe Bounded Multi-Producer Multi-Consumer (MPMC) Queue.
 */
template <typename T>
class ConcurrentQueue {
public:
  explicit ConcurrentQueue(size_t max_capacity = 1000)
      : max_capacity_(max_capacity) {}

  ~ConcurrentQueue() = default;

  ConcurrentQueue(const ConcurrentQueue&) = delete;
  ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

  /**
   * @brief Non-blocking push. Returns false if queue has reached max capacity.
   */
  bool push(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() >= max_capacity_) {
      return false;
    }
    queue_.push(std::move(item));
    cv_pop_.notify_one();
    return true;
  }

  /**
   * @brief Non-blocking pop into an output reference.
   */
  bool pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    item = std::move(queue_.front());
    queue_.pop();
    return true;
  }

  /**
   * @brief Non-blocking pop returning std::optional.
   */
  std::optional<T> pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      return std::nullopt;
    }
    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

private:
  std::queue<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_pop_;
  size_t max_capacity_;
};

} // namespace containers
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_CONTAINERS_CONCURRENT_QUEUE_HPP_