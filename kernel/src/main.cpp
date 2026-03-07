#include <cstdint>
#include <cstring>
#include <memory>
#include "kinit.h"
#include "memory/pallocator.h"
#include "process.h"
#include "utils/arch.h"
#include "utils/cpu.h"
#include "utils/identify.h"
#include "utils/operations.h"

alignas(std::max_align_t) static std::byte g_kernelProcessStorage[sizeof(kernel::ProcessControlBlock)]; // NOLINT

void KiIdleLoop()
{
     while (true)
     {
          operations::EnableInterrupts();
          operations::Yield();
          operations::Yield();
          operations::DisableInterrupts();

          operations::Halt();
     }
}
void Error(std::uint32_t* buffer, arch::LoaderParameterBlock* param)
{
     for (std::size_t i = 0; i < param->framebuffer.height; i++)
          std::uninitialized_fill_n(buffer + (i * param->framebuffer.scanlineSize), param->framebuffer.scanlineSize,
                                    0x300af0);

     operations::DisableInterrupts();
     while (true) operations::Halt();
}

constexpr std::uint32_t GetColourForPFNUse(memory::PFNUse use)
{
     switch (use)
     {
     case memory::PFNUse::Unused: return 0x202020;         // Dark grey
     case memory::PFNUse::ProcessPrivate: return 0x3498db; // Blue
     case memory::PFNUse::MappedFile: return 0x9b59b6;     // Purple
     case memory::PFNUse::DriverLocked: return 0xe74c3c;   // Red
     case memory::PFNUse::UserStack: return 0x1abc9c;      // Turquoise
     case memory::PFNUse::KernelStack: return 0xf39c12;    // Orange
     case memory::PFNUse::KernelHeap: return 0xe67e22;     // Dark orange
     case memory::PFNUse::MetaFile: return 0x8e44ad;       // Dark purple
     case memory::PFNUse::NonPagedPool: return 0xc0392b;   // Dark red
     case memory::PFNUse::PagedPool: return 0x16a085;      // Dark turquoise
     case memory::PFNUse::PTE: return 0xf1c40f;            // Yellow
     case memory::PFNUse::Shareable: return 0x27ae60;      // Green
     case memory::PFNUse::PageTable: return 0x2ecc71;      // Light green
     case memory::PFNUse::FSCache: return 0x95a5a6;        // Light gray
     default: return 0x000000;                             // Black
     }
}

constexpr std::uint32_t GetColourForPFNRegion(memory::PFNRegion use)
{
     switch (use)
     {
     case memory::PFNRegion::Active: return 0x0000ff;
     case memory::PFNRegion::Standby: return 0x2020fa;
     case memory::PFNRegion::Free: return 0x000000;
     case memory::PFNRegion::Modified: return 0xffff00;
     case memory::PFNRegion::Bad: return 0xff0000;
     case memory::PFNRegion::Zero: return 0x2e2e2e;
     }
}

constexpr std::size_t kMemoryBarHeight = 40;

constexpr std::uint32_t kTestPassColour = 0x00ff00; // Green
constexpr std::uint32_t kTestFailColour = 0xff0000; // Red
constexpr std::size_t kTestSquareSize = 25;
constexpr std::size_t kTestSquareSpacing = 5;

void DrawTestResult(std::uint32_t* buffer, const arch::Framebuffer& framebuffer, std::size_t testIndex, bool passed)
{
     const std::uint32_t colour = passed ? kTestPassColour : kTestFailColour;
     const std::size_t baseX = (testIndex * (kTestSquareSize + kTestSquareSpacing)) + 10;
     const std::size_t baseY = 10;

     for (std::size_t y = 0; y < kTestSquareSize; y++)
     {
          for (std::size_t x = 0; x < kTestSquareSize; x++)
          {
               const std::size_t pixelX = baseX + x;
               const std::size_t pixelY = baseY + y;
               if (pixelX < framebuffer.width && pixelY < framebuffer.height)
               {
                    buffer[(pixelY * framebuffer.scanlineSize) + pixelX] = colour;
               }
          }
     }
}

