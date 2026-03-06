#include <utils/identify.h>
#include <utils/operations.h>

#if defined(ARCH_ARM64) && defined(COMPILER_MSVC)
#include <intrin.h>
#endif

#ifdef ARCH_X8664 // x86-64 vvv
#ifdef COMPILER_MSVC
#include <intrin.h>

void operations::EnableInterrupts() { ::_enable(); }
void operations::DisableInterrupts() { ::_disable(); }
void operations::Yield() { ::_mm_pause(); }
std::uint64_t operations::ReadCurrentCycles() { return ::__rdtsc(); }
void operations::Halt() { ::__halt(); }
#elifdef COMPILER_CLANG // ^^^ MSVC / vvv clang
#include <x86intrin.h>

void operations::EnableInterrupts() { asm volatile("sti"); }
void operations::DisableInterrupts() { asm volatile("cli"); }
void operations::Yield() { asm volatile("pause"); }
std::uint64_t operations::ReadCurrentCycles()
{
     std::uint32_t low{};
     std::uint32_t high{};
     asm volatile("rdtsc" : "=a"(low), "=d"(high));
     return (static_cast<std::uint64_t>(high) << 32) | low;
}
void operations::Halt() { asm volatile("hlt"); }

#endif

#elifdef ARCH_X8632 // ^^^ x86-64 / x86-32 vvv
#ifdef COMPILER_MSVC
#include <intrin.h>

void operations::EnableInterrupts() { ::_enable(); }
void operations::DisableInterrupts() { ::_disable(); }
void operations::Yield() { ::_mm_pause(); }
std::uint64_t operations::ReadCurrentCycles() { return ::__rdtsc(); }
void operations::Halt() { ::__halt(); }
#elifdef COMPILER_CLANG // ^^^ MSVC / vvv clang
#include <x86intrin.h>

void operations::EnableInterrupts() { asm volatile("sti"); }
void operations::DisableInterrupts() { asm volatile("cli"); }
void operations::Yield() { asm volatile("pause"); }
std::uint64_t operations::ReadCurrentCycles()
{
     std::uint32_t low{};
     std::uint32_t high{};
     asm volatile("rdtsc" : "=a"(low), "=d"(high));
     return (static_cast<std::uint64_t>(high) << 32uz) | low;
}
void operations::Halt() { asm volatile("hlt"); }
#elifdef COMPILER_GCC // ^^^ clang / vvv gcc
#include <x86intrin.h>

void operations::EnableInterrupts() { asm volatile("sti"); }
void operations::DisableInterrupts() { asm volatile("cli"); }
void operations::Yield() { asm volatile("pause"); }
std::uint64_t operations::ReadCurrentCycles()
{
     std::uint32_t low{};
     std::uint32_t high{};
     asm volatile("rdtsc" : "=a"(low), "=d"(high));
     return (static_cast<std::uint64_t>(high) << 32) | low;
}
void operations::Halt() { asm volatile("hlt"); }
#endif

#elifdef ARCH_ARM64 // ^^^ x86-32 / ARM vvv
#ifdef COMPILER_MSVC
#include <intrin.h>

void operations::EnableInterrupts() { ::_enable(); }
void operations::DisableInterrupts() { ::_disable(); }
void operations::Yield() { ::__yield(); }
std::uint64_t operations::ReadCurrentCycles()
{
     std::uint64_t cycles = 0;
     cycles = ::_ReadStatusReg(ARM64_SYSREG(3, 3, 14, 0, 0));
     return cycles;
}
void operations::Halt() { ::__wfi(); }
#elifdef COMPILER_CLANG // ^^^ MSVC / vvv clang
void operations::EnableInterrupts() { asm volatile("msr daifclr, #2"); }
void operations::DisableInterrupts() { asm volatile("msr daifset, #2"); }
void operations::Yield() { asm volatile("yield"); }
std::uint64_t operations::ReadCurrentCycles()
{
     std::uint64_t cycles{};
     asm volatile("mrs %0, pmccntr_el0" : "=r"(cycles));
     return cycles;
}
void operations::Halt() { asm volatile("wfi"); }
#elifdef COMPILER_GCC   // ^^^ clang / vvv gcc
void operations::EnableInterrupts() { asm volatile("msr daifclr, #2"); }
void operations::DisableInterrupts() { asm volatile("msr daifset, #2"); }
void operations::Yield() { asm volatile("yield"); }
std::uint64_t operations::ReadCurrentCycles()
{
     std::uint64_t cycles{};
     asm volatile("mrs %0, pmccntr_el0" : "=r"(cycles));
     return cycles;
}
void operations::Halt() { asm volatile("wfi"); }
#endif

#else // ^^^ ARM
#error Invalid ISA
#endif
