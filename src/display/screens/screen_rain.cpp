// screen_rain.cpp — schermata Pioggia (CAP_RAIN)
#include "../../config/generated_config.h"
#ifdef SCREEN_RAIN
#include "../ui.h"
#include "../display.h"
#include "../../history.h"

static bool avail(uint32_t now) { return uiFieldFresh(g_ui.rainMm, now); }
static void draw() {
  uiHeaderSensor("Pioggia", g_ui.rainMm.model, g_ui.rainMm.id);
  char v[24];
  float r1  = history.rainDeltaMm(g_ui.rainMm.val, 3600UL * 1000UL);
  float r24 = history.rainDeltaMm(g_ui.rainMm.val, 24UL * 3600UL * 1000UL);
  snprintf(v, sizeof(v), "1h  %.1f mm", r1);  gfxText(0, 30, v);
  snprintf(v, sizeof(v), "24h %.1f mm", r24); gfxText(0, 44, v);
  snprintf(v, sizeof(v), "Tot %.1f mm", g_ui.rainMm.val); gfxText(0, 58, v);
}
extern const ScreenDef SCREEN_RAIN_DEF = { "Pioggia", avail, draw };
#endif
