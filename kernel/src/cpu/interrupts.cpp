#include "interrupts.h"
#include <utils/identify.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "utils/kdbg.h"
#include "utils/operations.h"

std::uint64_t cpu::g_systemBootTimeOffsetSeconds{};

struct RSDPDescriptor
{
     char signature[8]; // "RSD PTR " NOLINT
     std::uint8_t checksum;
     char oemId[6];         // NOLINT
     std::uint8_t revision; // 0 for ACPI 1.0; 2 for ACPI 2.0+
     std::uint32_t rsdtAddress;
};
struct RSDPDescriptor2
{
     RSDPDescriptor firstPart;
     std::uint32_t length;
     std::uint64_t xsdtAddress;
     std::uint8_t extendedChecksum;
     std::uint8_t reserved[3]; // NOLINT
};
struct ACPISDTHeader
{
     char signature[4]; // NOLINT
     std::uint32_t length;
     std::uint8_t revision;
     std::uint8_t checksum;
     char oemId[6];      // NOLINT
     char oemTableId[8]; // NOLINT
     std::uint32_t oemRevision;
     std::uint32_t creatorId;
     std::uint32_t creatorRevision;
};

struct MCFGAllocationClass
{
     std::uint64_t baseAddress;
     std::uint16_t pciSegmentGroup;
     std::uint8_t startBusNumber;
     std::uint8_t endBusNumber;
     std::uint32_t reserved;
};

struct RSDT
{
     ACPISDTHeader header;
     std::uint32_t tablePointers[0]; // NOLINT
};

struct XSDT
{
     ACPISDTHeader header;
     std::uint64_t tablePointers[0]; // NOLINT
};

struct MADT
{
     ACPISDTHeader header;
     std::uint32_t localApicAddress;
     std::uint32_t flags;
     std::uint8_t entries[]; // NOLINT
};

struct MADTEntry
{
     std::uint8_t type;
     std::uint8_t length;
};

struct MCFG
{
     ACPISDTHeader header;
     std::uint64_t reserved;
};

#ifdef ARCH_X8664
cpu::InterruptVector cpu::TimerIrqVector = 0xd0;

#include <emmintrin.h>

struct MADTLocalApic
{
     MADTEntry header;
     std::uint8_t acpiProcessorId;
     std::uint8_t apicId;
     std::uint32_t flags;
};

struct IOAPICEntry
{
     std::uint8_t type;
     std::uint8_t length;
     std::uint8_t ioApicId;
     std::uint8_t reserved;
     std::uint32_t ioApicAddress;
     std::uint32_t globalSystemInterruptBase;
};

struct FADT
{
     ACPISDTHeader header;

     std::uint32_t firmwareCtrl;
     std::uint32_t dsdt;

     std::uint8_t reserved;

     std::uint8_t preferredPmProfile;
     std::uint16_t sciInterrupt;
     std::uint32_t smiCommandPort;
     std::uint8_t acpiEnable;
     std::uint8_t acpiDisable;
     std::uint8_t s4BiosReq;
     std::uint8_t pstateControl;
     std::uint32_t pm1aEventBlock;
     std::uint32_t pm1bEventBlock;
     std::uint32_t pm1aControlBlock;
     std::uint32_t pm1bControlBlock;
     std::uint32_t pm2ControlBlock;
     std::uint32_t pmTimerBlock;
     std::uint32_t gpe0Block;
     std::uint32_t gpe1Block;
     std::uint8_t pm1EventLength;
     std::uint8_t pm1ControlLength;
     std::uint8_t pm2ControlLength;
     std::uint8_t pmTimerLength;
     std::uint8_t gpe0Length;
     std::uint8_t gpe1Length;
     std::uint8_t gpe1Base;
     std::uint8_t cStateControl;
     std::uint16_t worstC2Latency;
     std::uint16_t worstC3Latency;
     std::uint16_t flushSize;
     std::uint16_t flushStride;
     std::uint8_t dutyOffset;
     std::uint8_t dutyWidth;
     std::uint8_t dayAlarm;
     std::uint8_t monthAlarm;
     std::uint8_t century;

     std::uint16_t bootArchitectureFlags;

     std::uint8_t reserved2;
     std::uint32_t flags;

     struct GenericAddressStructure
     {
          std::uint8_t addressSpace;
          std::uint8_t bitWidth;
          std::uint8_t bitOffset;
          std::uint8_t accessSize;
          std::uint64_t address;
     };

     GenericAddressStructure resetReg;
     std::uint8_t resetValue;
     std::uint8_t reserved3[3]; // NOLINT

     std::uint64_t xFirmwareControl;
     std::uint64_t xDsdt;

     GenericAddressStructure xPm1aEventBlock;
     GenericAddressStructure xPm1bEventBlock;
     GenericAddressStructure xPm1aControlBlock;
     GenericAddressStructure xPm1bControlBlock;
     GenericAddressStructure xPm2ControlBlock;
     GenericAddressStructure xPmTimerBlock;
     GenericAddressStructure xGpe0Block;
     GenericAddressStructure xGpe1Block;
};

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
struct X8664InterruptFrame : cpu::IInterruptFrame // NOLINT
{
     InterruptFrame* frame;
     explicit X8664InterruptFrame(InterruptFrame* frame) : frame(frame) {}

     [[nodiscard]] std::uint64_t GetVector() const override { return frame->vector; }
     [[nodiscard]] cpu::InterruptError GetError() const override
     {
          switch (frame->vector)
          {
          case 0x00: return cpu::InterruptError::DivideByZero;
          case 0x06: return cpu::InterruptError::InvalidInstruction;
          case 0x0D: return cpu::InterruptError::ProtectionFault;
          case 0x0E:
               if (frame->errorCode & (1 << 1)) return cpu::InterruptError::MemoryWriteFault;
               if (frame->errorCode & (1 << 2)) return cpu::InterruptError::MemoryExecuteFault;
               return cpu::InterruptError::MemoryReadFault;
          default: return frame->vector >= 0x20 ? cpu::InterruptError::HardwareInterrupt : cpu::InterruptError::Unknown;
          }
     }

     [[nodiscard]] std::uintptr_t GetInstructionPointer() const override { return frame->rip; }
     [[nodiscard]] std::uintptr_t GetStackPointer() const override { return frame->rsp; }
     [[nodiscard]] std::uintptr_t GetFaultingAddress() const override
     {
#ifdef COMPILER_MSVC // MSVC vvv
          return __readcr2();
#elifdef COMPILER_CLANG // ^^^ MSVC / Clang vvv
          std::uintptr_t addr{};
          asm volatile("mov %%cr2, %0" : "=r"(addr));
          return addr;
#endif
     }

