cmake_minimum_required(VERSION 3.20)

if(DEFINED CACHE{CMAKE_SYSTEM_PROCESSOR})
    set(TARGET_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
endif()

project(UEFIHelloWorld CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEFINED TARGET_PROCESSOR)
    set(DETECTED_PROCESSOR ${TARGET_PROCESSOR})
else()
    set(DETECTED_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(DETECTED_PROCESSOR MATCHES "ARM64|aarch64|AARCH64")
    set(EFI_ARCH "AArch64")
    set(EFI_IMAGE "BOOTAA64")
    set(EFI_MACHINE_TYPE "0xaa64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(EFI_ARCH "X64")
    set(EFI_IMAGE "BOOTX64")
    set(EFI_MACHINE_TYPE "0x8664")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(EFI_ARCH "IA32")
    set(EFI_MACHINE_TYPE "0x14c")
    set(EFI_IMAGE "BOOTIA32")
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/_deps" CACHE PATH
    "Base directory for FetchContent")

include(FetchContent)
FetchContent_Declare(
    edk2
    GIT_REPOSITORY https://github.com/tianocore/edk2.git
    GIT_TAG edk2-stable202411
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
    SOURCE_SUBDIR "NONE"
)

message(STATUS "Fetching EDK2...")
FetchContent_MakeAvailable(edk2)

if(EXISTS ${edk2_SOURCE_DIR}/.git)
    file(WRITE ${edk2_SOURCE_DIR}/.git/info/sparse-checkout "MdePkg/Include/\n")
    execute_process(
        COMMAND git config core.sparseCheckout true
        WORKING_DIRECTORY ${edk2_SOURCE_DIR}
        OUTPUT_QUIET ERROR_QUIET
    )
    execute_process(
        COMMAND git read-tree -mu HEAD
        WORKING_DIRECTORY ${edk2_SOURCE_DIR}
        OUTPUT_QUIET ERROR_QUIET
    )
endif()

set(EDK2_PATH ${edk2_SOURCE_DIR})

set(UEFI_INCLUDES
    ${EDK2_PATH}/MdePkg/Include
    ${EDK2_PATH}/MdePkg/Include/${EFI_ARCH}
)

add_executable(HelloWorld
    src/main.cpp
)

target_include_directories(HelloWorld PRIVATE ${UEFI_INCLUDES})

if(MSVC)
    # Remove runtime checks added by CMake in Debug mode
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCc" "" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCs" "" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    
    target_compile_options(HelloWorld PRIVATE
        /GS-
        /Oi
        /GL-
        /Gy
        /DUNICODE
        /D_UNICODE
        "/DEFIAPI=__cdecl"
    )
    
    target_link_options(HelloWorld PRIVATE
        /SUBSYSTEM:EFI_APPLICATION
        /ENTRY:EfiMain
        /NODEFAULTLIB
        /DLL
        /MACHINE:${EFI_ARCH}
    )
    
    set_target_properties(HelloWorld PROPERTIES
        OUTPUT_NAME ${EFI_IMAGE}
        SUFFIX ".efi"
        LINK_FLAGS "/SUBSYSTEM:EFI_APPLICATION"
    )
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(HelloWorld PRIVATE
        -ffreestanding
        -fno-stack-protector
        -fno-stack-check
        -fshort-wchar
        -DUNICODE
        -D_UNICODE
    )
    
    if(EFI_ARCH STREQUAL "X64")
        target_compile_options(HelloWorld PRIVATE -m64 -mno-red-zone)
        target_link_options(HelloWorld PRIVATE -m64)
    elseif(EFI_ARCH STREQUAL "IA32")
        target_compile_options(HelloWorld PRIVATE -m32 -mno-red-zone)
        target_link_options(HelloWorld PRIVATE -m32)
    elseif(EFI_ARCH STREQUAL "AA64")
        # ARM64 specific options
    endif()
    
    target_link_options(HelloWorld PRIVATE
        -nostdlib
        -Wl,-dll
        -shared
        -Wl,/subsystem:EFI_APPLICATION
        -Wl,/ENTRY:EfiMain
    )
    
    set_target_properties(HelloWorld PROPERTIES
        OUTPUT_NAME ${EFI_IMAGE}
        SUFFIX ".efi"
    )
endif()

set_target_properties(HelloWorld PROPERTIES
    POSITION_INDEPENDENT_CODE OFF
)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    RESULT_VARIABLE _symlink_result
    ERROR_QUIET
)

message(STATUS "EDK2 Path: ${EDK2_PATH}")
message(STATUS "Architecture: ${EFI_ARCH}")
message(STATUS "Building UEFI Application: ${EFI_IMAGE}.efi")
