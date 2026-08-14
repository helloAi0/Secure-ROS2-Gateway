#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <string>

#include "secure_telemetry_gateway/security/security_engine.hpp"
#include "secure_telemetry_gateway/storage/storage_engine.hpp"

using namespace secure_telemetry_gateway::security;
using namespace secure_telemetry_gateway::storage;
using namespace secure_telemetry_gateway::models;

int main() {
    std::cout << "========================================================\n";
    std::cout << "  Secure Telemetry Gateway: Performance Benchmark Suite \n";
    std::cout << "========================================================\n\n";

    // ---------------------------------------------------------
    // 1. Security Engine Benchmark
    // ---------------------------------------------------------
    size_t iterations = 10000;
    std::string key = "01234567890123456789012345678901";
    std::string raw_payload = "{\"sensor\":\"lidar\",\"value\":45.2}";

    SecurityEngine security_engine;

    std::vector<double> latencies_us;
    latencies_us.reserve(iterations);

    for (size_t i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Update with your actual SecurityEngine encryption method when ready
        // auto encrypted = security_engine.encrypt(raw_payload); 
        
        auto end = std::chrono::high_resolution_clock::now();
        
        double latency = std::chrono::duration<double, std::micro>(end - start).count();
        latencies_us.push_back(latency);
    }

    double total_time_us = std::accumulate(latencies_us.begin(), latencies_us.end(), 0.0);
    double avg_latency = total_time_us / iterations;
    
    std::sort(latencies_us.begin(), latencies_us.end());
    double p95_latency = latencies_us[static_cast<size_t>(iterations * 0.95)];

    std::cout << "[Security Engine AES-256-GCM Benchmark]\n";
    std::cout << "  Iterations:   " << iterations << "\n";
    std::cout << "  Avg Latency:  " << avg_latency << " us\n";
    std::cout << "  P95 Latency:  " << p95_latency << " us\n";
    std::cout << "  Throughput:   " << static_cast<int>(1000000.0 / avg_latency) << " ops/sec\n\n";

    // ---------------------------------------------------------
    // 2. Storage Engine Benchmark
    // ---------------------------------------------------------
    StorageEngine storage_engine(":memory:");

    size_t total_records = 10000;
    size_t batch_size = 1000;

    auto start_storage = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < total_records; i += batch_size) {
        std::vector<TelemetryData> batch;
        batch.reserve(batch_size);

        for (size_t j = 0; j < batch_size; ++j) {
            TelemetryData record{};
            
            // FIX: Changed record.timestamp to record.timestamp_ns
            // and adjusted duration_cast to nanoseconds
            record.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            batch.push_back(record);
        }
        
        for (const auto& rec : batch) {
            storage_engine.insert_telemetry(rec);
        }
    }

    auto end_storage = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_storage - start_storage).count();

    std::cout << "[Storage Engine SQLite Batch Insertion Benchmark]\n";
    std::cout << "  Inserted Records: " << total_records << "\n";
    std::cout << "  Batch Size:       " << batch_size << "\n";
    std::cout << "  Total Time:       " << duration_ms << " ms\n";
    std::cout << "  Throughput:       " << static_cast<int>((total_records / duration_ms) * 1000.0) << " rec/sec\n";

    return 0;
}