// bakery.rpx - "THE BAKERY", the one that is a short film rather than a tech demo.
//
// A muffin gets made, on a loop: batter poured, rising in the heat, doming, browning,
// steam coming off it, then a title. No user input, no interaction - it plays.
//
// Everything is procedural. There is no video here and there cannot be: the Wii U has
// no path to a decoded video stream in this emulator (its H.264 decoder is HLE'd and
// has never been exercised), and a real film would be hundreds of megabytes in the app
// bundle for something the emulator would then fail to play. Generating the imagery on
// the CPU each frame is both the only way it runs and the only way it stays a rom
// rather than an attachment.
//
// The warm palette is deliberate and matches the app's own bakery theme rather than
// being invented here.

#include "muffin_gfx.h"

#define STAGE_FRAMES 150
#define STAGES 5

static uint32_t g_ovenGlow;

// A filled circle with a soft edge, in the internal buffer. The soft edge is what
// keeps a 160x90 image from looking like a spreadsheet.
static void disc(int cx, int cy, int r, uint32_t colour, int soft)
{
   for (int y = cy - r - soft; y <= cy + r + soft; y++)
   {
      for (int x = cx - r - soft; x <= cx + r + soft; x++)
      {
         int dx = x - cx, dy = y - cy;
         int d2 = dx * dx + dy * dy;
         if (d2 > (r + soft) * (r + soft)) continue;
         if (d2 <= r * r) { fb_put(x, y, colour); continue; }
         if (soft <= 0) continue;
         // Linear falloff in radius, computed without a square root by comparing
         // against the two bounding circles.
         int outer = (r + soft) * (r + soft);
         int t = 256 - ((d2 - r * r) * 256) / (outer - r * r + 1);
         if ((unsigned)(y * RW + x) < RW * RH && x >= 0 && x < RW && y >= 0 && y < RH)
            fb_put(x, y, mixc(g_fb[y * RW + x], colour, t));
      }
   }
}

// The muffin itself, drawn as a wrapper trapezoid plus a dome whose height and colour
// are driven by the animation. One function, four stages, because the shape IS the
// story.
static void draw_muffin(int cx, int baseY, int rise, int brown, int wrapperOnly)
{
   uint32_t wrapper = rgb(214, 122, 160);
   uint32_t wrapDark = shade(wrapper, 3, 5);

   // Wrapper: straight-sided, flaring slightly, with fluting suggested by alternating
   // columns rather than drawn as geometry.
   for (int y = 0; y < 18; y++)
   {
      int halfW = 14 + y / 3;
      for (int x = -halfW; x <= halfW; x++)
      {
         int flute = ((x + 64) / 3) & 1;
         fb_put(cx + x, baseY - y, flute ? wrapper : wrapDark);
      }
   }
   if (wrapperOnly) return;

   // Dome. rise is 0..100; brown is 0..255 and takes the crown from pale batter to
   // baked. The two are separate because a muffin rises before it colours, and doing
   // them together is what makes CG food look wrong.
   uint32_t pale  = rgb(226, 200, 150);
   uint32_t baked = rgb(150, 96, 52);
   uint32_t crown = mixc(pale, baked, brown);

   int domeH = rise * 20 / 100;
   int domeW = 16 + rise * 6 / 100;
   for (int y = 0; y <= domeH; y++)
   {
      // Circular-ish cap: width falls off with the square of height.
      int w = (domeW * (domeH - y) * (domeH + y)) / (domeH * domeH + 1);
      if (w < 0) w = 0;
      for (int x = -w; x <= w; x++)
      {
         // Key light from upper left, so the crown has a highlight and a shaded side.
         int lit = 190 - (x * 60 / (w + 1)) - (y * 40 / (domeH + 1));
         fb_put(cx + x, baseY - 17 - y, shade(crown, lit, 190));
      }
   }
}

static void steam(int cx, int topY, int frame)
{
   for (int i = 0; i < 3; i++)
   {
      int phase = frame * 3 + i * 340;
      int life = phase % 200;
      int y = topY - life / 5;
      int x = cx - 8 + i * 8 + (int)(fsin(phase * 2) >> 14);
      int alpha = 200 - life;
      if (alpha <= 0) continue;
      disc(x, y, 1, mixc(rgb(60, 40, 50), rgb(255, 255, 255), alpha), 2);
   }
}

int main(int argc, char **argv)
{
   (void)argc; (void)argv;
   ScreenSetup scr;
   trig_init();
   if (!screen_up(&scr)) return 1;
   OSReport("bakery.rpx: up, short film starting\n");

   int frame = 0;
   for (;;)
   {
      int stage = (frame / STAGE_FRAMES) % STAGES;
      int t = (frame % STAGE_FRAMES) * 256 / STAGE_FRAMES;   // 0..255 within the stage
      int cx = RW / 2, baseY = RH - 18;

      // The oven's own light warms and cools across the bake, which is what sells the
      // cut between stages as a scene change rather than a state change.
      int heat = (stage == 0) ? 0 : (stage >= 3 ? 255 : t);
      g_ovenGlow = mixc(rgb(28, 20, 26), rgb(96, 44, 30), heat);
      fb_gradient(shade(g_ovenGlow, 6, 10), g_ovenGlow);

      // A hot element line at the top, brighter as the bake proceeds.
      for (int x = 8; x < RW - 8; x++)
         fb_put(x, 6, mixc(rgb(70, 40, 40), rgb(255, 150, 70), heat));

      const char *caption = "";
      switch (stage)
      {
      case 0: // pour
         draw_muffin(cx, baseY, 0, 0, 1);
         {
            int pourY = 8 + t * (baseY - 26) / 256;
            for (int y = 8; y < pourY; y++) fb_put(cx, y, rgb(226, 200, 150));
            disc(cx, pourY, 2, rgb(226, 200, 150), 1);
         }
         caption = "BATTER";
         break;
      case 1: // rise
         draw_muffin(cx, baseY, t * 100 / 256, 0, 0);
         caption = "RISE";
         break;
      case 2: // dome and colour
         draw_muffin(cx, baseY, 100, t, 0);
         caption = "BAKE";
         break;
      case 3: // out of the oven, steaming
         draw_muffin(cx, baseY, 100, 255, 0);
         steam(cx, baseY - 40, frame);
         caption = "REST";
         break;
      default: // title
         draw_muffin(cx, baseY, 100, 255, 0);
         steam(cx, baseY - 40, frame);
         {
            const char *ttl = "MUFFINEMU";
            fb_text(ttl, (RW - text_width(ttl, 2)) / 2, 20, 2, rgb(255, 236, 214));
         }
         caption = "SERVED";
         break;
      }

      fb_text(caption, 6, RH - 10, 1, shade(rgb(255, 220, 190), 3, 5));

      fb_present(SCREEN_TV);
      screen_flip(&scr);
      frame++;
   }
   return 0;
}
