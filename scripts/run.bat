@echo off
if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
     call scripts\runx8664 %*
) else if "%PROCESSOR_ARCHITECTURE%" == "x86" (
     call scripts\runx8632 %*
) else if "%PROCESSOR_ARCHITECTURE%" == "ARM64" (
     call scripts\runarm64 %*
) else (
     echo Invalid architecture
     exit /b 1
)
