#include "pallocator.h"
#include <algorithm>
#include "utils/arch.h"

namespace memory
{
     constexpr std::size_t PageSize = 0x1000;
     constexpr std::size_t InvalidPFN = static_cast<std::size_t>(-1);

     bool PhysicalMemoryAllocator::Initialise(structures::LinkedList<arch::MemoryDescriptor> memoryDescriptors,
                                              std::uintptr_t kernelPhysicalBase, std::uintptr_t kernelVirtualBase)
     {
          std::size_t totalPages{};
          std::uintptr_t maxPage{};

          for (const auto& descriptor : memoryDescriptors)
          {
               if (descriptor.type == arch::MemoryType::Reserved) continue;

               std::uintptr_t endPage = descriptor.basePage + descriptor.baseCount;
               maxPage = std::max(maxPage, endPage);
          }

          databaseSize = maxPage;

          std::size_t databaseBytes = databaseSize * sizeof(PFNEntry);
          std::size_t listNodesBytes = databaseSize * sizeof(structures::SingleListEntry<PFNEntry*>);
          std::size_t totalBytes = databaseBytes + listNodesBytes;
          std::size_t totalPagesNeeded = (totalBytes + PageSize - 1) / PageSize;

          std::uintptr_t allocationAddress = 0;
          for (const auto& descriptor : memoryDescriptors)
          {
               if (descriptor.type == arch::MemoryType::Free || descriptor.type == arch::MemoryType::BootServicesData ||
                   descriptor.type == arch::MemoryType::BootServicesCode)
               {
                    if (descriptor.baseCount >= totalPagesNeeded)
                    {
                         allocationAddress = descriptor.basePage * PageSize;
                         break;
                    }
               }
          }

          if (allocationAddress == 0) return false;

          std::intptr_t physToVirtOffset = static_cast<std::intptr_t>(kernelVirtualBase - kernelPhysicalBase);
          std::uintptr_t virtualAddress = allocationAddress + physToVirtOffset;

          database = reinterpret_cast<PFNEntry*>(virtualAddress);
          listNodes = reinterpret_cast<structures::SingleListEntry<PFNEntry*>*>(virtualAddress + databaseBytes);

          for (std::size_t i = 0; i < databaseSize; i++)
          {
               database[i].use = PFNUse::Unused;
               database[i].region = PFNRegion::Bad;
               database[i].referenceCount = 0;

               listNodes[i].next = nullptr;
               listNodes[i].data = &database[i];
          }

          std::size_t stolenStartPfn = allocationAddress / PageSize;
          std::size_t stolenEndPfn = stolenStartPfn + totalPagesNeeded;

          for (const auto& descriptor : memoryDescriptors)
          {
               PFNRegion region = PFNRegion::Bad;
               PFNUse use = PFNUse::Unused;

               switch (descriptor.type)
               {
               case arch::MemoryType::Free:
                    region = PFNRegion::Free;
                    use = PFNUse::Unused;
                    break;
               case arch::MemoryType::BootServicesCode:
               case arch::MemoryType::BootServicesData:
               case arch::MemoryType::LoaderCode:
               case arch::MemoryType::LoaderData:
                    region = PFNRegion::Free;
                    use = PFNUse::Unused;
                    break;
               case arch::MemoryType::ACPIReclaimMemory:
                    region = PFNRegion::Free;
                    use = PFNUse::Unused;
                    break;
               case arch::MemoryType::Reserved:
               case arch::MemoryType::RuntimeServicesCode:
               case arch::MemoryType::RuntimeServicesData:
               case arch::MemoryType::UnusableMemory:
               case arch::MemoryType::ACPIMemoryNVS:
               case arch::MemoryType::MemoryMappedIO:
               case arch::MemoryType::MemoryMappedIOPortSpace:
               case arch::MemoryType::PalCode:
               case arch::MemoryType::PersistentMemory:
               case arch::MemoryType::Bad:
               default:
                    region = PFNRegion::Bad;
                    use = PFNUse::Unused;
                    break;
               }

               for (std::uint32_t i = 0; i < descriptor.baseCount; ++i)
               {
                    std::size_t pfn = descriptor.basePage + i;
                    if (pfn < databaseSize)
                    {
                         if (pfn >= stolenStartPfn && pfn < stolenEndPfn)
                         {
                              database[pfn].use = PFNUse::KernelHeap;
                              database[pfn].region = PFNRegion::Active;
                              database[pfn].referenceCount = 1;

                              activePages.fetch_add(1, std::memory_order_relaxed);
                              kernelHeapPages.fetch_add(1, std::memory_order_relaxed);
                              continue;
                         }

                         database[pfn].use = use;
                         database[pfn].region = region;
                         database[pfn].referenceCount = 0;

                         auto* listHead = reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(
                             region == PFNRegion::Free ? &freeList.head : &badList.head);

                         PushToList(*listHead, pfn);

                         if (region == PFNRegion::Free) freePages.fetch_add(1, std::memory_order::relaxed);
                         else if (region == PFNRegion::Bad)
                              badPages.fetch_add(1, std::memory_order::relaxed);

                         if (use == PFNUse::Unused) unusedPages.fetch_add(1, std::memory_order::relaxed);
                    }
               }
          }

          return true;
     }

