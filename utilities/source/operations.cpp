#include <utils/identify.h>
#include <utils/operations.h>

#if defined(ARCH_ARM64) && defined(COMPILER_MSVC)
#include <intrin.h>
#endif

#ifdef ARCH_X8664 // x86-64 vvv

namespace
{
     constexpr std::uint16_t COM1_PORT = 0x3F8;

#ifdef COMPILER_MSVC // MSVC vvv
     inline void Out8(std::uint16_t port, std::uint8_t value) { __outbyte(port, value); }

     inline std::uint8_t In8(std::uint16_t port) { return static_cast<std::uint8_t>(__inbyte(port)); }
#elifdef COMPILER_CLANG // ^^^ MSVC / Clang vvv
     inline void Out8(std::uint16_t port, std::uint8_t value)
     {
          asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
     }

     inline std::uint8_t In8(std::uint16_t port)
     {
          std::uint8_t value{};
          asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
          return value;
     }
#endif                  // ^^^ Clang
} // namespace

void operations::WriteSerialCharacter(char value)
{
     while ((In8(COM1_PORT + 5) & 0x20) == 0) {}
     Out8(COM1_PORT, static_cast<std::uint8_t>(value));
}
char operations::ReadSerialCharacter()
{
     while ((In8(COM1_PORT + 5) & 0x01) == 0) {}
     return static_cast<char>(In8(COM1_PORT));
}

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

namespace
{
     constexpr uintptr_t UART0_BASE = 0x09000000;

     struct UARTRegisters
     {
          volatile std::uint32_t dr;
          volatile std::uint32_t rsrEcr;
          std::uint32_t reserveD0[4]; // NOLINT
          volatile std::uint32_t fr;
          std::uint32_t reserveD1;
          volatile std::uint32_t ilpr;
          volatile std::uint32_t ibrd;
          volatile std::uint32_t fbrd;
          volatile std::uint32_t lcrh;
          volatile std::uint32_t cr;
          volatile std::uint32_t ifls;
          volatile std::uint32_t imsc;
          volatile std::uint32_t ris;
          volatile std::uint32_t mis;
          volatile std::uint32_t icr;
     };
} // namespace

void operations::WriteSerialCharacter(char value)
{
     auto* uart = reinterpret_cast<UARTRegisters*>(UART0_BASE + 0xffff'8000'0000'0000);

#ifdef COMPILER_MSVC    // MSVC vvv
     while (uart->FR & (1 << 5)) Yield();
     uart->DR = static_cast<std::uint32_t>(value);
#elifdef COMPILER_CLANG // ^^^ MSVC / Clang vvv
     while (uart->fr & (1u << 5)) Yield();
     uart->dr = static_cast<std::uint32_t>(static_cast<unsigned char>(value));
#endif                  // ^^^ Clang
}
char operations::ReadSerialCharacter()
{
     auto* uart = reinterpret_cast<UARTRegisters*>(UART0_BASE + 0xffff'8000'0000'0000);
     while (uart->fr & (1u << 4)) Yield();

     return static_cast<char>(uart->dr & 0xFF);
}

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
