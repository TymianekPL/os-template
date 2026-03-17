#include "kinit.h"
#include "cpu/interrupts.h"
#include "utils/operations.h"

std::uint64_t g_bootCycles{};
arch::LoaderParameterBlock* g_loaderBlock{};
kernel::ProcessControlBlock* g_kernelProcess{};
std::uintptr_t g_stackBase{};
std::uintptr_t g_stackSize{};
std::uintptr_t g_imageBase{};
std::uintptr_t g_imageSize{};

NO_ASAN void KeInitialiseCpu(std::uintptr_t acpiPhysical)
{
     KiInitialiseInterrupts(acpiPhysical);
     KeSetTimerFrequency(64);
     operations::EnableInterrupts();
}
