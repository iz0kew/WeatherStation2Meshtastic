// ============================================================================
// ui.cpp — store aggregato + rotazione schermate.
// ============================================================================
#include "ui.h"
#include "display.h"
#include "../config/generated_config.h"
#include "../mesh/meshtastic_pack.h"
#include "../timesync.h"
#include "../astro.h"
#include "../history.h"
#include "user_config.h"
#include <time.h>

UiStore g_ui;

bool uiFieldFresh(const UiField &f, uint32_t now) {
  return f.valid && (uint32_t)(now - f.lastSeen) < UI_STALE_MS;
}

static void setF(UiField &f, float v, uint32_t now, const char *model, uint32_t id) {
  f.valid = true; f.val = v; f.lastSeen = now; f.model = model; f.id = id;
}

// Intestazione: "Titolo" a sinistra, tag sensore a destra + riga separatrice.
void uiHeader(const char *title, const char *tag) {
  gfxText(0, 10, title);
  if (tag && *tag) gfxText(GFX_W - gfxTextWidth(tag), 10, tag);
  gfxLine(0, 13, GFX_W - 1, 13);
}

// Tag = "MODELLO IDhex" (l'ID distingue due sensori dello stesso modello).
void uiHeaderSensor(const char *title, const char *model, uint32_t id) {
  char tag[28];
  if (model && *model) snprintf(tag, sizeof(tag), "%s %lX", model, (unsigned long)id);
  else tag[0] = 0;
  uiHeader(title, tag);
}

// --- Grafici 24h -----------------------------------------------------------
int uiGraphCount(HistGetFn get) {
  uint16_t n = history.count();
  int c = 0;
  for (uint16_t i = 0; i < n; i++) { bool ok; (void)get(history.get(i), ok); if (ok) c++; }
  return c;
}

void uiDrawGraph(const char *title, const char *tag, HistGetFn get, const char *valFmt) {
  uiHeader(title, tag);
  uint16_t n = history.count();

  float mn = 1e9f, mx = -1e9f, last = 0;
  int valid = 0;
  for (uint16_t i = 0; i < n; i++) {
    bool ok; float v = get(history.get(i), ok);
    if (!ok) continue;
    valid++; if (v < mn) mn = v; if (v > mx) mx = v; last = v;
  }
  if (valid < 2) { gfxText(0, 38, "Dati insufficienti"); return; }
  if (mx - mn < 1e-3f) { mx += 0.5f; mn -= 0.5f; }

  const int X0 = 0, X1 = GFX_W - 1, Y0 = 17, Y1 = 51;   // area di plot
  gfxLine(X0, Y1 + 1, X1, Y1 + 1);                        // asse orizzontale

  int prevx = -1, prevy = 0;
  for (uint16_t i = 0; i < n; i++) {
    bool ok; float v = get(history.get(i), ok);
    if (!ok) continue;
    int x = (n > 1) ? X0 + (int)((long)(X1 - X0) * i / (n - 1)) : X0;
    int y = Y1 - (int)((v - mn) / (mx - mn) * (Y1 - Y0));
    if (prevx >= 0) gfxLine(prevx, prevy, x, y); else gfxPixel(x, y);
    prevx = x; prevy = y;
  }

  char a[8], b[8], c[8], lbl[28];
  snprintf(a, sizeof(a), valFmt, mn);
  snprintf(b, sizeof(b), valFmt, mx);
  snprintf(c, sizeof(c), valFmt, last);
  snprintf(lbl, sizeof(lbl), "%s/%s ora %s", a, b, c);
  gfxText(0, 63, lbl);
}

// ---------------------------------------------------------------------------
void uiInit() {}