template <std::size_t N>
void DrawBar(const memory::PFNRegion (&regionOrder)[N], std::size_t totalPages, std::size_t barWidth, // NOLINT
             const arch::Framebuffer& framebuffer, std::uint32_t* buffer, std::size_t offset)
{
     std::size_t orderedPageIndex = 0;
     for (const auto region : regionOrder)
     {
          for (std::size_t pfnIndex = 0; pfnIndex < totalPages; pfnIndex++)
          {
               if (pfnIndex >= memory::physicalAllocator.databaseSize) break;

               const auto& pfn = memory::physicalAllocator.database[pfnIndex];
               if (pfn.region != region) continue;

               const std::size_t startX = (orderedPageIndex * barWidth) / totalPages;
               const std::size_t endX = ((orderedPageIndex + 1) * barWidth) / totalPages;

               const std::uint32_t colour = GetColourForPFNUse(pfn.use);
               const std::uint32_t colour2 = GetColourForPFNRegion(pfn.region);

               for (std::size_t x = startX; x < endX; x++)
               {
                    for (std::size_t y = 0; y < kMemoryBarHeight; y++)
                    {
                         buffer[((y + offset) * framebuffer.scanlineSize) + x] = colour;
                    }
                    for (std::size_t y = kMemoryBarHeight / 2; y < kMemoryBarHeight; y++)
                    {
                         buffer[((y + offset) * framebuffer.scanlineSize) + x] = colour2;
                    }
               }

               orderedPageIndex++;
          }
     }
}

