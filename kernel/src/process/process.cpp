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
          if (size == 0)
          {
               debugging::DbgWrite(
                   u8"[AllocateVirtualMemory] WARNING: Allocating zero bytes with flags {} and protection {}\r\n",
                   reinterpret_cast<void*>(flags), reinterpret_cast<void*>(protection));
               return nullptr;
          }

          const bool isReserve = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Reserve) != 0;
          const bool isCommit = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Commit) != 0;
          const bool isReset = static_cast<std::uint32_t>(flags & memory::AllocationFlags::Reset) != 0;
          const bool isImmediate = static_cast<std::uint32_t>(flags & memory::AllocationFlags::ImmediatePhysical) != 0;

          if (!isReserve && !isCommit && !isReset)
          {
               debugging::DbgWrite(u8"[AllocateVirtualMemory] ERROR: No operation specified in flags {}\r\n",
                                   reinterpret_cast<void*>(flags));
               return nullptr;
          }

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

               if (resultAddress == 0)
               {
                    debugging::DbgWrite(u8"[AllocateVirtualMemory] ERROR: Failed to reserve virtual memory of size {} "
                                        u8"with protection {}!\r\n",
                                        size, reinterpret_cast<void*>(protection));
                    return nullptr;
               }

               if (!isCommit) return reinterpret_cast<void*>(resultAddress);

               baseAddress = resultAddress;
          }
          else if (isCommit)
          {
               if (baseAddress == 0) return nullptr;

               auto* node = _vadAllocator.FindContaining(baseAddress);
               if (!node)
               {
                    debugging::DbgWrite(u8"[AllocateVirtualMemory] ERROR: No VAD node contains base address {}!\r\n",
                                        reinterpret_cast<void*>(baseAddress));
                    return nullptr;
               }

               if (node->entry.state != memory::VADMemoryState::Reserved)
               {
                    debugging::DbgWrite(
                        u8"[AllocateVirtualMemory] ERROR: VAD node state is not Reserved for base address {}!\r\n",
                        reinterpret_cast<void*>(baseAddress));
                    return nullptr;
               }

               resultAddress = baseAddress;
          }

          if (isCommit || isReset)
          {
               if (!this->_vadAllocator.CommitMemory(baseAddress, size, isImmediate))
               {
                    debugging::DbgWrite(
                        u8"[AllocateVirtualMemory] ERROR: Failed to commit memory at base address {} with size {}!\r\n",
                        reinterpret_cast<void*>(baseAddress), size);
                    if (isReserve) this->_vadAllocator.Remove(baseAddress);
                    return nullptr;
               }
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
               debugging::DbgWrite(u8"[ReleaseVirtualMemory] ERROR: No VAD node contains base address {}!\r\n",
                                   reinterpret_cast<void*>(baseAddress));
               return false;
          }

          if (isRelease)
          {
               if (baseAddress != node->entry.baseAddress)
               {
                    debugging::DbgWrite(u8"[ReleaseVirtualMemory] ERROR: baseAddress != node->entry.baseAddress\r\n");
                    return false;
               }

               if (size != 0 && size != node->entry.size)
               {
                    debugging::DbgWrite(u8"[ReleaseVirtualMemory] ERROR: size != 0 && size != node->entry.size\r\n");
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
                        u8"[ReleaseVirtualMemory] ERROR: baseAddress + decommitSize > node->EndAddress()!\r\n");
                    return false;
               }

               return this->_vadAllocator.DecommitMemory(baseAddress, decommitSize);
          }
          debugging::DbgWrite(u8"[ReleaseVirtualMemory] ERROR: Unknown error :(\r\n");
          return false;
     }
     object::Handle ProcessControlBlock::CreateObject(const object::ObjectAttributes& attr) noexcept
     {
          return object::ObCreateObject(*this, attr);
     }

     object::Handle ProcessControlBlock::OpenObject(std::string_view name, object::AccessRights access) noexcept
     {
          return object::ObOpenObjectByName(*this, name, access);
     }

     object::ObjectStatus kernel::ProcessControlBlock::CloseHandle(object::Handle handle) noexcept
     {
          return object::ObCloseHandle(*this, handle);
     }

     object::Handle kernel::ProcessControlBlock::DuplicateHandle(object::Handle src,
                                                                 object::AccessRights access) noexcept
     {
          return object::ObDuplicateHandle(*this, *this, src, access);
     }

     object::Handle ProcessControlBlock::DuplicateHandleTo(kernel::ProcessControlBlock& dst, object::Handle src,
                                                           object::AccessRights access) noexcept
     {
          return object::ObDuplicateHandle(*this, dst, src, access);
     }

     object::ObjectHeader* ProcessControlBlock::QueryHandle(object::Handle handle) const noexcept
     {
          return object::ObQueryObject(const_cast<ProcessControlBlock&>(*this), handle); // NOLINT
     }

     object::Handle kernel::ProcessControlBlock::OpenObjectAbsolute(std::string_view path, object::AccessRights access,
                                                                    object::OpenFlags flags) noexcept
     {
          return object::ObOpenObjectByAbsolutePath(*this, path, access, flags);
     }
} // namespace kernel