void uiSubmit(const SensorReading &r) {
  uint32_t now = millis();
  g_ui.lastRssi    = r.rssi;
  g_ui.battLow     = r.battLow;
  g_ui.lastAnySeen = now;
  strncpy(g_ui.lastModel, r.model, sizeof(g_ui.lastModel) - 1);

  if (r.caps & CAP_TEMP)     setF(g_ui.tempC, r.tempC, now, r.model, r.id);
  if (r.caps & CAP_HUM)      setF(g_ui.humidity, r.humidity, now, r.model, r.id);
  if (r.caps & CAP_PRESSURE) setF(g_ui.pressureHpa, r.pressureHpa, now, r.model, r.id);
  if (r.caps & CAP_RAIN)     setF(g_ui.rainMm, r.rainMm, now, r.model, r.id);
  if (r.caps & CAP_WIND) {
    setF(g_ui.windAvg, r.windAvgMs, now, r.model, r.id);
    setF(g_ui.windGust, r.windGustMs, now, r.model, r.id);
    setF(g_ui.windDir, r.windDirDeg, now, r.model, r.id);
  }
  if (r.caps & CAP_UV) {
    setF(g_ui.uv, r.uvIndex, now, r.model, r.id);
    setF(g_ui.lux, r.lightLux, now, r.model, r.id);
  }
  if (r.caps & CAP_SOIL) setF(g_ui.soil, r.soilMoisture, now, r.model, r.id);
  if (r.caps & CAP_PM) { setF(g_ui.pm25, r.pm25, now, r.model, r.id); setF(g_ui.pm10, r.pm10, now, r.model, r.id); }
  if (r.caps & CAP_CO2)  setF(g_ui.co2, r.co2, now, r.model, r.id);
  if (r.caps & CAP_LEAK) setF(g_ui.leak, r.leak ? 1 : 0, now, r.model, r.id);

  if (r.caps & CAP_LIGHTNING) {
    g_ui.haveLightning     = true;
    g_ui.lightningLastSeen = now;
    g_ui.lightningModel    = r.model;
    g_ui.lightningId       = r.id;
    if (r.lightningDistKm != 63) g_ui.lastDistKm = r.lightningDistKm;
    if (r.lightningCountValid) {
      if (g_ui.lightningHaveCount) {
        uint32_t c = r.lightningCount, last = g_ui.lightningLastCount, delta;
        if (c < 256 && last < 256) delta = (uint8_t)(c - last);   // wrap 8 bit (WH57)
        else                       delta = (c >= last) ? (c - last) : c;
        if (delta > 0 && delta < 1000) g_ui.strikeTotal += delta;
      }
      g_ui.lightningLastCount = r.lightningCount;
      g_ui.lightningHaveCount = true;
    }
  }
}

// ===========================================================================
// Schermate di sistema (sempre presenti)
// ===========================================================================
static bool always(uint32_t) { return true; }

static void scrOverviewDraw() {
  uint32_t now = millis();
  uiHeader("Panoramica", "v" FW_VERSION);   // versione firmware (non il modello sensore)
  char line[40];
  int y = 26;
  if (uiFieldFresh(g_ui.tempC, now)) {
    snprintf(line, sizeof(line), "T %.1f C  RH %.0f%%",
             g_ui.tempC.val, uiFieldFresh(g_ui.humidity, now) ? g_ui.humidity.val : 0);
    gfxText(0, y, line); y += 13;
  }
  if (uiFieldFresh(g_ui.pressureHpa, now)) {
    snprintf(line, sizeof(line), "P %.0f hPa", g_ui.pressureHpa.val); gfxText(0, y, line); y += 13;
  }
  if (uiFieldFresh(g_ui.rainMm, now)) {
    float r1 = history.rainDeltaMm(g_ui.rainMm.val, 3600UL * 1000UL);
    snprintf(line, sizeof(line), "Pioggia 1h %.1f mm", r1); gfxText(0, y, line); y += 13;
  }
  if (uiFieldFresh(g_ui.windAvg, now)) {
    snprintf(line, sizeof(line), "Vento %.1f m/s", g_ui.windAvg.val); gfxText(0, y, line); y += 13;
  }
  if (g_ui.lastAnySeen == 0) gfxText(0, 40, "In attesa sensori...");
}

// pianificazione prossima telemetria (definita in main.cpp)
extern uint32_t g_nextTelemetryMs;

static void scrMeshDraw() {
  uiHeader("Meshtastic", meshShortName());   // short name del nodo in alto a destra
  char line[40];
  uint32_t now = millis();

  snprintf(line, sizeof(line), "Nodo !%08lx", (unsigned long)meshNodeId());
  gfxText(0, 25, line);

  // conto alla rovescia alla prossima trasmissione dati (telemetria)
  if (g_nextTelemetryMs) {
    int32_t rem = (int32_t)(g_nextTelemetryMs - now);
    if (rem < 0) rem = 0;
    int s = rem / 1000;
    snprintf(line, sizeof(line), "Prossimo TX %02d:%02d", s / 60, s % 60);
  } else {
    snprintf(line, sizeof(line), "TX disabilitato");
  }
  gfxText(0, 38, line);

  snprintf(line, sizeof(line), "TX %lu  RSSI %.0f", (unsigned long)meshPacketsSent(), g_ui.lastRssi);
  gfxText(0, 51, line);
}

