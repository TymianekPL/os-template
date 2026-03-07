#include <utils/identify.h>
#include <utils/memory.h>

#if defined(ARCH_ARM64) && defined(COMPILER_MSVC)
#include <arm64intr.h>
#include <intrin.h>
#endif
namespace memory
{
     std::uintptr_t virtualOffset{};
}

namespace memory::paging
{
#ifdef ARCH_X8664 // vvv x86-64

     constexpr std::size_t GetPageSize() noexcept { return 0x1000; } // 4KB pages

     struct X64PageEntry
     {
          std::uint64_t present : 1;
          std::uint64_t writable : 1;
          std::uint64_t userAccessible : 1;
          std::uint64_t writeThrough : 1;
          std::uint64_t cacheDisable : 1;
          std::uint64_t accessed : 1;
          std::uint64_t dirty : 1;
          std::uint64_t largePage : 1;
          std::uint64_t global : 1;
          std::uint64_t available : 3;
          std::uint64_t physicalAddress : 40;
          std::uint64_t available2 : 11;
          std::uint64_t noExecute : 1;
     };

     std::uintptr_t CreatePageTable(void* (*allocator)(std::size_t))
     {
          auto* pml4 = static_cast<X64PageEntry*>(allocator(0x1000));
          if (pml4 == nullptr) return 0;

          for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pml4)[i] = 0; }

          return reinterpret_cast<std::uintptr_t>(pml4);
     }

     bool MapPage(std::uintptr_t pageTableRoot, const PageMapping& mapping, void* (*allocator)(std::size_t))
     {
          auto* pml4 = reinterpret_cast<X64PageEntry*>(pageTableRoot + virtualOffset);

          for (std::uintptr_t offset = 0; offset < mapping.size; offset += 0x1000)
          {
               std::uintptr_t vAddr = mapping.virtualAddress + offset;
               std::uintptr_t pAddr = mapping.physicalAddress + offset;

               std::uint64_t pml4Index = (vAddr >> 39) & 0x1FF;
               std::uint64_t pdptIndex = (vAddr >> 30) & 0x1FF;
               std::uint64_t pdIndex = (vAddr >> 21) & 0x1FF;
               std::uint64_t ptIndex = (vAddr >> 12) & 0x1FF;

               if (!pml4[pml4Index].present)
               {
                    auto* pdpt = static_cast<X64PageEntry*>(allocator(0x1000));
                    if (!pdpt) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pdpt)[i] = 0; }

                    pml4[pml4Index].physicalAddress = (reinterpret_cast<std::uintptr_t>(pdpt) - virtualOffset) >> 12;
                    pml4[pml4Index].present = 1;
                    pml4[pml4Index].writable = 1;
               }

               auto* pdpt = reinterpret_cast<X64PageEntry*>(
                   virtualOffset + (static_cast<std::uintptr_t>(pml4[pml4Index].physicalAddress) << 12));

               if (!pdpt[pdptIndex].present)
               {
                    auto* pd = static_cast<X64PageEntry*>(allocator(0x1000));
                    if (!pd) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pd)[i] = 0; }

                    pdpt[pdptIndex].physicalAddress = (reinterpret_cast<std::uintptr_t>(pd) - virtualOffset) >> 12;
                    pdpt[pdptIndex].present = 1;
                    pdpt[pdptIndex].writable = 1;
               }

               auto* pd = reinterpret_cast<X64PageEntry*>(
                   virtualOffset + (static_cast<std::uintptr_t>(pdpt[pdptIndex].physicalAddress) << 12));

               if (!pd[pdIndex].present || pd[pdIndex].largePage)
               {
                    auto* pt = static_cast<X64PageEntry*>(allocator(0x1000));
                    if (!pt) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pt)[i] = 0; }

                    pd[pdIndex].physicalAddress = (reinterpret_cast<std::uintptr_t>(pt) - virtualOffset) >> 12;
                    pd[pdIndex].present = 1;
                    pd[pdIndex].writable = 1;
                    pd[pdIndex].largePage = 0;
               }

               auto* pt = reinterpret_cast<X64PageEntry*>(
                   virtualOffset + (static_cast<std::uintptr_t>(pd[pdIndex].physicalAddress) << 12));

               pt[ptIndex].physicalAddress = pAddr >> 12;
               pt[ptIndex].present = 1;
               pt[ptIndex].writable = mapping.writable ? 1 : 0;
               pt[ptIndex].userAccessible = mapping.userAccessible ? 1 : 0;
               pt[ptIndex].cacheDisable = mapping.cacheDisable ? 1 : 0;
          }

          return true;
     }

     bool MapPhysicalMemoryDirect(std::uintptr_t pageTableRoot, std::size_t maxPhysicalAddress,
                                  void* (*allocator)(std::size_t))
     {
          constexpr std::uintptr_t DirectMapOffset = 0xFFFF800000000000ULL;
          constexpr std::size_t LargePageSize = 0x200000; // 2MB

          auto* pml4 = reinterpret_cast<X64PageEntry*>(pageTableRoot);

          // TODO: Check 2MiB support (maybe add 1GiB support?)
          for (std::uintptr_t pAddr = 0; pAddr < maxPhysicalAddress; pAddr += LargePageSize)
          {
               std::uintptr_t vAddr = pAddr + DirectMapOffset;

               std::uint64_t pml4Index = (vAddr >> 39) & 0x1FF;
               std::uint64_t pdptIndex = (vAddr >> 30) & 0x1FF;
               std::uint64_t pdIndex = (vAddr >> 21) & 0x1FF;

               if (!pml4[pml4Index].present)
               {
                    auto* pdpt = static_cast<X64PageEntry*>(allocator(0x1000));
                    if (!pdpt) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pdpt)[i] = 0; }

                    pml4[pml4Index].physicalAddress = reinterpret_cast<std::uintptr_t>(pdpt) >> 12;
                    pml4[pml4Index].present = 1;
                    pml4[pml4Index].writable = 1;
                    pml4[pml4Index].noExecute = 1;
               }

               auto* pdpt =
                   reinterpret_cast<X64PageEntry*>(static_cast<std::uintptr_t>(pml4[pml4Index].physicalAddress) << 12);

               if (!pdpt[pdptIndex].present)
               {
                    auto* pd = static_cast<X64PageEntry*>(allocator(0x1000));
                    if (!pd) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pd)[i] = 0; }

                    pdpt[pdptIndex].physicalAddress = reinterpret_cast<std::uintptr_t>(pd) >> 12;
                    pdpt[pdptIndex].present = 1;
                    pdpt[pdptIndex].writable = 1;
                    pdpt[pdptIndex].noExecute = 1;
               }

               auto* pd =
                   reinterpret_cast<X64PageEntry*>(static_cast<std::uintptr_t>(pdpt[pdptIndex].physicalAddress) << 12);

               pd[pdIndex].physicalAddress = pAddr >> 12;
               pd[pdIndex].present = 1;
               pd[pdIndex].writable = 1;
               pd[pdIndex].writeThrough = 1;
               pd[pdIndex].cacheDisable = 0;
               pd[pdIndex].largePage = 1;
               pd[pdIndex].global = 1;
               pd[pdIndex].noExecute = 1;
               pd[pdIndex].userAccessible = 0;
          }

          return true;
     }

     void LoadPageTable(std::uintptr_t pageTableRoot)
     {
#ifdef COMPILER_MSVC
          __writecr3(pageTableRoot);
#else
          asm volatile("mov %0, %%cr3" ::"r"(pageTableRoot) : "memory");
#endif
     }

     std::uintptr_t GetCurrentPageTable()
     {
#ifdef COMPILER_MSVC
          return __readcr3();
#else
          std::uintptr_t cr3 = 0;
          asm volatile("mov %%cr3, %0" : "=r"(cr3));
          return cr3;
#endif
     }

     void InvalidatePage(std::uintptr_t virtualAddress)
     {
#ifdef COMPILER_MSVC
          __invlpg(reinterpret_cast<void*>(virtualAddress));
#else
          asm volatile("invlpg (%0)" ::"r"(virtualAddress) : "memory");
#endif
     }

     void FlushTLB()
     {
          std::uintptr_t cr3 = GetCurrentPageTable();
          LoadPageTable(cr3);
     }

     void EnablePaging()
     {
#ifdef COMPILER_MSVC
          std::uint64_t cr4 = __readcr4();
          cr4 |= (1ULL << 5); // CR4.PAE
          cr4 |= (1ULL << 7); // CR4.PGE (global pages)
          __writecr4(cr4);

          std::uint64_t efer = __readmsr(0xC0000080); // IA32_EFER
          efer |= (1ULL << 8);                        // EFER.LME
          __writemsr(0xC0000080, efer);

          std::uint64_t cr0 = __readcr0();
          cr0 |= (1ULL << 31); // CR0.PG
          __writecr0(cr0);
#else
          std::uint64_t cr4 = 0;
          asm volatile("mov %%cr4, %0" : "=r"(cr4));
          cr4 |= (1ULL << 5); // CR4.PAE
          cr4 |= (1ULL << 7); // CR4.PGE (global pages)
          asm volatile("mov %0, %%cr4" ::"r"(cr4) : "memory");

          std::uint32_t eferLow{};
          std::uint32_t eferHigh{};
          asm volatile("rdmsr" : "=a"(eferLow), "=d"(eferHigh) : "c"(0xC0000080));
          eferLow |= (1 << 8); // EFER.LME
          asm volatile("wrmsr" ::"c"(0xC0000080), "a"(eferLow), "d"(eferHigh) : "memory");

          std::uint64_t cr0 = 0;
          asm volatile("mov %%cr0, %0" : "=r"(cr0));
          cr0 |= (1ULL << 31); // CR0.PG
          asm volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
#endif
     }

