add_library(BootVideo SHARED src/main.cpp src/font.cpp)
target_include_directories(BootVideo PUBLIC include)
target_link_libraries(BootVideo PRIVATE utilitiesBOOT)

if(MSVC)
    # Remove runtime checks added by CMake in Debug mode
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCc" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCs" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/EHc" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "-D_DEBUG" "" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MDd" "/MD" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/MTd" "/MT" CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}")
    
    target_compile_options(BootVideo PRIVATE
        /GS-
        /Oi
        /GL-
        /Gy
        /GR-
        /EHs-c-
        /D_HAS_EXCEPTIONS=0
        /D_ITERATOR_DEBUG_LEVEL=0
        /D_SECURE_SCL=0
        /D_NO_DEBUG_HEAP=1
        /DUNICODE
        /D_UNICODE
        /D_HAS_EXCEPTIONS=0
        /D_MSVC_STL_HARDENING=0
    )
    
    target_link_options(BootVideo PRIVATE
        /SUBSYSTEM:NATIVE
        /NOENTRY
        /NODEFAULTLIB
        /DLL
    )
    
    set_target_properties(BootVideo PROPERTIES
        OUTPUT_NAME "BootVideo"
        SUFFIX ".dll"
        LINK_FLAGS "/SUBSYSTEM:NATIVE"
    )
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(BootVideo PRIVATE
        -fno-sanitize=all
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
        -D_ITERATOR_DEBUG_LEVEL=0
        -D_MSVC_STL_HARDENING=0
        -D_HAS_EXCEPTIONS=0
        -D_SECURE_SCL=0
        -DUNICODE
        -D_UNICODE
    )
    
    if(EFI_ARCH STREQUAL "X64")
        target_compile_options(BootVideo PRIVATE -m64 -mno-red-zone)
        target_link_options(BootVideo PRIVATE -m64)
    elseif(EFI_ARCH STREQUAL "IA32")
        target_compile_options(BootVideo PRIVATE -m32 -mno-red-zone)
        target_link_options(BootVideo PRIVATE -m32)
    elseif(EFI_ARCH STREQUAL "AA64")
        # ARM64 specific options
    endif()
    
    target_link_options(BootVideo PRIVATE
        -nostdlib
        -Wl,-dll
        -shared
        -Wl,/subsystem:NATIVE
        -Wl,/NOENTRY
        -Wl,/guard:no
        -Wl,/CETCOMPAT:NO
    )
    
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_link_options(BootVideo PRIVATE -Xlinker /SAFESEH:NO)
    endif()
endif()
