// ============================================================================
// display.h — astrazione display con spazio logico 128x64.
//   backend OLED (SSD1306, Heltec V3/V4) in display_oled.cpp
// Le schermate sono disegnate in screens/ usando queste primitive.
// ============================================================================
#pragma once
#include <Arduino.h>

void gfxInit();
void gfxClear();
void gfxFlush();
void gfxText(int x, int y, const char *s);     // font piccolo ~6x12, y = baseline
void gfxTextBig(int x, int y, const char *s);  // font grande per il valore principale
void gfxTextSmall(int x, int y, const char *s); // font minimo ~5x8 (titoli lunghi)
int  gfxTextWidth(const char *s);                 // larghezza con font piccolo
int  gfxTextWidthBig(const char *s);              // larghezza con font grande
int  gfxTextWidthSmall(const char *s);            // larghezza con font minimo
void gfxLine(int x0, int y0, int x1, int y1);
void gfxFillRect(int x, int y, int w, int h);
void gfxPixel(int x, int y);
void gfxXBM(int x, int y, int w, int h, const uint8_t *bits);  // bitmap XBM (LSB-first)
void gfxClearRect(int x, int y, int w, int h);   // area a sfondo (per finestre overlay)
void gfxFrame(int x, int y, int w, int h);       // solo bordo del rettangolo

#define GFX_W 128
#define GFX_H 64
