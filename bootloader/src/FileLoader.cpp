#include "FileLoader.h"

#include <Guid/FileInfo.h>
#include <Protocol/SimpleFileSystem.h>

namespace bootloader
{
     FileLoader::FileLoader(EFI_BOOT_SERVICES* bootServices) : _bootServices(bootServices), _lastStatus(EFI_SUCCESS) {}

     EFI_STATUS FileLoader::GetFileSize(EFI_FILE_PROTOCOL* file, UINTN* outSize)
     {
          UINTN fileInfoSize = 0;
          EFI_STATUS status = file->GetInfo(file, &gEfiFileInfoGuid, &fileInfoSize, nullptr);

          if (status != EFI_BUFFER_TOO_SMALL) return status;

          EFI_FILE_INFO* fileInfo = nullptr;
          status = this->_bootServices->AllocatePool(EfiLoaderData, fileInfoSize, reinterpret_cast<void**>(&fileInfo));
          if (EFI_ERROR(status)) return status;

          status = file->GetInfo(file, &gEfiFileInfoGuid, &fileInfoSize, fileInfo);
          if (EFI_ERROR(status))
          {
               this->_bootServices->FreePool(fileInfo);
               return status;
          }

          *outSize = fileInfo->FileSize;
          this->_bootServices->FreePool(fileInfo);
          return EFI_SUCCESS;
     }

     EFI_STATUS FileLoader::LoadFileToBuffer(EFI_FILE_PROTOCOL* file, UINTN size, void** outBuffer)
     {
          void* buffer = nullptr;
          EFI_STATUS status = this->_bootServices->AllocatePool(EfiLoaderData, size, &buffer);
          if (EFI_ERROR(status)) return status;

          UINTN readSize = size;
          status = file->Read(file, &readSize, buffer);
          if (EFI_ERROR(status) || readSize != size)
          {
               this->_bootServices->FreePool(buffer);
               return EFI_ERROR(status) ? status : EFI_LOAD_ERROR;
          }

          *outBuffer = buffer;
          return EFI_SUCCESS;
     }

     std::span<std::byte> FileLoader::LoadFile(const std::basic_string_view<CHAR16> filePath, EFI_STATUS* outStatus)
     {
          EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem = nullptr;
          this->_lastStatus = this->_bootServices->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, nullptr,
                                                                  reinterpret_cast<void**>(&fileSystem));
          if (EFI_ERROR(this->_lastStatus))
          {
               if (outStatus) *outStatus = this->_lastStatus;
               return {};
          }

          EFI_FILE_PROTOCOL* root = nullptr;
          this->_lastStatus = fileSystem->OpenVolume(fileSystem, &root);
          if (EFI_ERROR(_lastStatus))
          {
               if (outStatus) *outStatus = this->_lastStatus;
               return {};
          }

          EFI_FILE_PROTOCOL* file = nullptr;
          this->_lastStatus =
              root->Open(root, &file,
                         const_cast<CHAR16*>(filePath.data()), // NOLINT(cppcoreguidelines-pro-type-const-cast)
                         EFI_FILE_MODE_READ, 0);

          if (EFI_ERROR(this->_lastStatus))
          {
               root->Close(root);
               if (outStatus) *outStatus = _lastStatus;
               return {};
          }

          UINTN fileSize = 0;
          this->_lastStatus = GetFileSize(file, &fileSize);
          if (EFI_ERROR(this->_lastStatus))
          {
               file->Close(file);
               root->Close(root);
               if (outStatus) *outStatus = this->_lastStatus;
               return {};
          }

          void* buffer = nullptr;
          this->_lastStatus = LoadFileToBuffer(file, fileSize, &buffer);

          file->Close(file);
          root->Close(root);

          if (EFI_ERROR(this->_lastStatus))
          {
               if (outStatus) *outStatus = this->_lastStatus;
               return {};
          }

          if (outStatus) *outStatus = EFI_SUCCESS;
          return std::span{static_cast<std::byte*>(buffer), fileSize};
     }

     void FileLoader::FreeFile(std::span<std::byte> fileData)
     {
          if (!fileData.empty()) { _bootServices->FreePool(fileData.data()); }
     }

} // namespace bootloader
