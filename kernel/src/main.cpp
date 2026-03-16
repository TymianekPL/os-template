#include <cstdint>
#include <cstring>
#include <memory>
#include "BootVideo.h"
#include "cpu/interrupts.h"
#include "kinit.h"
#include "memory/kheap.h"
#include "memory/pallocator.h"
#include "memory/vallocator.h"
#include "object/object.h"
#include "process/process.h"
#include "utils/arch.h"
#include "utils/cpu.h"
#include "utils/identify.h"
#include "utils/kdbg.h"
#include "utils/memory.h"
#include "utils/operations.h"

#ifdef ARCH_ARM64
#ifdef COMPILER_MSVC
#include <intrin.h>
#endif
#endif
alignas(std::max_align_t) static std::byte g_kernelProcessStorage[sizeof(kernel::ProcessControlBlock)]; // NOLINT

void KiIdleLoop()
{
     while (true)
     {
          operations::EnableInterrupts();
          operations::Yield();
          operations::Yield();
          operations::DisableInterrupts();

          // TODO: DPC stuff

          operations::EnableInterrupts();
          operations::Halt();
          VidExchangeBuffers();
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

     g_kernelProcess = new (g_kernelProcessStorage) kernel::ProcessControlBlock(0, "System", 0xffff'e000'0000'0000);
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

     KeInitialiseCpu(param->acpiPhysical);
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

static std::size_t AlignUp(std::size_t size, std::size_t align) { return (size + align - 1) & ~(align - 1); }

extern "C" int KiStartup(arch::LoaderParameterBlock* param)
{
     if (param->systemMajor != OsVersionMajor || param->systemMinor != OsVersionMinor) return 1;
     cpu::g_systemBootTimeOffsetSeconds = param->bootTimeSeconds;

     KiInitialise(param);

     auto framebuffer = param->framebuffer;
     std::uint32_t* buffer = reinterpret_cast<std::uint32_t*>(param->framebuffer.physicalStart);
     std::uint32_t* bbuffer = static_cast<std::uint32_t*>(
         g_kernelProcess->AllocateVirtualMemory(nullptr, framebuffer.totalSize,
                                                memory::AllocationFlags::Commit | memory::AllocationFlags::Reserve |
                                                    memory::AllocationFlags::ImmediatePhysical,
                                                memory::MemoryProtection::ReadWrite));

     VidInitialise(VdiFrameBuffer{.framebuffer = buffer,
                                  .width = framebuffer.width,
                                  .height = framebuffer.height,
                                  .scalineSize = framebuffer.scanlineSize,
                                  .optionalBackbuffer = bbuffer});

     memory::KiHeapInitialise();
     object::KeInitialiseOb();

     while (true)
     {
          VidExchangeBuffers();

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
          case 't':
          {
               const auto millisecondsSinceUnixEpoch = KeCurrentSystemTime();
               std::uint64_t totalSeconds = millisecondsSinceUnixEpoch / 1000;
               std::uint64_t second = totalSeconds % 60;
               std::uint64_t totalMinutes = totalSeconds / 60;
               std::uint64_t minute = totalMinutes % 60;
               std::uint64_t totalHours = totalMinutes / 60;
               std::uint64_t hour = totalHours % 24;
               std::uint64_t totalDays = totalHours / 24;

               // Convert days since epoch to year/month/day
               auto isLeap = [](std::uint32_t y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); };

               std::uint32_t year = 1970;
               while (true)
               {
                    std::uint64_t daysInYear = isLeap(year) ? 366 : 365;
                    if (totalDays < daysInYear) break;
                    totalDays -= daysInYear;
                    year++;
               }

               constexpr std::uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
               std::uint32_t month = 1;
               for (std::uint32_t m = 0; m < 12; m++)
               {
                    std::uint64_t dim = daysInMonth[m] + (m == 1 && isLeap(year) ? 1 : 0);
                    if (totalDays < dim) break;
                    totalDays -= dim;
                    month++;
               }
               std::uint32_t day = static_cast<std::uint32_t>(totalDays) + 1;

               debugging::DbgWrite(u8"{}.{}.{} {}:{}:{}\r\n", day, month, year, hour, minute, second);
               break;
          }

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
                    static unsigned char storage{};

                    auto* node = g_kernelProcess->GetVadAllocator().FindContaining(byteAddr);
                    if (!node) return nullptr;
                    if (node->entry.state != memory::VADMemoryState::Committed) return nullptr;
                    storage = *reinterpret_cast<unsigned char*>(byteAddr);
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
          case 'L':
          {
               const auto start = KeReadHighResolutionTimer();
               VidClearScreen(0x1111ff);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
               break;
          }
          case 'i':
          {
               *reinterpret_cast<volatile char*>(0x8493247389) = 123;
               break;
          }
          case 'I':
          {
#ifdef ARCH_ARM64
#ifdef COMPILER_MSVC
               __svc(0x2c);
#else
               asm volatile("svc %0" : : "I"(44) : "memory");
#endif
#else
#ifdef COMPILER_MSVC
               __int2c();
#else
               asm volatile("int $44");
#endif
#endif
               break;
          }
          case 'l':
          {
               const auto loCurrent = KeReadLowResolutionTimer();
               const auto hiCurrent = KeReadHighResolutionTimer();
               const auto loFrequency = KeReadLowResolutionTimerFrequency();
               const auto hiFrequency = KeReadHighResolutionTimerFrequency();
               debugging::DbgWrite(u8"Low = {} at {}Hz (~{}s)\r\n", loCurrent, loFrequency,
                                   loFrequency == 0 ? 0 : loCurrent / loFrequency);
               debugging::DbgWrite(u8"High = {} at {}Hz (~{}s)\r\n", hiCurrent, hiFrequency,
                                   hiFrequency == 0 ? 0 : hiCurrent / hiFrequency);
               break;
          }
          case 'a':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 64, 'tst1'), 0xcd, 64);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;
          case 'A':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 64, 'tst2'), 0xcd, 64);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;
          case 'b':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 4096, 'tst1'), 0xcd, 4096);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;
          case 'B':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 4096, 'tst2'), 0xcd, 4096);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;

          case 'O':
          {
               const auto typeName = [](object::ObjectType t) -> const char8_t*
               {
                    switch (t)
                    {
                    case object::ObjectType::Process: return u8"Process";
                    case object::ObjectType::Thread: return u8"Thread";
                    case object::ObjectType::File: return u8"File";
                    case object::ObjectType::Event: return u8"Event";
                    case object::ObjectType::Mutex: return u8"Mutex";
                    case object::ObjectType::Semaphore: return u8"Semaphore";
                    case object::ObjectType::Timer: return u8"Timer";
                    case object::ObjectType::Section: return u8"Section";
                    case object::ObjectType::Port: return u8"Port";
                    case object::ObjectType::SymbolicLink: return u8"SymLink";
                    case object::ObjectType::Directory: return u8"Directory";
                    case object::ObjectType::Device: return u8"Device";
                    case object::ObjectType::Driver: return u8"Driver";
                    case object::ObjectType::TypeDescriptor: return u8"TypeDesc";
                    case object::ObjectType::Processor: return u8"Processor";
                    default: return u8"Unknown";
                    }
               };

               struct PrintCtx
               {
                    char indent[64]{};
                    std::size_t indentLen{};
                    const decltype(typeName)& typeNameFn; // NOLINT
               };

               constexpr std::size_t kMaxChildren = 256;

               struct PrintNodeCtx
               {
                    const object::ObjectHeader* children[kMaxChildren]{};
                    std::size_t childCount{};
               };

               struct Printer
               {
                    static void PrintNode(const object::ObjectHeader* node, char* indent, std::size_t& indentLen,
                                          const decltype(typeName)& typeNameFn, std::uint32_t depth) noexcept
                    {
                         if (depth > 16)
                         {
                              debugging::DbgWrite(u8"{}... (depth limit)\r\n", indent);
                              return;
                         }

                         if (node->type == object::ObjectType::Directory)
                         {
                              struct SnapCtx
                              {
                                   const object::ObjectHeader** children;
                                   std::size_t* count;
                              };

                              const object::ObjectHeader* children[kMaxChildren]{};
                              std::size_t childCount = 0;
                              SnapCtx snapCtx{.children = children, .count = &childCount};

                              node->BodyAs<object::DirectoryBody>()->Enumerate(
                                  [](const object::ObjectHeader* child, void* ctx) noexcept
                                  {
                                       auto* s = static_cast<SnapCtx*>(ctx);
                                       if (*s->count < kMaxChildren) s->children[(*s->count)++] = child;
                                  },
                                  &snapCtx);

                              for (std::size_t i = 0; i < childCount; ++i)
                              {
                                   const object::ObjectHeader* child = children[i];
                                   const bool last = (i == childCount - 1);

                                   debugging::DbgWrite(u8"{}{} [{}] \"{}\"", reinterpret_cast<const char8_t*>(indent),
                                                       last ? u8"└──" : u8"├──", typeNameFn(child->type),
                                                       child->accessName.View());

                                   if (child->type == object::ObjectType::SymbolicLink)
                                   {
                                        const auto* sl = child->BodyAs<object::SymbolicLinkBody>();
                                        debugging::DbgWrite(u8" -> \"{}\"", sl->targetPath);
                                   }
                                   else if (child->type == object::ObjectType::TypeDescriptor)
                                   {
                                        const auto* td = child->BodyAs<object::TypeDescriptorBody>();
                                        debugging::DbgWrite(u8" (typeId={})", static_cast<std::uint16_t>(td->typeId));
                                   }
                                   else if (child->type == object::ObjectType::Processor)
                                   {
                                        const auto* pr = child->BodyAs<object::ProcessorBody>();
                                        debugging::DbgWrite(u8" (CPU{} APIC={} {})", pr->logicalIndex, pr->apicId,
                                                            pr->isBsp ? u8"BSP" : u8"AP");
                                   }

                                   debugging::DbgWrite(u8"  refs={}\r\n",
                                                       child->referenceCount.load(std::memory_order::relaxed));

                                   if (child->type == object::ObjectType::Directory)
                                   {
                                        const char8_t* ext = last ? u8"    " : u8"│   ";
                                        std::size_t extLen = 0;
                                        while (ext[extLen]) ++extLen;

                                        if (indentLen + extLen + 1 < 64)
                                        {
                                             std::memcpy(indent + indentLen, reinterpret_cast<const char*>(ext),
                                                         extLen);
                                             indentLen += extLen;
                                             indent[indentLen] = '\0';

                                             PrintNode(child, indent, indentLen, typeNameFn, depth + 1);

                                             indentLen -= extLen;
                                             indent[indentLen] = '\0';
                                        }
                                   }
                              }

                              if (childCount == 0)
                                   debugging::DbgWrite(u8"{}(empty)\r\n", reinterpret_cast<const char8_t*>(indent));
                         }
                    }
               };

               if (!object::g_rootDirectoryHeader)
               {
                    debugging::DbgWrite(u8"Object namespace not initialised.\r\n");
                    break;
               }

               debugging::DbgWrite(u8"\r\nObject Namespace\r\n");
               debugging::DbgWrite(u8"[Directory] \"\\\"  refs={}\r\n",
                                   object::g_rootDirectoryHeader->referenceCount.load(std::memory_order::relaxed));

               char indent[64]{};
               std::size_t indentLen = 0;

               Printer::PrintNode(object::g_rootDirectoryHeader, indent, indentLen, typeName, 0);

               debugging::DbgWrite(u8"\r\n");
               break;
          }

          case 'c':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 1024uz * 1024uz * 8, 'tst1'), 0xcd,
                           1024uz * 1024uz * 8);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;
          case 'C':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 1024uz * 1024uz * 8, 'tst2'), 0xcd,
                           1024uz * 1024uz * 8);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;
          case 'd':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 1024uz * 1024uz * 96, 'tst1'), 0xcd,
                           1024uz * 1024uz * 96);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;
          case 'D':
          {
               const auto start = KeReadHighResolutionTimer();
               std::memset(memory::ExAllocatePool(memory::PoolType::NonPaged, 1024uz * 1024uz * 96, 'tst2'), 0xcd,
                           1024uz * 1024uz * 96);
               const auto end = KeReadHighResolutionTimer();
               debugging::DbgWrite(u8"Elapsed {}us\r\n",
                                   (end - start) * 1000'0000uz / KeReadHighResolutionTimerFrequency());
          }
          break;
          case 'H':
          {
               auto printPool = [](memory::PoolType type, const char8_t* label)
               {
                    static std::size_t totalSegs{};
                    static std::size_t totalAlloc{};
                    static std::size_t totalFree{};
                    static std::size_t totalCommitted{};
                    static std::size_t totalUsed{};
                    totalSegs = totalAlloc = totalFree = totalCommitted = totalUsed = 0;

                    memory::ExEnumerateSegments(type,
                                                [](void* handle, std::size_t segSize)
                                                {
                                                     ++totalSegs;
                                                     totalCommitted += segSize;
                                                     memory::ExEnumeratePool(handle,
                                                                             [](memory::PoolHeader* hdr)
                                                                             {
                                                                                  if (hdr->isAllocated)
                                                                                  {
                                                                                       ++totalAlloc;
                                                                                       totalUsed += hdr->size;
                                                                                  }
                                                                                  else
                                                                                       ++totalFree;
                                                                             });
                                                });

                    debugging::DbgWrite(u8"\r\n");
                    debugging::DbgWrite(u8"=== {} ===\r\n", label);
                    debugging::DbgWrite(u8"  segments  : {}\r\n", totalSegs);
                    debugging::DbgWrite(u8"  committed : {} bytes\r\n", totalCommitted);
                    debugging::DbgWrite(u8"  in use    : {} bytes ({} allocs, {} free)\r\n", totalUsed, totalAlloc,
                                        totalFree);

                    if (!totalSegs)
                    {
                         debugging::DbgWrite(u8"  (no segments committed)\r\n");
                         return;
                    }

                    memory::ExEnumerateSegments(
                        type,
                        [](void* handle, std::size_t segSize)
                        {
                             auto* seg = reinterpret_cast<memory::PoolSegment*>(handle);

                             static std::size_t segAlloc{};
                             static std::size_t segFree{};
                             static std::size_t segUsed{};
                             static std::size_t segBlocks{};
                             static std::size_t binIdx{};
                             segAlloc = segFree = segUsed = segBlocks = binIdx = 0;
                             memory::ExEnumeratePool(handle,
                                                     [](memory::PoolHeader* hdr)
                                                     {
                                                          if (segBlocks++ == 0) binIdx = hdr->binIndex;
                                                          if (hdr->isAllocated)
                                                          {
                                                               ++segAlloc;
                                                               segUsed += hdr->size;
                                                          }
                                                          else
                                                               ++segFree;
                                                     });

                             const std::size_t usable = static_cast<std::size_t>(seg->endAddress - seg->startAddress);
                             const std::size_t pct = usable ? segUsed * 100 / usable : 0;

                             debugging::DbgWrite(u8"\r\n");
                             debugging::DbgWrite(
                                 u8"  segment {} : bin {} : {} bytes usable : {}% used : {} alloc {} free\r\n", handle,
                                 binIdx, usable, pct, segAlloc, segFree);
                             debugging::DbgWrite(u8"\r\n");

                             if (!segBlocks)
                             {
                                  debugging::DbgWrite(u8"    (no carved blocks)\r\n");
                                  return;
                             }

                             debugging::DbgWrite(
                                 u8"    header                user ptr              size        tag    state\r\n");
                             debugging::DbgWrite(
                                 u8"    --------------------  --------------------  ----------  -----  ---------\r\n");

                             memory::ExEnumeratePool(
                                 handle,
                                 [](memory::PoolHeader* hdr)
                                 {
                                      char8_t tag[5]{};
                                      std::memcpy(tag, &hdr->tag, 4);
                                      const void* userPtr = hdr + 1;
                                      const char8_t* state = hdr->isAllocated ? u8"ALLOCATED" : u8"free";
                                      debugging::DbgWrite(u8"    {}    {}    {}          {}   {}\r\n", hdr, userPtr,
                                                          hdr->size, tag, state);
                                 });
                        });
               };

               printPool(memory::PoolType::NonPaged, u8"Non-paged pool");
               printPool(memory::PoolType::Paged, u8"Paged pool");
               debugging::DbgWrite(u8"\r\n");
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
extern "C" int _purecall() // NOLINT
{
     operations::DisableInterrupts();
     while (true) operations::Halt();
}
extern "C" int _fltused = 1;