void KiInitialise(arch::LoaderParameterBlock* param)
{
     g_bootCycles = operations::ReadCurrentCycles();
     g_loaderBlock = param;
     auto framebuffer = param->framebuffer;

     cpu::Initialise();

     std::uint32_t* buffer = reinterpret_cast<std::uint32_t*>(param->framebuffer.physicalStart);

     auto status = memory::physicalAllocator.Initialise(param->memoryDescriptors, param->kernelPhysicalBase,
                                                        0xffff'8000'0000'0000, param->kernelSize);
     if (!status) Error(buffer, param);

     g_kernelProcess = new (g_kernelProcessStorage) kernel::ProcessControlBlock(0, "System", 0xffff'a000'0000'0000);
     g_kernelProcess->SetState(kernel::ProcessState::Running);
     g_kernelProcess->SetPriority(kernel::ProcessPriority::Realtime);

     for (std::size_t i = 0; i < param->framebuffer.height; i++)
          std::uninitialized_fill_n(buffer + (i * framebuffer.scanlineSize), framebuffer.scanlineSize, 0);
}

extern "C" int KiStartup(arch::LoaderParameterBlock* param)
{
     if (param->systemMajor != OsVersionMajor || param->systemMinor != OsVersionMinor) return 1;

     KiInitialise(param);
     auto framebuffer = param->framebuffer;
     std::uint32_t* buffer = reinterpret_cast<std::uint32_t*>(param->framebuffer.physicalStart);

     constexpr memory::PFNRegion regionOrder[] = {memory::PFNRegion::Active,   memory::PFNRegion::Standby,
                                                  memory::PFNRegion::Modified, memory::PFNRegion::Zero,
                                                  memory::PFNRegion::Free,     memory::PFNRegion::Bad};

     const std::size_t barWidth = framebuffer.width;
     const std::size_t totalPages = memory::physicalAllocator.databaseSize;
     const auto size = framebuffer.totalSize;

     auto& vad = g_kernelProcess->GetVadAllocator();
     std::size_t testIndex = 0;

     // Test 1: Basic allocation
     bool test1 = false;
     const auto addr1 =
         g_kernelProcess->ReserveMemory(4096, memory::MemoryProtection::ReadWrite, memory::PFNUse::ProcessPrivate);
     test1 = (addr1 != 0);
     DrawTestResult(buffer, framebuffer, testIndex++, test1);

     // Test 2: Sequential allocation
     bool test2 = false;
     const auto addr2 =
         g_kernelProcess->ReserveMemory(8192, memory::MemoryProtection::ReadWrite, memory::PFNUse::ProcessPrivate);
     test2 = (addr2 != 0 && addr2 > addr1);
     DrawTestResult(buffer, framebuffer, testIndex++, test2);

     // Test 3: Fixed address reservation
     const std::uintptr_t fixedAddr = 0xffff'a000'0010'0000;
     bool test3 = g_kernelProcess->ReserveMemoryFixed(fixedAddr, 4096, memory::MemoryProtection::ReadWrite,
                                                      memory::PFNUse::KernelHeap);
     DrawTestResult(buffer, framebuffer, testIndex++, test3);

     // Test 4: Overlap detection - should fail
     bool test4 = !g_kernelProcess->ReserveMemoryFixed(fixedAddr, 4096, memory::MemoryProtection::ReadWrite,
                                                       memory::PFNUse::KernelHeap);
     DrawTestResult(buffer, framebuffer, testIndex++, test4);

     // Test 5: Partial overlap detection
     bool test5 = !g_kernelProcess->ReserveMemoryFixed(fixedAddr + 2048, 4096, memory::MemoryProtection::ReadWrite,
                                                       memory::PFNUse::KernelHeap);
     DrawTestResult(buffer, framebuffer, testIndex++, test5);

     // Test 6: FindContaining (exact base)
     auto* node = g_kernelProcess->QueryAddress(addr1);
     bool test6 = (node != nullptr && node->entry.baseAddress == addr1);
     DrawTestResult(buffer, framebuffer, testIndex++, test6);

     // Test 7: FindContaining (middle of allocation)
     node = g_kernelProcess->QueryAddress(addr1 + 2048);
     bool test7 = (node != nullptr && node->entry.baseAddress == addr1);
     DrawTestResult(buffer, framebuffer, testIndex++, test7);

     // Test 8: Query non-existent address
     node = g_kernelProcess->QueryAddress(0xffff'ffff'ffff'0000);
     bool test8 = (node == nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test8);

     // Test 9: Free memory
     bool test9 = g_kernelProcess->FreeMemory(addr1);
     DrawTestResult(buffer, framebuffer, testIndex++, test9);

     // Test 10: After freeing, query should return null
     node = g_kernelProcess->QueryAddress(addr1);
     bool test10 = (node == nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test10);

     // Test 11: Allocate at hint address
     const auto addr3 = g_kernelProcess->ReserveMemoryAt(
         0xffff'a000'0020'0000, 4096, memory::MemoryProtection::ReadWrite, memory::PFNUse::UserStack);
     bool test11 = (addr3 != 0);
     DrawTestResult(buffer, framebuffer, testIndex++, test11);

     // Test 12: Multiple allocations and search
     const auto addr4 =
         g_kernelProcess->ReserveMemory(16384, memory::MemoryProtection::Execute, memory::PFNUse::ProcessPrivate);
     node = g_kernelProcess->QueryAddress(addr4 + 8192);
     bool test12 = (addr4 != 0 && node != nullptr && node->entry.size == 16384 &&
                    node->entry.protection == memory::MemoryProtection::Execute);
     DrawTestResult(buffer, framebuffer, testIndex++, test12);

     // Test 13: Verify search still works after multiple operations
     node = g_kernelProcess->QueryAddress(addr2);
     bool test13 = (node != nullptr && node->entry.baseAddress == addr2 && node->entry.size == 8192);
     DrawTestResult(buffer, framebuffer, testIndex++, test13);

     // Test 14: Free non-existent address should fail
     bool test14 = !g_kernelProcess->FreeMemory(0xffff'a000'dead'beef);
     DrawTestResult(buffer, framebuffer, testIndex++, test14);

     // Test 15: Allocate after freed space
     const auto addr5 =
         g_kernelProcess->ReserveMemory(4096, memory::MemoryProtection::ReadOnly, memory::PFNUse::FSCache);
     bool test15 = (addr5 != 0);
     DrawTestResult(buffer, framebuffer, testIndex++, test15);

     // Test 16: Search boundaries
     auto* search1 = vad.Search(addr2);
     bool test16 = (search1 != nullptr && search1->entry.baseAddress == addr2);
     DrawTestResult(buffer, framebuffer, testIndex++, test16);

     // Test 17: Overlap detection with existing allocations
     auto* overlap = vad.FindOverlap(addr2, 4096);
     bool test17 = (overlap != nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test17);

     // Test 18: Overlap with reused freed space (addr5 reused addr1's location)
     overlap = vad.FindOverlap(addr2 - 4096, 2048);
     bool test18 = (overlap != nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test18);

     DrawBar(regionOrder, totalPages, barWidth, framebuffer, buffer,
             (static_cast<std::size_t>(framebuffer.height) / 2uz) - (kMemoryBarHeight / 2));

     KiIdleLoop();
     return 0;
}
