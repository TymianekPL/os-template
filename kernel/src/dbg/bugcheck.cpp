#include "bugcheck.h"
#include "BootVideo.h"
#include "utils/kdbg.h"
#include "utils/operations.h"

constexpr std::uint32_t BugCheckColour = 0x1e1e9e;

static std::uint32_t g_currentPosition = 0;
static std::uint32_t g_currentLine = 0;

NO_ASAN static void KiDbgPrintString(const char8_t* string)
{
     debugging::DbgWrite(u8"{}", string);
     std::size_t i{};
     for (; string[i]; i++)
          VidDrawChar(10 + g_currentPosition + (i * 8), 10 + (g_currentLine * 10), static_cast<char>(string[i]),
                      0xFFFFFF, 1);
     g_currentPosition += (i * 8);
}

NO_ASAN static void KiDbgPrintStringLn(const char8_t* string)
{
     debugging::DbgWrite(u8"{}\r\n", string);
     for (std::size_t i = 0; string[i]; i++)
          VidDrawChar(10 + g_currentPosition + (i * 8), 10 + (g_currentLine * 10), static_cast<char>(string[i]),
                      0xFFFFFF, 1);
     g_currentLine++;
     g_currentPosition = 0;
}

NO_ASAN static void KiDbgPrintHex(std::uintptr_t value)
{
     std::array<char, 17> buffer{};
     auto [p, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
     if (ec != std::errc()) return;

     std::size_t len = static_cast<std::size_t>(p - buffer.data());

     for (std::size_t i = 0; i < 16 - len; ++i)
          VidDrawChar(10 + g_currentPosition + (i * 8), 10 + (g_currentLine * 10), '0', 0xFFFFFF, 1);

     for (std::size_t i = 0; i < len; ++i)
          VidDrawChar(10 + g_currentPosition + ((16 - len + i) * 8), 10 + (g_currentLine * 10), buffer[i], 0xFFFFFF, 1);

     debugging::DbgWrite(u8"{}", reinterpret_cast<void*>(value));
     g_currentPosition += (16 * 8);
}

NO_ASAN void KiDbgDumpRegisters(const cpu::IInterruptFrame& frame);

NO_ASAN void dbg::KeBugCheck(const cpu::IInterruptFrame& frame)
{
     operations::DisableInterrupts();

     const BugCheckInfo info = TranslateBugCheck(frame);

     VidClearScreen(BugCheckColour);

     KiDbgPrintStringLn(u8"*** KERNEL BUGCHECK ***");
     KiDbgPrintStringLn(u8"An unrecoverable error has occurred. The system has been halted.");
     KiDbgPrintStringLn(u8"");

     if (info.description)
     {
          KiDbgPrintString(u8"Cause:  ");
          KiDbgPrintStringLn(info.description);
     }
     if (info.details)
     {
          KiDbgPrintString(u8"Detail: ");
          KiDbgPrintStringLn(info.details);
     }

     KiDbgPrintStringLn(u8"");
     KiDbgPrintString(u8"Bug Check Code: ");
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

     KiDbgPrintStringLn(u8"");
     KiDbgPrintStringLn(u8"Register Dump:");
     KiDbgDumpRegisters(frame);

     VidExchangeBuffers();
     while (true) operations::Halt();
}

#ifdef ARCH_X8664

struct InterruptFrame
{
     std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
     std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
     std::uint64_t vector;
     std::uint64_t errorCode;
     std::uint64_t rip, cs, rflags, rsp, ss;
};

NO_ASAN void KiDbgDumpRegisters(const cpu::IInterruptFrame& frame)
{
     const auto& f = *static_cast<const InterruptFrame*>(frame.GetContext());

     KiDbgPrintString(u8"R15: ");
     KiDbgPrintHex(f.r15);
     KiDbgPrintString(u8"  R14: ");
     KiDbgPrintHex(f.r14);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"R13: ");
     KiDbgPrintHex(f.r13);
     KiDbgPrintString(u8"  R12: ");
     KiDbgPrintHex(f.r12);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"R11: ");
     KiDbgPrintHex(f.r11);
     KiDbgPrintString(u8"  R10: ");
     KiDbgPrintHex(f.r10);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"R9:  ");
     KiDbgPrintHex(f.r9);
     KiDbgPrintString(u8"  R8:  ");
     KiDbgPrintHex(f.r8);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"RBP: ");
     KiDbgPrintHex(f.rbp);
     KiDbgPrintString(u8"  RDI: ");
     KiDbgPrintHex(f.rdi);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"RSI: ");
     KiDbgPrintHex(f.rsi);
     KiDbgPrintString(u8"  RDX: ");
     KiDbgPrintHex(f.rdx);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"RCX: ");
     KiDbgPrintHex(f.rcx);
     KiDbgPrintString(u8"  RBX: ");
     KiDbgPrintHex(f.rbx);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"RAX: ");
     KiDbgPrintHex(f.rax);
     KiDbgPrintString(u8"  RIP: ");
     KiDbgPrintHex(f.rip);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"RSP: ");
     KiDbgPrintHex(f.rsp);
     KiDbgPrintString(u8"  CS:  ");
     KiDbgPrintHex(f.cs);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"RFL: ");
     KiDbgPrintHex(f.rflags);
     KiDbgPrintString(u8"  SS:  ");
     KiDbgPrintHex(f.ss);
     KiDbgPrintStringLn(u8"");

     KiDbgPrintString(u8"VEC: ");
     KiDbgPrintHex(f.vector);
     KiDbgPrintString(u8"  ERR: ");
     KiDbgPrintHex(f.errorCode);
     KiDbgPrintStringLn(u8"");
}

