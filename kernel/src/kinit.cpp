#include "kinit.h"
#include "cpu/interrupts.h"
#include "utils/operations.h"

std::uint64_t g_bootCycles{};
arch::LoaderParameterBlock* g_loaderBlock{};
kernel::ProcessControlBlock* g_kernelProcess{};

void KeInitialiseCpu(std::uintptr_t acpiPhysical)
{
     KiInitialiseInterrupts(acpiPhysical);
     KeSetTimerFrequency(64);
     operations::EnableInterrupts();
}