     void DumpRegisters() const override
     {
          debugging::DbgWrite(u8"r15 {}\r\n", reinterpret_cast<void*>(frame->r15));
          debugging::DbgWrite(u8"r14 {}\r\n", reinterpret_cast<void*>(frame->r14));
          debugging::DbgWrite(u8"r13 {}\r\n", reinterpret_cast<void*>(frame->r13));
          debugging::DbgWrite(u8"r12 {}\r\n", reinterpret_cast<void*>(frame->r12));
          debugging::DbgWrite(u8"r11 {}\r\n", reinterpret_cast<void*>(frame->r11));
          debugging::DbgWrite(u8"r10 {}\r\n", reinterpret_cast<void*>(frame->r10));
          debugging::DbgWrite(u8"r9  {}\r\n", reinterpret_cast<void*>(frame->r9));
          debugging::DbgWrite(u8"r8  {}\r\n", reinterpret_cast<void*>(frame->r8));
          debugging::DbgWrite(u8"rbp {}\r\n", reinterpret_cast<void*>(frame->rbp));
          debugging::DbgWrite(u8"rdi {}\r\n", reinterpret_cast<void*>(frame->rdi));
          debugging::DbgWrite(u8"rsi {}\r\n", reinterpret_cast<void*>(frame->rsi));
          debugging::DbgWrite(u8"rdx {}\r\n", reinterpret_cast<void*>(frame->rdx));
          debugging::DbgWrite(u8"rcx {}\r\n", reinterpret_cast<void*>(frame->rcx));
          debugging::DbgWrite(u8"rbx {}\r\n", reinterpret_cast<void*>(frame->rbx));
          debugging::DbgWrite(u8"rax {}\r\n", reinterpret_cast<void*>(frame->rax));
          debugging::DbgWrite(u8"vec {}\r\n", reinterpret_cast<void*>(frame->vector));
          debugging::DbgWrite(u8"erc {}\r\n", reinterpret_cast<void*>(frame->errorCode));
          debugging::DbgWrite(u8"rsp {}\r\n", reinterpret_cast<void*>(frame->rsp));
          debugging::DbgWrite(u8"cs  {}\r\n", reinterpret_cast<void*>(frame->cs));
          debugging::DbgWrite(u8"rfl {}\r\n", reinterpret_cast<void*>(frame->rflags));
          debugging::DbgWrite(u8"ss  {}\r\n", reinterpret_cast<void*>(frame->ss));
     }
};

extern "C" InterruptFrame* KeHandleInterruptFrame(InterruptFrame* frame)
{
     X8664InterruptFrame vFrame{frame};
     HandleInterrupt(vFrame);
     KeAcknowledgeInterrupt();
     return vFrame.frame;
}

volatile std::uint32_t* g_lapic{};
MADT* g_madt{};
MCFG* g_mcfg{};
FADT* g_fadt{};

void KeAcknowledgeInterrupt() { g_lapic[0xB0 / 4] = 0; }
static std::atomic<std::uint64_t> lapicTicks{};
constexpr std::uintptr_t HpetBasePhysical = 0xFED00000;
std::uint64_t g_HpetFemtosecondsPerTick = 0;

std::uint64_t KiReadHPETRaw()
{
     constexpr std::uintptr_t HpetBase = HpetBasePhysical + 0xffff'8000'0000'0000;
     volatile std::uint64_t& HpetMainCounter = *reinterpret_cast<volatile std::uint64_t*>(HpetBase + 0x0F0);
     return HpetMainCounter;
}