     std::uintptr_t PhysicalMemoryAllocator::AllocatePage(PFNUse use)
     {
          std::size_t pfnIndex = InvalidPFN;

          auto* zeroHead = reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&zeroList.head);
          pfnIndex = PopFromList(*zeroHead);

          if (pfnIndex != InvalidPFN) { zeroPages.fetch_sub(1, std::memory_order_relaxed); }
          else
          {
               auto* freeHead = reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&freeList.head);
               pfnIndex = PopFromList(*freeHead);

               if (pfnIndex != InvalidPFN) { freePages.fetch_sub(1, std::memory_order_relaxed); }
               else
               {
                    auto* standbyHead =
                        reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&standbyList.head);
                    pfnIndex = PopFromList(*standbyHead);

                    if (pfnIndex != InvalidPFN) { standbyPages.fetch_sub(1, std::memory_order_relaxed); }
                    else
                         return ~0;
               }
          }

          PFNUse oldUse = database[pfnIndex].use;
          database[pfnIndex].use = use;
          database[pfnIndex].region = PFNRegion::Active;
          database[pfnIndex].referenceCount = 1;

          DecrementUseCounter(oldUse);
          IncrementUseCounter(use);
          activePages.fetch_add(1, std::memory_order_relaxed);

          return pfnIndex * PageSize;
     }
     std::uintptr_t PhysicalMemoryAllocator::AllocatePageOverwrite(PFNUse use)
     {
          std::size_t pfnIndex = InvalidPFN;

          auto* freeHead = reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&freeList.head);
          pfnIndex = PopFromList(*freeHead);

          if (pfnIndex != InvalidPFN) { freePages.fetch_sub(1, std::memory_order_relaxed); }
          else
          {
               auto* zeroHead = reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&zeroList.head);
               pfnIndex = PopFromList(*zeroHead);

               if (pfnIndex != InvalidPFN) { zeroPages.fetch_sub(1, std::memory_order_relaxed); }
               else
               {
                    auto* standbyHead =
                        reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&standbyList.head);
                    pfnIndex = PopFromList(*standbyHead);

                    if (pfnIndex != InvalidPFN) { standbyPages.fetch_sub(1, std::memory_order_relaxed); }
                    else
                         return ~0;
               }
          }

          PFNUse oldUse = database[pfnIndex].use;
          database[pfnIndex].use = use;
          database[pfnIndex].region = PFNRegion::Active;
          database[pfnIndex].referenceCount = 1;

          DecrementUseCounter(oldUse);
          IncrementUseCounter(use);
          activePages.fetch_add(1, std::memory_order_relaxed);

          return pfnIndex * PageSize;
     }

     void PhysicalMemoryAllocator::ReleaseZeroPage(std::uintptr_t address)
     {
          std::size_t pfnIndex = address / PageSize;

          if (pfnIndex >= databaseSize) return;

          PFNEntry& entry = database[pfnIndex];

          if (entry.referenceCount > 0) entry.referenceCount--;

          if (entry.referenceCount == 0)
          {
               PFNUse oldUse = entry.use;
               PFNRegion oldRegion = entry.region;

               if (oldRegion == PFNRegion::Active) activePages.fetch_sub(1, std::memory_order_relaxed);

               DecrementUseCounter(oldUse);

               entry.use = PFNUse::Unused;
               entry.region = PFNRegion::Zero;

               auto* zeroHead = reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&zeroList.head);
               PushToList(*zeroHead, pfnIndex);

               zeroPages.fetch_add(1, std::memory_order_relaxed);
               unusedPages.fetch_add(1, std::memory_order_relaxed);
          }
     }

     void PhysicalMemoryAllocator::ReleaseFreePage(std::uintptr_t address)
     {
          std::size_t pfnIndex = address / PageSize;

          if (pfnIndex >= databaseSize) return;

          PFNEntry& entry = database[pfnIndex];

          if (entry.referenceCount > 0) entry.referenceCount--;

          if (entry.referenceCount == 0)
          {
               PFNUse oldUse = entry.use;
               PFNRegion oldRegion = entry.region;

               if (oldRegion == PFNRegion::Active) activePages.fetch_sub(1, std::memory_order_relaxed);

               DecrementUseCounter(oldUse);

               entry.use = PFNUse::Unused;
               entry.region = PFNRegion::Free;

               auto* freeHead = reinterpret_cast<std::atomic<structures::SingleListEntry<PFNEntry*>*>*>(&freeList.head);
               PushToList(*freeHead, pfnIndex);

               freePages.fetch_add(1, std::memory_order_relaxed);
               unusedPages.fetch_add(1, std::memory_order_relaxed);
          }
     }

     void PhysicalMemoryAllocator::PushToList(std::atomic<structures::SingleListEntry<PFNEntry*>*>& listHead,
                                              std::size_t pfnIndex) const
     {
          structures::SingleListEntry<PFNEntry*>* node = &listNodes[pfnIndex];
          structures::SingleListEntry<PFNEntry*>* oldHead = nullptr;

          do
          {
               oldHead = listHead.load(std::memory_order_acquire);
               node->next = oldHead;
          } while (
              !listHead.compare_exchange_weak(oldHead, node, std::memory_order_release, std::memory_order_acquire));
     }

     std::size_t PhysicalMemoryAllocator::PopFromList(
         std::atomic<structures::SingleListEntry<PFNEntry*>*>& listHead) const
     {
          structures::SingleListEntry<PFNEntry*>* oldHead = nullptr;
          structures::SingleListEntry<PFNEntry*>* newHead = nullptr;

          do
          {
               oldHead = listHead.load(std::memory_order_acquire);
               if (oldHead == nullptr) return InvalidPFN;

               newHead = oldHead->next;
          } while (
              !listHead.compare_exchange_weak(oldHead, newHead, std::memory_order_release, std::memory_order_acquire));

          std::size_t pfnIndex = oldHead - listNodes;
          return pfnIndex;
     }

     void PhysicalMemoryAllocator::IncrementUseCounter(PFNUse use, std::memory_order order)
     {
          switch (use)
          {
          case PFNUse::Unused: unusedPages.fetch_add(1, order); break;
          case PFNUse::ProcessPrivate: processPrivatePages.fetch_add(1, order); break;
          case PFNUse::MappedFile: mappedFilePages.fetch_add(1, order); break;
          case PFNUse::DriverLocked: driverLockedPages.fetch_add(1, order); break;
          case PFNUse::UserStack: userStackPages.fetch_add(1, order); break;
          case PFNUse::KernelStack: kernelStackPages.fetch_add(1, order); break;
          case PFNUse::KernelHeap: kernelHeapPages.fetch_add(1, order); break;
          case PFNUse::MetaFile: metaFilePages.fetch_add(1, order); break;
          case PFNUse::NonPagedPool: nonPagedPoolPages.fetch_add(1, order); break;
          case PFNUse::PagedPool: pagedPoolPages.fetch_add(1, order); break;
          case PFNUse::PTE: ptePages.fetch_add(1, order); break;
          case PFNUse::Shareable: shareablePages.fetch_add(1, order); break;
          case PFNUse::PageTable: pageTablePages.fetch_add(1, order); break;
          case PFNUse::FSCache: fsCachePages.fetch_add(1, order); break;
          }
     }

     void PhysicalMemoryAllocator::DecrementUseCounter(PFNUse use, std::memory_order order)
     {
          switch (use)
          {
          case PFNUse::Unused: unusedPages.fetch_sub(1, order); break;
          case PFNUse::ProcessPrivate: processPrivatePages.fetch_sub(1, order); break;
          case PFNUse::MappedFile: mappedFilePages.fetch_sub(1, order); break;
          case PFNUse::DriverLocked: driverLockedPages.fetch_sub(1, order); break;
          case PFNUse::UserStack: userStackPages.fetch_sub(1, order); break;
          case PFNUse::KernelStack: kernelStackPages.fetch_sub(1, order); break;
          case PFNUse::KernelHeap: kernelHeapPages.fetch_sub(1, order); break;
          case PFNUse::MetaFile: metaFilePages.fetch_sub(1, order); break;
          case PFNUse::NonPagedPool: nonPagedPoolPages.fetch_sub(1, order); break;
          case PFNUse::PagedPool: pagedPoolPages.fetch_sub(1, order); break;
          case PFNUse::PTE: ptePages.fetch_sub(1, order); break;
          case PFNUse::Shareable: shareablePages.fetch_sub(1, order); break;
          case PFNUse::PageTable: pageTablePages.fetch_sub(1, order); break;
          case PFNUse::FSCache: fsCachePages.fetch_sub(1, order); break;
          }
     }

     PhysicalMemoryAllocator physicalAllocator{};
} // namespace memory
