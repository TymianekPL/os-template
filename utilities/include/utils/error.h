#pragma once

#include <cstdint>

namespace errors
{
     using ErrorType = std::uint64_t;

     // ok
     constexpr ErrorType Success = 0;
     // Not found
     constexpr ErrorType NotFound = 1;
     // Provided memory was allocated, but the operation requires privilages which the user does not have
     constexpr ErrorType AccessViolation = 2;
     // Provided memory was not allocated (reserved)
     constexpr ErrorType NotAllocated = 3;
     // Provided memory was allocated but not committed
     constexpr ErrorType NotCommitted = 4;
} // namespace errors
