#include "pcie.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include "../cpu/interrupts.h"
#include "../kinit.h"
#include "utils/kdbg.h"
#include "utils/memory.h"
#include "utils/operations.h"

extern MCFG* g_mcfg;

static bool KIIsDevicePresent(std::uintptr_t base, std::uint8_t bus, std::uint8_t device, std::uint8_t function)
{
     const std::uintptr_t address = (base + (static_cast<std::uintptr_t>(bus) << 20)) |
                                    (static_cast<std::uintptr_t>(device) << 15) |
                                    (static_cast<std::uintptr_t>(function) << 12);
     auto* hdr = reinterpret_cast<volatile PCIHeader*>(address);
     return hdr->vendorId != 0xFFFF;
}

static bool KiIsDeviceMultifunction(std::uintptr_t base, std::uint8_t bus, std::uint8_t device)
{
     const std::uintptr_t address =
         base + (static_cast<std::uintptr_t>(bus) << 20) + (static_cast<std::uintptr_t>(device) << 15);
     auto* hdr = reinterpret_cast<volatile PCIHeader*>(address);
     return (hdr->headerType & 0x80) != 0;
}

void KiPCIEScanBus(std::uintptr_t base, std::uint16_t segment, std::uint8_t bus, std::uint8_t startDevice,
                   std::uint8_t endDevice, PCIDevice* parent);

void KeGetDeviceBARs(volatile PCIHeader* hdr, PciBar (&bars)[6]) // NOLINT
{
     for (int i = 0; i < 6; ++i)
     {
          const std::uint32_t barVal = *(&hdr->bar0 + i);
          bars[i].value = barVal;
          bars[i].isMmio = (barVal & 1) == 0;

          if (bars[i].isMmio)
          {
               const std::uint32_t type = (barVal >> 1) & 0x3;
               bars[i].is64bit = type == 0x2;

               if (bars[i].is64bit && i < 5)
               {
                    bars[i].address = static_cast<std::uint64_t>(barVal & ~0xF) |
                                      (static_cast<std::uint64_t>(*(&hdr->bar0 + i + 1)) << 32);
                    ++i;
               }
               else
               {
                    bars[i].address = barVal & ~0xF;
               }
          }
          else // I/O BAR
          {
               bars[i].is64bit = false;
               bars[i].address = barVal & ~0x3;
          }
     }
}

std::uintptr_t KeFindControllerBAR(volatile PCIHeader* hdr, std::uint8_t progIf, PciBar (&bars)[6]) // NOLINT
{
     for (auto& bar : bars)
     {
          if (bar.address == 0) continue;

          if (progIf == 0x00 && !bar.isMmio) return static_cast<std::uintptr_t>(bar.address); // UHCI
          if (progIf == 0x10 && bar.isMmio) return static_cast<std::uintptr_t>(bar.address);  // OHCI
          if (progIf == 0x20 && bar.isMmio) return static_cast<std::uintptr_t>(bar.address);  // EHCI
          if (progIf == 0x30 && bar.isMmio) return static_cast<std::uintptr_t>(bar.address);  // XHCI
     }
     return 0;
}

static std::uintptr_t g_pcieBase = 0;