#elifdef ARCH_X8632 // ^^^ x86-64 / x86-32 vvv

     constexpr std::size_t GetPageSize() noexcept { return 0x1000; }

     struct X86PageEntry
     {
          std::uint64_t present : 1;
          std::uint64_t writable : 1;
          std::uint64_t userAccessible : 1;
          std::uint64_t writeThrough : 1;
          std::uint64_t cacheDisable : 1;
          std::uint64_t accessed : 1;
          std::uint64_t dirty : 1;
          std::uint64_t largePage : 1;
          std::uint64_t global : 1;
          std::uint64_t available : 3;
          std::uint64_t physicalAddress : 40;
          std::uint64_t available2 : 11;
          std::uint64_t noExecute : 1;
     };

     std::uintptr_t CreatePageTable(void* (*allocator)(std::size_t))
     {
          auto* pdpt = static_cast<X86PageEntry*>(allocator(0x1000));
          if (!pdpt) return 0;

          for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pdpt)[i] = 0; }

          return reinterpret_cast<std::uintptr_t>(pdpt);
     }

     bool MapPage(std::uintptr_t pageTableRoot, const PageMapping& mapping, void* (*allocator)(std::size_t))
     {
          auto* pdpt = reinterpret_cast<X86PageEntry*>(pageTableRoot);

          for (std::uintptr_t offset = 0; offset < mapping.size; offset += 0x1000)
          {
               std::uintptr_t vAddr = mapping.virtualAddress + offset;
               std::uintptr_t pAddr = mapping.physicalAddress + offset;

               std::uint32_t pdptIndex = (vAddr >> 30) & 0x3; // 2 bits
               std::uint32_t pdIndex = (vAddr >> 21) & 0x1FF; // 9 bits
               std::uint32_t ptIndex = (vAddr >> 12) & 0x1FF; // 9 bits

               if (!pdpt[pdptIndex].present)
               {
                    auto* pd = static_cast<X86PageEntry*>(allocator(0x1000));
                    if (!pd) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pd)[i] = 0; }

                    pdpt[pdptIndex].physicalAddress = reinterpret_cast<std::uintptr_t>(pd) >> 12;
                    pdpt[pdptIndex].present = 1;
                    pdpt[pdptIndex].writable = 1;
               }

               auto* pd =
                   reinterpret_cast<X86PageEntry*>(static_cast<std::uintptr_t>(pdpt[pdptIndex].physicalAddress) << 12);

               if (!pd[pdIndex].present || pd[pdIndex].largePage)
               {
                    auto* pt = static_cast<X86PageEntry*>(allocator(0x1000));
                    if (!pt) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pt)[i] = 0; }

                    pd[pdIndex].physicalAddress = reinterpret_cast<std::uintptr_t>(pt) >> 12;
                    pd[pdIndex].present = 1;
                    pd[pdIndex].writable = 1;
                    pd[pdIndex].largePage = 0;
               }

               auto* pt =
                   reinterpret_cast<X86PageEntry*>(static_cast<std::uintptr_t>(pd[pdIndex].physicalAddress) << 12);

               pt[ptIndex].physicalAddress = pAddr >> 12;
               pt[ptIndex].present = 1;
               pt[ptIndex].writable = mapping.writable ? 1 : 0;
               pt[ptIndex].userAccessible = mapping.userAccessible ? 1 : 0;
               pt[ptIndex].cacheDisable = mapping.cacheDisable ? 1 : 0;
          }

          return true;
     }

     bool MapPhysicalMemoryDirect(std::uintptr_t pageTableRoot, std::size_t maxPhysicalAddress,
                                  void* (*allocator)(std::size_t))
     {
          // For x86-32, use top of 32-bit address space
          constexpr std::uintptr_t DIRECT_MAP_OFFSET = 0xC0000000UL;
          constexpr std::size_t LARGE_PAGE_SIZE = 0x200000; // 2MB

          auto* pdpt = reinterpret_cast<X86PageEntry*>(pageTableRoot);

          // Map in 2MB chunks
          for (std::uintptr_t pAddr = 0; pAddr < maxPhysicalAddress && pAddr < (0x100000000ULL - DIRECT_MAP_OFFSET);
               pAddr += LARGE_PAGE_SIZE)
          {
               std::uintptr_t vAddr = pAddr + DIRECT_MAP_OFFSET;

               std::uint32_t pdptIndex = (vAddr >> 30) & 0x3;
               std::uint32_t pdIndex = (vAddr >> 21) & 0x1FF;

               // Ensure PDPT entry exists
               if (!pdpt[pdptIndex].present)
               {
                    auto* pd = static_cast<X86PageEntry*>(allocator(0x1000));
                    if (!pd) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(pd)[i] = 0; }

                    pdpt[pdptIndex].physicalAddress = reinterpret_cast<std::uintptr_t>(pd) >> 12;
                    pdpt[pdptIndex].present = 1;
                    pdpt[pdptIndex].writable = 1;
                    pdpt[pdptIndex].noExecute = 1;
               }

               auto* pd =
                   reinterpret_cast<X86PageEntry*>(static_cast<std::uintptr_t>(pdpt[pdptIndex].physicalAddress) << 12);

               // Create 2MB page entry with: WT, G, RW, XD
               pd[pdIndex].physicalAddress = pAddr >> 12;
               pd[pdIndex].present = 1;
               pd[pdIndex].writable = 1;
               pd[pdIndex].writeThrough = 1; // WT
               pd[pdIndex].cacheDisable = 0;
               pd[pdIndex].largePage = 1; // 2MB page
               pd[pdIndex].global = 1;    // G
               pd[pdIndex].noExecute = 1; // XD
               pd[pdIndex].userAccessible = 0;
          }

          return true;
     }

     void LoadPageTable(std::uintptr_t pageTableRoot) { asm volatile("mov %0, %%cr3" ::"r"(pageTableRoot) : "memory"); }

     std::uintptr_t GetCurrentPageTable()
     {
          std::uintptr_t cr3{};
          asm volatile("mov %%cr3, %0" : "=r"(cr3));
          return cr3;
     }

     void InvalidatePage(std::uintptr_t virtualAddress)
     {
          asm volatile("invlpg (%0)" ::"r"(virtualAddress) : "memory");
     }

     void FlushTLB()
     {
          std::uintptr_t cr3 = GetCurrentPageTable();
          LoadPageTable(cr3);
     }

     void EnablePaging()
     {
          std::uint32_t cr4 = 0;
          asm volatile("mov %%cr4, %0" : "=r"(cr4));
          cr4 |= (1 << 5); // CR4.PAE
          cr4 |= (1 << 4); // CR4.PSE
          cr4 |= (1 << 7); // CR4.PGE (global pages)
          asm volatile("mov %0, %%cr4" ::"r"(cr4) : "memory");

          std::uint32_t cr0 = 0;
          asm volatile("mov %%cr0, %0" : "=r"(cr0));
          cr0 |= (1U << 31); // CR0.PG
          asm volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
     }

