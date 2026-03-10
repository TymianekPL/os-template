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

add_executable(Bootloader
    src/main.cpp
    src/FileLoader.cpp
    src/ImageLoader.cpp
    src/BootContext.cpp
    src/crtstub.cpp
)

target_include_directories(Bootloader PRIVATE ${UEFI_INCLUDES})
target_link_libraries(Bootloader PRIVATE utilities)

if(MSVC)
    # Remove runtime checks added by CMake in Debug mode
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCc" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCs" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MDd" "/MD" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MTd" "/MT" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    
    target_compile_options(Bootloader PRIVATE
        /GS-
        /Oi
        /GL-
        /Gy
        /GR-
        /EHs-c-
        /D_HAS_EXCEPTIONS=0
        /D_ITERATOR_DEBUG_LEVEL=0
        /D_SECURE_SCL=0
        /D_MSVC_STL_HARDENING=0
        /DUNICODE
        /D_UNICODE
    )
    
    target_link_options(Bootloader PRIVATE
        /SUBSYSTEM:EFI_APPLICATION
        /ENTRY:EfiMain
        /NODEFAULTLIB
        /DLL
        /MACHINE:${EFI_ARCH}
    )
    
    set_target_properties(Bootloader PROPERTIES
        OUTPUT_NAME ${EFI_IMAGE}
        SUFFIX ".efi"
        LINK_FLAGS "/SUBSYSTEM:EFI_APPLICATION"
    )
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Bootloader PRIVATE
        -ffreestanding
        -fno-stack-protector
        -fno-stack-check
        -fshort-wchar
        -fno-exceptions
        -fno-rtti
        -fno-asynchronous-unwind-tables
        -D_GLIBCXX_DEBUG=0
        -D_GLIBCXX_ASSERTIONS=0
        -D_LIBCPP_ENABLE_ASSERTIONS=0
        -D_MSVC_STL_HARDENING=0
        -D_HAS_EXCEPTIONS=0
        -D_ITERATOR_DEBUG_LEVEL=0
        -D_SECURE_SCL=0
        -DUNICODE
        -D_UNICODE
    )
    
    if(EFI_ARCH STREQUAL "X64")
        target_compile_options(Bootloader PRIVATE -m64 -mno-red-zone)
        target_link_options(Bootloader PRIVATE -m64)
    elseif(EFI_ARCH STREQUAL "IA32")
        target_compile_options(Bootloader PRIVATE -m32 -mno-red-zone)
        target_link_options(Bootloader PRIVATE -m32)
    elseif(EFI_ARCH STREQUAL "AA64")
        # ARM64 specific options
    endif()
    
    target_link_options(Bootloader PRIVATE
        -nostdlib
        -Wl,-dll
        -shared
        -Wl,/subsystem:EFI_APPLICATION
        -Wl,/ENTRY:EfiMain
    )
    
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_link_options(Bootloader PRIVATE -Xlinker /SAFESEH:NO)
    endif()

    
    set_target_properties(Bootloader PROPERTIES
        OUTPUT_NAME ${EFI_IMAGE}
        SUFFIX ".efi"
    )
endif()

set_target_properties(Bootloader PROPERTIES
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
