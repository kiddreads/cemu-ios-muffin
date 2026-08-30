// prime.rpx - "MUFFIN PRIME", the cinematic one.
//
// A real 3D renderer: perspective projection, a z-buffer, backface culling and
// per-face lighting, all in fixed point on the PPC, presented through OSScreen.
// The subject is a muffin built from a lathe of revolution - a profile curve spun
// around Y - because that is what a muffin actually is geometrically, and because a
// lathe gives smooth silhouettes from very few vertices.
//
// It is a showcase and a benchmark at once. Every frame does real transform, raster
// and depth work, so the frame rate it reaches is a direct readout of emulated CPU
// throughput - far more honest than a spinning logo that costs nothing.

#include "muffin_gfx.h"

#define RINGS 14
#define SEGS  20
#define NVERT (RINGS * SEGS)

typedef struct { fx x, y, z; } V3;

static V3 g_vert[NVERT];
static uint32_t g_ringColour[RINGS];

// Muffin profile: radius and height at each ring, bottom to top. The wrapper flares
// out and is straight-sided; the top is a domed overhang wider than the wrapper, which
// is the silhouette that makes a muffin read as a muffin rather than as a cupcake.
static const int kProfileR[RINGS] = { 26, 30, 34, 38, 42, 46, 50, 66, 74, 78, 76, 68, 52, 22 };
static const int kProfileY[RINGS] = { -60,-52,-44,-34,-24,-14, -4,  2, 10, 20, 30, 40, 50, 60 };

static void build_muffin(void)
{
   for (int r = 0; r < RINGS; r++)
   {
      for (int s = 0; s < SEGS; s++)
      {
         int a = s * 1024 / SEGS;
         g_vert[r * SEGS + s].x = fxmul(FX(kProfileR[r]), fcos(a)) / 100;
         g_vert[r * SEGS + s].y = FX(kProfileY[r]) / 100;
         g_vert[r * SEGS + s].z = fxmul(FX(kProfileR[r]), fsin(a)) / 100;
      }
      // Wrapper below the lip, cake above it. The lip is ring 7, where the profile
      // jumps outward - the same place a paper case stops on the real thing.
      g_ringColour[r] = (r < 7) ? rgb(214, 122, 160)   // muffin-brand pink wrapper
                                : rgb(176, 124, 74);   // baked crown
   }
}

// Flat-shaded triangle with a z-buffer, half-space rasterised. Integer edge functions
// only; no divides in the inner loop.
static void tri(int x0,int y0,int32_t z0, int x1,int y1,int32_t z1, int x2,int y2,int32_t z2, uint32_t c)
{
   int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
   int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
   int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
   int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
   if (minx < 0) minx = 0; if (miny < 0) miny = 0;
   if (maxx > RW - 1) maxx = RW - 1; if (maxy > RH - 1) maxy = RH - 1;
   if (minx > maxx || miny > maxy) return;

   int area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
   if (area == 0) return;
   int sign = area > 0 ? 1 : -1;

   for (int y = miny; y <= maxy; y++)
   {
      for (int x = minx; x <= maxx; x++)
      {
         int w0 = ((x1 - x0) * (y - y0) - (x - x0) * (y1 - y0)) * sign;
         int w1 = ((x2 - x1) * (y - y1) - (x - x1) * (y2 - y1)) * sign;
         int w2 = ((x0 - x2) * (y - y2) - (x - x2) * (y0 - y2)) * sign;
         if (w0 < 0 || w1 < 0 || w2 < 0) continue;
         // Depth at the centroid is close enough at this resolution and costs one
         // divide per triangle instead of three multiplies per pixel.
         int32_t z = (z0 + z1 + z2) / 3;
         int idx = y * RW + x;
         if (z < g_depth[idx]) { g_depth[idx] = z; g_fb[idx] = c; }
      }
   }
}

