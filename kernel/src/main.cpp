#include <cstdint>
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
     case memory::PFNUse::Unused: return 0x202020;         // Dark gray
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

constexpr std::uint32_t ModifyColourByRegion(std::uint32_t baseColour, memory::PFNRegion region)
{
     std::uint32_t r = (baseColour >> 16) & 0xFF;
     std::uint32_t g = (baseColour >> 8) & 0xFF;
     std::uint32_t b = baseColour & 0xFF;

     switch (region)
     {
     case memory::PFNRegion::Active:
          // Full brightness
          break;
     case memory::PFNRegion::Free:
          // Slightly dim
          r = (r * 7) / 10;
          g = (g * 7) / 10;
          b = (b * 7) / 10;
          break;
     case memory::PFNRegion::Zero:
          // Very bright (white tint)
          r = (r + 255) / 2;
          g = (g + 255) / 2;
          b = (b + 255) / 2;
          break;
     case memory::PFNRegion::Standby:
          // Medium dim
          r = (r * 5) / 10;
          g = (g * 5) / 10;
          b = (b * 5) / 10;
          break;
     case memory::PFNRegion::Modified:
          // Add red tint
          r = (r + 128) / 2;
          break;
     case memory::PFNRegion::Bad:
          // Very dark
          return 0x000000;
     }

     return (r << 16) | (g << 8) | b;
}

constexpr std::size_t kMemoryBarHeight = 40;

extern "C" int KiStartup(arch::LoaderParameterBlock* param)
{
     if (param->systemMajor != OsVersionMajor || param->systemMinor != OsVersionMinor) return 1;

     g_bootCycles = operations::ReadCurrentCycles();
     g_loaderBlock = param;

     cpu::Initialise();

     std::uint32_t* buffer = reinterpret_cast<std::uint32_t*>(param->framebuffer.physicalStart);
     auto status = memory::physicalAllocator.Initialise(param->memoryDescriptors, 0, 0xffff'8000'0000'0000ui64);
     if (!status) Error(buffer, param);

     for (std::size_t i = 0; i < param->framebuffer.height; i++)
          std::uninitialized_fill_n(buffer + (i * param->framebuffer.scanlineSize), param->framebuffer.scanlineSize, 0);

     constexpr memory::PFNRegion regionOrder[] = {memory::PFNRegion::Active,   memory::PFNRegion::Standby,
                                                  memory::PFNRegion::Modified, memory::PFNRegion::Zero,
                                                  memory::PFNRegion::Free,     memory::PFNRegion::Bad};

     const std::size_t barWidth = param->framebuffer.width;
     const std::size_t totalPages = memory::physicalAllocator.databaseSize;

     std::size_t orderedPageIndex = 0;
     for (const auto region : regionOrder)
     {
          for (std::size_t pfnIndex = 0; pfnIndex < totalPages; pfnIndex++)
          {
               const auto& pfn = memory::physicalAllocator.database[pfnIndex];
               if (pfn.region != region) continue;

               const std::size_t startX = (orderedPageIndex * barWidth) / totalPages;
               const std::size_t endX = ((orderedPageIndex + 1) * barWidth) / totalPages;

               const std::uint32_t colour = GetColourForPFNUse(pfn.use);

               for (std::size_t x = startX; x < endX; x++)
               {
                    for (std::size_t y = 0; y < kMemoryBarHeight; y++)
                    {
                         buffer[((y + param->framebuffer.height / 2 - kMemoryBarHeight / 2) *
                                 param->framebuffer.scanlineSize) +
                                x] = colour;
                    }
               }

               orderedPageIndex++;
          }
     }

     for (const auto entry : param->memoryDescriptors) {}

     KiIdleLoop();
     return 0;
}
