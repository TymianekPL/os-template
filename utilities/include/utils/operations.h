#pragma once

#include <cstddef>
#include <cstdint>

namespace operations
{
     void DisableInterrupts();
     void EnableInterrupts();
     void Yield();
     std::uint64_t ReadCurrentCycles();
     void Halt();
     void WriteSerialCharacter(char value);
     char ReadSerialCharacter();
     template <int = 0> void WriteSerialString(const char* value)
     {
          while (value[0] != 0) WriteSerialCharacter(*value++);
     }
     template <int = 0> void WriteSerialString(const char* value, std::size_t nCharacters)
     {
          for (std::size_t i = 0; i < nCharacters; i++) WriteSerialCharacter(value[i]);
     }
     template <int = 0> void WriteSerialString(const char8_t* value, std::size_t nCharacters)
     {
          for (std::size_t i = 0; i < nCharacters; i++) WriteSerialCharacter(static_cast<char>(value[i]));
     }
     template <std::size_t N> void WriteSerialString(const char8_t (&string)[N]) // NOLINT
     {
          for (std::size_t i = 0; i < N; i++) WriteSerialCharacter(static_cast<char>(string[i]));
     }
} // namespace operations