static void scrTimeDraw() {
  uiHeader("Orario", "");
  char line[40];
  if (timeSyncValid()) {
    static const char *const GIORNI[] = {"Dom","Lun","Mar","Mer","Gio","Ven","Sab"};
    time_t now = time(nullptr);
    struct tm lt; localtime_r(&now, &lt);
    snprintf(line, sizeof(line), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
    gfxTextBig(8, 40, line);
    snprintf(line, sizeof(line), "%s %02d/%02d/%04d",
             GIORNI[lt.tm_wday % 7], lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900);
    gfxText((GFX_W - gfxTextWidth(line)) / 2, 60, line);
  } else {
    TimeSyncStatus s = timeSyncGetStatus();
    if (s.state == TS_TIMEOUT) gfxText(0, 30, "Sync fallita");
    else {
      snprintf(line, sizeof(line), "Sync... %lds", (long)s.secsLeft);
      gfxText(0, 30, line);
      snprintf(line, sizeof(line), "conferme %u/%d", s.confirms, TSYNC_CONFIRM_MIN);
      gfxText(0, 45, line);
    }
  }
}

static void scrAstroDraw() {
  uiHeader("Astro", "");
  char line[40];
  if (!timeSyncValid()) { gfxText(0, 30, "Orario non sync"); return; }
  SunTimes sun;
  int y = 26;
  if (astroGetSunTimes(sun)) {
    snprintf(line, sizeof(line), "Alba   %02d:%02d", sun.riseH, sun.riseM); gfxText(0, y, line); y += 12;
    snprintf(line, sizeof(line), "Tramonto %02d:%02d", sun.setH, sun.setM); gfxText(0, y, line); y += 12;
  }
  snprintf(line, sizeof(line), "Luna %s %.0f%%", astroMoonPhaseShort(), astroMoonIllum() * 100.0f);
  gfxText(0, y, line); y += 12;
  const char *trend = astroMoonTrend();
  if (*trend) gfxText(0, y, trend);   // Crescente / Calante
}

static const ScreenDef SCR_OVERVIEW = { "Panoramica", always, scrOverviewDraw };
static const ScreenDef SCR_MESH     = { "Mesh",       always, scrMeshDraw };
static const ScreenDef SCR_TIME     = { "Orario",     always, scrTimeDraw };
static const ScreenDef SCR_ASTRO    = { "Astro",      always, scrAstroDraw };

// ---- schermate per capability (definite nei file screens/, sotto #ifdef) ---
#ifdef SCREEN_TEMPHUM
extern const ScreenDef SCREEN_TEMPHUM_DEF;
extern const ScreenDef SCREEN_GRAPH_T_DEF;
extern const ScreenDef SCREEN_GRAPH_RH_DEF;
#endif
#ifdef SCREEN_PRESSURE
extern const ScreenDef SCREEN_PRESSURE_DEF;
#endif
#ifdef SCREEN_RAIN
extern const ScreenDef SCREEN_RAIN_DEF;
extern const ScreenDef SCREEN_GRAPH_RAIN_DEF;
#endif
#ifdef SCREEN_WIND
extern const ScreenDef SCREEN_WIND_DEF;
#endif
#ifdef SCREEN_UV
extern const ScreenDef SCREEN_UV_DEF;
#endif
#ifdef SCREEN_LIGHTNING
extern const ScreenDef SCREEN_LIGHTNING_DEF;
extern const ScreenDef SCREEN_GRAPH_LIGHT_DEF;
#endif
#ifdef SCREEN_AQI
extern const ScreenDef SCREEN_AQI_DEF;
#endif
#ifdef SCREEN_SOIL
extern const ScreenDef SCREEN_SOIL_DEF;
#endif
#ifdef SCREEN_LEAK
extern const ScreenDef SCREEN_LEAK_DEF;
#endif

static const ScreenDef *const SCREENS[] = {
  &SCR_OVERVIEW,
#ifdef SCREEN_TEMPHUM
  &SCREEN_TEMPHUM_DEF,
  &SCREEN_GRAPH_T_DEF,
  &SCREEN_GRAPH_RH_DEF,
#endif
#ifdef SCREEN_PRESSURE
  &SCREEN_PRESSURE_DEF,
#endif
#ifdef SCREEN_RAIN
  &SCREEN_RAIN_DEF,
  &SCREEN_GRAPH_RAIN_DEF,
#endif
#ifdef SCREEN_WIND
  &SCREEN_WIND_DEF,
#endif
#ifdef SCREEN_UV
  &SCREEN_UV_DEF,
#endif
#ifdef SCREEN_LIGHTNING
  &SCREEN_LIGHTNING_DEF,
  &SCREEN_GRAPH_LIGHT_DEF,
#endif
#ifdef SCREEN_AQI
  &SCREEN_AQI_DEF,
#endif
#ifdef SCREEN_SOIL
  &SCREEN_SOIL_DEF,
#endif
#ifdef SCREEN_LEAK
  &SCREEN_LEAK_DEF,
#endif
  &SCR_MESH, &SCR_TIME, &SCR_ASTRO,
};
static const size_t N_SCREENS = sizeof(SCREENS) / sizeof(SCREENS[0]);
static size_t s_cur = 0;

void uiNextScreen() {
  uint32_t now = millis();
  for (size_t i = 0; i < N_SCREENS; i++) {
    s_cur = (s_cur + 1) % N_SCREENS;
    if (SCREENS[s_cur]->available(now)) break;
  }
}

void uiDraw() {
  uint32_t now = millis();
  if (!SCREENS[s_cur]->available(now)) uiNextScreen();   // schermata scaduta: avanza
  gfxClear();
  SCREENS[s_cur]->draw();
  gfxFlush();
}
