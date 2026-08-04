#ifndef SECURE_TELEMETRY_GATEWAY_UTILS_CONCURRENT_QUEUE_HPP_
#define SECURE_TELEMETRY_GATEWAY_UTILS_CONCURRENT_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <cstddef>
#include <utility>

namespace secure_telemetry_gateway {
namespace utils {

/**
 * @brief Thread-safe, bounded, multi-producer multi-consumer (MPMC) concurrent queue.
 * @tparam T Data type stored in the queue.
 */
template <typename T>
class ConcurrentQueue {
public:
  /**
   * @brief Construct a new Concurrent Queue object.
   * @param max_capacity Maximum number of items allowed in the queue before backpressure occurs.
   */
  explicit ConcurrentQueue(size_t max_capacity = 1000)
      : max_capacity_(max_capacity), shutdown_requested_(false) {}

  ~ConcurrentQueue() {
    shutdown();
  }

  // Prevent copying to ensure lock ownership integrity
  ConcurrentQueue(const ConcurrentQueue&) = delete;
  ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

  // Move operations are also disabled to prevent synchronization corruption
  ConcurrentQueue(ConcurrentQueue&&) = delete;
  ConcurrentQueue& operator=(ConcurrentQueue&&) = delete;

  /**
   * @brief Pushes an item into the queue. Blocks if the queue is full.
   * @param item The item to be pushed.
   * @return true if item was successfully pushed, false if queue is shutting down.
   */
  bool push(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Block until queue has capacity or shutdown is requested
    cv_not_full_.wait(lock, [this]() {
      return queue_.size() < max_capacity_ || shutdown_requested_;
    });

    if (shutdown_requested_) {
      return false;
    }

    queue_.push(std::move(item));
    
    // Unlock before notifying to reduce thread context switching overhead
    lock.unlock();
    cv_not_empty_.notify_one();
    return true;
  }

  /**
   * @brief Pops an item from the queue. Blocks until an item is available.
   * @return std::optional<T> containing item if popped, or std::nullopt if shutting down.
   */
  std::optional<T> pop() {
    std::unique_lock<std::mutex> lock(mutex_);

    // Block until queue has data or shutdown is requested
    cv_not_empty_.wait(lock, [this]() {
      return !queue_.empty() || shutdown_requested_;
    });

    if (queue_.empty() && shutdown_requested_) {
      return std::nullopt;
    }

    T item = std::move(queue_.front());
    queue_.pop();

    lock.unlock();
    cv_not_full_.notify_one();
    return item;
  }

  /**
   * @brief Attempts to pop an item within a specified timeout duration.
   * @tparam Rep Duration representation type.
   * @tparam Period Duration ratio period type.
   * @param timeout_duration Time interval to wait before giving up.
   * @return std::optional<T> containing item if popped, or std::nullopt on timeout/shutdown.
   */
  template <typename Rep, typename Period>
  std::optional<T> pop_for(const std::chrono::duration<Rep, Period>& timeout_duration) {
    std::unique_lock<std::mutex> lock(mutex_);

    bool acquired = cv_not_empty_.wait_for(lock, timeout_duration, [this]() {
      return !queue_.empty() || shutdown_requested_;
    });

    if (!acquired || (queue_.empty() && shutdown_requested_)) {
      return std::nullopt;
    }

    T item = std::move(queue_.front());
    queue_.pop();

    lock.unlock();
    cv_not_full_.notify_one();
    return item;
  }

  /**
   * @brief Signals the queue to initiate graceful shutdown, unblocking all waiting threads.
   */
  void shutdown() noexcept {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      shutdown_requested_ = true;
    }
    cv_not_empty_.notify_all();
    cv_not_full_.notify_all();
  }

  /**
   * @brief Returns current size of the queue.
   */
  [[nodiscard]] size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  /**
   * @brief Checks if the queue is empty.
   */
  [[nodiscard]] bool empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

private:
  const size_t max_capacity_;
  std::queue<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_not_empty_;
  std::condition_variable cv_not_full_;
  bool shutdown_requested_;
};

} // namespace utils
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_UTILS_CONCURRENT_QUEUE_HPP_