// screen_aqi.cpp — schermata Qualita' aria PM / CO2 (CAP_PM / CAP_CO2)
#include "../../config/generated_config.h"
#ifdef SCREEN_AQI
#include "../ui.h"
#include "../display.h"

static bool avail(uint32_t now) {
  return uiFieldFresh(g_ui.pm25, now) || uiFieldFresh(g_ui.co2, now);
}
static void draw() {
  uint32_t now = millis();
  const UiField &f = uiFieldFresh(g_ui.pm25, now) ? g_ui.pm25 : g_ui.co2;
  uiHeaderSensor("Aria", f.model, f.id);
  char v[24];
  int y = 30;
  if (uiFieldFresh(g_ui.pm25, now)) {
    snprintf(v, sizeof(v), "PM2.5 %.0f ug/m3", g_ui.pm25.val); gfxText(0, y, v); y += 13;
  }
  if (uiFieldFresh(g_ui.pm10, now)) {
    snprintf(v, sizeof(v), "PM10  %.0f ug/m3", g_ui.pm10.val); gfxText(0, y, v); y += 13;
  }
  if (uiFieldFresh(g_ui.co2, now)) {
    snprintf(v, sizeof(v), "CO2   %.0f ppm", g_ui.co2.val); gfxText(0, y, v); y += 13;
  }
}
extern const ScreenDef SCREEN_AQI_DEF = { "Aria", avail, draw };
#endif
