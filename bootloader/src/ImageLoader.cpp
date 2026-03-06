#include "ImageLoader.h"

#include <utils/PE.h>
#include <algorithm>
#include <memory>
#include <new>

namespace bootloader
{
     ImageLoader::ImageLoader(EFI_BOOT_SERVICES* bootServices)
         : _bootServices(bootServices), _lastStatus(EFI_SUCCESS), _baseAddress(nullptr), _entryPoint(nullptr),
           _imageSize(0)
     {
     }

     bool ImageLoader::ValidateImage(std::span<const std::byte> imageData)
     {
          if (imageData.size() < sizeof(DosHeader)) return false;

          const DosHeader& dosHeader = *std::launder(reinterpret_cast<const DosHeader*>(imageData.data()));
          if (dosHeader.eMagic != 0x5a4d) return false;

          if (imageData.size() < static_cast<std::size_t>(dosHeader.eLfanew) + sizeof(NtHeaders)) return false;

          const NtHeaders& ntHeader =
              *std::launder(reinterpret_cast<const NtHeaders*>(imageData.data() + dosHeader.eLfanew));
          return ntHeader.signature == 0x4550;
     }

     void ImageLoader::CopyHeaders(std::span<const std::byte> imageData, void* imageBase, std::uint32_t headerSize)
     {
          auto* dest = static_cast<std::byte*>(imageBase);
          for (std::uint32_t i = 0; i < headerSize; ++i) { dest[i] = imageData[i]; }
     }

     void ImageLoader::MapSections(std::span<const std::byte> imageData, void* imageBase, const SectionHeader* sections,
                                   std::uint16_t sectionCount)
     {
          for (std::uint16_t i = 0; i < sectionCount; ++i)
          {
               const SectionHeader& section = sections[i];

               if (section.sizeOfRawData == 0) continue;

               std::byte* dest = static_cast<std::byte*>(imageBase) + section.virtualAddress;
               const std::byte* src = imageData.data() + section.pointerToRawData;

               std::uninitialized_copy_n(src, std::max(section.sizeOfRawData, section.virtualSize), dest);
          }
     }

