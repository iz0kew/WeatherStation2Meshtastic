// screen_wind.cpp — schermata Vento (CAP_WIND)
#include "../../config/generated_config.h"
#ifdef SCREEN_WIND
#include "../ui.h"
#include "../display.h"

static const char *dir16(float deg) {
  static const char *d[] = {"N","NNE","NE","ENE","E","ESE","SE","SSE",
                            "S","SSO","SO","OSO","O","ONO","NO","NNO"};
  int i = (int)((deg + 11.25f) / 22.5f) & 15;
  return d[i];
}
static bool avail(uint32_t now) { return uiFieldFresh(g_ui.windAvg, now); }
static void draw() {
  uint32_t now = millis();
  uiHeaderSensor("Vento", g_ui.windAvg.model, g_ui.windAvg.id);
  char v[24];
  snprintf(v, sizeof(v), "Media %.1f m/s", g_ui.windAvg.val); gfxText(0, 30, v);
  if (uiFieldFresh(g_ui.windGust, now)) {
    snprintf(v, sizeof(v), "Raffica %.1f m/s", g_ui.windGust.val); gfxText(0, 44, v);
  }
  if (uiFieldFresh(g_ui.windDir, now)) {
    snprintf(v, sizeof(v), "Dir %s (%.0f)", dir16(g_ui.windDir.val), g_ui.windDir.val);
    gfxText(0, 58, v);
  }
}
extern const ScreenDef SCREEN_WIND_DEF = { "Vento", avail, draw };
#endif
