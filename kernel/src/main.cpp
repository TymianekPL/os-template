#include <cstdint>
#include <cstring>
#include <memory>
#include "kinit.h"
#include "memory/pallocator.h"
#include "memory/vallocator.h"
#include "process.h"
#include "utils/arch.h"
#include "utils/cpu.h"
#include "utils/identify.h"
#include "utils/kdbg.h"
#include "utils/memory.h"
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
     debugging::DbgWrite(u8"Test {}: {}\r\n", testIndex, passed);

     const std::uint32_t colour = passed ? kTestPassColour : kTestFailColour;
     std::size_t baseX = (testIndex * (kTestSquareSize + kTestSquareSpacing));
     std::size_t baseY = 10;
     while (baseX + kTestSquareSize >= framebuffer.width)
     {
          baseX -= framebuffer.width;
          baseY += kTestSquareSize + kTestSquareSpacing;
     }
     baseX += 10;

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
     g_kernelProcess->SetPageTableBase(memory::paging::GetCurrentPageTable());

     for (std::size_t i = 0; i < param->framebuffer.height; i++)
          std::uninitialized_fill_n(buffer + (i * framebuffer.scanlineSize), framebuffer.scanlineSize, 0);

     memory::virtualOffset = 0xffff'8000'0000'0000;

     g_kernelProcess->ReserveMemoryFixedAsCommitted(param->stackVirtualBase, param->stackSize,
                                                    memory::MemoryProtection::ReadWrite, memory::PFNUse::KernelStack);
     g_kernelProcess->ReserveMemoryFixedAsCommitted(param->kernelVirtualBase, param->kernelSize,
                                                    memory::MemoryProtection::ExecuteReadWrite,
                                                    memory::PFNUse::KernelHeap);
     g_kernelProcess->ReserveMemoryFixed(memory::virtualOffset, memory::physicalAllocator.databaseSize * 0x1000uz,
                                         memory::MemoryProtection::ReadWrite, memory::PFNUse::DriverLocked);
}
void PrintVadEntryCallback(const memory::VADEntry& entry)
{
     using namespace debugging;

     DbgWrite(u8"Base: {}, Size: {}, Protection: ", reinterpret_cast<void*>(entry.baseAddress), entry.size);
     switch (entry.protection)
     {
     case memory::MemoryProtection::ReadOnly: DbgWrite(u8"RO"); break;
     case memory::MemoryProtection::ReadWrite: DbgWrite(u8"RW"); break;
     case memory::MemoryProtection::Execute: DbgWrite(u8"X"); break;
     case memory::MemoryProtection::ExecuteReadWrite: DbgWrite(u8"RWX"); break;
     default: DbgWrite(u8"?"); break;
     }

     DbgWrite(u8", State: ");
     switch (entry.state)
     {
     case memory::VADMemoryState::Free: DbgWrite(u8"Free"); break;
     case memory::VADMemoryState::Reserved: DbgWrite(u8"Reserved"); break;
     case memory::VADMemoryState::Committed: DbgWrite(u8"Committed"); break;
     case memory::VADMemoryState::PagedOut: DbgWrite(u8"PagedOut"); break;
     default: DbgWrite(u8"?"); break;
     }

     DbgWrite(u8", PFNUse: ");
     switch (entry.use)
     {
     case memory::PFNUse::ProcessPrivate: DbgWrite(u8"ProcessPrivate"); break;
     case memory::PFNUse::KernelHeap: DbgWrite(u8"KernelHeap"); break;
     case memory::PFNUse::UserStack: DbgWrite(u8"UserStack"); break;
     case memory::PFNUse::DriverLocked: DbgWrite(u8"DriverLocked"); break;
     case memory::PFNUse::FSCache: DbgWrite(u8"FSCache"); break;
     default: DbgWrite(u8"?"); break;
     }

     DbgWrite(u8"\r\n");
}
void PrintVMStats()
{
     const auto& stats = g_kernelProcess->GetVadAllocator().GetStatistics();

     debugging::DbgWrite(u8"==== Virtual Memory Statistics ====\r\n");
     debugging::DbgWrite(u8"Total Reserved: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.totalReservedBytes.load());
     debugging::DbgWrite(u8"\r\nTotal Committed: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.totalCommittedBytes.load());
     debugging::DbgWrite(u8"\r\nTotal Immediate: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.totalImmediateBytes.load());
     debugging::DbgWrite(u8"\r\nPeak Reserved: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.peakReservedBytes.load());
     debugging::DbgWrite(u8"\r\nPeak Committed: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.peakCommittedBytes.load());
     debugging::DbgWrite(u8"\r\nCommit Charge: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.commitCharge.load());
     debugging::DbgWrite(u8"\r\nReserve Ops: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.reserveOperations.load());
     debugging::DbgWrite(u8"\r\nCommit Ops: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.commitOperations.load());
     debugging::DbgWrite(u8"\r\nDecommit Ops: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.decommitOperations.load());
     debugging::DbgWrite(u8"\r\nRelease Ops: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.releaseOperations.load());
     debugging::DbgWrite(u8"\r\nFailed Allocations: ");
     debugging::SerialFormatter<std::size_t>::Write(stats.failedAllocations.load());
     debugging::DbgWrite(u8"\r\n==== End of VM Stats ====\r\n");
}
void PrintPhysicalStats()
{
     auto& pma = memory::physicalAllocator;

     debugging::DbgWrite(u8"==== Physical Memory Statistics ====\r\n");
     debugging::DbgWrite(u8"Active Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.activePages.load());
     debugging::DbgWrite(u8"\r\nFree Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.freePages.load());
     debugging::DbgWrite(u8"\r\nZero Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.zeroPages.load());
     debugging::DbgWrite(u8"\r\nStandby Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.standbyPages.load());
     debugging::DbgWrite(u8"\r\nModified Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.modifiedPages.load());
     debugging::DbgWrite(u8"\r\nBad Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.badPages.load());

     debugging::DbgWrite(u8"\r\nUnused Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.unusedPages.load());
     debugging::DbgWrite(u8"\r\nProcessPrivate Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.processPrivatePages.load());
     debugging::DbgWrite(u8"\r\nMappedFile Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.mappedFilePages.load());
     debugging::DbgWrite(u8"\r\nDriverLocked Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.driverLockedPages.load());
     debugging::DbgWrite(u8"\r\nUserStack Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.userStackPages.load());
     debugging::DbgWrite(u8"\r\nKernelStack Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.kernelStackPages.load());
     debugging::DbgWrite(u8"\r\nKernelHeap Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.kernelHeapPages.load());
     debugging::DbgWrite(u8"\r\nMetaFile Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.metaFilePages.load());
     debugging::DbgWrite(u8"\r\nNonPagedPool Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.nonPagedPoolPages.load());
     debugging::DbgWrite(u8"\r\nPagedPool Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.pagedPoolPages.load());
     debugging::DbgWrite(u8"\r\nPTE Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.ptePages.load());
     debugging::DbgWrite(u8"\r\nShareable Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.shareablePages.load());
     debugging::DbgWrite(u8"\r\nPageTable Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.pageTablePages.load());
     debugging::DbgWrite(u8"\r\nFSCache Pages: ");
     debugging::SerialFormatter<std::size_t>::Write(pma.fsCachePages.load());
     debugging::DbgWrite(u8"\r\n==== End of Physical Stats ====\r\n");
}
static std::size_t PrintSize(std::size_t bytes)
{
     constexpr std::size_t KiB = 1024;
     constexpr std::size_t MiB = 1024 * KiB;
     constexpr std::size_t GiB = 1024 * MiB;

     std::size_t written{};

     auto print = [&](std::size_t v, const char8_t* s)
     {
          debugging::DbgWrite(u8"{} {}", v, s);

          std::array<char, 32> buffer{};
          auto [p, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), v);
          written = static_cast<std::size_t>(p - buffer.data()) + 1;

          while (*s++) ++written;
     };

     if (bytes >= GiB) print(bytes / GiB, u8"GiB");
     else if (bytes >= MiB)
          print(bytes / MiB, u8"MiB");
     else if (bytes >= KiB)
          print(bytes / KiB, u8"KiB");
     else
          print(bytes, u8"B");

     return written;
}
static void Pad(std::size_t count)
{
     for (std::size_t i = 0; i < count; i++) debugging::DbgWrite(u8" ");
}
static void PrintCol(const char8_t* s, std::size_t width)
{
     std::size_t len{};

     while (s[len]) ++len;

     debugging::DbgWrite(u8"{}", s);

     if (len < width) Pad(width - len);
}
static void PrintVADHeader()
{
     debugging::DbgWrite(u8"\r\n");

     PrintCol(u8"Base", 18);
     PrintCol(u8"End", 18);
     PrintCol(u8"Size", 10);
     PrintCol(u8"State", 12);
     PrintCol(u8"Protect", 20);
     PrintCol(u8"Type", 16);

     debugging::DbgWrite(u8"\r\n");
}
static std::size_t PrintHex(std::uintptr_t v)
{
     std::array<char, 17> buffer{};
     auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), v, 16);
     *ptr = '\0';

     debugging::DbgWrite(u8"{}", reinterpret_cast<const char8_t*>(buffer.data()));
     return ptr - buffer.data();
}
static const char8_t* ProtToStr(memory::MemoryProtection p)
{
     using enum memory::MemoryProtection;

     switch (p)
     {
     case memory::MemoryProtection::ReadOnly: return u8"read-only";
     case memory::MemoryProtection::ReadWrite: return u8"read-write";
     case memory::MemoryProtection::Execute: return u8"execute-only";
     case memory::MemoryProtection::ExecuteRead: return u8"read-execute";
     case memory::MemoryProtection::ExecuteReadWrite: return u8"read-execute-write";
     default: return u8"Unknown";
     }
}
static const char8_t* StateToStr(memory::VADMemoryState s)
{
     using enum memory::VADMemoryState;

     switch (s)
     {
     case Free: return u8"Free";
     case Reserved: return u8"Reserved";
     case Committed: return u8"Committed";
     default: return u8"Unknown";
     }
}
static const char8_t* UseToStr(memory::PFNUse u)
{
     using enum memory::PFNUse;

     switch (u)
     {
     case ProcessPrivate: return u8"ProcessPrivate";
     case MappedFile: return u8"MappedFile";
     case DriverLocked: return u8"DriverLocked";
     case UserStack: return u8"UserStack";
     case KernelStack: return u8"KernelStack";
     case KernelHeap: return u8"KernelHeap";
     case NonPagedPool: return u8"NonPagedPool";
     case PagedPool: return u8"PagedPool";
     case PageTable: return u8"PageTable";
     case FSCache: return u8"FSCache";
     default: return u8"Other";
     }
}
static void PrintVADRow(const memory::VADEntry& e)
{
     const auto base = e.baseAddress;
     const auto end = e.baseAddress + e.size;

     PrintHex(base);
     Pad(18 - 16);

     PrintHex(end);
     Pad(18 - 16);

     const auto sizeWidth = PrintSize(e.size);
     Pad(10 - sizeWidth);

     PrintCol(StateToStr(e.state), 12);
     PrintCol(ProtToStr(e.protection), 20);
     PrintCol(UseToStr(e.use), 16);

     debugging::DbgWrite(u8"\r\n");
}
constexpr const char8_t* FormatSize(std::size_t size, std::size_t& outVal)
{
     if (size >= 1024uz * 1024uz * 1024)
     {
          outVal = size / (1024uz * 1024 * 1024);
          return u8"GiB";
     }
     if (size >= 1024uz * 1024)
     {
          outVal = size / (1024uz * 1024);
          return u8"MiB";
     }
     if (size >= 1024)
     {
          outVal = size / 1024;
          return u8"KiB";
     }
     outVal = size;
     return u8"B";
}
extern "C" int KiStartup(arch::LoaderParameterBlock* param)
{
     if (param->systemMajor != OsVersionMajor || param->systemMinor != OsVersionMinor) return 1;

     KiInitialise(param);

     auto& vad = g_kernelProcess->GetVadAllocator();
     std::size_t testIndex = 0;
     auto framebuffer = param->framebuffer;
     std::uint32_t* buffer = reinterpret_cast<std::uint32_t*>(param->framebuffer.physicalStart);

     constexpr memory::PFNRegion regionOrder[] = {memory::PFNRegion::Active,   memory::PFNRegion::Standby,
                                                  memory::PFNRegion::Modified, memory::PFNRegion::Zero,
                                                  memory::PFNRegion::Free,     memory::PFNRegion::Bad};

     const std::size_t barWidth = framebuffer.width;
     const std::size_t totalPages = memory::physicalAllocator.databaseSize;
     const auto size = framebuffer.totalSize;

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

     // ===== NEW COMMIT/DECOMMIT TESTS =====

     // Test 19: AllocateVirtualMemory with Reserve only
     void* allocAddr1 = g_kernelProcess->AllocateVirtualMemory(nullptr, 8192, memory::AllocationFlags::Reserve,
                                                               memory::MemoryProtection::ReadWrite);
     bool test19 = (allocAddr1 != nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test19);

     // Test 20: Verify reserved memory is in Reserved state
     if (allocAddr1)
     {
          auto* allocNode1 = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(allocAddr1));
          bool test20 = (allocNode1 != nullptr && allocNode1->entry.state == memory::VADMemoryState::Reserved);
          DrawTestResult(buffer, framebuffer, testIndex++, test20);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 21: AllocateVirtualMemory with Reserve + Commit (lazy)
     void* allocAddr2 = g_kernelProcess->AllocateVirtualMemory(
         nullptr, 16384, memory::AllocationFlags::Reserve | memory::AllocationFlags::Commit,
         memory::MemoryProtection::ReadWrite);
     bool test21 = (allocAddr2 != nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test21);

     // Test 22: Verify committed memory is in Committed state (lazy)
     if (allocAddr2)
     {
          auto* allocNode2 = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(allocAddr2));
          bool test22 = (allocNode2 != nullptr && allocNode2->entry.state == memory::VADMemoryState::Committed &&
                         !allocNode2->entry.immediatePhysical);
          DrawTestResult(buffer, framebuffer, testIndex++, test22);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 23: AllocateVirtualMemory with Reserve + Commit + ImmediatePhysical
     void* allocAddr3 =
         g_kernelProcess->AllocateVirtualMemory(nullptr, 12288,
                                                memory::AllocationFlags::Reserve | memory::AllocationFlags::Commit |
                                                    memory::AllocationFlags::ImmediatePhysical,
                                                memory::MemoryProtection::ReadWrite);
     bool test23 = (allocAddr3 != nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test23);

     // Test 24: Verify immediate physical memory has flag set
     if (allocAddr3)
     {
          auto* allocNode3 = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(allocAddr3));
          bool test24 = (allocNode3 != nullptr && allocNode3->entry.state == memory::VADMemoryState::Committed &&
                         allocNode3->entry.immediatePhysical);
          DrawTestResult(buffer, framebuffer, testIndex++, test24);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 25: Decommit previously committed memory
     bool test25 = false;
     if (allocAddr2)
     {
          test25 = g_kernelProcess->ReleaseVirtualMemory(allocAddr2, 0, memory::AllocationFlags::Decommit);
     }
     DrawTestResult(buffer, framebuffer, testIndex++, test25);

     // Test 26: Verify decommitted memory is back to Reserved state
     if (allocAddr2 && test25)
     {
          auto* allocNode2Check = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(allocAddr2));
          bool test26 =
              (allocNode2Check != nullptr && allocNode2Check->entry.state == memory::VADMemoryState::Reserved);
          DrawTestResult(buffer, framebuffer, testIndex++, test26);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 27: Release entire allocation
     bool test27 = false;
     if (allocAddr1)
     {
          test27 = g_kernelProcess->ReleaseVirtualMemory(allocAddr1, 0, memory::AllocationFlags::Release);
     }
     DrawTestResult(buffer, framebuffer, testIndex++, test27);

     // Test 28: Verify released memory is gone
     if (allocAddr1 && test27)
     {
          auto* allocNode1Check = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(allocAddr1));
          bool test28 = (allocNode1Check == nullptr);
          DrawTestResult(buffer, framebuffer, testIndex++, test28);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 29: Check statistics are being tracked
     const auto& stats = vad.GetStatistics();
     bool test29 = (stats.reserveOperations.load(std::memory_order_relaxed) > 0 &&
                    stats.commitOperations.load(std::memory_order_relaxed) > 0);
     DrawTestResult(buffer, framebuffer, testIndex++, test29);

     // Test 30: Verify commit charge and reserved bytes are tracked
     bool test30 = (stats.totalReservedBytes.load(std::memory_order_relaxed) > 0);
     DrawTestResult(buffer, framebuffer, testIndex++, test30);

     // ===== PARTIAL DECOMMIT TESTS =====

     // Test 31: Allocate and commit a large region for partial decommit testing
     void* partialAddr = g_kernelProcess->AllocateVirtualMemory(
         nullptr, 32768, memory::AllocationFlags::Reserve | memory::AllocationFlags::Commit,
         memory::MemoryProtection::ReadWrite);
     bool test31 = (partialAddr != nullptr);
     DrawTestResult(buffer, framebuffer, testIndex++, test31);

     // Test 32: Partially decommit the middle (8KB starting at offset 8KB)
     bool test32 = false;
     if (partialAddr)
     {
          void* middleAddr = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(partialAddr) + 8192);
          test32 = g_kernelProcess->ReleaseVirtualMemory(middleAddr, 8192, memory::AllocationFlags::Decommit);
     }
     DrawTestResult(buffer, framebuffer, testIndex++, test32);

     // Test 33: Verify the front part is still committed
     if (partialAddr && test32)
     {
          auto* frontNode = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(partialAddr));
          bool test33 = (frontNode != nullptr && frontNode->entry.state == memory::VADMemoryState::Committed &&
                         frontNode->entry.size == 8192);

          DrawTestResult(buffer, framebuffer, testIndex++, test33);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 34: Verify the middle part is reserved (decommitted)
     if (partialAddr && test32)
     {
          void* middleAddr = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(partialAddr) + 8192);
          auto* middleNode = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(middleAddr));
          bool test34 = (middleNode != nullptr && middleNode->entry.state == memory::VADMemoryState::Reserved &&
                         middleNode->entry.size == 8192);
          DrawTestResult(buffer, framebuffer, testIndex++, test34);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 35: Verify the back part is still committed
     if (partialAddr && test32)
     {
          void* backAddr = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(partialAddr) + 16384);
          auto* backNode = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(backAddr));
          bool test35 = (backNode != nullptr && backNode->entry.state == memory::VADMemoryState::Committed &&
                         backNode->entry.size == 16384);
          DrawTestResult(buffer, framebuffer, testIndex++, test35);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     // Test 36: Decommit from start (partial from beginning)
     void* headDecommitAddr = g_kernelProcess->AllocateVirtualMemory(
         nullptr, 16384, memory::AllocationFlags::Reserve | memory::AllocationFlags::Commit,
         memory::MemoryProtection::ReadWrite);
     bool test36 = false;
     if (headDecommitAddr)
     {
          test36 = g_kernelProcess->ReleaseVirtualMemory(headDecommitAddr, 4096, memory::AllocationFlags::Decommit);
     }
     DrawTestResult(buffer, framebuffer, testIndex++, test36);

     // Test 37: Verify head is decommitted and tail is still committed
     if (headDecommitAddr && test36)
     {
          auto* headNode = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(headDecommitAddr));
          void* tailAddr = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(headDecommitAddr) + 4096);
          auto* tailNode = g_kernelProcess->QueryAddress(reinterpret_cast<std::uintptr_t>(tailAddr));

          bool test37 = (headNode != nullptr && headNode->entry.state == memory::VADMemoryState::Reserved &&
                         headNode->entry.size == 4096 && tailNode != nullptr &&
                         tailNode->entry.state == memory::VADMemoryState::Committed && tailNode->entry.size == 12288);
          DrawTestResult(buffer, framebuffer, testIndex++, test37);
     }
     else
     {
          DrawTestResult(buffer, framebuffer, testIndex++, false);
     }

     DrawBar(regionOrder, totalPages, barWidth, framebuffer, buffer,
             (static_cast<std::size_t>(framebuffer.height) / 2uz) - (kMemoryBarHeight / 2));

     while (true)
     {
          std::array<char, 64> lineBuffer{};
          debugging::DbgWrite(u8"> ");
          const auto len = debugging::ReadSerialLine(lineBuffer.data(), lineBuffer.size());
          debugging::DbgWrite(u8"\r\n");
          if (len == 0) continue;
          std::string_view line{lineBuffer.data(), len};

          switch (line[0])
          {
          case 'm':
          {
               auto& vad = g_kernelProcess->GetVadAllocator();
               debugging::DbgWrite(u8"==== VAD Allocation Map ====\r\n");
               vad.InorderTraversal(vad.GetRoot(), PrintVadEntryCallback);
               debugging::DbgWrite(u8"==== End of VAD Map ====\r\n");
          }
          break;

          case 's': PrintVMStats(); break;

          case 'p': PrintPhysicalStats(); break;

          case 'v':
          {
               auto printVAD = [](const memory::VADEntry& entry)
               {
                    debugging::DbgWrite(u8"Base={}  Size=", reinterpret_cast<void*>(entry.baseAddress));
                    PrintSize(entry.size);

                    debugging::DbgWrite(u8"  State={}  Prot={}  Use={}  {}\r\n", StateToStr(entry.state),
                                        ProtToStr(entry.protection), UseToStr(entry.use),
                                        entry.immediatePhysical ? u8"Immediate" : u8"Demand");
               };

               g_kernelProcess->GetVadAllocator().InorderTraversal(g_kernelProcess->GetVadAllocator().GetRoot(),
                                                                   printVAD);

               break;
          }
          case 'r': // read memory
          {
               if (line.size() < 3) break;
               std::uintptr_t addr{};
               auto [ptr, ec] = std::from_chars(&line[2], &line[line.size()], addr, 16);
               if (ec != std::errc()) break;

               debugging::DbgWrite(u8"Address            00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  | ASCII\r\n");

               auto readByte = [&](std::uintptr_t byteAddr) -> std::uint8_t*
               {
                    static std::uint8_t storage{};

                    auto* node = g_kernelProcess->GetVadAllocator().FindContaining(byteAddr);
                    if (!node) return nullptr;
                    if (node->entry.state != memory::VADMemoryState::Committed) return nullptr;
                    storage = byteAddr;
                    return &storage;
               };

               for (std::size_t row = 0; row < 4; row++)
               {
                    std::uintptr_t rowAddr = addr + (row * 16);
                    debugging::DbgWrite(u8"{} ", reinterpret_cast<void*>(rowAddr));

                    for (std::size_t col = 0; col < 16; col++)
                    {
                         std::uintptr_t byteAddr = rowAddr + col;
                         const auto* val = readByte(byteAddr);
                         if (val != nullptr) debugging::DbgWrite(u8"{} ", std::byte{*val});
                         else
                              debugging::DbgWrite(u8"?? ");
                    }

                    debugging::DbgWrite(u8" | ");

                    for (std::size_t col = 0; col < 16; col++)
                    {
                         std::uintptr_t byteAddr = rowAddr + col;
                         const auto* val = readByte(byteAddr);
                         char8_t c = '.';
                         if (val != nullptr && *val >= 32 && *val < 127) c = static_cast<char8_t>(*val);
                         debugging::DbgWrite(u8"{}", c);
                    }

                    debugging::DbgWrite(u8"\r\n");
               }
               break;
          }
          case 'q': // query VAD
          {
               if (line.size() < 3) break;
               std::uintptr_t addr{};
               auto [ptr, ec] = std::from_chars(&line[2], &line[line.size()], addr, 16);
               if (ec != std::errc()) break;

               auto* node = g_kernelProcess->GetVadAllocator().FindContaining(addr);
               if (node)
               {
                    PrintVADHeader();

                    PrintVADRow(node->entry);
               }
               else
               {
                    debugging::DbgWrite(u8"No VAD covering {}\r\n", reinterpret_cast<void*>(addr));
               }
               break;
          }
          case 'w':
          {
               PrintVADHeader();

               auto print = [](const memory::VADEntry& e) { PrintVADRow(e); };

               g_kernelProcess->GetVadAllocator().InorderTraversal(g_kernelProcess->GetVadAllocator().GetRoot(), print);

               break;
          }
          case 'c':
          {
               for (std::size_t i = 0; i < param->framebuffer.height; i++)
                    std::uninitialized_fill_n(buffer + (i * framebuffer.scanlineSize), framebuffer.scanlineSize,
                                              0x111111);
               break;
          }
          case 'h': // help
               debugging::DbgWrite(u8"Commands:\r\n");
               debugging::DbgWrite(u8"m - full physical memory map\r\n");
               debugging::DbgWrite(u8"s - virtual memory statistics\r\n");
               debugging::DbgWrite(u8"p - physical memory counters\r\n");
               debugging::DbgWrite(u8"v - list all VAD allocations\r\n");
               debugging::DbgWrite(u8"r <addr> - read 64 bytes from memory\r\n");
               debugging::DbgWrite(u8"h - help\r\n");
               break;
          default: debugging::DbgWrite(u8"Unknown command {}\r\n", line); break;
          }
     }

     KiIdleLoop();
     return 0;
}