     void ImageLoader::ApplyRelocations(void* imageBase, const NtHeaders& ntHeader, std::uint64_t delta)
     {
          const DataDirectory& relocDir = ntHeader.optionalHeader.dataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
          if (relocDir.virtualAddress == 0 || relocDir.size == 0) return;

          auto* relocation =
              reinterpret_cast<ImageBaseRelocation*>(static_cast<std::byte*>(imageBase) + relocDir.virtualAddress);

          while (relocation->virtualAddress != 0)
          {
               std::uint8_t* dest = static_cast<std::uint8_t*>(imageBase) + relocation->virtualAddress;
               std::uint16_t* relocInfo = reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::byte*>(relocation) +
                                                                           sizeof(ImageBaseRelocation));

               std::uint32_t numRelocations =
                   (relocation->sizeOfBlock - sizeof(ImageBaseRelocation)) / sizeof(std::uint16_t);

               for (std::uint32_t i = 0; i < numRelocations; ++i)
               {
                    std::uint16_t type = relocInfo[i] >> 12;
                    std::uint16_t offset = relocInfo[i] & 0xFFF;

                    if (type == IMAGE_REL_BASED_DIR64)
                    {
                         std::uint64_t* patchAddr = reinterpret_cast<std::uint64_t*>(dest + offset);
                         *patchAddr += delta;
                    }
               }

               relocation = reinterpret_cast<ImageBaseRelocation*>(reinterpret_cast<std::byte*>(relocation) +
                                                                   relocation->sizeOfBlock);
          }
     }

     void* ImageLoader::LoadImage(std::span<const std::byte> imageData, EFI_STATUS* outStatus)
     {
          if (!ValidateImage(imageData))
          {
               this->_lastStatus = EFI_INVALID_PARAMETER;
               if (outStatus) *outStatus = this->_lastStatus;
               return nullptr;
          }

          const DosHeader& dosHeader = *std::launder(reinterpret_cast<const DosHeader*>(imageData.data()));
          const NtHeaders& ntHeader =
              *std::launder(reinterpret_cast<const NtHeaders*>(imageData.data() + dosHeader.eLfanew));

          void* imageBase = reinterpret_cast<void*>(ntHeader.optionalHeader.imageBase);
          UINTN imageSize = ntHeader.optionalHeader.sizeOfImage;
          UINTN pages = (imageSize + 0xFFF) / 0x1000;

          this->_lastStatus = this->_bootServices->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages,
                                                                 reinterpret_cast<EFI_PHYSICAL_ADDRESS*>(&imageBase));
          if (EFI_ERROR(this->_lastStatus))
          {
               if (outStatus) *outStatus = _lastStatus;
               return nullptr;
          }

          std::uninitialized_fill_n(static_cast<std::byte*>(imageBase), imageSize, std::byte{0});

          CopyHeaders(imageData, imageBase, ntHeader.optionalHeader.sizeOfHeaders);

          const auto* sectionHeaders =
              reinterpret_cast<const SectionHeader*>(imageData.data() + dosHeader.eLfanew + sizeof(std::uint32_t) +
                                                     sizeof(CoffFileHeader) + ntHeader.fileHeader.sizeOfOptionalHeader);
          MapSections(imageData, imageBase, sectionHeaders, ntHeader.fileHeader.numberOfSections);

          std::uint64_t delta = reinterpret_cast<std::uint64_t>(imageBase) - ntHeader.optionalHeader.imageBase;
          if (delta != 0) ApplyRelocations(imageBase, ntHeader, delta);

          this->_baseAddress = imageBase;
          this->_imageSize = imageSize;
          this->_entryPoint = static_cast<std::byte*>(imageBase) + ntHeader.optionalHeader.addressOfEntryPoint;

          this->_lastStatus = EFI_SUCCESS;
          if (outStatus) *outStatus = this->_lastStatus;
          return imageBase;
     }

     void ImageLoader::RelocateToVirtual(std::uintptr_t virtualBase)
     {
          if (!this->_baseAddress) return;

          const DosHeader& dosHeader = *static_cast<const DosHeader*>(this->_baseAddress);
          const NtHeaders& ntHeader =
              *reinterpret_cast<const NtHeaders*>(static_cast<std::byte*>(this->_baseAddress) + dosHeader.eLfanew);

          std::uintptr_t physicalBase = reinterpret_cast<std::uintptr_t>(this->_baseAddress);
          std::int64_t delta = static_cast<std::int64_t>(virtualBase) - static_cast<std::int64_t>(physicalBase);

          const DataDirectory& relocDir = ntHeader.optionalHeader.dataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
          if (relocDir.virtualAddress != 0 && relocDir.size != 0)
          {
               auto* relocation = reinterpret_cast<ImageBaseRelocation*>(static_cast<std::byte*>(this->_baseAddress) +
                                                                         relocDir.virtualAddress);

               while (relocation->virtualAddress != 0)
               {
                    std::uint8_t* dest = static_cast<std::uint8_t*>(this->_baseAddress) + relocation->virtualAddress;
                    std::uint16_t* relocInfo = reinterpret_cast<std::uint16_t*>(
                        reinterpret_cast<std::byte*>(relocation) + sizeof(ImageBaseRelocation));

                    std::uint32_t numRelocations =
                        (relocation->sizeOfBlock - sizeof(ImageBaseRelocation)) / sizeof(std::uint16_t);

                    for (std::uint32_t i = 0; i < numRelocations; ++i)
                    {
                         std::uint16_t type = relocInfo[i] >> 12;
                         std::uint16_t offset = relocInfo[i] & 0xFFF;

                         if (type == IMAGE_REL_BASED_DIR64)
                         {
                              std::uint64_t* patchAddr = reinterpret_cast<std::uint64_t*>(dest + offset);
                              *patchAddr += delta;
                         }
                         else if (type == IMAGE_REL_BASED_HIGHLOW) // 32-bit relocation
                         {
                              std::uint32_t* patchAddr = reinterpret_cast<std::uint32_t*>(dest + offset);
                              *patchAddr += static_cast<std::uint32_t>(delta);
                         }
                    }

                    relocation = reinterpret_cast<ImageBaseRelocation*>(reinterpret_cast<std::byte*>(relocation) +
                                                                        relocation->sizeOfBlock);
               }
          }

          std::uintptr_t entryOffset = ntHeader.optionalHeader.addressOfEntryPoint;
          this->_entryPoint = reinterpret_cast<void*>(virtualBase + entryOffset);
     }

} // namespace bootloader