int main(int argc, char **argv)
{
   (void)argc; (void)argv;
   ScreenSetup scr;
   trig_init();
   build_muffin();
   if (!screen_up(&scr)) return 1;
   OSReport("prime.rpx: up, software 3D showcase starting\n");

   int frame = 0;
   for (;;)
   {
      int spin = frame * 4;
      // A slow bob and a slight tilt, so it reads as a camera move rather than a
      // turntable. Both are cheap and both are what make it look shot rather than
      // rendered.
      fx bob  = fxmul(FX(4), fsin(frame * 3));
      int tilt = 140 + (int)(fsin(frame * 2) >> 13);

      fb_gradient(rgb(24, 16, 34), rgb(78, 40, 62));

      fx cs = fcos(spin), sn = fsin(spin);
      fx ct = fcos(tilt), st = fsin(tilt);

      // Transform every vertex once into screen space, then draw quads as two
      // triangles. Transform is the cheap half; raster is what costs.
      static int px[NVERT], py[NVERT]; static int32_t pz[NVERT];
      for (int i = 0; i < NVERT; i++)
      {
         fx x = g_vert[i].x, y = g_vert[i].y + bob, z = g_vert[i].z;
         fx rx = fxmul(x, cs) - fxmul(z, sn);
         fx rz = fxmul(x, sn) + fxmul(z, cs);
         fx ry = fxmul(y, ct) - fxmul(rz, st);
         fx rz2 = fxmul(y, st) + fxmul(rz, ct);

         fx camz = rz2 + FX(4);                       // push the model in front of the eye
         if (camz < FX(1) / 4) camz = FX(1) / 4;
         fx inv = fxdiv(FX(1), camz);
         px[i] = RW / 2 + (int)(fxmul(fxmul(rx, inv), FX(110)) >> 16);
         py[i] = RH / 2 - (int)(fxmul(fxmul(ry, inv), FX(110)) >> 16);
         pz[i] = camz;
      }

      for (int r = 0; r < RINGS - 1; r++)
      {
         for (int s = 0; s < SEGS; s++)
         {
            int s2 = (s + 1) % SEGS;
            int a = r * SEGS + s, b = r * SEGS + s2;
            int c = (r + 1) * SEGS + s2, d = (r + 1) * SEGS + s;

            // Backface cull on screen-space winding. Also the lighting term: a face
            // turned away from the key light is dimmer, and with a lathe the segment
            // index IS the angle, so the light falls out of the geometry for free.
            int area = (px[b] - px[a]) * (py[c] - py[a]) - (px[c] - px[a]) * (py[b] - py[a]);
            if (area <= 0) continue;

            int lightAngle = (s * 1024 / SEGS) - spin;
            int lambert = 128 + (int)(fcos(lightAngle) >> 10);   // 0..256-ish
            if (lambert < 40) lambert = 40; if (lambert > 255) lambert = 255;
            uint32_t col = shade(g_ringColour[r], lambert, 255);

            tri(px[a],py[a],pz[a], px[b],py[b],pz[b], px[c],py[c],pz[c], col);
            tri(px[a],py[a],pz[a], px[c],py[c],pz[c], px[d],py[d],pz[d], col);
         }
      }

      // Title card fades up over the first two seconds, then holds. Drawn after the
      // model and with no depth test, so it always sits in front.
      int fade = frame < 60 ? frame * 255 / 60 : 255;
      uint32_t ink = shade(rgb(255, 240, 225), fade, 255);
      const char *t1 = "MUFFIN PRIME";
      const char *t2 = "SOFTWARE 3D ON A WII U";
      fb_text(t1, (RW - text_width(t1, 2)) / 2, 6, 2, ink);
      fb_text(t2, (RW - text_width(t2, 1)) / 2, RH - 12, 1, shade(ink, 3, 5));

      fb_present(SCREEN_TV);
      screen_flip(&scr);
      frame++;
   }
   return 0;
}