NO_ASAN dbg::BugCheckInfo dbg::TranslateBugCheck(const cpu::IInterruptFrame& frame)
{
     BugCheckInfo info{};
     const auto& f = *static_cast<const InterruptFrame*>(frame.GetContext());

     const bool errPresent = (f.errorCode & (1u << 0)) != 0;
     const bool errWrite = (f.errorCode & (1u << 1)) != 0;
     const bool errUser = (f.errorCode & (1u << 2)) != 0;
     const bool errReserved = (f.errorCode & (1u << 3)) != 0;
     const bool errFetch = (f.errorCode & (1u << 4)) != 0;
     const bool errProtKey = (f.errorCode & (1u << 5)) != 0;
     const bool errShadowStack = (f.errorCode & (1u << 6)) != 0;

     switch (f.vector)
     {
     case 0x00:
          info.code = ErrorDivideError;
          info.description = u8"Divide-by-zero (#DE)";
          info.details = u8"DIV or IDIV with divisor == 0, or quotient too large for destination";
          info.param1 = f.rip;
          break;

     case 0x01:
          info.code = ErrorDebugException;
          info.description = u8"Debug exception (#DB)";
          info.details = u8"Hardware breakpoint or single-step trap hit in kernel mode";
          info.param1 = f.rip;
          break;

     case 0x02:
          info.code = ErrorNMI;
          info.description = u8"Non-maskable interrupt (NMI)";
          info.details = u8"Hardware failure, ECC error, or watchdog triggered an NMI";
          info.param1 = f.rip;
          break;

     case 0x03:
          info.code = ErrorBreakpoint;
          info.description = u8"Breakpoint (#BP)";
          info.details = u8"INT3 executed in kernel - unintended or orphaned debug break";
          info.param1 = f.rip;
          break;

     case 0x04:
          info.code = ErrorOverflow;
          info.description = u8"Overflow (#OF)";
          info.details = u8"INTO executed with OF set";
          info.param1 = f.rip;
          break;

     case 0x05:
          info.code = ErrorBoundRangeExceeded;
          info.description = u8"BOUND range exceeded (#BR)";
          info.details = u8"BOUND instruction operand out of range";
          info.param1 = f.rip;
          break;

     case 0x06:
          info.code = ErrorInvalidOpcode;
          info.description = u8"Invalid opcode (#UD)";
          info.details = u8"UD2, unrecognised encoding, or illegal use of a REX/VEX prefix";
          info.param1 = f.rip;
          break;

     case 0x07:
          info.code = ErrorDeviceNotAvailable;
          info.description = u8"Device not available (#NM)";
          info.details = u8"x87/SSE/AVX used with CR0.EM set or CR0.TS set without kernel FPU save";
          info.param1 = f.rip;
          break;

     case 0x08:
          info.code = ErrorDoubleFault;
          info.description = u8"Double fault (#DF)";
          info.details = u8"Exception occurred while handling another exception - stack or IDT corrupt";
          info.param1 = f.errorCode;
          info.param2 = f.rsp;
          info.param3 = f.rip;
          break;

     case 0x0A:
          info.code = ErrorInvalidTSS;
          info.description = u8"Invalid TSS (#TS)";
          info.details = u8"Task state segment selector is invalid or limit too small";
          info.param1 = f.errorCode;
          info.param2 = f.rip;
          break;

     case 0x0B:
          info.code = ErrorSegmentNotPresent;
          info.description = u8"Segment not present (#NP)";
          info.details = u8"Attempted to load a descriptor with P=0";
          info.param1 = f.errorCode;
          info.param2 = f.rip;
          break;

     case 0x0C:
          info.code = ErrorStackSegmentFault;
          info.description = u8"Stack segment fault (#SS)";
          info.details = u8"Stack operation exceeded limit or SS descriptor not present";
          info.param1 = f.errorCode;
          info.param2 = f.rsp;
          info.param3 = f.rip;
          break;

     case 0x0D:
          info.code = ErrorGeneralProtectionFault;
          info.description = u8"General protection fault (#GP)";
          if (f.errorCode != 0)
               info.details = u8"Segment selector in error code - bad descriptor or privilege violation";
          else
               info.details = u8"No selector - null deref, WRMSR to bad MSR, or privileged instruction in ring 3";
          info.param1 = f.errorCode;
          info.param2 = f.rip;
          info.param3 = f.rsp;
          break;

     case 0x0E:
     {
          const std::uint64_t faultAddr = frame.GetFaultingAddress();

          info.code = errFetch ? ErrorPageFaultInPageProtectedArea : ErrorPageFaultInNonPagedArea;

          if (!errPresent && !errFetch) info.description = u8"Page fault - not-present page read";
          else if (!errPresent && errFetch)
               info.description = u8"Page fault - instruction fetch from unmapped page";
          else if (errPresent && errWrite)
               info.description = u8"Page fault - write to read-only page";
          else if (errPresent && errFetch)
               info.description = u8"Page fault - instruction fetch from non-executable page (NX)";
          else if (errReserved)
               info.description = u8"Page fault - reserved PTE bits set (PTE corruption)";
          else if (errProtKey)
               info.description = u8"Page fault - protection-key violation";
          else if (errShadowStack)
               info.description = u8"Page fault - shadow-stack access violation (CET)";
          else
               info.description = u8"Page fault";

          if (faultAddr < 0x10000) info.details = u8"Faulting address is near NULL - likely null pointer dereference";
          else if (!errPresent)
               info.details = u8"Page not mapped - use-after-free, stack overflow, or bad pointer";
          else if (errWrite)
               info.details = u8"Write to protected page - COW violation or read-only mapping";
          else
               info.details = u8"Check RIP and faulting address for the offending access";

          info.param1 = faultAddr;
          info.param2 = f.errorCode;
          info.param3 = f.rip;
          info.param4 = f.rsp;
          break;
     }

     case 0x10:
          info.code = ErrorFloatingPointError;
          info.description = u8"x87 floating-point error (#MF)";
          info.details = u8"Unmasked x87 FPU exception - check x87 status word";
          info.param1 = f.rip;
          break;

     case 0x11:
          info.code = ErrorAlignmentCheck;
          info.description = u8"Alignment check (#AC)";
          info.details = u8"Misaligned memory access with EFLAGS.AC set - kernel should not set AC";
          info.param1 = frame.GetFaultingAddress();
          info.param2 = f.rip;
          info.param3 = f.errorCode;
          break;

     case 0x12:
          info.code = ErrorMachineCheck;
          info.description = u8"Machine check (#MC)";
          info.details = u8"Uncorrectable hardware error - CPU, memory, or bus fault";
          info.param1 = f.rip;
          break;

     case 0x13:
          info.code = ErrorSIMDFloatingPointException;
          info.description = u8"SIMD floating-point exception (#XM)";
          info.details = u8"Unmasked SSE/AVX exception - check MXCSR";
          info.param1 = f.rip;
          break;

     case 0x14:
          info.code = ErrorVirtualizationException;
          info.description = u8"Virtualisation exception (#VE)";
          info.details = u8"EPT violation or #VE injection from hypervisor";
          info.param1 = f.rip;
          break;

     case 0x15:
          info.code = ErrorControlProtectionException;
          info.description = u8"Control protection exception (#CP, CET)";
          info.details = u8"Shadow-stack or indirect-branch tracking violation";
          info.param1 = f.errorCode;
          info.param2 = f.rip;
          break;

     default:
          info.code = ErrorUnknownException;
          info.description = u8"Unknown or reserved exception vector";
          info.details = u8"Unexpected vector - IDT entry missing or spurious interrupt";
          info.param1 = f.vector;
          info.param2 = f.errorCode;
          info.param3 = f.rip;
          break;
     }

     return info;
}

