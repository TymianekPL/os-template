#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include <Uefi.h>

struct _EFI_FILE_PROTOCOL;
using EFI_FILE_PROTOCOL = struct _EFI_FILE_PROTOCOL;

namespace bootloader
{
     class FileLoader
     {
     public:
          explicit FileLoader(EFI_BOOT_SERVICES* bootServices);
          std::span<std::byte> LoadFile(std::basic_string_view<CHAR16> filePath, EFI_STATUS* outStatus);
          void FreeFile(std::span<std::byte> fileData);
          [[nodiscard]] EFI_STATUS GetLastStatus() const { return _lastStatus; }

     private:
          EFI_BOOT_SERVICES* _bootServices;
          EFI_STATUS _lastStatus;
          EFI_STATUS GetFileSize(EFI_FILE_PROTOCOL* file, UINTN* outSize);
          EFI_STATUS LoadFileToBuffer(EFI_FILE_PROTOCOL* file, UINTN size, void** outBuffer);
     };

} // namespace bootloader
