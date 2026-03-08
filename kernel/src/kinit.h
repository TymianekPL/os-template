#pragma once

#include <cstdint>
#include "process.h"
#include "utils/arch.h"

extern std::uint64_t g_bootCycles;
extern arch::LoaderParameterBlock* g_loaderBlock;
extern kernel::ProcessControlBlock* g_kernelProcess;