static void KiPCIEScanFunction(std::uintptr_t base, std::uint16_t segment, std::uint8_t bus, std::uint8_t device,
                               std::uint8_t function, PCIDevice* parent)
{
     auto* hdr = reinterpret_cast<volatile PCIHeader*>((base + (static_cast<std::uintptr_t>(bus) << 20)) |
                                                       (static_cast<std::uintptr_t>(device) << 15) |
                                                       (static_cast<std::uintptr_t>(function) << 12));

     if (hdr->vendorId == 0xFFFF) return;
     static std::size_t deviceIndex = 0;

     std::array<char, 64> name{};
     char* next = name.data();

     std::memcpy(next, u8"PCI", 3);
     next += 3;
     *(next++) = '\\';

     auto writeHex = [&](auto value, int width)
     {
          std::array<char, 16> buffer{};
          char* p = buffer.data() + width;
          for (int i = 0; i < width; ++i)
          {
               std::uint8_t digit = value & 0xF;
               *(--p) = static_cast<char>((digit < 10) ? '0' + digit : 'A' + (digit - 10));
               value >>= 4;
          }
          std::memcpy(next, buffer.data(), width);
          next += width;
     };

     auto writeDecimal = [&](auto value)
     {
          char* start = next;
          next += std::to_chars(next, name.data() + 64, value, 10).ptr - next;
          return start;
     };

     writeHex(hdr->vendorId, 4);
     *(next++) = ':';

     writeHex(hdr->deviceId, 4);
     *(next++) = '_';

     writeHex(hdr->subsystemVendorId, 4);
     *(next++) = ':';

     writeHex(hdr->subsystemId, 4);
     *(next++) = '_';

     writeHex(hdr->revisionId, 2);
     *(next++) = '_';

     writeDecimal(bus);
     *(next++) = ':';
     writeDecimal(device);
     *(next++) = ':';
     writeDecimal(function);
     *(next++) = '\\';

     writeDecimal(deviceIndex++);

     *next = '\0';

     object::ObjectAttributes attributes{.name = name.data(),
                                         .type = object::ObjectType::Device,
                                         .bodySize = sizeof(PCIDevice),
                                         .destructor = +[](void*) noexcept {},
                                         .desiredAccess = object::AccessRights::All};
     auto* hObject = object::ObCreateObjectIn(*g_kernelProcess, attributes, "Device");
     if (!hObject)
     {
          debugging::DbgWrite(u8"Failed to create object for PCI device {:x}:{:x}\r\n", hdr->vendorId, hdr->deviceId);
          return;
     }

     auto* lpDevice = new (object::ObGetBody<void>(*g_kernelProcess, hObject, object::ObjectType::Device)) PCIDevice{};
     if (!lpDevice)
     {
          debugging::DbgWrite(u8"Failed to get body for PCI device {:x}:{:x}\r\n", hdr->vendorId, hdr->deviceId);
          object::ObCloseHandle(*g_kernelProcess, hObject);
          return;
     }
     lpDevice->vendorId = hdr->vendorId;
     lpDevice->deviceId = hdr->deviceId;
     lpDevice->baseClass = hdr->baseClass;
     lpDevice->subClass = hdr->subclass;
     lpDevice->progIf = hdr->progIf;
     lpDevice->header = hdr;
     lpDevice->segment = segment;
     lpDevice->bus = static_cast<std::uint8_t>((reinterpret_cast<std::uintptr_t>(hdr) >> 20) & 0xFF);
     lpDevice->device = static_cast<std::uint8_t>((reinterpret_cast<std::uintptr_t>(hdr) >> 15) & 0x1F);
     lpDevice->function = static_cast<std::uint8_t>((reinterpret_cast<std::uintptr_t>(hdr) >> 12) & 0x07);
     lpDevice->parent = parent;

     KeGetDeviceBARs(hdr, lpDevice->bars);

     if (hdr->baseClass == 0x06 && hdr->subclass == 0x04)
     {
          const std::uint8_t secondaryBus = *reinterpret_cast<volatile std::uint8_t*>(
              base + (static_cast<std::uintptr_t>(bus) << 20) + (device << 15) + 0x19);
          debugging::DbgWrite(u8"      PCIe bridge to bus {}\r\n", secondaryBus);
          KiPCIEScanBus(base, segment, secondaryBus, 0, 31, lpDevice);
     }
}

void KiPCIEScanBus(std::uintptr_t base, std::uint16_t segment, std::uint8_t bus, std::uint8_t startDevice,
                   std::uint8_t endDevice, PCIDevice* parent)
{
     for (std::uint8_t device = startDevice; device <= endDevice; device++)
     {
          if (!KIIsDevicePresent(base, bus, device, 0)) continue;
          KiPCIEScanFunction(base, segment, bus, device, 0, parent);

          if (KiIsDeviceMultifunction(base, bus, device))
               for (std::uint8_t function = 1; function < 8; function++)
                    if (KIIsDevicePresent(base, bus, device, function))
                         KiPCIEScanFunction(base, segment, bus, device, function, parent);
     }
}

