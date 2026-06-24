// screen_graph_rain.cpp — grafico 24h della pioggia (contatore cumulativo) (CAP_RAIN)
#include "../../config/generated_config.h"
#ifdef SCREEN_RAIN
#include "../ui.h"

static float getR(const HistSample &s, bool &ok) { ok = (s.rain10 >= 0); return s.rain10 / 10.0f; }
static bool  avail(uint32_t) { return uiGraphCount(getR) >= 2; }
static void  draw()          { uiDrawGraph("Pioggia 24h", g_ui.rainMm.model, getR, "%.1f"); }

extern const ScreenDef SCREEN_GRAPH_RAIN_DEF = { "Pioggia24h", avail, draw };
#endif
