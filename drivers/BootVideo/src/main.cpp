#include <utils/identify.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#define DLL DLLEXPORT

#include <BootVideo.h>

#if defined(ARCH_X8664)
#include <immintrin.h>
#if defined(COMPILER_MSVC)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#elifdef ARCH_ARM64
#include <arm_neon.h>
#endif

struct VdiFrameBufferInternal
{
     std::uint32_t* framebuffer{};
     std::uint32_t width{};
     std::uint32_t height{};
     std::uint32_t size{};
     std::uint32_t scalineSize{};
     std::uint32_t* optionalBackbuffer{};
};

static VdiFrameBufferInternal g_buffer;

#if defined(ARCH_X8664)

enum class X86SimdLevel : std::uint8_t
{
     SSE2 = 1,
     AVX2 = 2
};

static X86SimdLevel DetectX86Simd() noexcept
{
#ifdef COMPILER_MSVC
     int info[4]{};
     __cpuid(info, 1);
     if (!(info[2] & (1 << 27))) // OSXSAVE absent -> no AVX
          return X86SimdLevel::SSE2;

     if ((_xgetbv(0) & 0x6u) != 0x6u) // OS saves XMM (bit1) + YMM (bit2)
          return X86SimdLevel::SSE2;

     __cpuidex(info, 7, 0);
     if (info[1] & (1 << 5)) return X86SimdLevel::AVX2;

     return X86SimdLevel::SSE2;

#else // COMPILER_CLANG / GCC
     unsigned eax{};
     unsigned ebx{};
     unsigned ecx{};
     unsigned edx{};

     if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) return X86SimdLevel::SSE2;

     if (!(ecx & (1u << 27))) return X86SimdLevel::SSE2;

     std::uint32_t xcr0Lo{};
     std::uint32_t xcr0Hi{};
#ifdef COMPILER_CLANG
     asm volatile("xgetbv" : "=a"(xcr0Lo), "=d"(xcr0Hi) : "c"(0u));
#else
     xcr0_lo = static_cast<std::uint32_t>(_xgetbv(0));
#endif
     if ((xcr0Lo & 0x6u) != 0x6u) return X86SimdLevel::SSE2;

     if (!__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) return X86SimdLevel::SSE2;

     if (ebx & (1u << 5)) return X86SimdLevel::AVX2;

     return X86SimdLevel::SSE2;
#endif
}

static const X86SimdLevel g_x86simd = DetectX86Simd();

static constexpr std::size_t kPrefetchAhead = 256;

static void CopyAVX2(const void* __restrict src, void* __restrict dst, std::size_t byteCount) noexcept
{
     const auto* s = static_cast<const std::uint8_t*>(src);
     auto* d = static_cast<std::uint8_t*>(dst);

     const std::size_t vec64 = byteCount / 64;
     const std::size_t tail = byteCount % 64;

#if defined(COMPILER_CLANG)
     for (std::size_t i = 0; i < vec64; ++i, s += 64, d += 64)
     {
          asm volatile("prefetchnta %[pfr]            \n\t"
                       "vmovdqu   (%[s]),    %%ymm0   \n\t"
                       "vmovdqu 32(%[s]),    %%ymm1   \n\t"
                       "vmovntdq  %%ymm0,   (%[d])    \n\t"
                       "vmovntdq  %%ymm1, 32(%[d])    \n\t"
                       :
                       : [s] "r"(s), [d] "r"(d), [pfr] "m"(*(s + kPrefetchAhead))
                       : "memory", "ymm0", "ymm1");
     }
     asm volatile("sfence" ::: "memory");
     asm volatile("vzeroupper" ::: "memory");
#else
     for (std::size_t i = 0; i < vec64; ++i, s += 64, d += 64)
     {
          _mm_prefetch(reinterpret_cast<const char*>(s + kPrefetchAhead), _MM_HINT_NTA);
          const __m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s));
          const __m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + 32));
          _mm256_stream_si256(reinterpret_cast<__m256i*>(d), v0);
          _mm256_stream_si256(reinterpret_cast<__m256i*>(d + 32), v1);
     }
     _mm_sfence();
     _mm256_zeroupper();
