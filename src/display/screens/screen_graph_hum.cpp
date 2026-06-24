// screen_graph_hum.cpp — grafico 24h dell'umidita' (CAP_HUM)
#include "../../config/generated_config.h"
#ifdef SCREEN_TEMPHUM
#include "../ui.h"

static float getH(const HistSample &s, bool &ok) { ok = (s.rh >= 0); return s.rh; }
static bool  avail(uint32_t) { return uiGraphCount(getH) >= 2; }
static void  draw()          { uiDrawGraph("Umid 24h", g_ui.humidity.model, getH, "%.0f"); }

extern const ScreenDef SCREEN_GRAPH_RH_DEF = { "Umid24h", avail, draw };
#endif
