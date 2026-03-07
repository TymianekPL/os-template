#pragma once

#include <cstddef>
#include <cstdint>
#include "memory/vallocator.h"

namespace kernel
{
     enum struct ProcessState : std::uint8_t
     {
          Created,
          Ready,
          Running,
          Blocked,
          Suspended,
          Terminated
     };

     enum struct ProcessPriority : std::uint8_t
     {
          Idle = 0,
          Low = 1,
          Normal = 2,
          High = 3,
          Realtime = 4
     };

     class ProcessControlBlock
     {
     private:
          std::uint64_t _processId;
          ProcessState _state;
          ProcessPriority _priority;
          std::uintptr_t _cr3;
          memory::VirtualMemoryAllocator _vadAllocator;

          std::uintptr_t _stackPointer;
          std::uintptr_t _instructionPointer;
          std::uintptr_t _kernelStackBase;
          std::uintptr_t _userStackBase;

          ProcessControlBlock* _parent;
          const char* _name;

     public:
          explicit ProcessControlBlock(std::uint64_t processId, const char* name, std::uintptr_t searchStart);

          std::uintptr_t ReserveMemory(std::size_t size, memory::MemoryProtection protection, memory::PFNUse use);
          std::uintptr_t ReserveMemoryAt(std::uintptr_t hint, std::size_t size, memory::MemoryProtection protection,
                                         memory::PFNUse use);
          bool ReserveMemoryFixed(std::uintptr_t baseAddress, std::size_t size, memory::MemoryProtection protection,
                                  memory::PFNUse use);
          bool FreeMemory(std::uintptr_t baseAddress);
          memory::VADNode* QueryAddress(std::uintptr_t address);

          void SetState(ProcessState state);
          [[nodiscard]] ProcessState GetState() const { return _state; }

          void SetPriority(ProcessPriority priority);
          [[nodiscard]] ProcessPriority GetPriority() const { return _priority; }

          void SetPageTableBase(std::uintptr_t cr3);
          [[nodiscard]] std::uintptr_t GetPageTableBase() const { return _cr3; }

          void SetStackPointer(std::uintptr_t sp);
          [[nodiscard]] std::uintptr_t GetStackPointer() const { return _stackPointer; }

          void SetInstructionPointer(std::uintptr_t ip);
          [[nodiscard]] std::uintptr_t GetInstructionPointer() const { return _instructionPointer; }

          void SetKernelStack(std::uintptr_t base);
          [[nodiscard]] std::uintptr_t GetKernelStack() const { return _kernelStackBase; }

          void SetUserStack(std::uintptr_t base);
          [[nodiscard]] std::uintptr_t GetUserStack() const { return _userStackBase; }

          [[nodiscard]] std::uint64_t GetProcessId() const { return _processId; }
          [[nodiscard]] const char* GetName() const { return _name; }
          [[nodiscard]] ProcessControlBlock* GetParent() const { return _parent; }
          void SetParent(ProcessControlBlock* parent) { _parent = parent; }

          [[nodiscard]] memory::VirtualMemoryAllocator& GetVadAllocator() { return _vadAllocator; }
          [[nodiscard]] const memory::VirtualMemoryAllocator& GetVadAllocator() const { return _vadAllocator; }
     };

} // namespace kernel
