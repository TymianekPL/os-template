#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

#include <utils/identify.h>

#ifdef COMPILER_MSVC
#pragma pack(push, 1)
#define PACK
#elifdef COMPILER_CLANG
#define PACK __attribute__((packed))
#endif

struct DosHeader
{
     std::uint16_t eMagic;    // Magic number (0x5A4D)
     std::uint16_t eCblp;     // Bytes on last page of file
     std::uint16_t eCp;       // Pages in file
     std::uint16_t eCrlc;     // Relocations
     std::uint16_t eCparhdr;  // Limit of header in paragraphs
     std::uint16_t eMinalloc; // Minimum extra paragraphs needed
     std::uint16_t eMaxalloc; // Maximum extra paragraphs needed
     std::uint16_t eSs;       // Initial (relative) SS
     std::uint16_t eSp;       // Initial SP
     std::uint16_t eCsum;     // Checksum
     std::uint16_t eIp;       // Initial IP
     std::uint16_t eCs;       // Initial (relative) CS
     std::uint16_t eLfarlc;   // File address of relocation table
     std::uint16_t eOvno;     // Overlay number
     std::uint16_t eRes[4];   // Reserved words NOLINT
     std::uint16_t eOemid;    // OEM identifier (for eOeminfo)
     std::uint16_t eOeminfo;  // OEM information; eOemid specific
     std::uint16_t eRes2[10]; // Reserved words NOLINT
     std::uint32_t eLfanew;   // File address of new exe header
} PACK;

// COFF File Header
struct CoffFileHeader
{
     std::uint16_t machine;              // Machine type
     std::uint16_t numberOfSections;     // Number of sections
     std::uint32_t timeDateStamp;        // Timestamp
     std::uint32_t pointerToSymbolTable; // Pointer to symbol table
     std::uint32_t numberOfSymbols;      // Number of symbols
     std::uint16_t sizeOfOptionalHeader; // Limit of the optional header
     std::uint16_t characteristics;      // File characteristics
} PACK;

struct DataDirectory
{
     std::uint32_t virtualAddress;
     std::uint32_t size;
} PACK;

// Optional Header (Image)
struct OptionalHeader
{
     std::uint16_t magic;                       // Magic number (0x010B for PE32, 0x020B for PE32+)
     std::uint8_t majorLinkerVersion;           // Linker version
     std::uint8_t minorLinkerVersion;           // Linker version
     std::uint32_t sizeOfCode;                  // Limit of code section
     std::uint32_t sizeOfInitialisedData;       // Limit of initialized data section
     std::uint32_t sizeOfUninitialisedData;     // Limit of uninitialized data section
     std::uint32_t addressOfEntryPoint;         // Entry point address
     std::uint32_t baseOfCode;                  // Base address of code section
     std::uint64_t imageBase;                   // Preferred base address
     std::uint32_t sectionAlignment;            // Section alignment
     std::uint32_t fileAlignment;               // File alignment
     std::uint16_t majorOperatingSystemVersion; // OS version
     std::uint16_t minorOperatingSystemVersion; // OS version
     std::uint16_t majorImageVersion;           // Image version
     std::uint16_t minorImageVersion;           // Image version
     std::uint16_t majorSubsystemVersion;       // Subsystem version
     std::uint16_t minorSubsystemVersion;       // Subsystem version
     std::uint32_t win32VersionValue;           // Reserved
     std::uint32_t sizeOfImage;                 // Limit of the image
     std::uint32_t sizeOfHeaders;               // Limit of headers
     std::uint32_t checkSum;                    // Checksum
     std::uint16_t subsystem;                   // Subsystem
     std::uint16_t dllCharacteristics;          // DLL characteristics
     std::uint64_t sizeOfStackReserve;          // Limit of stack reserve
     std::uint64_t sizeOfStackCommit;           // Limit of stack commit
     std::uint64_t sizeOfHeapReserve;           // Limit of heap reserve
     std::uint64_t sizeOfHeapCommit;            // Limit of heap commit
     std::uint32_t loaderFlags;                 // Loader flags
     std::uint32_t numberOfRvaAndSizes;         // Number of RVA and sizes
     // Optional: Data Directory (up to 16 entries)
     std::array<DataDirectory, 16> dataDirectory;
} PACK;