#elifdef ARCH_ARM64

struct InterruptFrame
{
     std::uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
     std::uint64_t x8, x9, x10, x11, x12, x13, x14, x15;
     std::uint64_t x16, x17, x18;
     std::uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
     std::uint64_t fp;
     std::uint64_t lr;
     std::uint64_t vector;
     std::uint64_t esr;
     std::uint64_t far;
     std::uint64_t svc;
     std::uint64_t sp;
};
static_assert(sizeof(InterruptFrame) == 36uz * sizeof(std::uint64_t));

static constexpr std::uint32_t EsrEc(std::uint64_t esr) { return static_cast<std::uint32_t>((esr >> 26) & 0x3F); }
static constexpr std::uint32_t EsrIss(std::uint64_t esr) { return static_cast<std::uint32_t>(esr & 0x01FFFFFF); }

static constexpr std::uint32_t FscTranslationL0 = 0x04;
static constexpr std::uint32_t FscTranslationL1 = 0x05;
static constexpr std::uint32_t FscTranslationL2 = 0x06;
static constexpr std::uint32_t FscTranslationL3 = 0x07;
static constexpr std::uint32_t FscAccessFlagL1 = 0x09;
static constexpr std::uint32_t FscAccessFlagL2 = 0x0A;
static constexpr std::uint32_t FscAccessFlagL3 = 0x0B;
static constexpr std::uint32_t FscPermissionL1 = 0x0D;
static constexpr std::uint32_t FscPermissionL2 = 0x0E;
static constexpr std::uint32_t FscPermissionL3 = 0x0F;
static constexpr std::uint32_t FscSyncExternal = 0x10;
static constexpr std::uint32_t FscAlignmentFault = 0x21;
static constexpr std::uint32_t FscTLBConflict = 0x30;

