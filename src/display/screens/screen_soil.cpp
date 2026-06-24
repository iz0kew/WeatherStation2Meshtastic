// screen_soil.cpp — schermata Umidita' suolo (CAP_SOIL)
#include "../../config/generated_config.h"
#ifdef SCREEN_SOIL
#include "../ui.h"
#include "../display.h"

static bool avail(uint32_t now) { return uiFieldFresh(g_ui.soil, now); }
static void draw() {
  uiHeaderSensor("Suolo", g_ui.soil.model, g_ui.soil.id);
  char v[12];
  snprintf(v, sizeof(v), "%.0f%%", g_ui.soil.val);
  gfxTextBig(0, 46, v);
  // barra
  int w = (int)(g_ui.soil.val * (GFX_W - 4) / 100.0f);
  gfxLine(0, 60, GFX_W - 1, 60);
  gfxFillRect(2, 55, w, 6);
}
extern const ScreenDef SCREEN_SOIL_DEF = { "Suolo", avail, draw };
#endif
