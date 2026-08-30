// Shared drawing layer for the three MuffinEMU showcase roms.
//
// WHY EVERYTHING HERE IS SOFTWARE, AND WHY THAT IS NOT A COMPROMISE
//
// These roms render on the CPU and present through OSScreen. Not GX2. That is a
// deliberate choice made from evidence, not a limitation we failed to work around:
// as of 2026-08-30 no homebrew on this port has ever rendered a GX2 frame, retail
// titles reach GX2Init and die inside the first textured draw, and on an A12Z the
// Metal backend silently drops every RECTS and geometry-shader draw for want of mesh
// shaders. A "showcase" built on GX2 today would showcase a black screen.
//
// OSScreen is the path that provably works - it is what rainbow.rpx booted through -
// and a software rasteriser on top of it is honest about what the emulator can do
// right now while still being a real 3D renderer.
//
// THE PERFORMANCE KNOB
//
// OSScreenPutPixelEx is an HLE call per pixel: PPC code calling out to the emulator.
// So cost scales with pixels WRITTEN, not with scene complexity. Everything renders
// into a small internal buffer and is blitted as square blocks, which makes
// PIXEL_SCALE the single dial that trades sharpness for frame rate. At 6 the image is
// 960x540 inside a letterbox, which is about 518k writes a frame - comfortable on the
// interpreter now that it clocks real MIPS, and the letterbox reads as intentional.
//
// Fixed point throughout, 16.16. The Wii U has an FPU and Cemu implements it, but
// integer paths are the best-tested part of any interpreter and this costs nothing.

#pragma once

#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#define TV_WIDTH   1280
#define TV_HEIGHT  720

// Internal render target. 160x90 is 16:9 and small enough that a full software
// rasterise costs far less than the blit that follows it.
#define RW 160
#define RH 90
#define PIXEL_SCALE 6
#define BLIT_W (RW * PIXEL_SCALE)
#define BLIT_H (RH * PIXEL_SCALE)
#define BLIT_X ((TV_WIDTH  - BLIT_W) / 2)
#define BLIT_Y ((TV_HEIGHT - BLIT_H) / 2)

typedef int32_t fx;                       // 16.16 fixed point
#define FX_ONE   (1 << 16)
#define FX(n)    ((fx)((n) * FX_ONE))
#define fxmul(a, b) ((fx)(((int64_t)(a) * (int64_t)(b)) >> 16))
#define fxdiv(a, b) ((fx)((((int64_t)(a)) << 16) / (b)))

// OSScreen framebuffers are RGBX8888: R,G,B,padding. The low byte is not alpha and
// nothing reads it; 0xFF is the conventional filler.
static inline uint32_t rgb(int r, int g, int b)
{
   if (r < 0) r = 0; if (r > 255) r = 255;
   if (g < 0) g = 0; if (g > 255) g = 255;
   if (b < 0) b = 0; if (b > 255) b = 255;
   return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | 0xFFu;
}

static inline uint32_t shade(uint32_t c, int numerator, int denominator)
{
   return rgb((int)(((c >> 24) & 0xFF) * numerator / denominator),
              (int)(((c >> 16) & 0xFF) * numerator / denominator),
              (int)(((c >>  8) & 0xFF) * numerator / denominator));
}

static inline uint32_t mixc(uint32_t a, uint32_t b, int t /*0..256*/)
{
   int ia = 256 - t;
   return rgb((int)((((a >> 24) & 0xFF) * ia + ((b >> 24) & 0xFF) * t) >> 8),
              (int)((((a >> 16) & 0xFF) * ia + ((b >> 16) & 0xFF) * t) >> 8),
              (int)((((a >>  8) & 0xFF) * ia + ((b >>  8) & 0xFF) * t) >> 8));
}

// The framebuffer these roms actually draw into. Static rather than malloc'd: it is
// 57 KB, it lives for the whole run, and a rom whose job is to prove the emulator
// works should not be able to fail on an allocator.
static uint32_t g_fb[RW * RH];
static int32_t  g_depth[RW * RH];

static inline void fb_clear(uint32_t colour)
{
   for (int i = 0; i < RW * RH; i++) { g_fb[i] = colour; g_depth[i] = 0x7FFFFFFF; }
}

static inline void fb_put(int x, int y, uint32_t colour)
{
   if ((unsigned)x < (unsigned)RW && (unsigned)y < (unsigned)RH)
      g_fb[y * RW + x] = colour;
}

// Vertical gradient background, one write per internal pixel. Cheap, and it is what
// stops every scene reading as "object floating on flat colour".
static inline void fb_gradient(uint32_t top, uint32_t bottom)
{
   for (int y = 0; y < RH; y++)
   {
      uint32_t row = mixc(top, bottom, y * 256 / RH);
      for (int x = 0; x < RW; x++) { g_fb[y * RW + x] = row; g_depth[y * RW + x] = 0x7FFFFFFF; }
   }
}

