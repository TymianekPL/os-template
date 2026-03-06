#include "BootContext.h"

#include <utils/identify.h>
#include <utils/struct.h>
#include "Uefi/UefiMultiPhase.h"
#include "utils/memory.h"

inline CHAR16* operator""_16(const wchar_t* string, [[maybe_unused]] const std::size_t length)
{
     return const_cast<CHAR16*>( // NOLINT(cppcoreguidelines-pro-type-const-cast)
         reinterpret_cast<const CHAR16*>(string));
}

namespace bootloader
{
     EFI_BOOT_SERVICES* BootContext::s_bootServices = nullptr;
     std::array<void*, BootContext::MaxPageTableAllocations> BootContext::s_pageTableAllocs{};
     std::size_t BootContext::s_pageTableAllocCount = 0;

     BootContext::BootContext(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
         : _imageHandle(imageHandle), _systemTable(systemTable), _bootServices(systemTable->BootServices),
           _lastStatus(EFI_SUCCESS), _pageTableRoot(0), _mappedRegions(nullptr), _mappedRegionCount(0),
           _mappedRegionCapacity(100)
     {
          s_bootServices = this->_bootServices;

          this->_bootServices->AllocatePool(EfiLoaderData, this->_mappedRegionCapacity * sizeof(MappedRegion),
                                            reinterpret_cast<void**>(&this->_mappedRegions));
     }

     void* BootContext::AllocatePageTableMemory(std::size_t size)
     {
          if (!s_bootServices) return nullptr;

          EFI_PHYSICAL_ADDRESS memory{};
          EFI_STATUS status = s_bootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, size / 0x1000, &memory);
          if (EFI_ERROR(status)) return nullptr;
          void* lpMemory = reinterpret_cast<void*>(memory);

          TrackPageTableAllocation(lpMemory, size);
          return lpMemory;
     }

     void BootContext::TrackPageTableAllocation(void* address, std::size_t size)
     {
          if (s_pageTableAllocCount < MaxPageTableAllocations) s_pageTableAllocs[s_pageTableAllocCount++] = address;
     }

     bool BootContext::MapPageTableStructures()
     {
          for (std::size_t i = 0; i < s_pageTableAllocCount; ++i)
          {
               std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(s_pageTableAllocs[i]);
               if (!MapRegion(addr, 0x1000, true, L"Page Table")) return false;
          }
          return true;
     }

     EFI_STATUS BootContext::GetFramebufferInfo(arch::Framebuffer* outFramebuffer)
     {
          EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
          EFI_STATUS status = this->_bootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, nullptr,
                                                                  reinterpret_cast<void**>(&gop));
          if (EFI_ERROR(status)) return status;

          EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = gop->Mode->Info;

          outFramebuffer->physicalStart = gop->Mode->FrameBufferBase;
          outFramebuffer->totalSize = gop->Mode->FrameBufferSize;
          outFramebuffer->width = info->HorizontalResolution;
          outFramebuffer->height = info->VerticalResolution;
          outFramebuffer->scanlineSize = info->PixelsPerScanLine;

          if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
          {
               outFramebuffer->maskRed = 0x000000FF;
               outFramebuffer->maskGreen = 0x0000FF00;
               outFramebuffer->maskBlue = 0x00FF0000;
          }
          else if (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
          {
               outFramebuffer->maskRed = 0x00FF0000;
               outFramebuffer->maskGreen = 0x0000FF00;
               outFramebuffer->maskBlue = 0x000000FF;
          }
          else if (info->PixelFormat == PixelBitMask)
          {
               outFramebuffer->maskRed = info->PixelInformation.RedMask;
               outFramebuffer->maskGreen = info->PixelInformation.GreenMask;
               outFramebuffer->maskBlue = info->PixelInformation.BlueMask;
          }
          else
          {
               outFramebuffer->maskRed = 0x00FF0000;
               outFramebuffer->maskGreen = 0x0000FF00;
               outFramebuffer->maskBlue = 0x000000FF;
          }

          return EFI_SUCCESS;
     }

