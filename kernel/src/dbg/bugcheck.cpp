#include "bugcheck.h"
#include "BootVideo.h"
#include "utils/kdbg.h"
#include "utils/operations.h"

constexpr std::uint32_t BugCheckColour = 0x1e1e9e;

static std::uint32_t g_currentPosition = 0;
static std::uint32_t g_currentLine = 0;

static void KiDbgPrintString(const char8_t* string)
{
     debugging::DbgWrite(u8"{}", string);
     std::size_t i{};

     for (; string[i]; i++)
          VidDrawChar(10 + g_currentPosition + (i * 8), 10 + (g_currentLine * 10), static_cast<char>(string[i]),
                      0xFFFFFF, 1);

     g_currentPosition += (i * 8);
}
static void KiDbgPrintStringLn(const char8_t* string)
{
     debugging::DbgWrite(u8"{}\r\n", string);

     for (std::size_t i = 0; string[i]; i++)
          VidDrawChar(10 + g_currentPosition + (i * 8), 10 + (g_currentLine * 10), static_cast<char>(string[i]),
                      0xFFFFFF, 1);

     g_currentLine++;
     g_currentPosition = 0;
}
static void KiDbgPrintHex(std::uintptr_t value)
{
     std::array<char, 17> buffer{};
     auto [p, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
     if (ec != std::errc()) return;

     std::size_t len = static_cast<std::size_t>(p - buffer.data());
     for (std::size_t i = len; i < 16; i++)
     {
          VidDrawChar(10 + g_currentPosition + (i * 8), 10 + (g_currentLine * 10), '0', 0xFFFFFF, 1);
     }
     for (std::size_t i = 0; i < len; i++)
     {
          VidDrawChar(10 + g_currentPosition + ((len - 1 - i) * 8), 10 + (g_currentLine * 10), buffer[len - 1 - i],
                      0xFFFFFF, 1);
     }

     debugging::DbgWrite(u8"{}", reinterpret_cast<void*>(value));

     g_currentPosition += (16 * 8);
}

void KiDbgDumpRegisters(const cpu::IInterruptFrame& frame);

void dbg::KeBugCheck(const cpu::IInterruptFrame& frame)
{
     operations::DisableInterrupts();

     const BugCheckInfo info = TranslateBugCheck(frame);

     VidClearScreen(BugCheckColour);
     KiDbgPrintStringLn(u8"*** KERNEL BUGCHECK ***");
     KiDbgPrintStringLn(u8"An unrecoverable error has occurred. The system has been halted to prevent damage.");
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"Bug Check Code:");
     KiDbgPrintHex(info.code);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintStringLn(u8"Parameters:");
     KiDbgPrintString(u8"* ");
     KiDbgPrintHex(info.param1);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"* ");
     KiDbgPrintHex(info.param2);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"* ");
     KiDbgPrintHex(info.param3);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"* ");
     KiDbgPrintHex(info.param4);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintStringLn(u8"Register Dump:");
     KiDbgDumpRegisters(frame);

     VidExchangeBuffers();

     while (true) operations::Halt();
}

#ifdef ARCH_X8664
struct InterruptFrame
{
     std::uint64_t r15;
     std::uint64_t r14;
     std::uint64_t r13;
     std::uint64_t r12;
     std::uint64_t r11;
     std::uint64_t r10;
     std::uint64_t r9;
     std::uint64_t r8;
     std::uint64_t rbp;
     std::uint64_t rdi;
     std::uint64_t rsi;
     std::uint64_t rdx;
     std::uint64_t rcx;
     std::uint64_t rbx;
     std::uint64_t rax;

     std::uint64_t vector;
     std::uint64_t errorCode;

     std::uint64_t rip;
     std::uint64_t cs;
     std::uint64_t rflags;
     std::uint64_t rsp;
     std::uint64_t ss;
};
void KiDbgDumpRegisters(const cpu::IInterruptFrame& frame)
{
     const auto& x64Frame = *static_cast<const InterruptFrame*>(frame.GetContext());
     KiDbgPrintString(u8"R15: ");
     KiDbgPrintHex(x64Frame.r15);
     KiDbgPrintString(u8"          R14: ");
     KiDbgPrintHex(x64Frame.r14);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"R13: ");
     KiDbgPrintHex(x64Frame.r13);
     KiDbgPrintString(u8"          R12: ");
     KiDbgPrintHex(x64Frame.r12);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"R11: ");
     KiDbgPrintHex(x64Frame.r11);
     KiDbgPrintString(u8"          R10: ");
     KiDbgPrintHex(x64Frame.r10);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"R9:  ");
     KiDbgPrintHex(x64Frame.r9);
     KiDbgPrintString(u8"          R8:  ");
     KiDbgPrintHex(x64Frame.r8);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"RBP: ");
     KiDbgPrintHex(x64Frame.rbp);
     KiDbgPrintString(u8"          RDI: ");
     KiDbgPrintHex(x64Frame.rdi);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"RSI: ");
     KiDbgPrintHex(x64Frame.rsi);
     KiDbgPrintString(u8"          RDX: ");
     KiDbgPrintHex(x64Frame.rdx);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"RCX: ");
     KiDbgPrintHex(x64Frame.rcx);
     KiDbgPrintString(u8"          RBX: ");
     KiDbgPrintHex(x64Frame.rbx);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"RAX: ");
     KiDbgPrintHex(x64Frame.rax);
     KiDbgPrintString(u8"          RIP: ");
     KiDbgPrintHex(x64Frame.rip);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"RSP: ");
     KiDbgPrintHex(x64Frame.rsp);
     KiDbgPrintString(u8"          CS:  ");
     KiDbgPrintHex(x64Frame.cs);
     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"RFL: ");
     KiDbgPrintHex(x64Frame.rflags);
     KiDbgPrintString(u8"          SS:  ");
     KiDbgPrintHex(x64Frame.ss);
}

dbg::BugCheckInfo dbg::TranslateBugCheck(const cpu::IInterruptFrame& frame)
{
     BugCheckInfo info{};
     const auto& x64Frame = *static_cast<const InterruptFrame*>(frame.GetContext());

     switch (x64Frame.vector)
     {
     case 0x0: // #DE - Divide Error
          info.code = ErrorDivideError;
          break;
     case 0xe: // #PF
          if (x64Frame.errorCode & 0x10) info.code = ErrorPageFaultInPageProtectedArea;
          else
               info.code = ErrorPageFaultInNonPagedArea;

          info.param1 = frame.GetFaultingAddress();

          const bool wasPresent = x64Frame.errorCode & 0x1;
          const bool wasWrite = x64Frame.errorCode & 0x2;
          const bool wasUser = x64Frame.errorCode & 0x4;
          const bool isReservedWrite = x64Frame.errorCode & 0x8;
          const bool isFetch = x64Frame.errorCode & 0x20;
          info.param2 = (wasPresent << 0) | (wasWrite << 1) | (wasUser << 2) | (isReservedWrite << 3) | (isFetch << 4);
          info.param3 = x64Frame.rip;
          break;
     }

     return info;
}
#endif
