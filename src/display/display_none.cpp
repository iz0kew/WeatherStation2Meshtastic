// ============================================================================
// display_none.cpp — backend "nessun display" per schede senza OLED (es.
//   Seeed XIAO nRF52840 + Wio-SX1262). Tutte le primitive sono no-op: ui.cpp
//   e screens/*.cpp restano invariati, calcolano i dati ma non disegnano
//   nulla, dato che non esiste hardware a cui scrivere.
// ============================================================================
#include "../board_config.h"
#ifndef HAS_OLED

#include "display.h"

void gfxInit() {}
void gfxClear() {}
void gfxFlush() {}
void gfxText(int, int, const char *) {}
void gfxTextBig(int, int, const char *) {}
void gfxTextSmall(int, int, const char *) {}
int  gfxTextWidth(const char *) { return 0; }
int  gfxTextWidthBig(const char *) { return 0; }
int  gfxTextWidthSmall(const char *) { return 0; }
void gfxLine(int, int, int, int) {}
void gfxFillRect(int, int, int, int) {}
void gfxPixel(int, int) {}
void gfxXBM(int, int, int, int, const uint8_t *) {}
void gfxClearRect(int, int, int, int) {}
void gfxFrame(int, int, int, int) {}

#endif // !HAS_OLED
