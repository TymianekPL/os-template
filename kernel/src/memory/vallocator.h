#pragma once

#include <cstddef>
#include <cstdint>
#include "pallocator.h"

namespace memory
{
     enum struct VADMemoryState : std::uint8_t
     {
          Free,
          Reserved,
          PagedOut,
     };
     enum struct MemoryProtection : std::uint16_t
     {
          NoAccess = 0,
          ReadOnly = 1,
          ReadWrite = 2,
          WriteCopy = 4,
          Execute = 8,
          NoCache = 16,
          WriteCombine = 32,
          LargePages = 64,
          Global = 128,
          Guard = 256,
          ExecuteRead = Execute | ReadOnly,
          ExecuteReadWrite = Execute | ReadWrite,
          ExecuteWriteCopy = Execute | WriteCopy,
     };

     struct VADEntry
     {
          VADMemoryState state;
          MemoryProtection protection;
          PFNUse use;
          std::uintptr_t baseAddress;
          std::size_t size;

          explicit VADEntry(std::uintptr_t baseAddress, std::size_t size, VADMemoryState state,
                            MemoryProtection protection, PFNUse use)
              : baseAddress(baseAddress), size(size), state(state), protection(protection), use(use)
          {
          }
     };

     enum struct RBColour : std::uint8_t
     {
          Red,
          Black
     };

     struct VADNode
     {
          VADEntry entry;
          RBColour colour{RBColour::Red};
          VADNode* parent{nullptr};
          VADNode* left{nullptr};
          VADNode* right{nullptr};
          std::uintptr_t maxEnd;

          explicit VADNode(const VADEntry& entry) : entry(entry), maxEnd(entry.baseAddress + entry.size) {}

          [[nodiscard]] std::uintptr_t EndAddress() const { return entry.baseAddress + entry.size; }
     };

     struct VADNodeAllocator
     {
     private:
          void* _freeList{nullptr};
          std::size_t _nodesAllocated{0};
          std::size_t _nodesFreed{0};

          bool AllocateNodePage();

     public:
          VADNodeAllocator() = default;

          VADNode* AllocateNode(const VADEntry& entry);
          void FreeNode(VADNode* node);

          [[nodiscard]] std::size_t GetNodesAllocated() const { return _nodesAllocated; }
          [[nodiscard]] std::size_t GetNodesFreed() const { return _nodesFreed; }
          [[nodiscard]] std::size_t GetNodesInUse() const { return _nodesAllocated - _nodesFreed; }
     };

     struct VirtualMemoryAllocator
     {
     private:
          VADNode* _root{nullptr};
          std::uintptr_t _searchStart{0x10000};
          VADNodeAllocator _nodeAllocator;

          void RotateLeft(VADNode* x);
          void RotateRight(VADNode* y);
          void FixInsert(VADNode* z);
          void FixDelete(VADNode* x);
          void Transplant(VADNode* u, VADNode* v);
          VADNode* Minimum(VADNode* node);
          void UpdateMaxEnd(VADNode* node);
          void UpdateMaxEndUpwards(VADNode* node);
          std::uintptr_t FindFreeRegion(std::size_t size, std::uintptr_t hint = 0);

     public:
          VirtualMemoryAllocator() = default;
          explicit VirtualMemoryAllocator(std::uintptr_t searchStart) : _searchStart(searchStart) {}

          bool Insert(const VADEntry& entry);
          bool Remove(std::uintptr_t baseAddress);
          VADNode* Search(std::uintptr_t baseAddress);
          VADNode* FindOverlap(std::uintptr_t baseAddress, std::size_t size);

          std::uintptr_t ReserveVirtualMemory(std::size_t size, MemoryProtection protection, PFNUse use);
          std::uintptr_t ReserveVirtualMemoryAt(std::uintptr_t hint, std::size_t size, MemoryProtection protection,
                                                PFNUse use);

          bool ReserveVirtualMemoryFixed(std::uintptr_t baseAddress, std::size_t size, MemoryProtection protection,
                                         PFNUse use);

          [[nodiscard]] std::uintptr_t GetSearchStart() const { return _searchStart; }

          VADNode* FindContaining(std::uintptr_t address);
          void InorderTraversal(VADNode* node, void (*callback)(const VADEntry&));

          [[nodiscard]] VADNode* GetRoot() const { return _root; }
          [[nodiscard]] const VADNodeAllocator& GetNodeAllocator() const { return _nodeAllocator; }
     };

} // namespace memory
