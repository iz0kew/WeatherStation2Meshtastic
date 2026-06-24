// screen_graph_lightning.cpp — grafico 24h dei fulmini (totale cumulativo) (CAP_LIGHTNING)
#include "../../config/generated_config.h"
#ifdef SCREEN_LIGHTNING
#include "../ui.h"

static float getL(const HistSample &s, bool &ok) { ok = (s.strikes >= 0); return (float)s.strikes; }
static bool  avail(uint32_t) { return uiGraphCount(getL) >= 2; }
static void  draw()          { uiDrawGraph("Fulmini 24h", g_ui.lightningModel, getL, "%.0f"); }

extern const ScreenDef SCREEN_GRAPH_LIGHT_DEF = { "Fulmini24h", avail, draw };
#endif
