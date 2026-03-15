#pragma once
#include <cstdint>
#include "../cpu/interrupts.h"

namespace dbg
{
     inline constexpr std::uint64_t ErrorDivideError = 0x00000001;
     inline constexpr std::uint64_t ErrorDebugException = 0x00000002;
     inline constexpr std::uint64_t ErrorNMI = 0x00000003;

     inline constexpr std::uint64_t ErrorBreakpoint = 0x00000004;

     inline constexpr std::uint64_t ErrorOverflow = 0x00000005;

     inline constexpr std::uint64_t ErrorBoundRangeExceeded = 0x00000006;

     inline constexpr std::uint64_t ErrorInvalidOpcode = 0x00000007;
     inline constexpr std::uint64_t ErrorDeviceNotAvailable = 0x00000008;

     inline constexpr std::uint64_t ErrorDoubleFault = 0x00000009;

     inline constexpr std::uint64_t ErrorInvalidTSS = 0x0000000A;

     inline constexpr std::uint64_t ErrorSegmentNotPresent = 0x0000000B;

     inline constexpr std::uint64_t ErrorStackSegmentFault = 0x0000000C;

     inline constexpr std::uint64_t ErrorGeneralProtectionFault = 0x0000000D;

     inline constexpr std::uint64_t ErrorPageFaultInNonPagedArea = 0x0000000E;

     inline constexpr std::uint64_t ErrorPageFaultInPageProtectedArea = 0x0000000F;

     inline constexpr std::uint64_t ErrorFloatingPointError = 0x00000010;

     inline constexpr std::uint64_t ErrorAlignmentCheck = 0x00000011;

     inline constexpr std::uint64_t ErrorMachineCheck = 0x00000012;

     inline constexpr std::uint64_t ErrorSIMDFloatingPointException = 0x00000013;

     inline constexpr std::uint64_t ErrorVirtualizationException = 0x00000014;

     inline constexpr std::uint64_t ErrorControlProtectionException = 0x00000015;

     inline constexpr std::uint64_t ErrorSynchronousException = 0x00000020;

     inline constexpr std::uint64_t ErrorDataAbort = 0x00000021;

     inline constexpr std::uint64_t ErrorInstructionAbort = 0x00000022;

     inline constexpr std::uint64_t ErrorSPAlignmentFault = 0x00000023;

     inline constexpr std::uint64_t ErrorPCAlignmentFault = 0x00000024;

     inline constexpr std::uint64_t ErrorUnknownException = 0x000000FF;

     struct BugCheckInfo
     {
          std::uint64_t code{};
          std::uint64_t param1{};
          std::uint64_t param2{};
          std::uint64_t param3{};
          std::uint64_t param4{};
          const char8_t* description{};
          const char8_t* details{};
     };

     BugCheckInfo TranslateBugCheck(const cpu::IInterruptFrame& frame);
     void KeBugCheck(const cpu::IInterruptFrame& frame);

} // namespace dbg
