// handson.rpx - "HANDS ON", the interactive one, and the only one of the three that
// tests something no other rom on this port ever has.
//
// Everything the emulator has booted so far has been output-only. The whole input
// chain - touch on glass, ControllerLayout, cemu_bridge_set_stick_axis, the atomic
// override map, InputManager, VPADRead - has never once been verified end to end on a
// device, because nothing running on the emulator has ever READ a controller.
//
// So this is a toy and an instrument. You fly a muffin around with the left stick and
// the buttons change how it behaves, which is the demo. And the bottom of the screen
// prints the exact values arriving from VPADRead, which is the instrument: if the
// stick reads 0,0 while your thumb is somewhere else, that is the bug, on screen, with
// the number that proves it. Analog is shown as a number and not just as motion
// deliberately - a d-pad pretending to be a stick still moves you; only the number
// shows it is snapping to eight directions instead of reporting a magnitude.

#include "muffin_gfx.h"
#include <vpad/input.h>

#define TRAIL 24

typedef struct { int x, y; } Pt;

static Pt g_trail[TRAIL];
static int g_trailHead;

static void draw_disc(int cx, int cy, int r, uint32_t colour)
{
   for (int y = -r; y <= r; y++)
      for (int x = -r; x <= r; x++)
         if (x * x + y * y <= r * r)
            fb_put(cx + x, cy + y, colour);
}

// Print a signed value as text. No snprintf: it pulls in a chunk of libc for something
// this rom needs four times a frame, and every avoided dependency is one less thing
// that can fail on an interpreter nobody has stress-tested.
static void fb_number(int value, int x, int y, int scale, uint32_t colour)
{
   char buf[12];
   int i = 0, neg = value < 0;
   unsigned v = (unsigned)(neg ? -value : value);
   if (v == 0) buf[i++] = '0';
   while (v && i < 10) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
   if (neg && i < 11) buf[i++] = '-';
   char out[12]; int n = 0;
   while (i > 0) out[n++] = buf[--i];
   out[n] = '\0';
   fb_text(out, x, y, scale, colour);
}

int main(int argc, char **argv)
{
   (void)argc; (void)argv;
   ScreenSetup scr;
   trig_init();
   if (!screen_up(&scr)) return 1;
   VPADInit();
   OSReport("handson.rpx: up, reading VPAD\n");

   int px = RW / 2 * 256, py = RH / 2 * 256;   // position in 1/256ths of a pixel
   int vx = 0, vy = 0;
   int mode = 0;
   int frame = 0;
   int everSawStick = 0;

   for (int i = 0; i < TRAIL; i++) { g_trail[i].x = px >> 8; g_trail[i].y = py >> 8; }

   for (;;)
   {
      VPADStatus pad;
      VPADReadError err;
      int got = (VPADRead(VPAD_CHAN_0, &pad, 1, &err) > 0) && (err == VPAD_READ_SUCCESS);

      // VPADStatus reports the sticks as floats in -1..1. Converted to 1/1000ths
      // immediately: it is the only float this rom touches, the value is displayed as
      // an integer anyway, and keeping it out of the physics keeps the physics exact.
      int sx = 0, sy = 0;
      uint32_t held = 0;
      if (got)
      {
         sx = (int)(pad.leftStick.x * 1000.0f);
         sy = (int)(pad.leftStick.y * 1000.0f);
         held = pad.hold;
         if (sx || sy) everSawStick = 1;
         if (pad.trigger & VPAD_BUTTON_A) mode = (mode + 1) % 3;
      }

      // Mode is the "what does the stick mean" switch, and each one reads differently
      // enough that a broken axis is obvious in all three.
      switch (mode)
      {
      case 0: // direct - position follows the stick, so deflection maps to speed
         vx = sx / 8; vy = -sy / 8;
         break;
      case 1: // momentum - the stick accelerates, nothing decelerates but drag
         vx += sx / 60; vy -= sy / 60;
         vx = vx * 63 / 64; vy = vy * 63 / 64;
         break;
      default: // orbit - the stick aims, and the muffin circles what it aims at
      {
         int a = frame * 6;
         vx = sx / 12 + (int)(fcos(a) >> 12);
         vy = -sy / 12 + (int)(fsin(a) >> 12);
         break;
      }
      }

      px += vx; py += vy;
      if (px < 6 * 256)        { px = 6 * 256;        vx = -vx / 2; }
      if (px > (RW - 6) * 256) { px = (RW - 6) * 256; vx = -vx / 2; }
      if (py < 6 * 256)        { py = 6 * 256;        vy = -vy / 2; }
      if (py > (RH - 20) * 256){ py = (RH - 20) * 256; vy = -vy / 2; }

      g_trail[g_trailHead].x = px >> 8;
      g_trail[g_trailHead].y = py >> 8;
      g_trailHead = (g_trailHead + 1) % TRAIL;

      fb_gradient(rgb(18, 22, 40), rgb(52, 30, 66));

      // Trail, oldest faintest. Costs 24 small discs and does more for the sense of
      // response than anything else in the rom - latency is visible as a gap.
      for (int i = 0; i < TRAIL; i++)
      {
         int idx = (g_trailHead + i) % TRAIL;
         int age = i * 255 / TRAIL;
         draw_disc(g_trail[idx].x, g_trail[idx].y, 1 + i / 10,
                   mixc(rgb(40, 30, 60), rgb(214, 122, 160), age));
      }
      draw_disc(px >> 8, py >> 8, 4, rgb(226, 168, 196));
      draw_disc((px >> 8) - 1, (py >> 8) - 1, 2, rgb(255, 236, 214));

      // The instrument half.
      const char *modeName = mode == 0 ? "DIRECT" : (mode == 1 ? "MOMENTUM" : "ORBIT");
      fb_text("A - MODE", 4, 4, 1, shade(rgb(255,255,255), 2, 5));
      fb_text(modeName, RW - text_width(modeName, 1) - 4, 4, 1, rgb(255, 236, 214));

      fb_text("X", 4, RH - 8, 1, rgb(150, 200, 255));
      fb_number(sx, 12, RH - 8, 1, rgb(255, 255, 255));
      fb_text("Y", 44, RH - 8, 1, rgb(150, 200, 255));
      fb_number(sy, 52, RH - 8, 1, rgb(255, 255, 255));

      // A live button strip. Eight lamps, lit while held - the fastest way to find a
      // button wired to the wrong bit.
      static const uint32_t kMask[8] = {
         VPAD_BUTTON_A, VPAD_BUTTON_B, VPAD_BUTTON_X, VPAD_BUTTON_Y,
         VPAD_BUTTON_L, VPAD_BUTTON_R, VPAD_BUTTON_PLUS, VPAD_BUTTON_MINUS
      };
      for (int i = 0; i < 8; i++)
      {
         uint32_t c = (held & kMask[i]) ? rgb(120, 255, 160) : rgb(60, 60, 80);
         for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
               fb_put(RW - 40 + i * 5 + x, RH - 8 + y, c);
      }

      // The one line that turns "it feels wrong" into a fact.
      if (!got)
         fb_text("NO PAD - VPADREAD FAILED", 4, RH - 18, 1, rgb(255, 120, 120));
      else if (!everSawStick)
         fb_text("PAD OK - STICK STILL READING ZERO", 4, RH - 18, 1, rgb(255, 210, 120));

      fb_present(SCREEN_TV);
      screen_flip(&scr);
      frame++;
   }
   return 0;
}