// Blit the internal buffer to a screen as PIXEL_SCALE-square blocks.
//
// This is the whole per-frame cost. Written as a tight double loop with the colour
// hoisted out of the inner block so the per-pixel work is one HLE call and nothing
// else - no recomputation, no bounds maths the caller already did.
static inline void fb_present(OSScreenID screen)
{
   for (int y = 0; y < RH; y++)
   {
      int sy = BLIT_Y + y * PIXEL_SCALE;
      for (int x = 0; x < RW; x++)
      {
         uint32_t c = g_fb[y * RW + x];
         int sx = BLIT_X + x * PIXEL_SCALE;
         for (int by = 0; by < PIXEL_SCALE; by++)
            for (int bx = 0; bx < PIXEL_SCALE; bx++)
               OSScreenPutPixelEx(screen, sx + bx, sy + by, c);
      }
   }
}

// 5x7 font covering A-Z, 0-9 and a few marks. Enough for a title card.
#define GW 5
#define GH 7
static const unsigned char kFont[39][GH] = {
 {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
 {0x1E,0x11,0x1E,0x11,0x11,0x11,0x1E}, // B
 {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
 {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
 {0x1F,0x10,0x1E,0x10,0x10,0x10,0x1F}, // E
 {0x1F,0x10,0x1E,0x10,0x10,0x10,0x10}, // F
 {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
 {0x11,0x11,0x1F,0x11,0x11,0x11,0x11}, // H
 {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
 {0x01,0x01,0x01,0x01,0x11,0x11,0x0E}, // J
 {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
 {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
 {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
 {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
 {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
 {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
 {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
 {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
 {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
 {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
 {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
 {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
 {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // W
 {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
 {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
 {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
 {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
 {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
 {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
 {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, // 3
 {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
 {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
 {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
 {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
 {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
 {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
 {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
 {0x04,0x04,0x04,0x04,0x04,0x00,0x04}, // !
 {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // -
};

static inline const unsigned char *glyph(char c)
{
   if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
   if (c >= 'A' && c <= 'Z') return kFont[c - 'A'];
   if (c >= '0' && c <= '9') return kFont[26 + (c - '0')];
   if (c == '!') return kFont[37];
   if (c == '-') return kFont[38];
   return kFont[36];
}

// Text into the internal buffer, so it scales with everything else and costs the
// same as any other internal pixel rather than one HLE call per screen pixel.
static inline void fb_text(const char *s, int x, int y, int scale, uint32_t colour)
{
   for (const char *p = s; *p; p++)
   {
      const unsigned char *g = glyph(*p);
      for (int gy = 0; gy < GH; gy++)
         for (int gx = 0; gx < GW; gx++)
            if ((g[gy] >> (GW - 1 - gx)) & 1)
               for (int sy = 0; sy < scale; sy++)
                  for (int sx = 0; sx < scale; sx++)
                     fb_put(x + gx * scale + sx, y + gy * scale + sy, colour);
      x += (GW + 1) * scale;
   }
}

static inline int text_width(const char *s, int scale)
{
   int n = 0; for (const char *p = s; *p; p++) n++;
   return n * (GW + 1) * scale;
}

// 1024-entry sine, 16.16, quarter-wave symmetric. Built once at startup by integrating
// rather than by calling libm: it keeps these roms free of any float dependency, and a
// table this size is exact enough that the error never shows at 160x90.
static fx g_sin[1024];
static inline void trig_init(void)
{
   // Small-angle integration around the circle. s' = c, c' = -s, step 2*pi/1024.
   fx s = 0, c = FX_ONE;
   const fx step = 402; // (2*pi/1024) in 16.16 = 0.006135 * 65536
   for (int i = 0; i < 1024; i++)
   {
      g_sin[i] = s;
      fx ns = s + fxmul(c, step);
      fx nc = c - fxmul(s, step);
      s = ns; c = nc;
   }
}
static inline fx fsin(int a) { return g_sin[((a % 1024) + 1024) % 1024]; }
static inline fx fcos(int a) { return fsin(a + 256); }

// Framebuffer plumbing shared by all three roms. Returns 0 on failure, having said
// why - a showcase rom that dies silently is worse than no showcase rom.
typedef struct { void *tv; uint32_t tvSize; } ScreenSetup;

static inline int screen_up(ScreenSetup *out)
{
   OSScreenInit();
   out->tvSize = OSScreenGetBufferSizeEx(SCREEN_TV);
   out->tv = memalign(0x100, out->tvSize);
   if (!out->tv) { OSReport("showcase: framebuffer alloc failed (%u bytes)\n", out->tvSize); return 0; }
   OSScreenSetBufferEx(SCREEN_TV, out->tv);
   OSScreenEnableEx(SCREEN_TV, 1);
   return 1;
}

static inline void screen_flip(ScreenSetup *s)
{
   DCFlushRange(s->tv, s->tvSize);
   OSScreenFlipBuffersEx(SCREEN_TV);
}