     EFI_STATUS BootContext::BuildMemoryDescriptors(arch::LoaderParameterBlock* outBlock)
     {
          UINTN memoryMapSize = 0;
          UINTN mapKey = 0;
          UINTN descriptorSize = 0;
          UINT32 descriptorVersion = 0;

          EFI_STATUS status =
              this->_bootServices->GetMemoryMap(&memoryMapSize, nullptr, &mapKey, &descriptorSize, &descriptorVersion);
          if (status != EFI_BUFFER_TOO_SMALL) return status;

          memoryMapSize += 10 * descriptorSize;
          EFI_MEMORY_DESCRIPTOR* memoryMap = nullptr;
          status =
              this->_bootServices->AllocatePool(EfiLoaderData, memoryMapSize, reinterpret_cast<void**>(&memoryMap));
          if (EFI_ERROR(status)) return status;

          status = this->_bootServices->GetMemoryMap(&memoryMapSize, memoryMap, &mapKey, &descriptorSize,
                                                     &descriptorVersion);
          if (EFI_ERROR(status))
          {
               this->_bootServices->FreePool(memoryMap);
               return status;
          }

          UINTN numberOfDescriptors = memoryMapSize / descriptorSize;

          structures::ListEntry<arch::MemoryDescriptor>* entries = nullptr;
          status = this->_bootServices->AllocatePool(
              EfiLoaderData, numberOfDescriptors * sizeof(structures::ListEntry<arch::MemoryDescriptor>),
              reinterpret_cast<void**>(&entries));
          if (EFI_ERROR(status))
          {
               this->_bootServices->FreePool(memoryMap);
               return status;
          }

          structures::ListEntry<arch::MemoryDescriptor>* previous = nullptr;
          UINTN entryIndex = 0;

          for (UINTN i = 0; i < numberOfDescriptors; ++i)
          {
               EFI_MEMORY_DESCRIPTOR* descriptor = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(
                   reinterpret_cast<std::byte*>(memoryMap) + (i * descriptorSize));

               structures::ListEntry<arch::MemoryDescriptor>* entry = &entries[entryIndex++];

               switch (descriptor->Type)
               {
               case EfiConventionalMemory: entry->data.type = arch::MemoryType::Free; break;
               case EfiReservedMemoryType: entry->data.type = arch::MemoryType::Reserved; break;
               case EfiLoaderCode: entry->data.type = arch::MemoryType::LoaderCode; break;
               case EfiLoaderData: entry->data.type = arch::MemoryType::LoaderData; break;
               case EfiUnusableMemory: entry->data.type = arch::MemoryType::UnusableMemory; break;
               case EfiACPIReclaimMemory: entry->data.type = arch::MemoryType::ACPIReclaimMemory; break;
               case EfiBootServicesCode: entry->data.type = arch::MemoryType::BootServicesCode; break;
               case EfiBootServicesData: entry->data.type = arch::MemoryType::BootServicesData; break;
               case EfiRuntimeServicesCode: entry->data.type = arch::MemoryType::RuntimeServicesCode; break;
               case EfiRuntimeServicesData: entry->data.type = arch::MemoryType::RuntimeServicesData; break;
               case EfiACPIMemoryNVS: entry->data.type = arch::MemoryType::ACPIMemoryNVS; break;
               case EfiMemoryMappedIO: entry->data.type = arch::MemoryType::MemoryMappedIO; break;
               case EfiMemoryMappedIOPortSpace: entry->data.type = arch::MemoryType::MemoryMappedIOPortSpace; break;
               case EfiPalCode: entry->data.type = arch::MemoryType::PalCode; break;
               case EfiPersistentMemory: entry->data.type = arch::MemoryType::PersistentMemory; break;
               default: entry->data.type = arch::MemoryType::Reserved; break;
               }

               entry->data.basePage = descriptor->PhysicalStart / 0x1000;
               entry->data.baseCount = static_cast<std::uint32_t>(descriptor->NumberOfPages);
               entry->next = nullptr;
               entry->previous = previous;

               if (previous != nullptr) previous->next = entry;
               else
                    outBlock->memoryDescriptors.head = entry;

               previous = entry;
          }

          this->_bootServices->FreePool(memoryMap);
          return EFI_SUCCESS;
     }

     bool BootContext::PrepareLoaderBlock(arch::LoaderParameterBlock* outBlock)
     {
          this->_lastStatus = GetFramebufferInfo(&outBlock->framebuffer);
          if (EFI_ERROR(this->_lastStatus)) return false;

          this->_lastStatus = BuildMemoryDescriptors(outBlock);
          return !EFI_ERROR(this->_lastStatus);
     }

     void BootContext::TrackMappedRegion(std::uintptr_t start, std::size_t size, const wchar_t* name)
     {
          if (this->_mappedRegions && this->_mappedRegionCount < this->_mappedRegionCapacity)
          {
               this->_mappedRegions[this->_mappedRegionCount].start = start;
               this->_mappedRegions[this->_mappedRegionCount].size = size;
               this->_mappedRegions[this->_mappedRegionCount].name = name;
               this->_mappedRegionCount++;
          }
     }

