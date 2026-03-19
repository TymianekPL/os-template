#include "taskScheduler.h"
#include <utils/identify.h>
#include <array>
#include "../dbg/kasan.h"
#include "../kinit.h"
#include "thread.h"
#include "utils/kdbg.h"
#include "utils/operations.h"

static std::atomic<bool> g_schedulerInitialised{false};

process::CpuLocal* process::KeCurrentCpu()
{
     if (!g_schedulerInitialised.load(std::memory_order::relaxed)) return nullptr;

#ifdef ARCH_X8664
#ifdef COMPILER_MSVC
     CpuLocal* cpuLocal;
     __readgsbase(&cpuLocal);
     return cpuLocal;
#else
     CpuLocal* cpuLocal{};
     asm("mov %%gs:0, %0" : "=r"(cpuLocal));
     return cpuLocal;
#endif
#elifdef ARCH_ARM64
#ifdef COMPILER_MSVC
     CpuLocal* cpuLocal;
     __readtp(&cpuLocal);
     return cpuLocal;
#else
     CpuLocal* cpuLocal{};
     asm("mrs %0, tpidr_el1" : "=r"(cpuLocal));
     return cpuLocal;
#endif
#endif
}

static void KiSetCpuLocal(process::CpuLocal* cpuLocal)
{
#ifdef ARCH_X8664
#ifdef COMPILER_MSVC
     __writemsr(0xC000'0101, reinterpret_cast<std::uintptr_t>(cpuLocal));
     __writemsr(0xC000'0102, reinterpret_cast<std::uintptr_t>(cpuLocal));
#else
     const std::uintptr_t value = reinterpret_cast<std::uintptr_t>(cpuLocal);
     asm volatile("wrmsr"
                  :
                  : "c"(0xC0000101u), "a"(static_cast<std::uint32_t>(value)),
                    "d"(static_cast<std::uint32_t>(value >> 32))
                  : "memory");

     asm volatile("wrmsr"
                  :
                  : "c"(0xC0000102u), "a"(static_cast<std::uint32_t>(value)),
                    "d"(static_cast<std::uint32_t>(value >> 32))
                  : "memory");
#endif
#elifdef ARCH_ARM64
#ifdef COMPILER_MSVC
     _WriteStatusReg(ARM64_SYSREG(3, 0, 13, 0, 4), reinterpret_cast<std::uintptr_t>(cpuLocal));
#else
     const std::uintptr_t value = reinterpret_cast<std::uintptr_t>(cpuLocal);
     asm volatile("msr tpidr_el1, %0" : : "r"(value) : "memory");
#endif
#endif
}

kernel::ProcessControlBlock* process::KeCurrentProcess()
{
     auto* cpuLocal = KeCurrentCpu();
     return cpuLocal->thread ? cpuLocal->thread->parentProcess : nullptr;
}

process::Thread* process::KeCurrentThread()
{
     auto* cpuLocal = KeCurrentCpu();
     return cpuLocal->thread;
}

std::uint64_t process::KiAllocateThreadId()
{
     static std::atomic<std::uint64_t> nextThreadId{1};
     return nextThreadId.fetch_add(1, std::memory_order::relaxed);
}

namespace
{
     struct SleepEntry
     {
          process::Thread* thread{};
          std::uint64_t wakeupTimeMs{};
          SleepEntry* next{};
     };

     struct SleepQueue
     {
          SleepEntry* head{};
          std::atomic<bool> lock{false};
     };

     SleepQueue g_sleepQueue{};

     void AcquireSleepQueueLock() noexcept
     {
          bool expected = false;
          while (!g_sleepQueue.lock.compare_exchange_weak(expected, true, std::memory_order::acquire,
                                                          std::memory_order::relaxed))
          {
               expected = false;
               operations::Yield();
          }
     }

     void ReleaseSleepQueueLock() noexcept { g_sleepQueue.lock.store(false, std::memory_order::release); }
} // namespace

namespace
{
     [[nodiscard]] constexpr bool IsThreadRunnable(const process::Thread* thread) noexcept
     {
          if (!thread) return false;
          return thread->state == process::ThreadState::Runnable || thread->state == process::ThreadState::Running;
     }

     [[nodiscard]] process::Thread* FindHighestPriorityThread(process::CpuLocal* cpu) noexcept
     {
          for (int i = static_cast<int>(process::ThreadPriority::Count_) - 1; i >= 0; --i)
          {
               process::Thread* queueHead = cpu->readyQueues[i];
               if (queueHead && IsThreadRunnable(queueHead)) { return queueHead; }
          }
          return nullptr;
     }

     void RemoveFromReadyQueue(process::CpuLocal* cpu, process::Thread* thread) noexcept
     {
          if (!thread) return;

          const auto priorityIndex = static_cast<std::size_t>(thread->priority);
          process::Thread** queueHead = &cpu->readyQueues[priorityIndex];

          if (!*queueHead) return;

          if (*queueHead == thread)
          {
               *queueHead = thread->next;
               thread->next = nullptr;
               return;
          }

          process::Thread* prev = *queueHead;
          while (prev->next && prev->next != thread) { prev = prev->next; }

          if (prev->next == thread)
          {
               prev->next = thread->next;
               thread->next = nullptr;
          }
     }

     void AddToReadyQueue(process::CpuLocal* cpu, process::Thread* thread) noexcept
     {
          if (!thread) return;

          const auto priorityIndex = static_cast<std::size_t>(thread->priority);
          process::Thread** queueHead = &cpu->readyQueues[priorityIndex];

          thread->next = nullptr;

          if (!*queueHead)
          {
               *queueHead = thread;
               return;
          }

          process::Thread* tail = *queueHead;
          while (tail->next) tail = tail->next;

          tail->next = thread;
     }

     void ResetQuantum(process::Thread* thread) noexcept
     {
          if (!thread) return;

          const auto priorityIndex = static_cast<std::size_t>(thread->priority);
          thread->quantumCounter = process::ThreadPriorityQuantum[priorityIndex];
     }

     void RotateThreadInQueue(process::CpuLocal* cpu, process::Thread* thread) noexcept
     {
          RemoveFromReadyQueue(cpu, thread);
          AddToReadyQueue(cpu, thread);
     }

     [[nodiscard]] std::uint64_t GetCurrentTimeMs() noexcept
     {
          const auto now = KeReadHighResolutionTimer();
          const auto nowMs = now / (KeReadHighResolutionTimerFrequency() / 1000);
          return nowMs;
     }

     void ProcessSleepQueue(process::CpuLocal* cpu) noexcept
     {
          const std::uint64_t currentTimeMs = GetCurrentTimeMs();

          AcquireSleepQueueLock();
          {
               while (g_sleepQueue.head && g_sleepQueue.head->wakeupTimeMs <= currentTimeMs)
               {
                    SleepEntry* entry = g_sleepQueue.head;
                    g_sleepQueue.head = entry->next;

                    process::Thread* thread = entry->thread;
                    if (thread && thread->state == process::ThreadState::Sleeping)
                    {
                         thread->state = process::ThreadState::Runnable;
                         AddToReadyQueue(cpu, thread);
                    }

                    delete entry;
               }
          }
          ReleaseSleepQueueLock();
     }
} // namespace

static process::Thread* KiPsFindNextThread()
{
     auto* currentCpu = process::KeCurrentCpu();
     auto* currentThread = currentCpu->thread;
     if (!currentThread) return nullptr;

     ProcessSleepQueue(currentCpu);

     if (currentThread->quantumCounter > 0 && IsThreadRunnable(currentThread))
     {
          currentThread->quantumCounter--;
          return currentThread;
     }

     if (currentThread->state == process::ThreadState::Running)
     {
          currentThread->state = process::ThreadState::Runnable;
          AddToReadyQueue(currentCpu, currentThread); // ← ADD THIS
     }
     else if (currentThread->state == process::ThreadState::Finished ||
              currentThread->state == process::ThreadState::DeletedWaitingForRelease)
     {
          RemoveFromReadyQueue(currentCpu, currentThread);
     }

     process::Thread* nextThread = FindHighestPriorityThread(currentCpu);

     if (!nextThread)
     {
          nextThread = currentCpu->idleThread;
          if (nextThread)
          {
               nextThread->state = process::ThreadState::Running;
               ResetQuantum(nextThread);
          }
          return nextThread;
     }

     RemoveFromReadyQueue(currentCpu, nextThread);
     nextThread->state = process::ThreadState::Running;
     ResetQuantum(nextThread);

     return nextThread;
}

void* process::KiSwitchThread(void* lpRegisters)
{
     if (!g_schedulerInitialised.load(std::memory_order::relaxed)) return lpRegisters;

     auto* currentCpu = KeCurrentCpu();
     Thread* currentThread = currentCpu->thread;
     if (!currentThread) return lpRegisters;

     currentThread->stackPointer = lpRegisters;

     auto* nextThread = KiPsFindNextThread();
     if (!nextThread) return lpRegisters;

     currentCpu->thread = nextThread;
     auto* nextStackPointer = static_cast<std::uint8_t*>(nextThread->stackPointer);
     nextThread->state = ThreadState::Running;

     return nextStackPointer;
}

static std::array<process::CpuLocal*, 256> g_cpuLocalArray{};

#ifdef ARCH_X8664
struct InterruptFrame
{
     std::uint64_t r15;
     std::uint64_t r14;
     std::uint64_t r13;
     std::uint64_t r12;
     std::uint64_t r11;
     std::uint64_t r10;
     std::uint64_t r9;
     std::uint64_t r8;
     std::uint64_t rbp;
     std::uint64_t rdi;
     std::uint64_t rsi;
     std::uint64_t rdx;
     std::uint64_t rcx;
     std::uint64_t rbx;
     std::uint64_t rax;

     std::uint64_t vector;
     std::uint64_t errorCode;

     std::uint64_t rip;
     std::uint64_t cs;
     std::uint64_t rflags;
     std::uint64_t rsp;
     std::uint64_t ss;
};
#elifdef ARCH_ARM64
struct InterruptFrame
{
     std::uint64_t x0;
     std::uint64_t x1;
     std::uint64_t x2;
     std::uint64_t x3;
     std::uint64_t x4;
     std::uint64_t x5;
     std::uint64_t x6;
     std::uint64_t x7;
     std::uint64_t x8;
     std::uint64_t x9;
     std::uint64_t x10;
     std::uint64_t x11;
     std::uint64_t x12;
     std::uint64_t x13;
     std::uint64_t x14;
     std::uint64_t x15;
     std::uint64_t x16;
     std::uint64_t x17;
     std::uint64_t x18;

     std::uint64_t x19;
     std::uint64_t x20;
     std::uint64_t x21;
     std::uint64_t x22;
     std::uint64_t x23;
     std::uint64_t x24;
     std::uint64_t x25;
     std::uint64_t x26;
     std::uint64_t x27;
     std::uint64_t x28;

     std::uint64_t fp;
     std::uint64_t lr;

     std::uint64_t vector;
     std::uint64_t esr;
     std::uint64_t far;
     std::uint64_t svc;
     std::uint64_t sp;
};
#endif

static NO_ASAN void KiInitialiseThreadStacks(void* stackPointer, bool isUsermode, void* entryPoint, void* parameter)
{
     auto* stack = static_cast<InterruptFrame*>(stackPointer);
#ifdef ARCH_X8664
     stack->rip = reinterpret_cast<std::uintptr_t>(entryPoint);
     stack->cs = isUsermode ? 0x1B : 0x08;
     stack->rflags = 0x202;
     stack->rsp = reinterpret_cast<std::uintptr_t>(stackPointer) + sizeof(InterruptFrame);
     stack->ss = isUsermode ? 0x23 : 0x10;
     stack->rcx = reinterpret_cast<std::uintptr_t>(parameter);
#elifdef ARCH_ARM64
     stack->lr = reinterpret_cast<std::uintptr_t>(entryPoint);
     stack->sp = reinterpret_cast<std::uintptr_t>(stackPointer) + sizeof(InterruptFrame);
     stack->x0 = reinterpret_cast<std::uintptr_t>(parameter);
#endif
}

void process::KiInitialiseTaskScheduler(std::uint64_t cpuId, void* idleProcedure, std::uintptr_t stackPointer)
{
     auto* cpuLocal = new CpuLocal{};
     Thread* kernelThread = new Thread{};
     kernelThread->id = KiAllocateThreadId();
     kernelThread->stackBase = reinterpret_cast<void*>(stackPointer);
     kernelThread->stackPointer = nullptr;
     kernelThread->state = ThreadState::Running;
     kernelThread->priority = ThreadPriority::Normal;
     kernelThread->parentProcess = g_kernelProcess;
     kernelThread->next = nullptr;

     debugging::DbgWrite(u8"Initialising CPU Local for CPU {} with stack at {}\r\n", cpuId,
                         reinterpret_cast<void*>(stackPointer));

     cpuLocal->thread = kernelThread;
     cpuLocal->self = cpuLocal;
     cpuLocal->cpuId = cpuId;
     cpuLocal->kernelRSP = stackPointer;
     cpuLocal->userRSP = 0;
     cpuLocal->idleThread = nullptr;
     cpuLocal->isSleeping = false;
     cpuLocal->irql = cpu::IRQL::Passive;
     cpuLocal->lastTSC = 0;
     cpuLocal->selectionLock.store(false, std::memory_order::release);
     cpuLocal->canReschedule.store(true, std::memory_order::release);

     for (auto& queue : cpuLocal->readyQueues) queue = nullptr;

     Thread* idleThread = new Thread{};
     idleThread->id = KiAllocateThreadId();
     idleThread->stackBase = static_cast<std::uint8_t*>(
         g_kernelProcess->AllocateVirtualMemory(nullptr, process::ThreadStackSize,
                                                memory::AllocationFlags::Commit | memory::AllocationFlags::Reserve |
                                                    memory::AllocationFlags::ImmediatePhysical,
                                                memory::MemoryProtection::ReadWrite));

     debugging::DbgWrite(u8"Created idle thread with ID {} and stack at {}\r\n", idleThread->id, idleThread->stackBase);

     KASANAllocateHeap(idleThread->stackBase, process::ThreadStackSize);

     idleThread->stackPointer =
         static_cast<std::byte*>(idleThread->stackBase) + process::ThreadStackSize - sizeof(InterruptFrame) - 8;
     idleThread->state = ThreadState::Runnable;
     idleThread->priority = ThreadPriority::Idle;
     idleThread->parentProcess = nullptr;
     idleThread->next = nullptr;

     cpuLocal->idleThread = idleThread;

     KiInitialiseThreadStacks(idleThread->stackPointer, false, idleProcedure, nullptr);

     g_cpuLocalArray[cpuId] = cpuLocal;
     KiSetCpuLocal(cpuLocal);

     g_schedulerInitialised.store(true, std::memory_order::relaxed);
     operations::EnableInterrupts();
}

namespace
{
     constexpr std::size_t DefaultKernelStackSize = 16384; // 16 KB

     void ThreadDestructor(void* objectBody) noexcept
     {
          debugging::DbgWrite(u8"Thread destructor called for thread object at {}\r\n", objectBody);
          auto* thread = static_cast<process::Thread*>(objectBody);
          if (!thread) return;
          debugging::DbgWrite(u8"Releasing resources for thread with ID {}\r\n", thread->id);

          if (thread->stackBase)
          {
               g_kernelProcess->ReleaseVirtualMemory(thread->stackBase, DefaultKernelStackSize,
                                                     memory::AllocationFlags::Release);
               thread->stackBase = nullptr;
               thread->stackPointer = nullptr;
          }

          thread->state = process::ThreadState::DeletedWaitingForRelease;
          debugging::DbgWrite(u8"Thread with ID {} marked as DeletedWaitingForRelease\r\n", thread->id);
     }
} // namespace

bool process::KeHasRunnableThreads() noexcept { return false; }

object::Handle process::KiCreateKernelThread(ThreadRoutine entryPoint, void* parameter, ThreadPriority priority)
{
     if (!entryPoint)
     {
          debugging::DbgWrite(u8"Cannot create kernel thread with null entry point\r\n");
          return object::kInvalidHandle;
     }

     auto attributes = object::ObjectAttributes{
         .name = "",
         .type = object::ObjectType::Thread,
         .bodySize = sizeof(Thread),
         .destructor = ThreadDestructor,
         .desiredAccess = object::AccessRights::All,
     };

     object::Handle handle = object::ObCreateObject(*KeCurrentProcess(), attributes);

     if (handle == object::kInvalidHandle)
     {
          debugging::DbgWrite(u8"Failed to create thread object\r\n");
          return object::kInvalidHandle;
     }

     auto* thread = object::ObGetBody<Thread>(*KeCurrentProcess(), handle, object::ObjectType::Thread);
     if (thread == nullptr)
     {
          debugging::DbgWrite(u8"Failed to get thread object body for handle {}\r\n", handle);
          object::ObCloseHandle(*KeCurrentProcess(), handle);
          return object::kInvalidHandle;
     }

     new (thread) Thread{};

     thread->stackBase = static_cast<std::uint8_t*>(
         g_kernelProcess->AllocateVirtualMemory(nullptr, DefaultKernelStackSize,
                                                memory::AllocationFlags::Commit | memory::AllocationFlags::Reserve |
                                                    memory::AllocationFlags::ImmediatePhysical,
                                                memory::MemoryProtection::ReadWrite));
     KASANAllocateHeap(thread->stackBase, DefaultKernelStackSize);

     if (!thread->stackBase)
     {
          debugging::DbgWrite(u8"Failed to allocate kernel stack for thread\r\n");
          object::ObCloseHandle(*KeCurrentProcess(), handle);
          return object::kInvalidHandle;
     }

     thread->stackPointer =
         static_cast<std::byte*>(thread->stackBase) + DefaultKernelStackSize - sizeof(InterruptFrame) - 8;
     thread->id = KiAllocateThreadId();
     thread->state = ThreadState::Created;
     thread->priority = priority;
     thread->parentProcess = KeCurrentProcess();
     thread->next = nullptr;
     thread->argument = 0;

     const auto priorityIndex = static_cast<std::size_t>(priority);
     thread->quantumCounter = ThreadPriorityQuantum[priorityIndex];

     KiInitialiseThreadStacks(thread->stackPointer, false, reinterpret_cast<void*>(entryPoint), parameter);

     thread->state = ThreadState::Runnable;
     AddToReadyQueue(KeCurrentCpu(), thread);

     return handle;
}

void process::KeSleepCurrentThread(std::uint64_t milliseconds)
{
     auto* currentThread = KeCurrentThread();
     if (!currentThread)
     {
          debugging::DbgWrite(u8"KeSleepCurrentThread called with no current thread\r\n");
          return;
     }

     if (milliseconds == 0)
     {
          debugging::DbgWrite(u8"Thread ID {} yielding CPU\r\n", currentThread->id);
          currentThread->quantumCounter = 0;
          return;
     }

     const std::uint64_t currentTimeMs = GetCurrentTimeMs();
     const std::uint64_t wakeupTimeMs = currentTimeMs + milliseconds;

     SleepEntry* entry = new SleepEntry{
         .thread = currentThread,
         .wakeupTimeMs = wakeupTimeMs,
         .next = nullptr,
     };

     AcquireSleepQueueLock();
     {
          if (!g_sleepQueue.head || g_sleepQueue.head->wakeupTimeMs > wakeupTimeMs)
          {
               entry->next = g_sleepQueue.head;
               g_sleepQueue.head = entry;
          }
          else
          {
               SleepEntry* prev = g_sleepQueue.head;
               while (prev->next && prev->next->wakeupTimeMs <= wakeupTimeMs) { prev = prev->next; }
               entry->next = prev->next;
               prev->next = entry;
          }
     }
     ReleaseSleepQueueLock();

     currentThread->argument = wakeupTimeMs;
     currentThread->state = ThreadState::Sleeping;
     currentThread->quantumCounter = 0;

     while (currentThread->state == ThreadState::Sleeping) operations::Halt();
}

void process::KiPsWakeThread(Thread* thread)
{
     if (!thread) return;

     auto* cpu = KeCurrentCpu();

     if (thread->state == ThreadState::Sleeping || thread->state == ThreadState::EventWait ||
         thread->state == ThreadState::InterruptWait)
     {
          thread->state = ThreadState::Runnable;
          AddToReadyQueue(cpu, thread);
     }
}

void process::KiPsYieldThread()
{
     auto* currentThread = KeCurrentThread();
     if (!currentThread) return;

     currentThread->quantumCounter = 0;
}

void process::KiPsBlockThread(ThreadState newState)
{
     auto* currentThread = KeCurrentThread();
     if (!currentThread) return;

     currentThread->state = newState;
     currentThread->quantumCounter = 0;
}
