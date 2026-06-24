// screen_pressure.cpp — schermata Pressione (CAP_PRESSURE)
#include "../../config/generated_config.h"
#ifdef SCREEN_PRESSURE
#include "../ui.h"
#include "../display.h"

static bool avail(uint32_t now) { return uiFieldFresh(g_ui.pressureHpa, now); }
static void draw() {
  uiHeaderSensor("Pressione", g_ui.pressureHpa.model, g_ui.pressureHpa.id);
  char v[16];
  snprintf(v, sizeof(v), "%.0f", g_ui.pressureHpa.val);
  gfxTextBig(0, 44, v);
  gfxText(0, 60, "hPa");
}
extern const ScreenDef SCREEN_PRESSURE_DEF = { "Pressione", avail, draw };
#endif
