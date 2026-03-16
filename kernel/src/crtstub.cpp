#include "utils/kdbg.h"
extern "C" int _CrtDbgReport(int reportType, // NOLINT(readability-identifier-naming)
                             const char* filename, int linenumber, const char* moduleName, const char* format, ...)
{
     (void)reportType;
     (void)filename;
     (void)linenumber;
     (void)moduleName;
     (void)format;
     return 0;
}

extern "C" float __cdecl ceilf(const float x) // NOLINT
{
     const auto i = static_cast<int>(x);
     if (static_cast<float>(i) == x) return x;
     if (x > 0.0f) return static_cast<float>(i + 1);
     return static_cast<float>(i);
}

// chkstk
extern "C" void __chkstk() // NOLINT
{
     // No-op on x86-64 since the architecture guarantees a 4KiB red zone below the stack pointer.
}
extern "C" std::uintptr_t __CxxFrameHandler4(void*, void*, void*, void*) noexcept
{
     debugging::DbgWrite(u8"*** Unhandled C++ exception - system is likely unstable! ***\r\n");
     operations::DisableInterrupts();
     while (true) operations::Halt();
}

extern "C" const void* memchr(const void* ptr, int value, std::size_t count) noexcept
{
     const auto* p = static_cast<const unsigned char*>(ptr);
     const auto v = static_cast<unsigned char>(value);
     for (std::size_t i = 0; i < count; ++i)
          if (p[i] == v) return p + i;
     return nullptr;
}
unsigned int KiRotateLeft32(unsigned int value, int shift) noexcept
{
     const auto s = static_cast<unsigned int>(shift) & 31u;
     return (value << s) | (value >> (32u - s));
}
namespace std
{
     [[noreturn]] void _Xlength_error(const char*) noexcept
     {
          operations::DisableInterrupts();
          while (true) operations::Halt();
     }

     [[noreturn]] void _Xout_of_range(const char*) noexcept
     {
          operations::DisableInterrupts();
          while (true) operations::Halt();
     }
} // namespace std

namespace std
{
     void (*_Raise_handler)(const stdext::exception&) = nullptr;
}

const void* __stdcall __std_find_trivial_1(const void* _First, const void* _Last, std::uint8_t _Val) noexcept
{
     const auto* first = static_cast<const std::uint8_t*>(_First);
     const auto* last = static_cast<const std::uint8_t*>(_Last);
     const auto value = _Val;
     for (; first != last; ++first)
          if (*first == value) return first;
     return last;
}
