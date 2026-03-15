#pragma once

#include <atomic>
namespace perf
{
     struct SystemMetrics
     {
          std::atomic<std::size_t> commitCharge{};
          std::atomic<std::size_t> commitQuota{};
     };

     SystemMetrics& GetSystemMetrics();
} // namespace perf