#elifdef ARCH_ARM64 // ^^^ x86-32 / ARM64 vvv

     constexpr std::size_t GetPageSize() noexcept { return 0x1000; } // 4KB pages

     struct ARMPageEntry
     {
          std::uint64_t valid : 1;
          std::uint64_t table : 1; // 1 = table/page, 0 = block
          std::uint64_t attrIndex : 3;
          std::uint64_t ns : 1;
          std::uint64_t ap : 2; // Access permissions
          std::uint64_t sh : 2; // Shareability
          std::uint64_t af : 1; // Access flag
          std::uint64_t ng : 1; // Not global
          std::uint64_t address : 36;
          std::uint64_t reserved : 4;
          std::uint64_t contiguous : 1;
          std::uint64_t pxn : 1; // Privileged execute never
          std::uint64_t uxn : 1; // Unprivileged execute never
          std::uint64_t ignored : 9;
     };

     std::uintptr_t CreatePageTable(void* (*allocator)(std::size_t))
     {
          auto* l0 = static_cast<ARMPageEntry*>(allocator(0x1000));
          if (!l0) return 0;

          for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(l0)[i] = 0; }

          return reinterpret_cast<std::uintptr_t>(l0);
     }

     bool MapPage(std::uintptr_t pageTableRoot, const PageMapping& mapping, void* (*allocator)(std::size_t))
     {
          auto* l0 = reinterpret_cast<ARMPageEntry*>(pageTableRoot);

          for (std::uintptr_t offset = 0; offset < mapping.size; offset += 0x1000)
          {
               std::uintptr_t vAddr = mapping.virtualAddress + offset;
               std::uintptr_t pAddr = mapping.physicalAddress + offset;

               std::uint64_t l0Index = (vAddr >> 39) & 0x1FF;
               std::uint64_t l1Index = (vAddr >> 30) & 0x1FF;
               std::uint64_t l2Index = (vAddr >> 21) & 0x1FF;
               std::uint64_t l3Index = (vAddr >> 12) & 0x1FF;

               if (!l0[l0Index].valid)
               {
                    auto* l1 = static_cast<ARMPageEntry*>(allocator(0x1000));
                    if (!l1) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(l1)[i] = 0; }

                    l0[l0Index].address = reinterpret_cast<std::uintptr_t>(l1) >> 12;
                    l0[l0Index].valid = 1;
                    l0[l0Index].table = 1;
               }

               auto* l1 = reinterpret_cast<ARMPageEntry*>(static_cast<std::uintptr_t>(l0[l0Index].address) << 12);

               if (!l1[l1Index].valid)
               {
                    auto* l2 = static_cast<ARMPageEntry*>(allocator(0x1000));
                    if (!l2) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(l2)[i] = 0; }

                    l1[l1Index].address = reinterpret_cast<std::uintptr_t>(l2) >> 12;
                    l1[l1Index].valid = 1;
                    l1[l1Index].table = 1;
               }

               auto* l2 = reinterpret_cast<ARMPageEntry*>(static_cast<std::uintptr_t>(l1[l1Index].address) << 12);

               if (!l2[l2Index].valid || !l2[l2Index].table)
               {
                    auto* l3 = static_cast<ARMPageEntry*>(allocator(0x1000));
                    if (!l3) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(l3)[i] = 0; }

                    l2[l2Index].address = reinterpret_cast<std::uintptr_t>(l3) >> 12;
                    l2[l2Index].valid = 1;
                    l2[l2Index].table = 1;
               }

               auto* l3 = reinterpret_cast<ARMPageEntry*>(static_cast<std::uintptr_t>(l2[l2Index].address) << 12);

               l3[l3Index].address = pAddr >> 12;
               l3[l3Index].valid = 1;
               l3[l3Index].table = 1;
               l3[l3Index].af = 1;
               l3[l3Index].ap = (mapping.writable ? 0 : 2) | (mapping.userAccessible ? 1 : 0);
               l3[l3Index].sh = 3;
               l3[l3Index].attrIndex = 0;
               l3[l3Index].ns = 0;
               l3[l3Index].ng = 0;
               l3[l3Index].pxn = 0;
               l3[l3Index].uxn = mapping.userAccessible ? 0 : 1;
          }

          return true;
     }

     bool MapPhysicalMemoryDirect(std::uintptr_t pageTableRoot, std::size_t maxPhysicalAddress,
                                  void* (*allocator)(std::size_t))
     {
          constexpr std::uintptr_t DIRECT_MAP_OFFSET = 0xFFFF800000000000ULL;
          constexpr std::size_t BLOCK_SIZE = 0x200000;

          auto* l0 = reinterpret_cast<ARMPageEntry*>(pageTableRoot);

          for (std::uintptr_t pAddr = 0; pAddr < maxPhysicalAddress; pAddr += BLOCK_SIZE)
          {
               std::uintptr_t vAddr = pAddr + DIRECT_MAP_OFFSET;

               std::uint64_t l0Index = (vAddr >> 39) & 0x1FF;
               std::uint64_t l1Index = (vAddr >> 30) & 0x1FF;
               std::uint64_t l2Index = (vAddr >> 21) & 0x1FF;

               if (!l0[l0Index].valid)
               {
                    auto* l1 = static_cast<ARMPageEntry*>(allocator(0x1000));
                    if (!l1) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(l1)[i] = 0; }

                    l0[l0Index].address = reinterpret_cast<std::uintptr_t>(l1) >> 12;
                    l0[l0Index].valid = 1;
                    l0[l0Index].table = 1;
               }

               auto* l1 = reinterpret_cast<ARMPageEntry*>(static_cast<std::uintptr_t>(l0[l0Index].address) << 12);

               if (!l1[l1Index].valid)
               {
                    auto* l2 = static_cast<ARMPageEntry*>(allocator(0x1000));
                    if (!l2) return false;
                    for (std::size_t i = 0; i < 512; ++i) { reinterpret_cast<std::uint64_t*>(l2)[i] = 0; }

                    l1[l1Index].address = reinterpret_cast<std::uintptr_t>(l2) >> 12;
                    l1[l1Index].valid = 1;
                    l1[l1Index].table = 1;
               }

               auto* l2 = reinterpret_cast<ARMPageEntry*>(static_cast<std::uintptr_t>(l1[l1Index].address) << 12);

               l2[l2Index].address = pAddr >> 12;
               l2[l2Index].valid = 1;
               l2[l2Index].table = 0;
               l2[l2Index].af = 1;
               l2[l2Index].ap = 0;
               l2[l2Index].sh = 3;
               l2[l2Index].attrIndex = 1;
               l2[l2Index].ns = 0;
               l2[l2Index].ng = 0;
               l2[l2Index].pxn = 1;
               l2[l2Index].uxn = 1;
          }

          return true;
     }

     void LoadPageTable(std::uintptr_t pageTableRoot)
     {
#ifdef COMPILER_MSVC
          std::uint64_t tcr = 0;
          tcr |= (16ULL << 0);   // T0SZ = 16 => 48-bit VA
          tcr |= (1ULL << 8);    // IRGN0 = write-back, write-allocate
          tcr |= (1ULL << 10);   // ORGN0 = write-back, write-allocate
          tcr |= (3ULL << 12);   // SH0 = inner shareable
          tcr |= (0ULL << 14);   // TG0 = 4KB granule
          tcr |= (16ULL << 16);  // T1SZ = 16 => 48-bit VA
          tcr |= (1ULL << 24);   // IRGN1 = write-back, write-allocate
          tcr |= (1ULL << 26);   // ORGN1 = write-back, write-allocate
          tcr |= (3ULL << 28);   // SH1 = inner shareable
          tcr |= (2ULL << 30);   // TG1 = 4KB granule
          tcr |= (0x5ULL << 32); // IPS = 48-bit PA
          ::_WriteStatusReg(ARM64_SYSREG(3, 0, 2, 0, 2), tcr);

          // MAIR: Index 0 = 0xFF (write-back), Index 1 = 0xBB (write-through)
          std::uint64_t mair = 0xBBFFULL;
          ::_WriteStatusReg(ARM64_SYSREG(3, 0, 10, 2, 0), mair);

          ::__dsb(_ARM64_BARRIER_ISH);
          ::_WriteStatusReg(ARM64_SYSREG(3, 0, 2, 0, 0), pageTableRoot);
          ::_WriteStatusReg(ARM64_SYSREG(3, 0, 2, 0, 1), pageTableRoot);
          ::__isb(_ARM64_BARRIER_SY);
#else
          std::uint64_t tcr = 0;
          tcr |= (16ULL << 0);   // T0SZ = 16 => 48-bit VA
          tcr |= (1ULL << 8);    // IRGN0 = write-back, write-allocate
          tcr |= (1ULL << 10);   // ORGN0 = write-back, write-allocate
          tcr |= (3ULL << 12);   // SH0 = inner shareable
          tcr |= (0ULL << 14);   // TG0 = 4KB granule
          tcr |= (16ULL << 16);  // T1SZ = 16 => 48-bit VA
          tcr |= (1ULL << 24);   // IRGN1 = write-back, write-allocate
          tcr |= (1ULL << 26);   // ORGN1 = write-back, write-allocate
          tcr |= (3ULL << 28);   // SH1 = inner shareable
          tcr |= (2ULL << 30);   // TG1 = 4KB granule
          tcr |= (0x5ULL << 32); // IPS = 48-bit PA
          asm volatile("msr tcr_el1, %0" ::"r"(tcr));

          // MAIR: Index 0 = 0xFF (write-back), Index 1 = 0xBB (write-through)
          std::uint64_t mair = 0xBBFFULL;
          asm volatile("msr mair_el1, %0" ::"r"(mair));

          asm volatile("dsb ish");
          asm volatile("msr ttbr0_el1, %0" ::"r"(pageTableRoot));
          asm volatile("msr ttbr1_el1, %0" ::"r"(pageTableRoot));
          asm volatile("isb");
#endif
     }

     std::uintptr_t GetCurrentPageTable()
     {
#ifdef COMPILER_MSVC
          return ::_ReadStatusReg(ARM64_SYSREG(3, 0, 2, 0, 0));
#else
          std::uintptr_t ttbr0 = 0;
          asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0));
          return ttbr0;
