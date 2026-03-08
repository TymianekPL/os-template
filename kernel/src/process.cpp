#include "process.h"
#include "utils/kdbg.h"

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

     bool ProcessControlBlock::ReserveMemoryFixedAsCommitted(std::uintptr_t baseAddress, std::size_t size,
                                                             memory::MemoryProtection protection, memory::PFNUse use)
     {
          return this->_vadAllocator.ReserveVirtualMemoryFixedAsCommitted(baseAddress, size, protection, use);
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
     void ProcessControlBlock::SetPageTableBase(std::uintptr_t cr3)
     {
          this->_cr3 = cr3;
          this->_vadAllocator.SetPageTableRoot(cr3);
     }
     void ProcessControlBlock::SetStackPointer(std::uintptr_t sp) { this->_stackPointer = sp; }
     void ProcessControlBlock::SetInstructionPointer(std::uintptr_t ip) { this->_instructionPointer = ip; }
     void ProcessControlBlock::SetKernelStack(std::uintptr_t base) { this->_kernelStackBase = base; }
     void ProcessControlBlock::SetUserStack(std::uintptr_t base) { this->_userStackBase = base; }

     void* ProcessControlBlock::AllocateVirtualMemory(void* address, std::size_t size, memory::AllocationFlags flags,
                                                      memory::MemoryProtection protection)
     {
          if (size == 0) return nullptr;

          const bool isReserve = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Reserve) != 0;
          const bool isCommit = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Commit) != 0;
          const bool isReset = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Reset) != 0;
          const bool isImmediate = static_cast<std::uint32_t>(flags & memory::AllocationFlags::ImmediatePhysical) != 0;

          if (!isReserve && !isCommit && !isReset) return nullptr;

          std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(address);
          std::uintptr_t resultAddress{};

          if (isReserve)
          {
               if (baseAddress != 0)
               {
                    if (!this->_vadAllocator.ReserveVirtualMemoryFixed(baseAddress, size, protection,
                                                                       memory::PFNUse::ProcessPrivate))
                    {
                         resultAddress = _vadAllocator.ReserveVirtualMemoryAt(baseAddress, size, protection,
                                                                              memory::PFNUse::ProcessPrivate);
                    }
                    else
                         resultAddress = baseAddress;
               }
               else
               {
                    resultAddress =
                        _vadAllocator.ReserveVirtualMemory(size, protection, memory::PFNUse::ProcessPrivate);
               }

               if (resultAddress == 0) return nullptr;

               if (!isCommit) return reinterpret_cast<void*>(resultAddress);

               baseAddress = resultAddress;
          }
          else if (isCommit)
          {
               if (baseAddress == 0) return nullptr;

               auto* node = _vadAllocator.FindContaining(baseAddress);
               if (!node) return nullptr;

               if (node->entry.state != memory::VADMemoryState::Reserved) return nullptr;

               resultAddress = baseAddress;
          }

          if (isCommit || isReset)
          {
               if (!this->_vadAllocator.CommitMemory(baseAddress, size, isImmediate)) return nullptr;
          }

          return reinterpret_cast<void*>(resultAddress);
     }

     bool ProcessControlBlock::ReleaseVirtualMemory(void* address, std::size_t size, memory::AllocationFlags flags)
     {
          if (address == nullptr)
          {
               debugging::DbgWrite(u8"[ReleaseVirtualMemory] address == nullptr\r\n");
               return false;
          }

          const std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(address);

          const bool isRelease = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Release) != 0;
          const bool isDecommit = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Decommit) != 0;

          if (!isRelease && !isDecommit)
          {
               debugging::DbgWrite(u8"[ReleaseVirtualMemory] !isRelease && !isDecommit\r\n");
               return false;
          }

          auto* node = this->_vadAllocator.FindContaining(baseAddress);
          if (!node)
          {
               debugging::DbgWrite(u8"[ReleaseVirtualMemory] FindContaining()\r\n");
               return false;
          }

          if (isRelease)
          {
               if (baseAddress != node->entry.baseAddress)
               {
                    debugging::DbgWrite(u8"[ReleaseVirtualMemory] baseAddress != node->entry.baseAddress\r\n");
                    return false;
               }

               if (size != 0 && size != node->entry.size)
               {
                    debugging::DbgWrite(u8"[ReleaseVirtualMemory] size != 0 && size != node->entry.size\r\n");
                    return false;
               }

               return this->_vadAllocator.Remove(baseAddress);
          }

          if (isDecommit)
          {
               std::size_t decommitSize = size;

               if (decommitSize == 0) decommitSize = node->EndAddress() - baseAddress;

               if (baseAddress + decommitSize > node->EndAddress())
               {
                    debugging::DbgWrite(
                        u8"[ReleaseVirtualMemory] baseAddress + decommitSize > node->EndAddress()!\r\n");
                    return false;
               }

               return this->_vadAllocator.DecommitMemory(baseAddress, decommitSize);
          }
          debugging::DbgWrite(u8"[ReleaseVirtualMemory] :(\r\n");
          return false;
     }

} // namespace kernel
