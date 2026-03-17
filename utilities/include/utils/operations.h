#pragma once

#include <cstddef>
#include <cstdint>

#if defined(__clang__) || defined(__GNUC__)
#define NO_ASAN __attribute__((no_sanitize("address")))
#else
#define NO_ASAN
#endif

namespace operations
{
     void DisableInterrupts();
     void EnableInterrupts();
     void Yield();
     std::uint64_t ReadCurrentCycles();
     void Halt();

     void InitialiseSerial();
     void WriteSerialCharacter(char value);
     char ReadSerialCharacter();
     char TryReadSerialCharacter();
     void SerialPushCharacter(char c);
     void SerialHoldLineHigh();

     std::uint8_t ReadPort8(std::uint16_t port);
     std::uint16_t ReadPort16(std::uint16_t port);
     std::uint32_t ReadPort32(std::uint16_t port);

     void WritePort8(std::uint16_t port, std::uint8_t value);
     void WritePort16(std::uint16_t port, std::uint16_t value);
     void WritePort32(std::uint16_t port, std::uint32_t value);

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