#endif

     if (tail) std::memcpy(d, s, tail);
}

static void CopySSE2(const void* __restrict src, void* __restrict dst, std::size_t byteCount) noexcept
{
     const auto* s = static_cast<const std::uint8_t*>(src);
     auto* d = static_cast<std::uint8_t*>(dst);

     const std::size_t vec32 = byteCount / 32;
     const std::size_t tail = byteCount % 32;

#if defined(COMPILER_CLANG)
     for (std::size_t i = 0; i < vec32; ++i, s += 32, d += 32)
     {
          asm volatile("prefetchnta %[pfr]            \n\t"
                       "movdqu   (%[s]),   %%xmm0     \n\t"
                       "movdqu 16(%[s]),   %%xmm1     \n\t"
                       "movntdq  %%xmm0,  (%[d])      \n\t"
                       "movntdq  %%xmm1, 16(%[d])     \n\t"
                       :
                       : [s] "r"(s), [d] "r"(d), [pfr] "m"(*(s + kPrefetchAhead))
                       : "memory", "xmm0", "xmm1");
     }
     asm volatile("sfence" ::: "memory");
#else
     for (std::size_t i = 0; i < vec32; ++i, s += 32, d += 32)
     {
          _mm_prefetch(reinterpret_cast<const char*>(s + kPrefetchAhead), _MM_HINT_NTA);
          const __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s));
          const __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + 16));
          _mm_stream_si128(reinterpret_cast<__m128i*>(d), v0);
          _mm_stream_si128(reinterpret_cast<__m128i*>(d + 16), v1);
     }
     _mm_sfence();
#endif

     if (tail) std::memcpy(d, s, tail);
}

#elifdef ARCH_ARM64 // ^^^ ARCH_X8664 / ARCH_ARM64 vvv

static constexpr std::size_t kArmPrefetchAhead = 256;

static void CopyNEON(const void* __restrict src, void* __restrict dst, std::size_t byteCount) noexcept
{
     const auto* s = static_cast<const std::uint8_t*>(src);
     auto* d = static_cast<std::uint8_t*>(dst);

     const std::size_t vec64 = byteCount / 64;
     const std::size_t tail = byteCount % 64;

#if defined(COMPILER_CLANG)
     for (std::size_t i = 0; i < vec64; ++i, s += 64, d += 64)
     {
          asm volatile("prfm  pldl1strm, [%[s], %[pf]] \n\t"
                       "ldp   q0, q1,   [%[s]]         \n\t"
                       "ldp   q2, q3,   [%[s], #32]    \n\t"
                       "stnp  q0, q1,   [%[d]]         \n\t"
                       "stnp  q2, q3,   [%[d], #32]    \n\t"
                       :
                       : [s] "r"(s), [d] "r"(d), [pf] "r"(static_cast<std::ptrdiff_t>(kArmPrefetchAhead))
                       : "memory", "v0", "v1", "v2", "v3");
     }

     asm volatile("dsb st" ::: "memory");
#else
     for (std::size_t i = 0; i < vec64; ++i, s += 64, d += 64)
     {
          __prefetch2(s + kArmPrefetchAhead, /*locality=*/0);
          const uint8x16x4_t v = vld1q_u8_x4(s);
          vst1q_u8(d, v.val[0]);
          vst1q_u8(d + 16, v.val[1]);
          vst1q_u8(d + 32, v.val[2]);
          vst1q_u8(d + 48, v.val[3]);
     }

     __dsb(_ARM64_BARRIER_ISHST);
#endif

     if (tail) std::memcpy(d, s, tail);
}

#endif // ^^^ ARCH_ARM64

static void SimdMemcpyToVRAM(const void* __restrict src, void* __restrict dst, std::size_t byteCount) noexcept
{
#if defined(ARCH_X8664)
     if (g_x86simd >= X86SimdLevel::AVX2) CopyAVX2(src, dst, byteCount);
     else
          CopySSE2(src, dst, byteCount);

#elif defined(ARCH_ARM64)
     CopyNEON(src, dst, byteCount);

#else
     std::memcpy(dst, src, byteCount);
#endif
}

