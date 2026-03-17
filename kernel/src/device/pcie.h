#pragma once

#include <cstddef>
#include "device.h"
#include "utils/struct.h"
#pragma once
#include <cstdint>

#pragma pack(push, 1)

struct PCIHeader
{
     std::uint16_t vendorId;
     std::uint16_t deviceId;
     std::uint16_t command;
     std::uint16_t status;
     std::uint8_t revisionId;
     std::uint8_t progIf;
     std::uint8_t subclass;
     std::uint8_t baseClass;
     std::uint8_t cacheLineSize;
     std::uint8_t latencyTimer;
     std::uint8_t headerType;
     std::uint8_t bist;
     std::uint32_t bar0;
     std::uint32_t bar1;
     std::uint32_t bar2;
     std::uint32_t bar3;
     std::uint32_t bar4;
     std::uint32_t bar5;
     std::uint32_t cardbusCisPtr;
     std::uint16_t subsystemVendorId;
     std::uint16_t subsystemId;
     std::uint32_t expansionRomBar;
     std::uint8_t capabilitiesPtr;
     std::uint8_t reserved1[3]; // NOLINT
     std::uint32_t reserved2;
     std::uint8_t interruptLine;
     std::uint8_t interruptPin;
     std::uint8_t minGrant;
     std::uint8_t maxLatency;
};

struct XhciCap
{
     std::uint8_t capLength;
     std::uint8_t reserved;
     std::uint16_t hcVersion;
     std::uint32_t hcsParams1;
     std::uint32_t hcsParams2;
};

struct EhciCap
{
     std::uint8_t capLength;
     std::uint8_t reserved;
     std::uint16_t hcVersion;
     std::uint32_t hcsParams;
};

#pragma pack(pop)

struct PciBar
{
     std::uint32_t value{};
     bool isMmio{};
     bool is64bit{};
     std::uint64_t address{};
     bool isPrefetchable{};
};

struct PCICapability
{
     std::uint8_t id;
     std::uint8_t next;
     std::uintptr_t address;
     std::size_t size;
};

struct PCIDevice : device::Device
{
     explicit PCIDevice() : Device{.type = device::DeviceType::PCI} {}

     std::uint16_t vendorId{};
     std::uint16_t deviceId{};
     std::uint8_t baseClass{};
     std::uint8_t subClass{};
     std::uint8_t progIf{};
     PciBar bars[6]; // NOLINT

     volatile PCIHeader* header{};
     std::uint16_t segment{};
     std::uint8_t bus{};
     std::uint8_t device{};
     std::uint8_t function{};

     structures::LinkedList<PCICapability> capabilities{};

     void* driverData{};
};

void KeInitialisePCIE();
void KeGetDeviceBARs(volatile PCIHeader* hdr, PciBar (&bars)[6]);                                    // NOLINT
std::uintptr_t KeFindControllerBAR(volatile PCIHeader* hdr, std::uint8_t progIf, PciBar (&bars)[6]); // NOLINT
void KeWritePCIWord(std::uint32_t addr, std::uint8_t reg, std::uint16_t value);
std::uint16_t KeReadPCIWord(std::uint32_t addr, std::uint8_t reg);
