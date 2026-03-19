#pragma once

#include <atomic>
#include <cstdint>
#include "../cpu/interrupts.h"
#include "process.h"
#include "thread.h"
#include "utils/operations.h"

namespace process
{
     struct alignas(8) CpuLocal
     {
          CpuLocal* self;
          std::uint32_t cpuId;
          Thread* thread;
          std::uintptr_t kernelRSP;
          std::uintptr_t userRSP;
          Thread* idleThread;
          bool isSleeping{};
          std::atomic<cpu::IRQL> irql{};
          std::uint64_t lastTSC{};
          std::atomic<bool> selectionLock{};
          std::atomic<bool> canReschedule{};
          std::array<Thread*, static_cast<std::size_t>(ThreadPriority::Count_)> readyQueues{};

          void LockSelection()
          {
               bool expected = false;
               while (!selectionLock.compare_exchange_weak(expected, true, std::memory_order::acquire,
                                                           std::memory_order::relaxed))
               {
                    expected = false;
                    operations::Yield();
               }
          }

          bool TryLockSelection()
          {
               bool expected = false;
               return selectionLock.compare_exchange_strong(expected, true, std::memory_order::acquire,
                                                            std::memory_order::relaxed);
          }

          void UnlockSelection() { selectionLock.store(false, std::memory_order::release); }
     };

     CpuLocal* KeCurrentCpu();
     kernel::ProcessControlBlock* KeCurrentProcess();
     Thread* KeCurrentThread();

     std::uint64_t KiAllocateThreadId();
     void* KiSwitchThread(void* lpRegisters);
     void KiInitialiseTaskScheduler(std::uint64_t cpuId, void* idleProcedure, std::uintptr_t stackPointer = 0);
     bool KeHasRunnableThreads() noexcept;

     object::Handle KiCreateKernelThread(ThreadRoutine entryPoint, void* parameter, ThreadPriority priority);

     void KeSleepCurrentThread(std::uint64_t milliseconds);

     void KiPsWakeThread(Thread* thread);
     void KiPsYieldThread();
     void KiPsBlockThread(ThreadState newState);

}; // namespace process