NO_ASAN static const char8_t* FscDescription(std::uint32_t fsc)
{
     switch (fsc & 0x3F)
     {
     case FscTranslationL0: return u8"translation fault at level 0";
     case FscTranslationL1: return u8"translation fault at level 1";
     case FscTranslationL2: return u8"translation fault at level 2";
     case FscTranslationL3: return u8"translation fault at level 3 (page not mapped)";
     case FscAccessFlagL1: return u8"access flag fault at level 1";
     case FscAccessFlagL2: return u8"access flag fault at level 2";
     case FscAccessFlagL3: return u8"access flag fault at level 3";
     case FscPermissionL1: return u8"permission fault at level 1";
     case FscPermissionL2: return u8"permission fault at level 2";
     case FscPermissionL3: return u8"permission fault at level 3 (access denied)";
     case FscSyncExternal: return u8"synchronous external abort (bus/memory error)";
     case FscAlignmentFault: return u8"alignment fault";
     case FscTLBConflict: return u8"TLB conflict abort";
     default: return u8"unknown fault status code";
     }
}

NO_ASAN void KiDbgDumpRegisters(const cpu::IInterruptFrame& frame)
{
     const auto& f = *static_cast<const InterruptFrame*>(frame.GetContext());

#define PAIR(la, va, lb, vb)                                                                                           \
     KiDbgPrintString(la);                                                                                             \
     KiDbgPrintHex(va);                                                                                                \
     KiDbgPrintString(u8"  " lb ": ");                                                                                 \
     KiDbgPrintHex(vb);                                                                                                \
     KiDbgPrintStringLn(u8"")

     PAIR(u8"x0:  ", f.x0, u8"x1 ", f.x1);
     PAIR(u8"x2:  ", f.x2, u8"x3 ", f.x3);
     PAIR(u8"x4:  ", f.x4, u8"x5 ", f.x5);
     PAIR(u8"x6:  ", f.x6, u8"x7 ", f.x7);
     PAIR(u8"x8:  ", f.x8, u8"x9 ", f.x9);
     PAIR(u8"x10: ", f.x10, u8"x11", f.x11);
     PAIR(u8"x12: ", f.x12, u8"x13", f.x13);
     PAIR(u8"x14: ", f.x14, u8"x15", f.x15);
     PAIR(u8"x16: ", f.x16, u8"x17", f.x17);
     PAIR(u8"x18: ", f.x18, u8"x19", f.x19);
     PAIR(u8"x20: ", f.x20, u8"x21", f.x21);
     PAIR(u8"x22: ", f.x22, u8"x23", f.x23);
     PAIR(u8"x24: ", f.x24, u8"x25", f.x25);
     PAIR(u8"x26: ", f.x26, u8"x27", f.x27);
     PAIR(u8"x28: ", f.x28, u8"fp ", f.fp);
     PAIR(u8"lr:  ", f.lr, u8"sp ", f.sp);
     PAIR(u8"vec: ", f.vector, u8"esr", f.esr);
     KiDbgPrintString(u8"far: ");
     KiDbgPrintHex(f.far);
     KiDbgPrintStringLn(u8"");

     if (f.vector == 0x15) // SVC
     {
          KiDbgPrintString(u8"svc: ");
          KiDbgPrintHex(f.svc);
          KiDbgPrintStringLn(u8"");
     }

#undef PAIR
}

