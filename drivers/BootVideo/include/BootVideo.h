#pragma once

#include <utils/identify.h>
#include <cstdint>

struct VdiFrameBuffer
{
     std::uint32_t* framebuffer{};
     std::uint32_t width{};
     std::uint32_t height{};
     std::uint32_t scalineSize{};
     std::uint32_t* optionalBackbuffer{};
};

#ifndef DLL
#define DLL DLLIMPORT
#endif

DLL void VidInitialise(VdiFrameBuffer buffer);
DLL void VidClearScreen(std::uint32_t colour);
DLL void VidExchangeBuffers();
DLL void VidSetPixel(std::uint32_t x, std::uint32_t y, std::uint32_t colour);
DLL void VidDrawRect(std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t colour);
DLL void VidDrawChar(std::uint32_t x, std::uint32_t y, char c, std::uint32_t colour, std::uint8_t scale);
DLL void VidGetDimensions(std::uint32_t& width, std::uint32_t& height);
DLL void VidSetCursorPosition(std::uint32_t x, std::uint32_t y);

#undef DLL
