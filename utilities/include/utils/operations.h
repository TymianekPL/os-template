#pragma once

#include <cstdint>

namespace operations
{
     void DisableInterrupts();
     void EnableInterrupts();
     void Yield();
     std::uint64_t ReadCurrentCycles();
     void Halt();
} // namespace operations
