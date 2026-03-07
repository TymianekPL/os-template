#include "process.h"

namespace kernel
{
     ProcessControlBlock::ProcessControlBlock(std::uint64_t processId, const char* name, std::uintptr_t searchStart)
         : _processId(processId), _state(ProcessState::Created), _priority(ProcessPriority::Normal), _cr3(0),
           _stackPointer(0), _instructionPointer(0), _kernelStackBase(0), _userStackBase(0), _parent(nullptr),
           _name(name), _vadAllocator(searchStart)
     {
     }

     std::uintptr_t ProcessControlBlock::ReserveMemory(std::size_t size, memory::MemoryProtection protection,
                                                       memory::PFNUse use)
     {
          return this->_vadAllocator.ReserveVirtualMemory(size, protection, use);
     }

     std::uintptr_t ProcessControlBlock::ReserveMemoryAt(std::uintptr_t hint, std::size_t size,
                                                         memory::MemoryProtection protection, memory::PFNUse use)
     {
          return this->_vadAllocator.ReserveVirtualMemoryAt(hint, size, protection, use);
     }

     bool ProcessControlBlock::ReserveMemoryFixed(std::uintptr_t baseAddress, std::size_t size,
                                                  memory::MemoryProtection protection, memory::PFNUse use)
     {
          return this->_vadAllocator.ReserveVirtualMemoryFixed(baseAddress, size, protection, use);
     }

     bool ProcessControlBlock::FreeMemory(std::uintptr_t baseAddress)
     {
          return this->_vadAllocator.Remove(baseAddress);
     }

     memory::VADNode* ProcessControlBlock::QueryAddress(std::uintptr_t address)
     {
          return this->_vadAllocator.FindContaining(address);
     }

     void ProcessControlBlock::SetState(ProcessState state) { this->_state = state; }
     void ProcessControlBlock::SetPriority(ProcessPriority priority) { this->_priority = priority; }
     void ProcessControlBlock::SetPageTableBase(std::uintptr_t cr3) { this->_cr3 = cr3; }
     void ProcessControlBlock::SetStackPointer(std::uintptr_t sp) { this->_stackPointer = sp; }
     void ProcessControlBlock::SetInstructionPointer(std::uintptr_t ip) { this->_instructionPointer = ip; }
     void ProcessControlBlock::SetKernelStack(std::uintptr_t base) { this->_kernelStackBase = base; }
     void ProcessControlBlock::SetUserStack(std::uintptr_t base) { this->_userStackBase = base; }

} // namespace kernel
