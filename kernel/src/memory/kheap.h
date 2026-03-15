#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace memory
{
     enum struct PoolType : std::uint8_t
     {
          NonPaged,
          Paged
     };
     inline constexpr std::size_t ALIGNMENT = 16;
     inline constexpr std::size_t NUM_SMALL_BINS = 128;
     inline constexpr std::size_t SMALL_BIN_STEP = 16;
     inline constexpr std::size_t SMALL_MAX = NUM_SMALL_BINS * SMALL_BIN_STEP; // 2048
     inline constexpr std::size_t NUM_LARGE_BINS = 8;
     inline constexpr std::size_t LARGE_BIN_BASE = 4096;

     inline constexpr std::size_t NUM_BINS = NUM_SMALL_BINS + NUM_LARGE_BINS + 1 /*overflow*/;

     struct alignas(ALIGNMENT) PoolHeader
     {
          PoolHeader* lpNextFree{};
          std::size_t size{};
          PoolType type{};
          bool isAllocated{};
          std::uint8_t binIndex{};
          std::uint32_t tag{};
     };
     struct alignas(64) BinFreeList
     {
          std::atomic<std::uint64_t> head{0};

          static constexpr std::uint64_t PTR_MASK = (1ULL << 48) - 1;
          static constexpr std::uint64_t TAG_SHIFT = 48;

          static std::uint64_t Pack(PoolHeader* ptr, std::uint64_t tag) noexcept
          {
               std::uint64_t lo48 = reinterpret_cast<std::uintptr_t>(ptr) & PTR_MASK;
               return (tag << TAG_SHIFT) | lo48;
          }
          static PoolHeader* Ptr(std::uint64_t v) noexcept
          {
               std::uint64_t lo48 = v & PTR_MASK;
               std::uintptr_t addr = static_cast<std::uintptr_t>(static_cast<std::int64_t>(lo48 << 16) >> 16);
               return reinterpret_cast<PoolHeader*>(addr);
          }
          static std::uint64_t Tag(std::uint64_t v) noexcept { return v >> TAG_SHIFT; }
          void Push(PoolHeader* block) noexcept
          {
               std::uint64_t old = head.load(std::memory_order::relaxed);
               std::uint64_t next{};
               do
               {
                    block->lpNextFree = Ptr(old);
                    next = Pack(block, Tag(old) + 1);
               } while (!head.compare_exchange_weak(old, next, std::memory_order::release, std::memory_order::relaxed));
          }
          PoolHeader* Pop() noexcept
          {
               std::uint64_t old = head.load(std::memory_order::acquire);
               PoolHeader* top{};
               std::uint64_t next{};
               do
               {
                    top = Ptr(old);
                    if (!top) return nullptr;
                    next = Pack(top->lpNextFree, Tag(old) + 1);
               } while (!head.compare_exchange_weak(old, next, std::memory_order::acquire, std::memory_order::relaxed));
               return top;
          }
     };

     struct PoolSegment
     {
          std::byte* startAddress{};
          std::byte* endAddress{};
          PoolSegment* lpNext{};

          std::atomic<std::size_t> bumpOffset{0};
     };

     void KiHeapInitialise();
     void* ExAllocatePool(PoolType type, std::size_t size, std::uint32_t tag);
     void ExFreePool(void* ptr);
     void ExEnumerateSegments(PoolType type, void (*callback)(void* poolHandle, std::size_t size));
     void ExEnumeratePool(void* poolHandle, void (*callback)(PoolHeader* header));

     constexpr std::uint32_t NonPagedPoolTag = 0x4b4f4e50; // 'KONP'
     constexpr std::uint32_t PagedPoolTag = 0x4b4f5045;    // 'KOPE'
} // namespace memory
