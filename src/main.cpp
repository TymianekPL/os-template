#include <cstddef>
#include <string_view>

#include <Uefi.h>

#ifdef __clang__
#define COMPILER L"clang"
#elifdef _MSC_VER
#define COMPILER L"msvc"
#elifdef __GNUC__
#define COMPILER L"gcc"
#else
#define COMPILER L"unknown"
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define ARCHITECTURE L"x86-64"
#elifdef _M_IX86
#define ARCHITECTURE L"x86-32"
#elifdef __i386__
#define ARCHITECTURE L"x86-32"
#elifdef _M_ARM64
#define ARCHITECTURE L"ARM64"
#elifdef __aarch64__
#define ARCHITECTURE L"ARM64"
#else
#define ARCHITECTURE L"unknown"
#endif
constexpr auto Configuration = std::wstring_view{COMPILER L" " ARCHITECTURE};

inline CHAR16* operator""_16(const wchar_t* string, [[maybe_unused]] const std::size_t length)
{
     return const_cast<CHAR16*>( // NOLINT(cppcoreguidelines-pro-type-const-cast)
         reinterpret_cast<const CHAR16*>(string));
}

extern "C" EFI_STATUS EFIAPI EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
     EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut = SystemTable->ConOut;

     ConOut->ClearScreen(ConOut);

     ConOut->OutputString(ConOut, L"Hi "_16);
     ConOut->OutputString(ConOut, operator""_16(Configuration.data(), Configuration.size()));
     ConOut->OutputString(ConOut, L"\r\n"_16);

     EFI_INPUT_KEY Key;
     SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);

     UINTN Index{};
     SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Index);
     SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);

     return EFI_SUCCESS;
}