#endif
     }

     void InvalidatePage(std::uintptr_t virtualAddress)
     {
#ifdef COMPILER_MSVC
          ::__isb(_ARM64_BARRIER_SY);
#else
          asm volatile("tlbi vaae1is, %0" ::"r"(virtualAddress >> 12));
          asm volatile("dsb ish");
          asm volatile("isb");
#endif
     }

     void FlushTLB()
     {
#ifdef COMPILER_MSVC
          ::__isb(_ARM64_BARRIER_SY);
#else
          asm volatile("tlbi vmalle1is");
          asm volatile("dsb ish");
          asm volatile("isb");
#endif
     }

     void EnablePaging()
     {
#ifdef COMPILER_MSVC
          std::uint64_t sctlr = ::_ReadStatusReg(ARM64_SYSREG(3, 0, 1, 0, 0));

          sctlr |= (1ULL << 0);
          sctlr |= (1ULL << 2);
          sctlr |= (1ULL << 12);

          ::__dsb(_ARM64_BARRIER_ISH);
          ::_WriteStatusReg(ARM64_SYSREG(3, 0, 1, 0, 0), sctlr);
          ::__isb(_ARM64_BARRIER_SY);
#else
          std::uint64_t sctlr = 0;
          asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));

          sctlr |= (1ULL << 0);
          sctlr |= (1ULL << 2);
          sctlr |= (1ULL << 12);

          asm volatile("dsb ish");
          asm volatile("msr sctlr_el1, %0" ::"r"(sctlr));
          asm volatile("isb");
#endif
     }
#endif
} // namespace memory::paging
