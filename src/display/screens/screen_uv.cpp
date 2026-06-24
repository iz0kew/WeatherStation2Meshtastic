// screen_uv.cpp — schermata UV / Luce (CAP_UV)
#include "../../config/generated_config.h"
#ifdef SCREEN_UV
#include "../ui.h"
#include "../display.h"

static bool avail(uint32_t now) {
  return uiFieldFresh(g_ui.uv, now) || uiFieldFresh(g_ui.lux, now);
}
static void draw() {
  uint32_t now = millis();
  const UiField &f = uiFieldFresh(g_ui.uv, now) ? g_ui.uv : g_ui.lux;
  uiHeaderSensor("UV/Luce", f.model, f.id);
  char v[24];
  if (uiFieldFresh(g_ui.uv, now)) {
    snprintf(v, sizeof(v), "%.1f", g_ui.uv.val);
    gfxTextBig(0, 44, v);
    gfxText(0, 60, "UV index");
  }
  if (uiFieldFresh(g_ui.lux, now)) {
    snprintf(v, sizeof(v), "%.0f lux", g_ui.lux.val);
    gfxText(GFX_W - gfxTextWidth(v), 30, v);
  }
}
extern const ScreenDef SCREEN_UV_DEF = { "UV/Luce", avail, draw };
#endif