     bool BootContext::MapRegion(std::uintptr_t physicalAddress, std::size_t size, bool writable, const wchar_t* name)
     {
          if (this->_pageTableRoot == 0) return false;

          std::uintptr_t alignedStart = physicalAddress & ~0xFFFull;
          std::size_t alignedSize = ((size + 0xFFF) & ~0xFFFull) + (physicalAddress - alignedStart);

          memory::PageMapping mapping{};
          mapping.virtualAddress = alignedStart;
          mapping.physicalAddress = alignedStart;
          mapping.size = alignedSize;
          mapping.writable = writable;
          mapping.userAccessible = false;
          mapping.cacheDisable = false;

          bool result = memory::paging::MapPage(this->_pageTableRoot, mapping, AllocatePageTableMemory);
          if (result) TrackMappedRegion(alignedStart, alignedSize, name);
          return result;
     }

     bool BootContext::MapEfiRuntimeRegions()
     {
          UINTN memoryMapSize = 0;
          UINTN mapKey = 0;
          UINTN descriptorSize = 0;
          UINT32 descriptorVersion = 0;

          EFI_STATUS status =
              this->_bootServices->GetMemoryMap(&memoryMapSize, nullptr, &mapKey, &descriptorSize, &descriptorVersion);
          if (status != EFI_BUFFER_TOO_SMALL) return false;

          memoryMapSize += 10 * descriptorSize;
          EFI_MEMORY_DESCRIPTOR* memoryMap = nullptr;
          status =
              this->_bootServices->AllocatePool(EfiLoaderData, memoryMapSize, reinterpret_cast<void**>(&memoryMap));
          if (EFI_ERROR(status)) return false;

          status = this->_bootServices->GetMemoryMap(&memoryMapSize, memoryMap, &mapKey, &descriptorSize,
                                                     &descriptorVersion);
          if (EFI_ERROR(status))
          {
               this->_bootServices->FreePool(memoryMap);
               return false;
          }

          UINTN numberOfDescriptors = memoryMapSize / descriptorSize;
          bool success = true;

          for (UINTN i = 0; i < numberOfDescriptors; i++)
          {
               EFI_MEMORY_DESCRIPTOR* desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(
                   reinterpret_cast<std::byte*>(memoryMap) + (i * descriptorSize));

               if (desc->Type == EfiACPIReclaimMemory || desc->Type == EfiACPIMemoryNVS ||
                   desc->Type == EfiMemoryMappedIO || desc->Type == EfiMemoryMappedIOPortSpace ||
                   desc->Type == EfiLoaderData || desc->Type == EfiLoaderCode)
               {
                    std::uintptr_t physAddr = desc->PhysicalStart;
                    std::size_t size = desc->NumberOfPages * 0x1000;

                    const wchar_t* typeName = L"EFI Runtime";
                    if (desc->Type == EfiRuntimeServicesCode) typeName = L"EFI RT Code";
                    else if (desc->Type == EfiRuntimeServicesData)
                         typeName = L"EFI RT Data";
                    else if (desc->Type == EfiACPIReclaimMemory)
                         typeName = L"ACPI Reclaim";
                    else if (desc->Type == EfiACPIMemoryNVS)
                         typeName = L"ACPI NVS";
                    else if (desc->Type == EfiMemoryMappedIO)
                         typeName = L"MMIO";
                    else if (desc->Type == EfiMemoryMappedIOPortSpace)
                         typeName = L"MMIO Port";
                    else if (desc->Type == EfiLoaderData)
                         typeName = L"Loader Data";
                    else if (desc->Type == EfiLoaderCode)
                         typeName = L"Loader Code";
                    else
                         typeName = L"MMIO Port";

                    if (!MapRegion(physAddr, size, true, typeName))
                    {
                         success = false;
                         break;
                    }
               }
          }

          this->_bootServices->FreePool(memoryMap);
          return success;
     }

