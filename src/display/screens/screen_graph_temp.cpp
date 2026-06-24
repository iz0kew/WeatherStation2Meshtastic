// screen_graph_temp.cpp — grafico 24h della temperatura (CAP_TEMP)
#include "../../config/generated_config.h"
#ifdef SCREEN_TEMPHUM
#include "../ui.h"

static float getT(const HistSample &s, bool &ok) { ok = (s.t10 != INT16_MIN); return s.t10 / 10.0f; }
static bool  avail(uint32_t) { return uiGraphCount(getT) >= 2; }
static void  draw()          { uiDrawGraph("Temp 24h", g_ui.tempC.model, getT, "%.1f"); }

extern const ScreenDef SCREEN_GRAPH_T_DEF = { "Temp24h", avail, draw };
#endif
