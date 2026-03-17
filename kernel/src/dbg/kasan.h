#pragma once

#include <cstddef>
namespace kasan
{
     void KeInitialise();
     void Allocated(void* ptr, std::size_t size);
     void Freed(void* ptr);

     template<std::size_t size>
     void Load(void* ptr);
     template<std::size_t size>
     void Store(void* ptr);
}

#define KASAN_GUARD_ALLOC(ptr, size) kasan::Allocated(ptr, size)
#define KASAN_GUARD_FREE(ptr) kasan::Freed(ptr)
#define KASAN_GUARD_LOAD(ptr) kasan::Load<sizeof(*(ptr))>(ptr)
#define KASAN_GUARD_STORE(ptr) kasan::Store<sizeof(*(ptr))>(ptr)

void KASANAllocateHeap(void* base, std::size_t size);
void KASANFreeHeap(void* base, std::size_t size);
