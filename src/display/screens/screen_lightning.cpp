// screen_lightning.cpp — schermata Fulmini (CAP_LIGHTNING)
#include "../../config/generated_config.h"
#ifdef SCREEN_LIGHTNING
#include "../ui.h"
#include "../display.h"

static bool avail(uint32_t now) {
  return g_ui.haveLightning && (uint32_t)(now - g_ui.lightningLastSeen) < UI_STALE_MS;
}
static void draw() {
  uiHeaderSensor("Fulmini", g_ui.lightningModel, g_ui.lightningId);
  char v[24];
  snprintf(v, sizeof(v), "Totale: %lu", (unsigned long)g_ui.strikeTotal);
  gfxText(0, 30, v);
  if (g_ui.lastDistKm != 63) {
    snprintf(v, sizeof(v), "Distanza ~%u km", g_ui.lastDistKm);
    gfxText(0, 44, v);
  } else {
    gfxText(0, 44, "Nessun fulmine vicino");
  }
}
extern const ScreenDef SCREEN_LIGHTNING_DEF = { "Fulmini", avail, draw };
#endif
