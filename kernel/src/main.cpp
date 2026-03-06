#include <cstdint>
#include <memory>
#include "kinit.h"
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

extern "C" int KiStartup(arch::LoaderParameterBlock* param)
{
     if (param->systemMajor != OsVersionMajor || param->systemMinor != OsVersionMinor) return 1;

     g_bootCycles = operations::ReadCurrentCycles();
     g_loaderBlock = param;

     cpu::Initialise();

     std::uint32_t* buffer = reinterpret_cast<std::uint32_t*>(param->framebuffer.physicalStart);

     for (std::size_t i = 0; i < param->framebuffer.height; i++)
          std::uninitialized_fill_n(buffer + (i * param->framebuffer.scanlineSize), param->framebuffer.scanlineSize,
                                    0x111111);

     for (const auto entry : param->memoryDescriptors) {}

     KiIdleLoop();
     return 0;
}
