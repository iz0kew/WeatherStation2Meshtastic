// screen_leak.cpp — schermata Perdita acqua (CAP_LEAK)
#include "../../config/generated_config.h"
#ifdef SCREEN_LEAK
#include "../ui.h"
#include "../display.h"

static bool avail(uint32_t now) { return uiFieldFresh(g_ui.leak, now); }
static void draw() {
  uiHeaderSensor("Perdita", g_ui.leak.model, g_ui.leak.id);
  if (g_ui.leak.val > 0.5f) {
    gfxTextBig(0, 44, "ALLARME");
    gfxText(0, 60, "Perdita rilevata!");
  } else {
    gfxText(0, 36, "Nessuna perdita");
  }
}
extern const ScreenDef SCREEN_LEAK_DEF = { "Perdita", avail, draw };
#endif