// NT Headers
struct NtHeaders
{
     std::uint32_t signature;       // PE signature ("PE\0\0")
     CoffFileHeader fileHeader;     // COFF file header
     OptionalHeader optionalHeader; // Optional header
} PACK;

struct SectionHeader
{
     std::array<char, 8> name;
     std::uint32_t virtualSize;
     std::uint32_t virtualAddress; // RVA of the section
     std::uint32_t sizeOfRawData;
     std::uint32_t pointerToRawData;
     std::uint32_t pointerToRelocations;
     std::uint32_t pointerToLinenumbers;
     std::uint16_t numberOfRelocations;
     std::uint16_t numberOfLinenumbers;
     std::uint32_t characteristics;
} PACK;

struct ImageExportDirectory
{
     std::uint32_t characteristics;
     std::uint32_t timeDateStamp;
     std::uint16_t majorVersion;
     std::uint16_t minorVersion;
     std::uint32_t name;                  // RVA of the DLL name (char*)
     std::uint32_t base;                  // Starting ordinal number
     std::uint32_t numberOfFunctions;     // Total functions in export table
     std::uint32_t numberOfNames;         // Number of named functions
     std::uint32_t addressOfFunctions;    // RVA to std::uint32_t[] of function RVAs
     std::uint32_t addressOfNames;        // RVA to std::uint32_t[] of function name RVAs
     std::uint32_t addressOfNameOrdinals; // RVA to std::uint16_t[] of ordinals (indexes into AddressOfFunctions)
} PACK;

struct PEImage
{
     DosHeader dosHeader{};
     NtHeaders ntHeaders{};
     ImageExportDirectory* exportDirectory{};
     void* baseAddress{};
     void* entryPoint{};
} PACK;

struct ImageBaseRelocation
{
     std::uint32_t virtualAddress; // The RVA of the block to which the relocations apply
     std::uint32_t sizeOfBlock;    // Limit of the relocation block, including this header and relocations
} PACK;

struct ImportEntry
{
     const char* name;
     void* address;
} PACK;

struct ImageImportByName
{
     std::uint16_t hint; // Ordinal hint for the imported function
     char name[1];       // val NOLINT
} PACK;

struct ImageImportDescriptor
{
     std::uint32_t characteristics; // 0 for the first entry, otherwise the date/time stamp of the DLL
     std::uint32_t timeDateStamp;   // The date and time when the DLL was created
     std::uint32_t forwarderChain;  // Forwarder chain for the DLL
     std::uint32_t name;            // RVA to the name of the DLL
     std::uint32_t firstThunk;      // RVA to the first IAT (Import Address Table)
} PACK;
#ifdef COMPILER_MSVC
#pragma pack(pop)
#undef PACK
#elifdef COMPILER_CLANG
#undef PACK
#endif

constexpr std::size_t IMAGE_DIRECTORY_ENTRY_EXPORT = 0;
constexpr std::size_t IMAGE_DIRECTORY_ENTRY_IMPORT = 1;
constexpr std::size_t IMAGE_DIRECTORY_ENTRY_BASERELOC = 5;
constexpr std::uint16_t IMAGE_REL_BASED_ABSOLUTE = 0;
constexpr std::uint16_t IMAGE_REL_BASED_HIGHLOW = 3;
constexpr std::uint16_t IMAGE_REL_BASED_DIR64 = 10;

constexpr std::uint16_t IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE = 0x0040;
constexpr std::uintptr_t IMAGeORDINAL_FLAG = 0x80000000;

constexpr std::uint32_t KiExtractOrdinalNumber(std::uint32_t thunkData) { return thunkData & 0x7FFFFFFF; }
