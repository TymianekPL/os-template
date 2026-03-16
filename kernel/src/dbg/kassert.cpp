#include "kassert.h"
#include "utils/kdbg.h"

[[noreturn]] void debugging::KAssertFail(const char8_t* expression, const char8_t* message, const char8_t* file,
                                         int line)
{
     debugging::DbgWrite(u8"KASSERT failed: {} - {} at {}:{}\r\n", expression, message, file, line);

     __debugbreak();
     operations::DisableInterrupts();
     while (true) operations::Halt();
}

void debugging::Assert(bool condition, const char8_t* expression, const char8_t* message, const char8_t* file, int line)
{
     if (!condition) KAssertFail(expression, message, file, line);
}
