#pragma once

#include <cstdint>

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
          [[nodiscard]] virtual std::uintptr_t GetExtra() const { return GetVector(); };
          [[nodiscard]] virtual void* GetContext() const { return nullptr; };

          virtual void DumpRegisters() const = 0;
     };
} // namespace cpu

void KiInitialiseInterrupts(std::uintptr_t acpiPhysical);
void HandleInterrupt(cpu::IInterruptFrame& frame);
void KeAcknowledgeInterrupt();
void KeSetTimerFrequency(std::uint32_t frequency);
std::uint64_t KeReadLowResolutionTimer();
std::uint64_t KeReadHighResolutionTimer();
std::uint64_t KeReadLowResolutionTimerFrequency();
std::uint64_t KeReadHighResolutionTimerFrequency();
std::uint64_t KeCurrentSystemTime(); // milliseconds!!!