std::uint64_t KiGetHPETFrequency()
{
     constexpr std::uint64_t FemtoPerSecond = 1'000'000'000'000'000ULL;
     return FemtoPerSecond / g_HpetFemtosecondsPerTick;
}
std::uint64_t KiGetHPETRawFrequency() { return 1'000'000'000'000'000ULL / g_HpetFemtosecondsPerTick; }

std::uint64_t KiReadHPET()
{
     constexpr std::uintptr_t HpetBase = HpetBasePhysical + 0xffff'8000'0000'0000;
     volatile std::uint64_t& HpetMainCounter = *reinterpret_cast<volatile std::uint64_t*>(HpetBase + 0xF0);
     return HpetMainCounter * ((g_HpetFemtosecondsPerTick + 500'000ULL) / 10'000'000ULL);
}

void KiInitialiseHPET()
{
     constexpr std::uintptr_t HpetBaseVirtual = HpetBasePhysical + 0xffff'8000'0000'0000;

     volatile std::uint64_t& HpetGenCapabilities = *reinterpret_cast<volatile std::uint64_t*>(HpetBaseVirtual + 0x0);
     volatile std::uint64_t& HpetGenConfig = *reinterpret_cast<volatile std::uint64_t*>(HpetBaseVirtual + 0x10);
     volatile std::uint64_t& HpetMainCounter = *reinterpret_cast<volatile std::uint64_t*>(HpetBaseVirtual + 0xF0);
     volatile std::uint64_t& HpetTimerConfig = *reinterpret_cast<volatile std::uint64_t*>(HpetBaseVirtual + 0x100);
     volatile std::uint64_t& HpetTimerComparator = *reinterpret_cast<volatile std::uint64_t*>(HpetBaseVirtual + 0x108);

     HpetGenConfig &= ~(1ULL << 0);

     HpetMainCounter = 0;

     HpetTimerConfig |= (1ULL << 2);
     HpetTimerConfig &= ~(1ULL << 3);
     HpetTimerConfig |= (1ULL << 1);
     HpetTimerConfig |= (32ULL << 9);

     g_HpetFemtosecondsPerTick = (HpetGenCapabilities >> 32) & 0xFFFFFFFF;

     double hpetFrequencyHz = 1e15 / static_cast<double>(g_HpetFemtosecondsPerTick);
     std::uint64_t tickInterval = static_cast<std::uint64_t>(hpetFrequencyHz / 1000.0);
     HpetTimerComparator = tickInterval;

     HpetGenConfig |= (1ULL << 0);
     debugging::DbgWrite(u8"HPET at {}Hz\r\n", KiGetHPETRawFrequency());
}
static std::uint64_t approximation = 0;

std::uint64_t KiGetLAPICEstimation()
{
     if (approximation != 0) return approximation;

     volatile std::uint32_t* lapicBase = reinterpret_cast<volatile std::uint32_t*>(
         static_cast<std::uintptr_t>(g_madt->localApicAddress) + 0xffff'8000'0000'0000);

     std::uint32_t TimerVector = cpu::TimerIrqVector;

     auto measure = [&](std::uint32_t initialCount, std::size_t targetTicks) -> double
     {
          lapicBase[0x320 / 4] = TimerVector | (1 << 17);
          lapicBase[0x380 / 4] = initialCount;

          lapicTicks.store(0);
          operations::EnableInterrupts();
          while (lapicTicks.load(std::memory_order::relaxed) < targetTicks) operations::Yield();
          return static_cast<double>(KiReadHPET()) / static_cast<double>(lapicTicks.load(std::memory_order::relaxed));
     };

     double freq1 = measure(0xFFFF, 50);

     std::uint32_t initialCount2 = static_cast<std::uint32_t>(0xFFFF / (freq1 / 1'000'000));
     double freq2 = measure(initialCount2, 50);

     std::uint32_t initialCount3 = static_cast<std::uint32_t>(initialCount2 / (freq2 / freq1));
     double freq3 = measure(initialCount3, 50);

     approximation = static_cast<std::uint64_t>(freq3);

     debugging::DbgWrite(u8"Approx LAPIC frequency = {}\r\n", approximation);
     lapicTicks.store(0);
     volatile auto* hpetBase = reinterpret_cast<volatile std::uint64_t*>(HpetBasePhysical + 0xffff'8000'0000'0000);
     *hpetBase = 0;
     return approximation;
}

void KiInitialiseLAPICTimer(std::uintptr_t acpiPhysical)
{
     KiInitialiseHPET();

     RSDPDescriptor* lpRsp = reinterpret_cast<RSDPDescriptor*>(acpiPhysical + 0xffff'8000'0000'0000);
     MADT* lpMadt = nullptr;
     debugging::DbgWrite(u8"RSDPv{} at {}\r\n", lpRsp->revision, lpRsp);

     if (lpRsp->revision >= 2)
     {
          RSDPDescriptor2* lpRsp20 = reinterpret_cast<RSDPDescriptor2*>(lpRsp);

          XSDT* pXsdt = reinterpret_cast<XSDT*>(lpRsp20->xsdtAddress + 0xffff'8000'0000'0000);

          const std::uint32_t entryCount =
              (pXsdt->header.length - sizeof(struct ACPISDTHeader)) / sizeof(std::uintptr_t);

          for (std::size_t i = 0; i < entryCount; i++)
          {
               ACPISDTHeader* pHeader =
                   reinterpret_cast<ACPISDTHeader*>(pXsdt->tablePointers[i] + 0xffff'8000'0000'0000);

               if (memcmp(pHeader->signature, "APIC", 4) == 0)
               {
                    lpMadt = reinterpret_cast<MADT*>(pHeader);
                    debugging::DbgWrite(u8"Found MADT at {}\r\n", lpMadt);
               }
               else if (memcmp(pHeader->signature, "FADT", 4) == 0)
               {
                    g_fadt = reinterpret_cast<FADT*>(pHeader);
                    debugging::DbgWrite(u8"Found FADT at {}\r\n", g_fadt);
               }
               else if (memcmp(pHeader->signature, "MCFG", 4) == 0)
               {
                    g_mcfg = reinterpret_cast<MCFG*>(pHeader);
                    debugging::DbgWrite(u8"Found MCFG at {}\r\n", g_mcfg);
               }

               if (lpMadt != nullptr && g_mcfg != nullptr) break;
          }
     }
     else
     {
          RSDT* lpRsdt = reinterpret_cast<RSDT*>(lpRsp->rsdtAddress + 0xffff'8000'0000'0000);

          const std::uint32_t entryCount = (lpRsdt->header.length - sizeof(ACPISDTHeader)) / sizeof(std::uint32_t);

          for (std::size_t i = 0; i < entryCount; i++)
          {
               ACPISDTHeader* pHeader =
                   reinterpret_cast<ACPISDTHeader*>(lpRsdt->tablePointers[i] + 0xffff'8000'0000'0000);

               if (memcmp(pHeader->signature, "APIC", 4) == 0)
               {
                    lpMadt = reinterpret_cast<MADT*>(pHeader);
                    debugging::DbgWrite(u8"Found MADT at {}\r\n", lpMadt);
               }
               else if (memcmp(pHeader->signature, "FADT", 4) == 0)
               {
                    g_fadt = reinterpret_cast<FADT*>(pHeader);
                    debugging::DbgWrite(u8"Found FADT at {}\r\n", g_fadt);
               }
               else if (memcmp(pHeader->signature, "MCFG", 4) == 0)
               {
                    g_mcfg = reinterpret_cast<MCFG*>(pHeader);
                    debugging::DbgWrite(u8"Found MCFG at {}\r\n", g_mcfg);
               }
               if (lpMadt != nullptr && g_mcfg != nullptr) break;
          }
     }

     if (lpMadt == nullptr || lpMadt->localApicAddress == 0)
     {
          debugging::DbgWrite(u8"[KiInitialiseLAPICTimer] lpMadt == nullptr || lpMadt->localApicAddress == 0\r\n");
     }

     std::uintptr_t lapicBasePhysical = __readmsr(0x1b); // IA32_APIC_BASE
     g_madt = lpMadt;
     volatile std::uint32_t* lapicBase = reinterpret_cast<volatile std::uint32_t*>(
         static_cast<std::uintptr_t>(g_madt->localApicAddress) + 0xffff'8000'0000'0000);

     debugging::DbgWrite(u8"LAPIC at {}\r\n", lapicBase);

#ifdef COMPILER_MSVC // MSVC vvv
     __writemsr(0x1b, __readmsr(0x1b) | (1uz << 11));
#elifdef COMPILER_CLANG // ^^^ MSVC / Clang vvv
     std::uint32_t low{};
     std::uint32_t high{};

     asm volatile("rdmsr\n"
                  "bts $11, %%eax\n"
                  "wrmsr"
                  : "=a"(low), "=d"(high)
                  : "c"(0x1B)
                  : "memory");
#endif                  // ^^^ Clang
     lapicBase[0x320 / 4] = (1 << 16);
     lapicBase[0x380 / 4] = 0;

     g_lapic = lapicBase;

     volatile std::uint32_t* ioApicBase =
         reinterpret_cast<volatile std::uint32_t*>(0xffff'8000'FEC0'0000ULL); // TODO: Please fix this shit

     debugging::DbgWrite(u8"IOAPIC at {}\r\n", ioApicBase);
     approximation = 0;
     const auto initialCount = KiGetLAPICEstimation() / 1000;

     lapicBase[0xF0 / 4] = 0x1FF;
     lapicBase[0xB0 / 4] = 0;
}
static std::atomic<std::uint32_t> g_lapicFrequency{};

void KeSetTimerFrequency(std::uint32_t frequency)
{
     g_lapicFrequency.store(frequency, std::memory_order::relaxed);

     volatile std::uint64_t* lapicBase = reinterpret_cast<volatile std::uint64_t*>(
         static_cast<std::uintptr_t>(g_madt->localApicAddress) + 0xffff'8000'0000'0000);

     const auto initialCount = KiGetLAPICEstimation() / frequency;
     lapicBase[0x320 / 4] = cpu::TimerIrqVector | (1 << 17);
     lapicBase[0x380 / 4] = initialCount;
     lapicTicks.store((KeReadHighResolutionTimer() * frequency) / KeReadHighResolutionTimerFrequency(),
                      std::memory_order::relaxed);
}
std::uint64_t KeReadLowResolutionTimer() { return lapicTicks.load(std::memory_order::relaxed); }
std::uint64_t KeReadHighResolutionTimer() { return KiReadHPET(); }
std::uint64_t KeReadLowResolutionTimerFrequency() { return g_lapicFrequency.load(std::memory_order::relaxed); }
std::uint64_t KeReadHighResolutionTimerFrequency() { return KiGetHPETFrequency(); }
std::uint64_t KeCurrentSystemTime()
{
     return (KeReadHighResolutionTimer() / (KeReadHighResolutionTimerFrequency() / 1000uz)) +
            (cpu::g_systemBootTimeOffsetSeconds * 1000uz);
}

void KiInitialiseInterrupts(std::uintptr_t acpiPhysical) { KiInitialiseLAPICTimer(acpiPhysical); }
#elifdef ARCH_ARM64

#include <atomic>
#include <cstdint>

static std::atomic<std::uint64_t> g_armTicks{};
static std::atomic<std::uint32_t> g_armFrequency{10};
cpu::InterruptVector cpu::TimerIrqVector = 30;

#ifdef COMPILER_MSVC // MSVC vvv
#include <arm64intr.h>
#endif

static inline std::uint64_t ReadCNTFRQ()
{
#ifdef COMPILER_MSVC
     return _ReadStatusReg(ARM64_SYSREG(3, 3, 14, 0, 0)); // CNTFRQ_EL0
#elif defined(COMPILER_CLANG)
     std::uint64_t freq{};
     asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     return freq;
#endif
}

static inline std::uint64_t ArmReadCNTPCT()
{
#ifdef COMPILER_MSVC
     return _ReadStatusReg(ARM64_SYSREG(3, 3, 14, 0, 1)); // CNTPCT_EL0
#elif defined(COMPILER_CLANG)
     std::uint64_t cnt{};
     asm volatile("mrs %0, cntpct_el0" : "=r"(cnt));
     return cnt;
#endif
}

std::uint64_t KeReadLowResolutionTimer() { return g_armTicks.load(std::memory_order::relaxed); }
std::uint64_t KeReadHighResolutionTimer() { return ArmReadCNTPCT(); }
std::uint64_t KeReadLowResolutionTimerFrequency() { return g_armFrequency.load(std::memory_order::relaxed); }
std::uint64_t KeReadHighResolutionTimerFrequency() { return ReadCNTFRQ(); }

struct InterruptFrame
{
     std::uint64_t x0;
     std::uint64_t x1;
     std::uint64_t x2;
     std::uint64_t x3;
     std::uint64_t x4;
     std::uint64_t x5;
     std::uint64_t x6;
     std::uint64_t x7;
     std::uint64_t x8;
     std::uint64_t x9;
     std::uint64_t x10;
     std::uint64_t x11;
     std::uint64_t x12;
     std::uint64_t x13;
     std::uint64_t x14;
     std::uint64_t x15;
     std::uint64_t x16;
     std::uint64_t x17;
     std::uint64_t x18;

     std::uint64_t x19;
     std::uint64_t x20;
     std::uint64_t x21;
     std::uint64_t x22;
     std::uint64_t x23;
     std::uint64_t x24;
     std::uint64_t x25;
     std::uint64_t x26;
     std::uint64_t x27;
     std::uint64_t x28;

     std::uint64_t fp;
     std::uint64_t lr;

     std::uint64_t vector;
     std::uint64_t esr;
     std::uint64_t far;
     std::uint64_t svc;
     std::uint64_t sp;
};
static_assert(sizeof(InterruptFrame) == 36uz * sizeof(std::uint64_t));

struct ARM64InterruptFrame : cpu::IInterruptFrame // NOLINT
{
     InterruptFrame* frame;
     explicit ARM64InterruptFrame(InterruptFrame* frame) : frame(frame) {}

     [[nodiscard]] std::uint64_t GetVector() const override { return frame->vector; }
     [[nodiscard]] cpu::InterruptError GetError() const override
     {
          switch (frame->vector)
          {
          case 0x03: return cpu::InterruptError::MemoryExecuteFault;
          case 0x04:
          {
               std::uint32_t w = frame->esr & 0x3F;
               bool write = (w & 0b10) != 0;
               return write ? cpu::InterruptError::MemoryWriteFault : cpu::InterruptError::MemoryReadFault;
          }
          case 0x15: return cpu::InterruptError::SupervisorCall;
          case 0x20: return cpu::InterruptError::HardwareInterrupt;
          case 0x21: return cpu::InterruptError::FastInterrupt;
          case 0x25:
          {
               std::uint32_t iss = frame->esr & 0xFFFFFF;
               bool write = (iss >> 6) & 1;
               bool user = (iss >> 5) & 1;
               bool dfscPresent = true;

               return write ? cpu::InterruptError::MemoryWriteFault : cpu::InterruptError::MemoryReadFault;
          }
          default: return cpu::InterruptError::Unknown;
          }
     }
     [[nodiscard]] std::uintptr_t GetExtra() const override { return frame->svc; };

     [[nodiscard]] std::uint64_t GetInstructionPointer() const override { return frame->lr; }
     [[nodiscard]] std::uint64_t GetStackPointer() const override { return frame->sp; }
     [[nodiscard]] std::uint64_t GetFaultingAddress() const override { return frame->far; }

     void DumpRegisters() const override
     {
          debugging::DbgWrite(u8"x0  {}\r\n", reinterpret_cast<void*>(frame->x0));
          debugging::DbgWrite(u8"x1  {}\r\n", reinterpret_cast<void*>(frame->x1));
          debugging::DbgWrite(u8"x2  {}\r\n", reinterpret_cast<void*>(frame->x2));
          debugging::DbgWrite(u8"x3  {}\r\n", reinterpret_cast<void*>(frame->x3));
          debugging::DbgWrite(u8"x4  {}\r\n", reinterpret_cast<void*>(frame->x4));
          debugging::DbgWrite(u8"x5  {}\r\n", reinterpret_cast<void*>(frame->x5));
          debugging::DbgWrite(u8"x6  {}\r\n", reinterpret_cast<void*>(frame->x6));
          debugging::DbgWrite(u8"x7  {}\r\n", reinterpret_cast<void*>(frame->x7));
          debugging::DbgWrite(u8"x8  {}\r\n", reinterpret_cast<void*>(frame->x8));
          debugging::DbgWrite(u8"x9  {}\r\n", reinterpret_cast<void*>(frame->x9));
          debugging::DbgWrite(u8"x10 {}\r\n", reinterpret_cast<void*>(frame->x10));
          debugging::DbgWrite(u8"x11 {}\r\n", reinterpret_cast<void*>(frame->x11));
          debugging::DbgWrite(u8"x12 {}\r\n", reinterpret_cast<void*>(frame->x12));
          debugging::DbgWrite(u8"x13 {}\r\n", reinterpret_cast<void*>(frame->x13));
          debugging::DbgWrite(u8"x14 {}\r\n", reinterpret_cast<void*>(frame->x14));
          debugging::DbgWrite(u8"x15 {}\r\n", reinterpret_cast<void*>(frame->x15));
          debugging::DbgWrite(u8"x16 {}\r\n", reinterpret_cast<void*>(frame->x16));
          debugging::DbgWrite(u8"x17 {}\r\n", reinterpret_cast<void*>(frame->x17));
          debugging::DbgWrite(u8"x18 {}\r\n", reinterpret_cast<void*>(frame->x18));
          debugging::DbgWrite(u8"x19 {}\r\n", reinterpret_cast<void*>(frame->x19));
          debugging::DbgWrite(u8"x20 {}\r\n", reinterpret_cast<void*>(frame->x20));
          debugging::DbgWrite(u8"x21 {}\r\n", reinterpret_cast<void*>(frame->x21));
          debugging::DbgWrite(u8"x22 {}\r\n", reinterpret_cast<void*>(frame->x22));
          debugging::DbgWrite(u8"x23 {}\r\n", reinterpret_cast<void*>(frame->x23));
          debugging::DbgWrite(u8"x24 {}\r\n", reinterpret_cast<void*>(frame->x24));
          debugging::DbgWrite(u8"x25 {}\r\n", reinterpret_cast<void*>(frame->x25));
          debugging::DbgWrite(u8"x26 {}\r\n", reinterpret_cast<void*>(frame->x26));
          debugging::DbgWrite(u8"x27 {}\r\n", reinterpret_cast<void*>(frame->x27));
          debugging::DbgWrite(u8"x28 {}\r\n", reinterpret_cast<void*>(frame->x28));
          debugging::DbgWrite(u8" fp {}\r\n", reinterpret_cast<void*>(frame->fp));
          debugging::DbgWrite(u8" lr {}\r\n", reinterpret_cast<void*>(frame->lr));
          debugging::DbgWrite(u8"vec {}\r\n", reinterpret_cast<void*>(frame->vector));
          debugging::DbgWrite(u8"esr {}\r\n", reinterpret_cast<void*>(frame->esr));
          debugging::DbgWrite(u8"far {}\r\n", reinterpret_cast<void*>(frame->far));
          if (frame->vector == 0x15) debugging::DbgWrite(u8"SVC {}\r\n", frame->svc);
     }
};

static inline void ArmSetTimerTicks(std::uint64_t ticks)
{
#ifdef COMPILER_MSVC
     _WriteStatusReg(ARM64_SYSREG(3, 0, 14, 2, 0), ticks);
     __isb(0xf);
#else
     asm volatile("msr cntp_tval_el0, %0" : : "r"(ticks));
     asm volatile("isb");
#endif
}

static std::atomic<std::uint64_t> g_nextTimerDeadline{0};

static inline void ArmSetTimerDeadline(std::uint64_t cval)
{
#ifdef COMPILER_MSVC
     _WriteStatusReg(ARM64_SYSREG(3, 3, 14, 2, 2), cval);
     __dsb(0xF);
     __isb(0xF);
#else
     asm volatile("msr cntp_cval_el0, %0\n"
                  "dsb sy\n"
                  "isb\n"
                  :
                  : "r"(cval)
                  : "memory");
#endif
}

static inline void ArmReloadTimer(std::uint32_t frequency)
{
     const std::uint64_t interval = ReadCNTFRQ() / frequency;
     const std::uint64_t now = ArmReadCNTPCT();

     std::uint64_t last = g_nextTimerDeadline.load(std::memory_order::relaxed);

     while (last <= now) last += interval;

     g_nextTimerDeadline.store(last, std::memory_order::relaxed);
     ArmSetTimerDeadline(last);
}

static std::uintptr_t g_gicdPhysBase = 0;
static std::uintptr_t g_gicrPhysBase = 0;

static inline std::uint32_t GicReadIAR1()
{
     std::uint32_t iar{};
#ifdef COMPILER_MSVC
     iar = static_cast<std::uint32_t>(_ReadStatusReg(ARM64_SYSREG(3, 0, 12, 12, 0)));
#else
     asm volatile("mrs %x0, icc_iar1_el1" : "=r"(iar));
#endif
     return iar;
}

static inline void GicWriteEOIR1(std::uint32_t intid)
{
#ifdef COMPILER_MSVC
     _WriteStatusReg(ARM64_SYSREG(3, 0, 12, 12, 1), intid);
     __isb(0xf);
#else
     asm volatile("msr icc_eoir1_el1, %x0" : : "r"(static_cast<std::uint64_t>(intid)));
     asm volatile("isb");
#endif
}
void KeAcknowledgeInterrupt() {}

static inline void ArmDisablePhysicalTimer()
{
     std::uint64_t ctl{};
#ifdef COMPILER_MSVC
     ctl = _ReadStatusReg(ARM64_SYSREG(3, 3, 14, 2, 1));
     ctl &= ~1; // ENABLE=0
     _WriteStatusReg(ARM64_SYSREG(3, 3, 14, 2, 1), ctl);
     __isb(0xf);
#else
     asm volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
     ctl &= ~1ULL;
     asm volatile("msr cntp_ctl_el0, %0" : : "r"(ctl));
     asm volatile("isb");
#endif
}

static inline void ArmEnablePhysicalTimer()
{
     std::uint64_t ctl{};
#ifdef COMPILER_MSVC
     ctl = _ReadStatusReg(ARM64_SYSREG(3, 3, 14, 2, 1));
     ctl |= 1;  // ENABLE
     ctl &= ~2; // IMASK = 0 (unmasked)
     _WriteStatusReg(ARM64_SYSREG(3, 3, 14, 2, 1), ctl);
     __isb(0xf);
#else
     asm volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
     ctl |= 1;
     ctl &= ~2;
     asm volatile("msr cntp_ctl_el0, %0" : : "r"(ctl));
     asm volatile("isb");
#endif
}

extern "C" InterruptFrame* KeHandleInterruptFrame(InterruptFrame* frame)
{
     operations::DisableInterrupts();
     if (frame->vector == 5)
     {
          const std::uint32_t iar = GicReadIAR1();

          if (iar != static_cast<std::uint32_t>(cpu::TimerIrqVector))

               if (iar == 1023u)
               {
                    debugging::DbgWrite(u8"[GIC] Spurious IRQ\r\n");
                    operations::EnableInterrupts();
                    return frame;
               }

          if (iar == static_cast<std::uint32_t>(cpu::TimerIrqVector))
          {
               static std::atomic<std::uint64_t> lastPct{0};
               std::uint64_t now = ArmReadCNTPCT();
               std::uint64_t last = lastPct.exchange(now);

               const std::uint64_t interval = ReadCNTFRQ() / g_armFrequency.load(std::memory_order::relaxed);
               std::uint64_t next = g_nextTimerDeadline.load(std::memory_order::relaxed);
               while (next <= now) next += interval;
               g_nextTimerDeadline.store(next, std::memory_order::relaxed);
               ArmSetTimerDeadline(next);

               GicWriteEOIR1(iar);

               frame->vector = 0x20;
               frame->svc = iar;
               frame->esr = 0;
               frame->far = 0;
               ARM64InterruptFrame vFrame{frame};
               HandleInterrupt(vFrame);

               operations::EnableInterrupts();
               return vFrame.frame;
          }

          GicWriteEOIR1(iar);

          frame->vector = 0x20;
          frame->svc = iar;
          frame->esr = 0;
          frame->far = 0;
          ARM64InterruptFrame vFrame{frame};
          HandleInterrupt(vFrame);
          operations::EnableInterrupts();
          return vFrame.frame;
     }
#ifdef COMPILER_MSVC    // MSVC vvv
     std::uint64_t esr = _ReadStatusReg(ARM64_SYSREG(3, 0, 5, 2, 0));
     std::uint64_t far = _ReadStatusReg(ARM64_SYSREG(3, 0, 6, 0, 0));
#elifdef COMPILER_CLANG // ^^^ MSVC / Clang vvv
     std::uint64_t esr{};
     std::uint64_t far{};

     asm volatile("mrs %0, esr_el1\n"
                  "mrs %1, far_el1"
                  : "=r"(esr), "=r"(far));
#endif

     frame->esr = esr;
     frame->far = far;
     frame->vector = (esr >> 26) & 0x3F;

     if (frame->vector == 0x15) frame->svc = esr & 0xFFFF;
     else
          frame->svc = 0xFFFF;
     ARM64InterruptFrame vFrame{frame};
     HandleInterrupt(vFrame);
     operations::EnableInterrupts();
     return vFrame.frame;
}

struct PPTT
{
     ACPISDTHeader header;
     std::uint32_t reserved;
     std::uint8_t entries[]; // NOLINT
};

struct GTDT
{
     ACPISDTHeader header;
     std::uint32_t blockCount;
     std::uint64_t timerBlockAddress;
     std::uint32_t flags;
     std::uint32_t reserved;
     std::uint8_t entries[]; // NOLINT
};

struct SPCR
{
     ACPISDTHeader header;
     std::uint8_t interfaceType;
     std::uint8_t reserved0;
     std::uint16_t reserved1;
     std::uint64_t baseAddress;
     std::uint8_t interruptType;
     std::uint8_t irq;
     std::uint32_t globalSystemInterrupt;
     std::uint8_t baudRate;
     std::uint8_t parity;
     std::uint8_t stopBits;
     std::uint8_t flowControl;
     std::uint8_t terminalType;
     std::uint8_t reserved2[3]; // NOLINT
     std::uint32_t pciDeviceId;
     std::uint32_t pciVendorId;
     std::uint8_t pciBus;
     std::uint8_t pciDevice;
     std::uint8_t pciFunction;
     std::uint8_t pciFlags;
     std::uint8_t pciSegment;
     std::uint8_t reserved3;
};

struct DBG2
{
     ACPISDTHeader header;
     std::uint16_t infoCount;
     std::uint16_t reserved;
     std::uint8_t entries[]; // NOLINT
};

struct IORT
{
     ACPISDTHeader header;
     std::uint32_t nodeCount;
     std::uint32_t nodeOffset;
     std::uint32_t reserved;
};
#pragma pack(push, 1)
struct IORTNodeHeader
{
     std::uint8_t type;
     std::uint16_t length;
     std::uint8_t revision;
     char identifier[4]; // NOLINT
     std::uint32_t numberOfMappings;
     std::uint32_t refToMappings;
};
#pragma pack(pop)

struct GICv3CPUInterfaceNode
{
     IORTNodeHeader header;
     std::uint32_t id;
     std::uint32_t reserved;
     std::uint64_t baseAddress;
     std::uint32_t gicVersion;
     std::uint32_t reserved1;
     std::uint64_t gicRedistributorBase;
};

struct BGRT
{
     ACPISDTHeader header;
     std::uint16_t version;
     std::uint8_t status;
     std::uint8_t imageType;
     std::uint64_t imageAddress;
     std::uint32_t imageOffsetX;
     std::uint32_t imageOffsetY;
};

MADT* g_madt{};
MCFG* g_mcfg{};
PPTT* g_pptt{};
GTDT* g_gtdt{};
SPCR* g_spcr{};
DBG2* g_dbg2{};
IORT* g_iort{};
BGRT* g_bgrt{};

constexpr std::uint8_t MADT_TYPE_GICC = 11;
constexpr std::uint8_t MADT_TYPE_GICD = 12;
constexpr std::uint8_t MADT_TYPE_GICR = 14;

#pragma pack(push, 1)
struct MADTEntryGICC
{
     MADTEntry header; // type = 11, length = 80
     std::uint16_t reserved0;
     std::uint32_t cpuInterfaceNumber;
     std::uint32_t acpiProcessorUid;
     std::uint32_t flags;
     std::uint32_t parkingProtocolVersion;
     std::uint32_t performanceInterruptGsiv;
     std::uint64_t parkedAddress;
     std::uint64_t physicalBaseAddress;
     std::uint64_t gicv;
     std::uint64_t gich;
     std::uint32_t vgicMaintenanceInterrupt;
     std::uint64_t gicRedistributorBaseAddress;
     std::uint64_t mpidr;
     std::uint8_t processorPowerEfficiencyClass;
     std::uint8_t reserved1;
     std::uint16_t speOverflowInterrupt;
};

struct MADTEntryGICD
{
     MADTEntry header; // type = 12, length = 24
     std::uint16_t reserved0;
     std::uint32_t gicId;
     std::uint64_t physicalBaseAddress;
     std::uint32_t systemVectorBase;
     std::uint8_t gicVersion;   // 1=GICv1 … 4=GICv4
     std::uint8_t reserved1[3]; // NOLINT
};

struct MADTEntryGICR
{
     MADTEntry header; // type = 14, length = 16
     std::uint16_t reserved0;
     std::uint64_t discoveryRangeBaseAddress;
     std::uint32_t discoveryRangeLength;
};
#pragma pack(pop)

static inline void ArmDSBISB()
{
#ifdef COMPILER_MSVC
     __dsb(0xF);
#elifdef COMPILER_CLANG
     asm volatile("dsb sy\n isb" ::: "memory");
#endif
}

static void KiParseMADTForGIC(MADT* madt)
{
     g_gicrPhysBase = 0;

     const std::uint8_t* p = madt->entries;
     const std::uint8_t* end = reinterpret_cast<const std::uint8_t*>(madt) + madt->header.length;

     while (p < end)
     {
          const MADTEntry* e = reinterpret_cast<const MADTEntry*>(p);
          if (e->length < 2) break;

          switch (e->type)
          {
          case MADT_TYPE_GICD:
          {
               const auto* gicd = reinterpret_cast<const MADTEntryGICD*>(e);
               g_gicdPhysBase = static_cast<std::uintptr_t>(gicd->physicalBaseAddress);
               debugging::DbgWrite(u8"MADT: GICD phys={}\r\n", reinterpret_cast<void*>(g_gicdPhysBase));
               break;
          }
          case MADT_TYPE_GICR:
          {
               if (g_gicrPhysBase == 0)
               {
                    const auto* gicr = reinterpret_cast<const MADTEntryGICR*>(e);
                    g_gicrPhysBase = static_cast<std::uintptr_t>(gicr->discoveryRangeBaseAddress);
                    debugging::DbgWrite(u8"MADT: GICR phys={}\r\n", reinterpret_cast<void*>(g_gicrPhysBase));
               }
               break;
          }
          case MADT_TYPE_GICC:
          {
               const auto* gicc = reinterpret_cast<const MADTEntryGICC*>(e);
               if (g_gicrPhysBase == 0 && gicc->gicRedistributorBaseAddress != 0)
               {
                    g_gicrPhysBase = static_cast<std::uintptr_t>(gicc->gicRedistributorBaseAddress);
                    debugging::DbgWrite(u8"MADT: GICR physical (from GICC)={}\r\n",
                                        reinterpret_cast<void*>(g_gicrPhysBase));
               }
               break;
          }
          default: break;
          }

          p += e->length;
     }
}

static volatile std::uint32_t* g_gicd = nullptr;
static volatile std::uint32_t* g_gicr = nullptr;

constexpr std::uint32_t GICD_CTLR = 0x000 / 4;
constexpr std::uint32_t GICD_ISENABLER = 0x100 / 4;
constexpr std::uint32_t GICD_ICENABLER = 0x180 / 4;
constexpr std::uint32_t GICD_IPRIORITYR = 0x400 / 4;
constexpr std::uint32_t GICD_ITARGETSR = 0x800 / 4;
constexpr std::uint32_t GICD_ICFGR = 0xC00 / 4;
constexpr std::uint32_t GICD_IROUTER = 0x6000 / 8;

constexpr std::uint32_t GICR_CTLR = 0x000 / 4;
constexpr std::uint32_t GICR_WAKER = 0x014 / 4;

constexpr std::uint32_t GICR_SGI_BASE = 0x10000;
constexpr std::uint32_t GICR_ISENABLER0 = (GICR_SGI_BASE + 0x100) / 4;
constexpr std::uint32_t GICR_ICENABLER0 = (GICR_SGI_BASE + 0x180) / 4;
constexpr std::uint32_t GICR_IPRIORITYR = (GICR_SGI_BASE + 0x400) / 4;
constexpr std::uint32_t GICR_ICFGR1 = (GICR_SGI_BASE + 0xC04) / 4;

static void KiInitialiseGICv3()
{
     constexpr std::uintptr_t HhdmOffset = 0xffff'8000'0000'0000ULL;

     if (g_gicdPhysBase == 0)
     {
          debugging::DbgWrite(u8"[GIC] No GICD found in MADT\r\n");
          return;
     }
     if (g_gicrPhysBase == 0)
     {
          debugging::DbgWrite(u8"[GIC] No GICR found in MADT\r\n");
          return;
     }

     g_gicd = reinterpret_cast<volatile std::uint32_t*>(g_gicdPhysBase + HhdmOffset);
     g_gicr = reinterpret_cast<volatile std::uint32_t*>(g_gicrPhysBase + HhdmOffset);

     volatile std::uint32_t* gicrRd = g_gicr;
     volatile std::uint32_t* gicrSgi = reinterpret_cast<volatile std::uint32_t*>(g_gicrPhysBase + HhdmOffset + 0x10000);

     g_gicd[0x000 / 4] = 0;
     ArmDSBISB();
     for (std::uint32_t i = 1; i < 32; i++) g_gicd[(0x180 / 4) + i] = 0xFFFFFFFF;
     ArmDSBISB();

     g_gicd[0x000 / 4] = (1u << 5) | (1u << 4) | (1u << 2) | (1u << 1);
     ArmDSBISB();
     std::uint32_t waker = gicrRd[0x014 / 4];
     waker &= ~(1u << 1);
     gicrRd[0x014 / 4] = waker;
     ArmDSBISB();

     g_gicd[(0x180 / 4) + 1] = (1u << 1);
     ArmDSBISB();

     while (gicrRd[0x014 / 4] & (1u << 2))
     {
#ifdef COMPILER_MSVC
          __isb(0xf);
#elifdef COMPILER_CLANG
          asm volatile("isb");
#endif
     }
     debugging::DbgWrite(u8"[GIC] Redistributor awake {}\r\n", gicrSgi);

     gicrSgi[0x080 / 4] = 0xFFFFFFFF;
     gicrSgi[0x480 / 4] = 0x00000000;
     ArmDSBISB();

     gicrSgi[0x180 / 4] = ~(1u << 30);
     gicrSgi[0x100 / 4] = (1u << 30);
     ArmDSBISB();

     volatile std::uint8_t* priBase =
         reinterpret_cast<volatile std::uint8_t*>(g_gicrPhysBase + HhdmOffset + 0x10000 + 0x400);
     priBase[30] = 0x80;
     ArmDSBISB();

     std::uint32_t icfgr1 = gicrSgi[0xC04 / 4];
     icfgr1 &= ~(0b11u << ((30 - 16) * 2));
     gicrSgi[0xC04 / 4] = icfgr1;
     ArmDSBISB();

#ifdef COMPILER_MSVC
     std::uint64_t icc_ctlr = _ReadStatusReg(ARM64_SYSREG(3, 0, 12, 12, 4));
     icc_ctlr &= ~1ULL;
     _WriteStatusReg(ARM64_SYSREG(3, 0, 12, 12, 4), icc_ctlr);
     __isb(0xF);
     _WriteStatusReg(ARM64_SYSREG(3, 0, 4, 6, 0), 0xFFULL);
     __isb(0xF);

     _WriteStatusReg(ARM64_SYSREG(3, 0, 12, 12, 7), 1ULL);
     __isb(0xF);

     std::uint64_t daif = _ReadStatusReg(ARM64_SYSREG(3, 3, 4, 2, 1)); // DAIF register
     daif &= ~(1 << 9);                                                // clear I bit
     _WriteStatusReg(ARM64_SYSREG(3, 3, 4, 2, 1), daif);
     __isb(0xF);
#elifdef COMPILER_CLANG

     std::uint64_t icc_ctlr{};
     asm volatile("mrs %0, icc_ctlr_el1" : "=r"(icc_ctlr));
     icc_ctlr &= ~1ULL;
     asm volatile("msr icc_ctlr_el1, %0\nisb" : : "r"(icc_ctlr));

     asm volatile("msr icc_pmr_el1, %x0\nisb" : : "r"(0xFFULL));
     asm volatile("msr icc_igrpen1_el1, %x0\nisb" : : "r"(1ULL));
     asm volatile("msr daifclr, #2\nisb");
#endif

     debugging::DbgWrite(u8"[GIC] GICv3 initialised\r\n");
}

void KeSetTimerFrequency(std::uint32_t frequency)
{
     g_armFrequency.store(frequency, std::memory_order::relaxed);
     g_armTicks.store((KeReadHighResolutionTimer() * frequency) / KeReadHighResolutionTimerFrequency(),
                      std::memory_order::relaxed);

     const std::uint64_t cntFrq = ReadCNTFRQ();
     const std::uint64_t interval = cntFrq / frequency;
     const std::uint64_t now = ArmReadCNTPCT();

     debugging::DbgWrite(u8"[Timer] cntfrq={} freq={} interval={} now={}\r\n", cntFrq, frequency, interval, now);

     if (cntFrq == 0 || interval == 0)
     {
          debugging::DbgWrite(u8"[Timer] BAD FREQUENCY\r\n");
          return;
     }
#ifdef COMPILER_MSVC
     std::uint64_t ctl = _ReadStatusReg(ARM64_SYSREG(3, 3, 14, 2, 1));
     ctl |= 2ULL;
     _WriteStatusReg(ARM64_SYSREG(3, 3, 14, 2, 1), ctl);
     __isb(0xF);
#elifdef COMPILER_CLANG
     std::uint64_t ctl{};
     asm volatile("mrs %0, cntp_ctl_el0" : "=r"(ctl));
     ctl |= 2ULL;
     asm volatile("msr cntp_ctl_el0, %0\nisb" : : "r"(ctl));
#endif

     const std::uint64_t deadline = now + interval;
     g_nextTimerDeadline.store(deadline, std::memory_order::relaxed);
     ArmSetTimerDeadline(deadline);

     ctl |= 1ULL;
     ctl &= ~2ULL;
#ifdef COMPILER_MSVC
     _WriteStatusReg(ARM64_SYSREG(3, 3, 14, 2, 1), ctl);
     __isb(0xF);
#elifdef COMPILER_CLANG
     asm volatile("msr cntp_ctl_el0, %0\nisb" : : "r"(ctl));
#endif
}

void KiInitialiseInterrupts(std::uintptr_t acpiPhysical)
{
     g_armFrequency.store(100);
     g_nextTimerDeadline.store(0);

     RSDPDescriptor* lpRsp = reinterpret_cast<RSDPDescriptor*>(acpiPhysical + 0xffff'8000'0000'0000);
     MADT* lpMadt = nullptr;
     debugging::DbgWrite(u8"RSDPv{} at {}\r\n", lpRsp->revision, lpRsp);

     if (lpRsp->revision >= 2)
     {
          RSDPDescriptor2* lpRsp20 = reinterpret_cast<RSDPDescriptor2*>(lpRsp);

          XSDT* pXsdt = reinterpret_cast<XSDT*>(lpRsp20->xsdtAddress + 0xffff'8000'0000'0000);
          debugging::DbgWrite(u8"XSDTv{} at {}\r\n", pXsdt->header.revision, pXsdt);

          const std::uint32_t entryCount =
              (pXsdt->header.length - sizeof(struct ACPISDTHeader)) / sizeof(std::uintptr_t);
          debugging::DbgWrite(u8"Entries = {}\r\n", entryCount);

          for (std::size_t i = 0; i < entryCount; i++)
          {
               std::uint32_t upper = static_cast<std::uint32_t>(pXsdt->tablePointers[i] >> 32);
               ACPISDTHeader* pHeader = reinterpret_cast<ACPISDTHeader*>(upper + 0xffff'8000'0000'0000);

               if (upper == 0) break;

               if (memcmp(pHeader->signature, "APIC", 4) == 0)
               {
                    lpMadt = reinterpret_cast<MADT*>(pHeader);
                    debugging::DbgWrite(u8"Found MADT at {}\r\n", lpMadt);
               }
               else if (memcmp(pHeader->signature, "MCFG", 4) == 0)
               {
                    g_mcfg = reinterpret_cast<MCFG*>(pHeader);
                    debugging::DbgWrite(u8"Found MCFG at {}\r\n", g_mcfg);
               }
               else if (memcmp(pHeader->signature, "PPTT", 4) == 0)
               {
                    g_pptt = reinterpret_cast<PPTT*>(pHeader);
                    debugging::DbgWrite(u8"Found PPTT at {}\r\n", g_pptt);
               }
               else if (memcmp(pHeader->signature, "GTDT", 4) == 0)
               {
                    g_gtdt = reinterpret_cast<GTDT*>(pHeader);
                    debugging::DbgWrite(u8"Found GTDT at {}\r\n", g_gtdt);
               }
               else if (memcmp(pHeader->signature, "SPCR", 4) == 0)
               {
                    g_spcr = reinterpret_cast<SPCR*>(pHeader);
                    debugging::DbgWrite(u8"Found SPCR at {}\r\n", g_spcr);
               }
               else if (memcmp(pHeader->signature, "DBG2", 4) == 0)
               {
                    g_dbg2 = reinterpret_cast<DBG2*>(pHeader);
                    debugging::DbgWrite(u8"Found DBG2 at {}\r\n", g_dbg2);
               }
               else if (memcmp(pHeader->signature, "BGRT", 4) == 0)
               {
                    g_bgrt = reinterpret_cast<BGRT*>(pHeader);
                    debugging::DbgWrite(u8"Found BGRT at {}\r\n", g_bgrt);
               }
               else if (memcmp(pHeader->signature, "IORT", 4) == 0)
               {
                    g_iort = reinterpret_cast<IORT*>(pHeader);
                    debugging::DbgWrite(u8"Found IORT at {}\r\n", g_iort);
               }
               else
                    debugging::DbgWrite(u8"Unknown entry {} ('{}')\r\n", pHeader,
                                        reinterpret_cast<const char8_t (&)[5]>(pHeader->signature)); // NOLINT
          }
     }
     else
     {
          RSDT* lpRsdt = reinterpret_cast<RSDT*>(lpRsp->rsdtAddress + 0xffff'8000'0000'0000);

          const std::uint32_t entryCount = (lpRsdt->header.length - sizeof(ACPISDTHeader)) / sizeof(std::uint32_t);

          for (std::size_t i = 0; i < entryCount; i++)
          {
               std::uint32_t address = lpRsdt->tablePointers[i];
               ACPISDTHeader* pHeader = reinterpret_cast<ACPISDTHeader*>(address + 0xffff'8000'0000'0000);

               if (address == 0) break;

               if (memcmp(pHeader->signature, "APIC", 4) == 0)
               {
                    lpMadt = reinterpret_cast<MADT*>(pHeader);
                    debugging::DbgWrite(u8"Found MADT at {}\r\n", lpMadt);
               }
               else if (memcmp(pHeader->signature, "MCFG", 4) == 0)
               {
                    g_mcfg = reinterpret_cast<MCFG*>(pHeader);
                    debugging::DbgWrite(u8"Found MCFG at {}\r\n", g_mcfg);
               }
               else if (memcmp(pHeader->signature, "PPTT", 4) == 0)
               {
                    g_pptt = reinterpret_cast<PPTT*>(pHeader);
                    debugging::DbgWrite(u8"Found PPTT at {}\r\n", g_pptt);
               }
               else if (memcmp(pHeader->signature, "GTDT", 4) == 0)
               {
                    g_gtdt = reinterpret_cast<GTDT*>(pHeader);
                    debugging::DbgWrite(u8"Found GTDT at {}\r\n", g_gtdt);
               }
               else if (memcmp(pHeader->signature, "SPCR", 4) == 0)
               {
                    g_spcr = reinterpret_cast<SPCR*>(pHeader);
                    debugging::DbgWrite(u8"Found SPCR at {}\r\n", g_spcr);
               }
               else if (memcmp(pHeader->signature, "DBG2", 4) == 0)
               {
                    g_dbg2 = reinterpret_cast<DBG2*>(pHeader);
                    debugging::DbgWrite(u8"Found DBG2 at {}\r\n", g_dbg2);
               }
               else if (memcmp(pHeader->signature, "BGRT", 4) == 0)
               {
                    g_bgrt = reinterpret_cast<BGRT*>(pHeader);
                    debugging::DbgWrite(u8"Found BGRT at {}\r\n", g_bgrt);
               }
               else
                    debugging::DbgWrite(u8"Unknown entry {} ('{}')\r\n", pHeader,
                                        reinterpret_cast<const char8_t (&)[5]>(pHeader->signature)); // NOLINT
          }
     }

     g_madt = lpMadt;

     if (lpMadt == nullptr)
     {
          debugging::DbgWrite(u8"[KiInitialiseInterrupts] MADT not found!\r\n");
          return;
     }

     KiParseMADTForGIC(lpMadt);
     KiInitialiseGICv3();
     KeSetTimerFrequency(64);
}

std::uint64_t KeCurrentSystemTime()
{
     return (KeReadHighResolutionTimer() / (KeReadHighResolutionTimerFrequency() / 1000uz)) +
            (cpu::g_systemBootTimeOffsetSeconds * 1000uz);
}

#endif
void HandleInterrupt(cpu::IInterruptFrame& frame)
{
     if (frame.GetError() == cpu::InterruptError::HardwareInterrupt)
     {
          if (frame.GetExtra() == cpu::TimerIrqVector)
          {
#ifdef ARCH_X8664
               lapicTicks.fetch_add(1, std::memory_order::relaxed);
#elifdef ARCH_ARM64
               g_armTicks.fetch_add(1, std::memory_order::relaxed);
#endif
          }
          else
          {
               debugging::DbgWrite(u8"int = {}\r\n", frame.GetVector());
               operations::DisableInterrupts();
               while (true) operations::Halt();
          }

          return;
     }

     frame.DumpRegisters();
     debugging::DbgWrite(u8"Vector  = {}\r\n", reinterpret_cast<void*>(frame.GetVector()));
     debugging::DbgWrite(u8"Error   = {}\r\n", ToString(frame.GetError()));
     debugging::DbgWrite(u8"Where   = {}\r\n", reinterpret_cast<void*>(frame.GetInstructionPointer()));
     debugging::DbgWrite(u8"Stack   = {}\r\n", reinterpret_cast<void*>(frame.GetStackPointer()));
     debugging::DbgWrite(u8"Address = {}\r\n", reinterpret_cast<void*>(frame.GetFaultingAddress()));

     operations::DisableInterrupts();
     while (true) operations::Halt();
}