     bool BootContext::SetupPageTables(void* imageBase, std::size_t imageSize, std::uintptr_t stackPointer,
                                       const arch::Framebuffer& framebuffer)
     {
          this->_pageTableRoot = memory::paging::CreatePageTable(AllocatePageTableMemory);

          if (this->_pageTableRoot == 0)
          {
               this->_lastStatus = EFI_OUT_OF_RESOURCES;
               return false;
          }

          if (!MapEfiRuntimeRegions())
          {
               this->_lastStatus = EFI_OUT_OF_RESOURCES;
               return false;
          }

          std::uintptr_t imageAddr = reinterpret_cast<std::uintptr_t>(imageBase);
          if (!MapRegion(imageAddr, imageSize, true, L"Kernel Image"))
          {
               this->_lastStatus = EFI_OUT_OF_RESOURCES;
               return false;
          }

          constexpr std::size_t stackSize = 1024ULL * 1024ULL; // 1MB
          std::uintptr_t stackBase = (stackPointer - stackSize) & ~0xFFFull;
          if (!MapRegion(stackBase, stackSize + 0x1000, true, L"Stack"))
          {
               this->_lastStatus = EFI_OUT_OF_RESOURCES;
               return false;
          }

          if (framebuffer.physicalStart != 0 && framebuffer.totalSize > 0)
          {
               if (!MapRegion(framebuffer.physicalStart, framebuffer.totalSize, true, L"Framebuffer"))
               {
                    this->_lastStatus = EFI_OUT_OF_RESOURCES;
                    return false;
               }
          }

          this->_lastStatus = EFI_SUCCESS;
          return true;
     }

     bool BootContext::RemapKernelVirtual(void* physicalBase, std::size_t imageSize, std::uintptr_t virtualBase)
     {
          if (this->_pageTableRoot == 0) return false;

          std::uintptr_t physAddr = reinterpret_cast<std::uintptr_t>(physicalBase);
          std::uintptr_t alignedPhysStart = physAddr & ~0xFFFull;
          std::uintptr_t alignedVirtStart = virtualBase & ~0xFFFull;
          std::size_t alignedSize = ((imageSize + 0xFFF) & ~0xFFFull) + (physAddr - alignedPhysStart);

          memory::PageMapping mapping{};
          mapping.virtualAddress = alignedVirtStart;
          mapping.physicalAddress = alignedPhysStart;
          mapping.size = alignedSize;
          mapping.writable = true;
          mapping.userAccessible = false;
          mapping.cacheDisable = false;

          bool result = memory::paging::MapPage(this->_pageTableRoot, mapping, AllocatePageTableMemory);
          if (result) TrackMappedRegion(alignedVirtStart, alignedSize, L"Kernel Virtual");
          return result;
     }

     bool BootContext::ExitBootServices(UINTN* outMapKey)
     {
          UINTN memoryMapSize = 0;
          UINTN mapKey = 0;
          UINTN descriptorSize = 0;
          UINT32 descriptorVersion = 0;

          this->_lastStatus =
              this->_bootServices->GetMemoryMap(&memoryMapSize, nullptr, &mapKey, &descriptorSize, &descriptorVersion);
          if (this->_lastStatus != EFI_BUFFER_TOO_SMALL) return false;

          memoryMapSize += 2 * descriptorSize;
          EFI_MEMORY_DESCRIPTOR* memoryMap = nullptr;
          this->_lastStatus =
              this->_bootServices->AllocatePool(EfiLoaderData, memoryMapSize, reinterpret_cast<void**>(&memoryMap));
          if (EFI_ERROR(this->_lastStatus)) return false;

          this->_lastStatus = this->_bootServices->GetMemoryMap(&memoryMapSize, memoryMap, &mapKey, &descriptorSize,
                                                                &descriptorVersion);
          if (EFI_ERROR(this->_lastStatus))
          {
               this->_bootServices->FreePool(memoryMap);
               return false;
          }

          this->_lastStatus = this->_bootServices->ExitBootServices(this->_imageHandle, mapKey);
          if (EFI_ERROR(this->_lastStatus)) return false;

          if (outMapKey) *outMapKey = mapKey;
          return !EFI_ERROR(this->_lastStatus);
     }

     void BootContext::ReclaimBootServices(arch::LoaderParameterBlock* loaderBlock)
     {
          if (!loaderBlock) return;

          structures::ListEntry<arch::MemoryDescriptor>* current = loaderBlock->memoryDescriptors.head;
          while (current != nullptr)
          {
               if (current->data.type == arch::MemoryType::BootServicesCode ||
                   current->data.type == arch::MemoryType::BootServicesData ||
                   current->data.type == arch::MemoryType::RuntimeServicesCode ||
                   current->data.type == arch::MemoryType::RuntimeServicesData)
               {
                    current->data.type = arch::MemoryType::Free;
               }
               current = current->next;
          }
     }

} // namespace bootloader
