#include <cstdint>
#include <cstring>
#include <memory>
#include "kinit.h"
#include "memory/pallocator.h"
#include "utils/arch.h"
#include "utils/cpu.h"
#include "utils/identify.h"
#include "utils/operations.h"

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

template <std::size_t N>
void DrawBar(const memory::PFNRegion (&regionOrder)[N], std::size_t totalPages, std::size_t barWidth,
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

     DrawBar(regionOrder, totalPages, barWidth, framebuffer, buffer,
             (static_cast<std::size_t>(framebuffer.height) / 2uz) - (kMemoryBarHeight / 2));

     KiIdleLoop();
     return 0;
}
