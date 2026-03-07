#pragma once

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>
#include "operations.h"

namespace debugging
{
     constexpr std::size_t MaxSerialLine = 256;

     template <typename T> struct SerialFormatter
     {
          static void Write(const T& unnamed)
          {
               static_assert(sizeof(unnamed) == 0, "SerialFormatter not implemented for this type");
          }
     };

     template <> struct SerialFormatter<char8_t>
     {
          static void Write(char8_t value) { operations::WriteSerialCharacter(static_cast<char>(value)); }
     };

     template <> struct SerialFormatter<char8_t*>
     {
          static void Write(const char8_t* value)
          {
               while (value[0] != 0) operations::WriteSerialCharacter(static_cast<char>(*value++));
          }
     };

     template <> struct SerialFormatter<const char8_t*>
     {
          static void Write(const char8_t* value)
          {
               while (value[0] != 0) operations::WriteSerialCharacter(static_cast<char>(*value++));
          }
     };

     template <> struct SerialFormatter<std::string_view>
     {
          static void Write(const std::string_view value) { operations::WriteSerialString(value.data(), value.size()); }
     };

     template <std::integral TInteger> struct SerialFormatter<TInteger>
     {
          static void Write(TInteger value)
          {
               std::array<char, 32> buffer{};
               const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
               operations::WriteSerialString(buffer.data(), result.ptr - buffer.data());
          }
     };

     template <> struct SerialFormatter<bool>
     {
          static void Write(bool value) { operations::WriteSerialString(value ? "true" : "false", value ? 4 : 5); }
     };

     template <typename T> struct SerialFormatter<T*>
     {
          static void Write(T* ptr)
          {
               std::array<char, 2 + (sizeof(std::uintptr_t) * 2) + 1> buffer{};
               auto* destination = buffer.data();
               *destination++ = '0';
               *destination++ = 'x';

               constexpr auto hexDigits = "0123456789ABCDEF";
               std::uintptr_t value = reinterpret_cast<std::uintptr_t>(ptr);

               for (int i = static_cast<int>((sizeof(std::uintptr_t) * 2) - 1); i >= 0; --i)
               {
                    const auto shift = i * 4;
                    *destination++ = hexDigits[(value >> shift) & 0xF];
               }
               *destination = 0;
               SerialFormatter<char8_t*>::Write(reinterpret_cast<char8_t*>(buffer.data()));
          }
     };

     template <> struct SerialFormatter<std::byte>
     {
          static void Write(std::byte value)
          {
               std::array<char, 5> buffer{};
               auto* destination = buffer.data();
               constexpr auto hexDigits = "0123456789ABCDEF";

               for (int i = 1; i >= 0; --i)
               {
                    const auto shift = i * 4;
                    *destination++ = hexDigits[(std::to_integer<int>(value) >> shift) & 0xF];
               }
               *destination = 0;
               SerialFormatter<char8_t*>::Write(reinterpret_cast<char8_t*>(buffer.data()));
          }
     };

     template <std::size_t N> struct SerialFormatter<char8_t[N]> // NOLINT
     {
          static void Write(const char8_t (&value)[N]) // NOLINT
          {
               operations::WriteSerialString(value, N - 1);
          }
     };

     template <typename... TArgs> void DbgWrite(const char8_t* format, TArgs&&... args)
     {
          const char8_t* ptr = format;
          std::tuple<TArgs...> tupleArgs(std::forward<TArgs>(args)...);
          std::size_t argIndex = 0;

          auto printArg = [&](std::size_t index)
          {
               std::apply(
                   [&]<typename... T>(T&&... unpackedArgs)
                   {
                        std::size_t i = 0;
                        ((i++ == index ? (void)SerialFormatter<typename std::remove_cvref_t<T>>::Write(
                                             std::forward<T>(unpackedArgs))
                                       : (void)0),
                         ...);
                   },
                   tupleArgs);
          };

          while (*ptr)
          {
               if (ptr[0] == '{' && ptr[1] == '}')
               {
                    printArg(argIndex++);
                    ptr += 2;
               }
               else
               {
                    SerialFormatter<char8_t>::Write(static_cast<char8_t>(*ptr++));
               }
          }
     }

     inline std::size_t ReadSerialLine(char* buffer, std::size_t maxLength)
     {
          std::size_t count = 0;
          while (count + 1 < maxLength)
          {
               char c = operations::ReadSerialCharacter();

               if (c == 127)
               {
                    if (count > 0)
                    {
                         count--;
                         operations::WriteSerialCharacter('\b');
                         operations::WriteSerialCharacter(' ');
                         operations::WriteSerialCharacter('\b');
                    }
                    continue;
               }
               if (c < 127) operations::WriteSerialCharacter(c);
               // else DbgWrite(u8"{}", int(c));

               if (c == '\n' || c == '\r')
               {
                    buffer[count] = '\0';
                    return count;
               }

               buffer[count++] = c;
          }

          buffer[count] = '\0';
          return count;
     }
} // namespace debugging
