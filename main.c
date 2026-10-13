#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <math.h>
#include <stdlib.h>
PSP_MODULE_INFO("PSP Zakulise", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
#define W 480
#define H 272
#define RGB(r,g,b) (0xFF000000u|((b)<<16)|((g)<<8)|(r))
static u32 *fb = (u32 *)0x44000000;
static int exit_cb(int a, int b, void *c) { sceKernelExitGame(); return 0; }
static int cb_thread(SceSize s, void *p) { int id = sceKernelCreateCallback("exit", exit_cb, NULL); sceKernelRegisterExitCallback(id); sceKernelSleepThreadCB(); return 0; }
static void rect(int x, int y, int w, int h, u32 c) { for (int j = y; j < y + h; j++) { if (j < 0 || j >= H) continue; for (int i = x; i < x + w; i++) if (i >= 0 && i < W) fb[j * 512 + i] = c; } }
static void txt(int col, int row, const char *s, u32 c) { pspDebugScreenSetXY(col, row); pspDebugScreenSetTextColor(c); pspDebugScreenPrintf("%s", s); }
/* ASCII-only: the built-in PSP debug font has no Cyrillic. */
typedef struct { const char *n; int need; const char *a[4]; int g[4]; const char *r[4]; const char *intro, *win; } En;
static En E[2] = {
 {"Ozhidalka", 3, {"Wait together", "Ask what she waits for", "Say: I am here", "Press Cancel"}, {1, 1, 1, 0},
  {"You stand still. Circles slow down.", "She blinks: Someone. For long.", "Her circles warm up.", "She flinches. Do not."},
  "A loading icon spins forever.", "...Finally. I waited for YOU."},
 {"Firmware 6.60", 5, {"Show error log", "Recall console name", "Show Last Chance", "Call icons friends"}, {0, 1, 2, 1},
  {"It sees no errors.", "...PSP. I was called PSP.", "The backup glows. It freezes.", "Far icons light up."},
  "Firmware 6.60 blocks the way.", "Backup intact. No more erasing."}};
typedef struct { float x, y, vx, vy; int on; } Bl;
static Bl B[96];
static void spawn(float x, float y, float vx, float vy) { for (int i = 0; i < 96; i++) if (!B[i].on) { B[i] = (Bl){x, y, vx, vy, 1}; return; } }
enum { TITLE, MENU, ACT, TEXT, ATK, DEAD, END };
int main(void) {
  int t = sceKernelCreateThread("cb", cb_thread, 0x11, 0xFA0, 0, NULL); if (t >= 0) sceKernelStartThread(t, 0, NULL);
  pspDebugScreenInit(); sceCtrlSetSamplingCycle(0); sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  int mode = TITLE, k = 0, sel = 0, mercy = 0, hp = 20, inv = 0, f = 0, rd = 0, items = 2, after = MENU, tick = 0; const char *msg = "";
  float sx = 240, sy = 190; unsigned old = 0; const int ax = 150, ay = 100, aw = 180, ah = 110;
  while (1) {
    sceDisplayWaitVblankStart(); tick++;
    SceCtrlData p; sceCtrlReadBufferPositive(&p, 1); unsigned dn = p.Buttons & ~old; old = p.Buttons;
    int X = dn & PSP_CTRL_CROSS, O = dn & PSP_CTRL_CIRCLE;
    int up = dn & PSP_CTRL_UP, dw = dn & PSP_CTRL_DOWN, lf = dn & PSP_CTRL_LEFT, rt = dn & PSP_CTRL_RIGHT;
    En *e = &E[k];
    if (mode == TITLE) { if (X) { k = 0; mode = TEXT; msg = E[0].intro; after = MENU; hp = 20; mercy = 0; items = 2; rd = 0; } }
    else if (mode == TEXT) { if (X) mode = after; }
    else if (mode == MENU) {
      if (lf && sel > 0) sel--; if (rt && sel < 2) sel++;
      if (X) {
        if (sel == 0) { mode = ACT; sel = 0; }
        else if (sel == 1) { if (items > 0) { items--; hp = hp + 8 > 20 ? 20 : hp + 8; msg = "Charged. Battery +8."; mode = TEXT; after = ATK; f = 0; sx = 240; sy = 190; } }
        else if (mercy >= e->need) { msg = e->win; mode = TEXT; after = (k == 0) ? MENU : END; if (k == 0) { k = 1; mercy = 0; rd = 0; hp = 20; items = 2; } }
        else { msg = "Not ready yet. Support first."; mode = TEXT; after = ATK; f = 0; sx = 240; sy = 190; }
        if (sel == 2 && mode == TEXT && after == MENU) sel = 0;
      }
    } else if (mode == ACT) {
      if (up && sel > 0) sel--; if (dw && sel < 3) sel++; if (O) { mode = MENU; sel = 0; }
      if (X) { mercy += e->g[sel]; if (mercy > e->need) mercy = e->need; msg = e->r[sel]; mode = TEXT; after = ATK; f = 0; sx = 240; sy = 190; sel = 0; for (int i = 0; i < 96; i++) B[i].on = 0; }
    } else if (mode == ATK) {
      f++; float sp = 2.5f; int ax_ = p.Lx - 128, ay_ = p.Ly - 128;
      if (abs(ax_) > 30) sx += sp * ax_ / 128.0f; if (abs(ay_) > 30) sy += sp * ay_ / 128.0f;
      if (p.Buttons & PSP_CTRL_LEFT) sx -= sp; if (p.Buttons & PSP_CTRL_RIGHT) sx += sp;
      if (p.Buttons & PSP_CTRL_UP) sy -= sp; if (p.Buttons & PSP_CTRL_DOWN) sy += sp;
      if (sx < ax + 6) sx = ax + 6; if (sx > ax + aw - 6) sx = ax + aw - 6; if (sy < ay + 6) sy = ay + 6; if (sy > ay + ah - 6) sy = ay + ah - 6;
      float L = 1.0f + rd * 0.12f;
      if (k == 0 && f % 46 == 1) { float px = ax + rand() % aw, py = ay + rand() % (ah / 2), o = rand() % 6; for (int i = 0; i < 9; i++) { float a = o + i * 6.2832f / 9; spawn(px, py, cosf(a) * 1.3f * L, sinf(a) * 1.3f * L); } }
      if (k == 1 && f % 40 == 1) { int g = ax + 20 + rand() % (aw - 70); for (int px = ax; px <= ax + aw; px += 16) if (px < g || px > g + 46) spawn(px, ay - 6, 0, 1.9f * L); }
      if (k == 1 && f % 64 == 30) { float a = atan2f(sy - 50, sx - 240); spawn(240, 50, cosf(a) * 2.6f * L, sinf(a) * 2.6f * L); }
      if (inv) inv--;
      for (int i = 0; i < 96; i++) if (B[i].on) { B[i].x += B[i].vx; B[i].y += B[i].vy;
        if (B[i].x < -10 || B[i].x > W + 10 || B[i].y < -10 || B[i].y > H + 10) B[i].on = 0;
        else if (!inv && fabsf(B[i].x - sx) < 7 && fabsf(B[i].y - sy) < 7) { hp -= 3; inv = 50; B[i].on = 0; } }
      if (hp <= 0) { mode = DEAD; }
      else if (f >= 330) { rd++; mode = MENU; sel = 0; msg = ""; for (int i = 0; i < 96; i++) B[i].on = 0; }
    } else if (mode == DEAD) { if (X) { hp = 20; mercy = 0; items = 2; rd = 0; mode = MENU; sel = 0; } }
    else if (mode == END) { if (X) mode = TITLE; }
    for (int i = 0; i < W * 512 && 0; i++) fb[i] = 0;
    for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) fb[j * 512 + i] = 0xFF000000u;
    if (mode == TITLE) { txt(24, 10, "PSP: ZAKULISE", RGB(127, 212, 255)); txt(20, 14, "(PSP: Backrooms)", RGB(255, 255, 255)); if (tick % 60 < 40) txt(22, 20, "Press X", RGB(255, 183, 3)); }
    else if (mode == END) { txt(22, 12, "RESTART", RGB(127, 212, 255)); txt(14, 15, "All data is safe. Thanks for playing!", RGB(255, 255, 255)); txt(22, 20, "Press X", RGB(255, 183, 3)); }
    else {
      if (k == 0) { for (int i = 0; i < 8; i++) { float a = tick / 12.0f + i * 0.7854f; rect(240 + (int)(cosf(a) * 22), 36 + (int)(sinf(a) * 22), 6 + i / 2, 6 + i / 2, RGB(30 + i * 28, 80 + i * 18, 255)); } }
      else { rect(210, 18, 60, 48, RGB(28, 28, 58)); rect(210, 18, 60, 3, RGB(127, 212, 255)); rect(222, 36, 8, 8, RGB(255, 183, 3)); rect(250, 36, 8, 8, RGB(255, 183, 3)); }
      txt(30, 8, e->n, RGB(255, 255, 255));
      for (int i = 0; i < aw; i++) { rect(ax + i, ay, 1, 2, RGB(255, 255, 255)); rect(ax + i, ay + ah, 1, 2, RGB(255, 255, 255)); } rect(ax, ay, 2, ah, RGB(255, 255, 255)); rect(ax + aw, ay, 2, ah + 2, RGB(255, 255, 255));
      for (int i = 0; i < 96; i++) if (B[i].on) rect((int)B[i].x - 3, (int)B[i].y - 3, 6, 6, k ? RGB(255, 77, 154) : RGB(127, 212, 255));
      if (mode == ATK && !(inv && tick % 6 < 3)) { rect((int)sx - 5, (int)sy - 5, 10, 10, RGB(77, 225, 255)); rect((int)sx - 1, (int)sy - 1, 3, 3, 0xFF000000u); }
      rect(20, 236, hp * 4 > 0 ? hp * 4 : 0, 8, RGB(255, 212, 0)); txt(2, 29, "BAT", RGB(255, 255, 255));
      for (int i = 0; i < e->need; i++) rect(180 + i * 14, 237, 8, 8, i < mercy ? RGB(127, 212, 255) : RGB(40, 40, 90));
      if (mode == MENU) { const char *m[3] = {"ACT", "ITEM", "SPARE"}; for (int i = 0; i < 3; i++) txt(6 + i * 14, 32, m[i], i == sel ? RGB(255, 183, 3) : RGB(255, 255, 255)); }
      if (mode == ACT) for (int i = 0; i < 4; i++) { txt(4 + (i % 2) * 28, 31 + i / 2, e->a[i], i == sel ? RGB(255, 183, 3) : RGB(255, 255, 255)); }
      if (mode == TEXT) { txt(2, 28, msg, RGB(255, 255, 255)); txt(2, 33, "X: next", RGB(150, 150, 150)); }
      if (mode == DEAD) { txt(6, 28, "Battery died. The disc still remembers why it is here.", RGB(255, 80, 80)); txt(2, 33, "X: retry", RGB(150, 150, 150)); }
    }
  }
  return 0;
}
