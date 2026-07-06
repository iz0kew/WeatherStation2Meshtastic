// ============================================================================
// display_oled.cpp — backend OLED SSD1306 (Heltec V3/V4) via U8g2, HW I2C.
// ============================================================================
#include "../board_config.h"
#ifdef HAS_OLED

#include "display.h"
#include <U8g2lib.h>
#include <Wire.h>

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, PIN_OLED_RST,
                                                PIN_OLED_SCL, PIN_OLED_SDA);

void gfxInit() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);
}

void gfxClear() { u8g2.clearBuffer(); }
void gfxFlush() { u8g2.sendBuffer(); }

void gfxText(int x, int y, const char *s) {
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawUTF8(x, y, s);
}
void gfxTextBig(int x, int y, const char *s) {
  u8g2.setFont(u8g2_font_logisoso22_tf);
  u8g2.drawUTF8(x, y, s);
}
void gfxTextSmall(int x, int y, const char *s) {
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawUTF8(x, y, s);
}
int gfxTextWidth(const char *s) {
  u8g2.setFont(u8g2_font_6x12_tf);
  return u8g2.getUTF8Width(s);
}
int gfxTextWidthBig(const char *s) {
  u8g2.setFont(u8g2_font_logisoso22_tf);
  return u8g2.getUTF8Width(s);
}
int gfxTextWidthSmall(const char *s) {
  u8g2.setFont(u8g2_font_5x8_tf);
  return u8g2.getUTF8Width(s);
}
void gfxLine(int x0, int y0, int x1, int y1) { u8g2.drawLine(x0, y0, x1, y1); }
void gfxFillRect(int x, int y, int w, int h) { if (w > 0 && h > 0) u8g2.drawBox(x, y, w, h); }
void gfxPixel(int x, int y) { u8g2.drawPixel(x, y); }
void gfxXBM(int x, int y, int w, int h, const uint8_t *bits) { u8g2.drawXBM(x, y, w, h, bits); }
void gfxClearRect(int x, int y, int w, int h) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(x, y, w, h);
  u8g2.setDrawColor(1);
}
void gfxFrame(int x, int y, int w, int h) { u8g2.drawFrame(x, y, w, h); }

#endif // HAS_OLED
