// screen_temphum.cpp — schermata Temperatura / Umidita' (CAP_TEMP / CAP_HUM)
#include "../../config/generated_config.h"
#ifdef SCREEN_TEMPHUM
#include "../ui.h"
#include "../display.h"

static bool avail(uint32_t now) {
  return uiFieldFresh(g_ui.tempC, now) || uiFieldFresh(g_ui.humidity, now);
}
static void draw() {
  uint32_t now = millis();
  const UiField &f = uiFieldFresh(g_ui.tempC, now) ? g_ui.tempC : g_ui.humidity;
  uiHeaderSensor("Temp/Umid", f.model, f.id);
  char v[16];
  if (uiFieldFresh(g_ui.tempC, now)) {
    snprintf(v, sizeof(v), "%.1f", g_ui.tempC.val);
    gfxTextBig(0, 44, v);
    // "°C" subito a destra dell'ultima cifra (larghezza misurata sul font grande)
    gfxText(gfxTextWidthBig(v) + 3, 30, "\xC2\xB0" "C");
  }
  if (uiFieldFresh(g_ui.humidity, now)) {
    snprintf(v, sizeof(v), "RH %.0f%%", g_ui.humidity.val);
    gfxText(0, 60, v);
  }
}
extern const ScreenDef SCREEN_TEMPHUM_DEF = { "Temp/Umid", avail, draw };
#endif