DLLEXPORT void VidInitialise(VdiFrameBuffer buffer)
{
     g_buffer.framebuffer = buffer.optionalBackbuffer == nullptr ? buffer.framebuffer : buffer.optionalBackbuffer;
     g_buffer.width = buffer.width;
     g_buffer.height = buffer.height;
     g_buffer.scalineSize = buffer.scalineSize;
     g_buffer.size = g_buffer.scalineSize * g_buffer.height;
     g_buffer.optionalBackbuffer = buffer.optionalBackbuffer == nullptr ? nullptr : buffer.framebuffer;

     VidClearScreen(0x10101a);
}

DLLEXPORT void VidClearScreen(std::uint32_t colour)
{
     if (g_buffer.framebuffer == nullptr) return;

     const std::size_t stride = g_buffer.scalineSize;
     const std::size_t height = g_buffer.height;

     for (std::size_t y = 0; y < height; ++y)
     {
          auto* row = g_buffer.framebuffer + (y * stride);
          std::ranges::fill(row, row + g_buffer.width, colour);
     }
}
DLLEXPORT void VidDrawRect(std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height,
                           std::uint32_t colour)
{
     for (std::uint32_t j = 0; j < height; ++j)
     {
          auto* row = g_buffer.framebuffer + (static_cast<std::size_t>((y + j) * g_buffer.scalineSize)) + x;
          std::ranges::fill(row, row + width, colour);
     }
}

extern "C" const std::uint8_t kFontData[256][16]; // NOLINT

DLLEXPORT void VidDrawChar(std::uint32_t x, std::uint32_t y, char c, std::uint32_t colour, std::uint8_t scale)
{
     const auto& glyph = kFontData[static_cast<std::uint8_t>(c)];

     for (std::uint32_t j = 0; j < 16; ++j)
     {
          std::uint32_t* rowStart =
              g_buffer.framebuffer + (static_cast<std::size_t>(y + (j * scale)) * g_buffer.scalineSize) + x;

          for (std::uint32_t sy = 0; sy < scale; ++sy)
          {
               std::uint32_t* lpPixel = rowStart + static_cast<std::size_t>(sy * g_buffer.scalineSize);

               if (glyph[j] & 0x80) std::ranges::fill(lpPixel, lpPixel + scale, colour);
               lpPixel += scale;
               if (glyph[j] & 0x40) std::ranges::fill(lpPixel, lpPixel + scale, colour);
               lpPixel += scale;
               if (glyph[j] & 0x20) std::ranges::fill(lpPixel, lpPixel + scale, colour);
               lpPixel += scale;
               if (glyph[j] & 0x10) std::ranges::fill(lpPixel, lpPixel + scale, colour);
               lpPixel += scale;
               if (glyph[j] & 0x08) std::ranges::fill(lpPixel, lpPixel + scale, colour);
               lpPixel += scale;
               if (glyph[j] & 0x04) std::ranges::fill(lpPixel, lpPixel + scale, colour);
               lpPixel += scale;
               if (glyph[j] & 0x02) std::ranges::fill(lpPixel, lpPixel + scale, colour);
               lpPixel += scale;
               if (glyph[j] & 0x01) std::ranges::fill(lpPixel, lpPixel + scale, colour);
          }
     }
}

DLLEXPORT void VidExchangeBuffers()
{
     if (g_buffer.optionalBackbuffer == nullptr) return;

     SimdMemcpyToVRAM(g_buffer.framebuffer, g_buffer.optionalBackbuffer,
                      static_cast<std::size_t>(g_buffer.size) * sizeof(std::uint32_t));
}

DLLEXPORT void VidSetPixel(std::uint32_t x, std::uint32_t y, std::uint32_t colour)
{
     if (g_buffer.framebuffer == nullptr) return;

     g_buffer.framebuffer[(y * g_buffer.scalineSize) + x] = colour;
}