NO_ASAN dbg::BugCheckInfo dbg::TranslateBugCheck(const cpu::IInterruptFrame& frame)
{
     BugCheckInfo info{};
     const auto& f = *static_cast<const InterruptFrame*>(frame.GetContext());

     const std::uint32_t ec = EsrEc(f.esr);
     const std::uint32_t iss = EsrIss(f.esr);
     const std::uint32_t fsc = iss & 0x3F;
     const bool wnr = (iss & (1u << 6)) != 0;

     switch (ec)
     {
     case 0x00:
          info.code = ErrorUnknownException;
          info.description = u8"Unknown exception (EC=0x00)";
          info.details = u8"Unclassified synchronous exception - possible firmware or hardware bug";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x01:
          info.code = ErrorSynchronousException;
          info.description = u8"Trapped WFI/WFE instruction (EC=0x01)";
          info.details = u8"WFI or WFE trapped - hypervisor intercept in kernel mode";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x03:
     case 0x04:
     case 0x05:
     case 0x06:
          info.code = ErrorInvalidOpcode;
          info.description = u8"AArch32 coprocessor instruction trap";
          info.details = u8"AArch32 CP14/CP15 instruction executed in AArch64 kernel - binary mismatch?";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x07:
          info.code = ErrorDeviceNotAvailable;
          info.description = u8"SIMD/FP access trap (EC=0x07)";
          info.details = u8"FP/SIMD/SVE used with FPEN or ZEN not enabled - missing context save?";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x0C:
     case 0x1C:
          info.code = ErrorFloatingPointError;
          info.description = u8"Floating-point exception";
          info.details = u8"Unmasked FP exception (divide-by-zero, overflow, invalid op, etc.)";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x0E:
          info.code = ErrorInvalidOpcode;
          info.description = u8"Illegal execution state (EC=0x0E)";
          info.details = u8"PSTATE.IL set - return from exception restored invalid execution state";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x11:
     case 0x15:
          info.code = ErrorGeneralProtectionFault;
          info.description = u8"Supervisor call in kernel mode (SVC)";
          info.details = u8"SVC executed from EL1 - syscall dispatched without ring transition";
          info.param1 = f.svc;
          info.param2 = f.lr;
          break;

     case 0x18:
          info.code = ErrorGeneralProtectionFault;
          info.description = u8"System instruction trap (MSR/MRS, EC=0x18)";
          info.details = u8"Access to a trapped or undefined system register";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x19:
          info.code = ErrorDeviceNotAvailable;
          info.description = u8"SVE access trap (EC=0x19)";
          info.details = u8"SVE instruction executed with ZEN not enabled";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x1A:
          info.code = ErrorGeneralProtectionFault;
          info.description = u8"ERET/ERETAA/ERETAB trap (EC=0x1A)";
          info.details = u8"ERET with PAC authentication failure - return address corrupt (ROP?)";
          info.param1 = f.esr;
          info.param2 = f.lr;
          info.param3 = f.sp;
          break;

     case 0x1F:
          info.code = ErrorUnknownException;
          info.description = u8"Implementation-defined exception (EC=0x1F)";
          info.details = u8"CPU vendor-specific trap - check core errata";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x20:
     case 0x21:
          info.code = ErrorInstructionAbort;
          info.description = u8"Instruction abort (page fault on fetch)";
          info.details = FscDescription(fsc);
          info.param1 = f.far;
          info.param2 = f.esr;
          info.param3 = f.lr;
          break;

     case 0x22:
          info.code = ErrorPCAlignmentFault;
          info.description = u8"PC alignment fault (EC=0x22)";
          info.details = u8"PC not 4-byte aligned - corrupt branch target or stack smash";
          info.param1 = f.far;
          info.param2 = f.lr;
          break;

     case 0x24:
     case 0x25:
     {
          info.code = ErrorDataAbort;

          const std::uint32_t fscBase = fsc & 0x3C;

          if (fsc == FscAlignmentFault)
          {
               info.description = u8"Data abort - alignment fault";
               info.details = u8"Misaligned access to device memory or SCTLR.A enforcing alignment";
          }
          else if (fscBase == 0x04)
          {
               info.description =
                   wnr ? u8"Data abort - write to unmapped page" : u8"Data abort - read from unmapped page";
               info.details = FscDescription(fsc);
               if (f.far < 0x10000) info.details = u8"Faulting address near NULL - likely null pointer dereference";
          }
          else if (fscBase == 0x0C)
          {
               info.description = wnr ? u8"Data abort - write permission fault (read-only page)"
                                      : u8"Data abort - read permission fault";
               info.details = FscDescription(fsc);
          }
          else if (fscBase == 0x08)
          {
               info.description = u8"Data abort - access flag fault";
               info.details = u8"Page table entry has AF=0 and hardware management is off";
          }
          else if (fsc == FscSyncExternal)
          {
               info.description = u8"Data abort - synchronous external abort";
               info.details = u8"Bus error, ECC uncorrectable error, or device fault";
          }
          else
          {
               info.description = u8"Data abort";
               info.details = FscDescription(fsc);
          }

          info.param1 = f.far;
          info.param2 = f.esr;
          info.param3 = f.lr;
          info.param4 = f.sp;
          break;
     }

     case 0x26:
          info.code = ErrorSPAlignmentFault;
          info.description = u8"SP alignment fault (EC=0x26)";
          info.details = u8"SP not 16-byte aligned at EL1 - stack corruption or misaligned alloca";
          info.param1 = f.sp;
          info.param2 = f.lr;
          break;

     case 0x2C:
          info.code = ErrorControlProtectionException;
          info.description = u8"Branch Target Identification failure (BTI, EC=0x2C)";
          info.details = u8"Indirect branch to non-BTI landing pad - probable ROP/JOP attack or compiler bug";
          info.param1 = f.far;
          info.param2 = f.esr;
          info.param3 = f.lr;
          break;

     case 0x2F:
          info.code = ErrorMachineCheck;
          info.description = u8"SError interrupt (EC=0x2F)";
          info.details = u8"Asynchronous external abort - hardware error, ECC failure, or bus fault";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     case 0x30:
     case 0x31:
          info.code = ErrorDebugException;
          info.description = u8"Hardware breakpoint";
          info.details = u8"Hardware breakpoint triggered in kernel mode";
          info.param1 = f.far;
          info.param2 = f.esr;
          break;

     case 0x32:
     case 0x33:
          info.code = ErrorDebugException;
          info.description = u8"Software step exception";
          info.details = u8"Single-step debug exception fired in kernel - MDSCR_EL1.SS set";
          info.param1 = f.far;
          info.param2 = f.esr;
          break;

     case 0x34:
     case 0x35:
          info.code = ErrorDebugException;
          info.description = u8"Watchpoint exception";
          info.details = u8"Hardware data watchpoint triggered in kernel mode";
          info.param1 = f.far;
          info.param2 = f.esr;
          info.param3 = f.lr;
          break;

     case 0x38:
     case 0x3C:
          info.code = ErrorBreakpoint;
          info.description = u8"Software breakpoint (BRK/BKPT)";
          info.details = u8"BRK or BKPT instruction executed - orphaned debug break in kernel";
          info.param1 = iss & 0xFFFF;
          info.param2 = f.lr;
          break;

     case 0x3D:
          info.code = ErrorSynchronousException;
          info.description = u8"AArch32 vector catch (EC=0x3D)";
          info.details = u8"AArch32 exception vector intercepted - unexpected 32-bit execution";
          info.param1 = f.esr;
          info.param2 = f.lr;
          break;

     default:
          info.code = ErrorUnknownException;
          info.description = u8"Unrecognised exception class (EC)";
          info.details = u8"Unexpected EC value - new CPU feature or firmware injecting spurious exception";
          info.param1 = f.esr;
          info.param2 = f.far;
          info.param3 = f.lr;
          break;
     }

     return info;
}

#endif // ARCH_ARM64
