@echo off

set "TYPE="
set "BUILD_DIR="

if "%2" == "debug" (
     set "TYPE=debug"
) else if "%2" == "release" (
     set "TYPE=release"
) else (
     echo Invalid preset
     exit /b 1
)

if "%1" == "build-msvc" (
     cmake --build --preset windows-x64-msvc-%TYPE%
     set BUILD_DIR=build\windows-x64-msvc-%TYPE%
) else if "%1" == "build-clang" (
     cmake --build --preset windows-x64-clang-%TYPE%
     set BUILD_DIR=build\windows-x64-clang-%TYPE%
) else if "%1" == "configure-msvc" (
     cmake --preset windows-x64-msvc-%TYPE%
     set BUILD_DIR=build\windows-x64-msvc-%TYPE%
     exit /b 0
) else if "%1" == "configure-clang" (
     cmake --preset windows-x64-clang-%TYPE%
     set BUILD_DIR=build\windows-x64-clang-%TYPE%
     exit /b 0
)
set "error=%errorlevel%"
if NOT %error% == 0 (
     echo Failed: %error%
     exit /b %error%
)

setlocal enabledelayedexpansion
if not exist "%BUILD_DIR%\BOOTX64.efi" (
     echo ERROR: BOOTX64.efi not found in build directory
     echo.
     pause
     exit /b 1
)

echo [1/4] Creating EFI boot directory structure...
if not exist "%BUILD_DIR%\guest_root\EFI\BOOT" mkdir "%BUILD_DIR%\guest_root\EFI\BOOT"

echo [2/4] Copying BOOTX64.efi to EFI\BOOT\BOOTX64.EFI...
copy /Y "%BUILD_DIR%\BOOTX64.efi" "%BUILD_DIR%\guest_root\EFI\BOOT\BOOTX64.EFI" >nul

echo [3/4] Checking for QEMU installation...
where qemu-system-x86_64.exe >nul 2>&1
if %errorlevel% neq 0 (
     echo qemu-system-x86_64 not found
     pause
     exit /b 1
)

echo [4/4] Locating OVMF firmware...
set "OVMF_PATH="

if exist "C:\Program Files\qemu\share\edk2-x86_64-code.fd" (
    set "OVMF_PATH=C:\Program Files\qemu\share\edk2-x86_64-code.fd"
) else if exist "C:\qemu\share\edk2-x86_64-code.fd" (
    set "OVMF_PATH=C:\qemu\share\edk2-x86_64-code.fd"
) else if exist "C:\Program Files\qemu\share\OVMF_CODE.fd" (
    set "OVMF_PATH=C:\Program Files\qemu\share\OVMF_CODE.fd"
) else if exist "C:\qemu\share\OVMF_CODE.fd" (
    set "OVMF_PATH=C:\qemu\share\OVMF_CODE.fd"
) else if exist "%~dp0OVMF_CODE.fd" (
    set "OVMF_PATH=%~dp0OVMF_CODE.fd"
)

if "%OVMF_PATH%"=="" (
     if exist "OVMF_CODE.fd" (
          set "OVMF_PATH=OVMF_CODE.fd"
          echo Found: OVMF_CODE.fd
     ) else (
          echo No OVMF found.
          pause
          exit /b 1
     )
)

echo OVMF: !OVMF_PATH!

qemu-system-x86_64.exe ^
     -drive if=pflash,format=raw,readonly=on,file="!OVMF_PATH!" ^
     -drive file=fat:rw:%BUILD_DIR%\guest_root,format=raw,media=disk ^
     -m 512M ^
     -net none ^
     -vga std ^
     -serial stdio

echo.
echo QEMU session ended.
pause
