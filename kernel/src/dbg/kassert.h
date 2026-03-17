#pragma once

namespace debugging
{
     [[noreturn]] void KAssertFail(const char8_t* expression, const char8_t* message, const char8_t* file, int line);

     void Assert(bool condition, const char8_t* expression, const char8_t* message, const char8_t* file, int line);
#define KASSERT_U8(x) u8##x
#define KASSERT_FILE(x) KASSERT_U8(x)
#define KASSERT_LINEIMP(x) KASSERT_U8(#x)
#define KASSERT_LINE(x) KASSERT_LINEIMP(x)

#define KASSERT(cond, message) debugging::Assert((cond), u8## #cond, message, KASSERT_FILE(__FILE__), __LINE__)
} // namespace debugging
