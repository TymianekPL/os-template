#pragma once

#include <cstdint>

#pragma pack(push, 1)
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
     MCFGAllocationClass allocationClasses[]; // NOLINT
};

#pragma pack(pop)

namespace cpu
{
     extern std::uint64_t g_systemBootTimeOffsetSeconds;
     using InterruptVector = std::uint16_t;
     extern InterruptVector TimerIrqVector;

     enum class InterruptError : std::uint8_t
     {
          None,
          DivideByZero,
          InvalidInstruction,
          MemoryReadFault,
          MemoryWriteFault,
          MemoryExecuteFault,
          ProtectionFault,
          SupervisorCall,
          HardwareInterrupt,
          FastInterrupt,
          Unknown
     };

     inline const char8_t* ToString(const InterruptError error)
     {
          switch (error)
          {
          case InterruptError::None: return u8"none";
          case InterruptError::DivideByZero: return u8"divide by zero";
          case InterruptError::InvalidInstruction: return u8"invalid instruction";
          case InterruptError::MemoryReadFault: return u8"read fault";
          case InterruptError::MemoryWriteFault: return u8"write fault";
          case InterruptError::MemoryExecuteFault: return u8"execute fault";
          case InterruptError::ProtectionFault: return u8"protection fault";
          case InterruptError::SupervisorCall: return u8"supervisor call";
          case InterruptError::HardwareInterrupt: return u8"hardware int";
          case InterruptError::FastInterrupt: return u8"fast int";
          default: return u8"?";
          }
     }

     struct IInterruptFrame // NOLINT(cppcoreguidelines-virtual-class-destructor)
     {
          [[nodiscard]] virtual std::uint64_t GetVector() const = 0;
          [[nodiscard]] virtual InterruptError GetError() const = 0;

          [[nodiscard]] virtual std::uintptr_t GetInstructionPointer() const = 0;
          [[nodiscard]] virtual std::uintptr_t GetStackPointer() const = 0;
          [[nodiscard]] virtual std::uintptr_t GetFaultingAddress() const = 0;
          [[nodiscard]] virtual std::uintptr_t GetExtra() const { return GetVector(); }
          [[nodiscard]] virtual void* GetContext() const { return nullptr; }
          virtual void SetContext(void* context) {}

          virtual void DumpRegisters() const = 0;
     };

     constexpr auto PassiveIrql = 0;
     constexpr auto ApcIrql = 1;
     constexpr auto DispatchIrql = 2;
     constexpr auto DeviceLowIrql = 3;
     constexpr auto DeviceBelowNormalIrql = 4;
     constexpr auto DeviceNormalIrql = 5;
     constexpr auto DeviceHighIrql = 6;
     constexpr auto NetworkIrql = 7;
     constexpr auto StorageIrql = 8;
     constexpr auto DeviceRealTimeIrql = 9;
     constexpr auto IPIIrql = 10;
     constexpr auto ClockIrql = 11;

     enum struct IRQL : std::uint8_t
     {
          Passive = PassiveIrql,
          Apc = ApcIrql,
          Dispatch = DispatchIrql,
          DeviceLow = DeviceLowIrql,
          DeviceBelowNormal = DeviceBelowNormalIrql,
          DeviceNormal = DeviceNormalIrql,
          DeviceHigh = DeviceHighIrql,
          Network = NetworkIrql,
          Storage = StorageIrql,
          DeviceRealTime = DeviceRealTimeIrql,
          IPI = IPIIrql,
          Clock = ClockIrql,
     };

     constexpr std::uint8_t KeIrqlToApicClass(IRQL irql) noexcept
     {
          switch (irql)
          {
          case IRQL::Passive:
          case IRQL::Apc:
          case IRQL::Dispatch: return 0;

          case IRQL::DeviceLow: return 2;
          case IRQL::DeviceBelowNormal: return 3;
          case IRQL::DeviceNormal: return 4;
          case IRQL::DeviceHigh: return 5;
          case IRQL::Network: return 6;
          case IRQL::Storage: return 7;
          case IRQL::DeviceRealTime: return 8;
          case IRQL::IPI: return 0xE;
          case IRQL::Clock: return 0xF;
          }
          return 0;
     }
     constexpr IRQL KeApicClassToIrql(std::uint8_t cls) noexcept
     {
          switch (cls)
          {
          case 0: return IRQL::Dispatch; // Passive/Apc/Dispatch all here
          case 2: return IRQL::DeviceLow;
          case 3: return IRQL::DeviceBelowNormal;
          case 4: return IRQL::DeviceNormal;
          case 5: return IRQL::DeviceHigh;
          case 6: return IRQL::Network;
          case 7: return IRQL::Storage;
          case 8: return IRQL::DeviceRealTime;
          case 0xE: return IRQL::IPI;
          case 0xF: return IRQL::Clock;
          default: return IRQL::Dispatch;
          }
     }

     constexpr IRQL KeVectorToIrql(std::uint8_t vector) noexcept { return KeApicClassToIrql(vector >> 4); }
} // namespace cpu

using InterruptHandler = bool (*)(cpu::IInterruptFrame&, void* argument);

void KiInitialiseInterrupts(std::uintptr_t acpiPhysical);
void HandleInterrupt(cpu::IInterruptFrame& frame);
void KeAcknowledgeInterrupt();
void KeSetTimerFrequency(std::uint32_t frequency);
std::uint64_t KeReadLowResolutionTimer();
std::uint64_t KeReadHighResolutionTimer();
std::uint64_t KeReadLowResolutionTimerFrequency();
std::uint64_t KeReadHighResolutionTimerFrequency();
std::uint64_t KeCurrentSystemTime(); // milliseconds!!!
void KeRegisterInterruptHandler(cpu::InterruptVector physical, cpu::InterruptVector vector, InterruptHandler handler,
                                void* argument = nullptr);
