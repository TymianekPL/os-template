#pragma once

#include <cstdint>
#include "../cpu/interrupts.h"

namespace dbg
{
     struct BugCheckInfo
     {
          std::uint64_t code;
          std::uint64_t param1;
          std::uint64_t param2;
          std::uint64_t param3;
          std::uint64_t param4;
     };

     constexpr std::uint64_t ErrorDivideError = 0x1;
     constexpr std::uint64_t ErrorDebug = 0x2;
     constexpr std::uint64_t ErrorPageFaultInPageProtectedArea = 0x3;
     constexpr std::uint64_t ErrorPageFaultInNonPagedArea = 0x4;

     constexpr std::uint64_t PageFaultFreePTE = 1;
     constexpr std::uint64_t PageFaultFreePTL = 2;
     constexpr std::uint64_t PageFaultNonCanonicalAddress = 3;

     inline const char* GetErrorString(std::uint64_t code)
     {
          switch (code)
          {
          case ErrorDivideError: return "Divide Error";
          case ErrorDebug: return "Debug";
          case ErrorPageFaultInPageProtectedArea: return "Page Fault in Page Protected Area";
          case ErrorPageFaultInNonPagedArea: return "Page Fault in Non-Paged Area";
          default: return "Unknown";
          }
     }

     BugCheckInfo TranslateBugCheck(const cpu::IInterruptFrame& frame);

     void KeBugCheck(const cpu::IInterruptFrame& frame);
} // namespace dbg