void KeInitialisePCIE()
{
     if (g_mcfg == nullptr)
     {
          debugging::DbgWrite(u8"MCFG table not found, PCI-e initialisation failed\r\n");
          return;
     }

     const std::size_t entries = (g_mcfg->header.length - sizeof(ACPISDTHeader)) / sizeof(MCFGAllocationClass);

     debugging::DbgWrite(u8"Scanning the PCI-e bus\r\n");
     for (std::size_t i = 0; i < entries; i++)
     {
          const auto& alloc = g_mcfg->allocationClasses[i];
          debugging::DbgWrite(u8"  MCFG Entry {}: base {}, segment {}, bus {}-{}\r\n", i,
                              reinterpret_cast<void*>(alloc.baseAddress), alloc.pciSegmentGroup, alloc.startBusNumber,
                              alloc.endBusNumber);

          const std::uintptr_t baseVA = alloc.baseAddress + 0xffff'8000'0000'0000; // kernel identity-mapped offset

          const std::size_t numBuses = alloc.endBusNumber - alloc.startBusNumber + 1;
          const std::size_t ecamSize = numBuses * 32 /*devices*/ * 8 /*functions*/ * 0x1000 /*4KB per function*/;
          const std::size_t pages = (ecamSize + 0xfff) / 0x1000;

          for (std::size_t p = 0; p < pages; p++)
          {
               const std::uintptr_t pageVA = baseVA + (p * 0x1000);
               auto* vadNode = g_kernelProcess->QueryAddress(pageVA);
               if (!vadNode)
               {
                    g_kernelProcess->ReserveMemoryFixed(pageVA, 0x1000, memory::MemoryProtection::ReadWrite,
                                                        memory::PFNUse::DriverLocked);
                    memory::paging::MapPage(g_kernelProcess->GetPageTableBase(),
                                            {.virtualAddress = pageVA,
                                             .physicalAddress = alloc.baseAddress + (p * 0x1000),
                                             .size = 0x1000,
                                             .writable = true,
                                             .userAccessible = false,
                                             .cacheDisable = false},
                                            [](std::size_t) -> void*
                                            {
                                                 std::uintptr_t page = memory::physicalAllocator.AllocatePage(
                                                     memory::PFNUse::DriverLocked);
                                                 if (page == ~0uz) return nullptr;
                                                 return reinterpret_cast<void*>(page + memory::virtualOffset);
                                            });
               }
          }

          const std::uintptr_t base = alloc.baseAddress + 0xffff'8000'0000'0000; // identity-mapped
          g_pcieBase = base;
          for (std::uint16_t bus = alloc.startBusNumber; bus <= alloc.endBusNumber; bus++)
               KiPCIEScanBus(base, alloc.pciSegmentGroup, static_cast<std::uint8_t>(bus), 0, 31, nullptr);
     }
}

constexpr std::uint32_t PciConfigAddress(std::uint8_t bus, std::uint8_t device, std::uint8_t func, std::uint8_t reg)
{
     return g_pcieBase | (static_cast<std::uint32_t>(bus) << 16) | (static_cast<std::uint32_t>(device) << 11) |
            (static_cast<std::uint32_t>(func) << 8) | (reg & 0xFC);
}

std::uint16_t KeReadPCIWord(std::uint32_t addr, std::uint8_t reg)
{
     operations::WritePort32(0xCF8, PciConfigAddress((addr >> 8) & 0xFF, (addr >> 3) & 0x1F, addr & 0x7, reg));
     std::uint32_t data = operations::ReadPort32(0xCFC);
     const std::uint8_t shift = (reg & 2) * 8;
     return static_cast<std::uint16_t>((data >> shift) & 0xFFFF);
}

void KeWritePCIWord(std::uint32_t addr, std::uint8_t reg, std::uint16_t value)
{
     operations::WritePort32(0xCF8, PciConfigAddress((addr >> 8) & 0xFF, (addr >> 3) & 0x1F, addr & 0x7, reg));
     std::uint32_t data = operations::ReadPort32(0xCFC);
     const std::uint8_t shift = (reg & 2) * 8;
     data &= ~(0xFFFFu << shift);
     data |= static_cast<std::uint32_t>(value) << shift;
     operations::WritePort32(0xCFC, data);
}
