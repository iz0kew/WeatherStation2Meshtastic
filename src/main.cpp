// ============================================================================
// main.cpp — WeatherStation2Meshtastic
//   Gateway sensori meteo 868 MHz (GFSK) -> rete Meshtastic (LoRa).
//   Schede: Heltec WiFi LoRa 32 V3 / V4 (ESP32-S3 + SX1262 + OLED).
//
// Architettura: parser -> SensorReading (capability bitmask) -> consumatori
// (UI + mesh). Il dispatch cicla SOLO i driver abilitati in questa build
// (registry, popolato da generated_config.h) e per ogni frame esegue:
//   match -> parse -> uiSubmit -> meshSubmit
//
// La radio e' una sola: RX GFSK per i sensori, transitoria in LoRa-TX per
// Meshtastic e in LoRa-RX per la finestra di sincronizzazione orario.
// ============================================================================
#include <Arduino.h>
#include "board_config.h"
#include "user_config.h"
#include "config/generated_config.h"
#include "radio/radio.h"
#include "sensors/registry.h"
#include "sensors/sensor_types.h"
#include "display/display.h"
#include "display/ui.h"
#include "mesh/meshtastic_pack.h"
#include "timesync.h"
#include "astro.h"
#include "history.h"
#include <time.h>

History  history;
uint32_t g_fskOk = 0, g_fskBad = 0;
uint32_t g_nextNodeInfoMs  = 0;
uint32_t g_nextTelemetryMs = 0;
uint32_t g_nextPositionMs  = 0;

// ---------------------------------------------------------------------------
// Campionatura periodica per grafici e finestre pioggia (1h/24h)
// ---------------------------------------------------------------------------
static void sampleHistory() {
  const uint32_t STALE = UI_STALE_MS;
  uint32_t now = millis();
  HistSample s;
  s.ms      = now;
  s.t10     = uiFieldFresh(g_ui.tempC, now)    ? (int16_t)lroundf(g_ui.tempC.val * 10) : INT16_MIN;
  s.rh      = uiFieldFresh(g_ui.humidity, now) ? (int16_t)g_ui.humidity.val : -1;
  s.rain10  = uiFieldFresh(g_ui.rainMm, now)   ? (int32_t)lroundf(g_ui.rainMm.val * 10) : -1;
  s.strikes = g_ui.haveLightning ? (int32_t)g_ui.strikeTotal : -1;
  (void)STALE;
  history.add(s);
}

#if MESH_ENABLED
// ---------------------------------------------------------------------------
// Allarme fulmini: score = nuovi_fulmini_nella_finestra / distanza_km
// ---------------------------------------------------------------------------
#if MESH_LIGHTNING_TEXT
namespace {
  struct LightEntry { uint32_t ms; uint32_t strikes; };
  const uint8_t LWIN_SIZE = 16;
  LightEntry lwin[LWIN_SIZE];
  uint8_t    lwinHead = 0, lwinCount = 0;
  uint32_t   lwinPrevSeen = 0, lastAlertMs = 0, lastAlertStrikes = 0;
}
static void checkLightningAlert(uint32_t now) {
  if (!g_ui.haveLightning || g_ui.lastDistKm == 63) return;
  if (g_ui.lightningLastSeen != lwinPrevSeen) {
    lwinPrevSeen   = g_ui.lightningLastSeen;
    lwin[lwinHead] = { now, g_ui.strikeTotal };
    lwinHead = (lwinHead + 1) % LWIN_SIZE;
    if (lwinCount < LWIN_SIZE) lwinCount++;
  }
  if (lwinCount == 0) return;
  if (now - lastAlertMs < (uint32_t)MESH_LIGHTNING_WINDOW_MIN * 60000UL) return;

  const uint32_t windowMs = (uint32_t)MESH_LIGHTNING_WINDOW_MIN * 60000UL;
  uint32_t oldest = g_ui.strikeTotal;
  for (uint8_t i = 0; i < lwinCount; i++) {
    uint8_t idx = (lwinHead - 1 - i + LWIN_SIZE) % LWIN_SIZE;
    if (now - lwin[idx].ms <= windowMs) oldest = lwin[idx].strikes; else break;
  }
  uint32_t delta = (g_ui.strikeTotal >= oldest) ? g_ui.strikeTotal - oldest : 0;
  if (delta == 0) return;
  float score = (float)delta / (float)g_ui.lastDistKm;
  if (score < MESH_LIGHTNING_THRESHOLD) return;
  if (g_ui.strikeTotal <= lastAlertStrikes) return;

  lastAlertMs = now; lastAlertStrikes = g_ui.strikeTotal;
  char msg[96];
  snprintf(msg, sizeof(msg), "⚡ " MESH_LONG_NAME "\n%lu fulmini  ~%u km",
           (unsigned long)delta, (unsigned)g_ui.lastDistKm);
  meshSendText(msg);   // canale testo (default)
  Serial.printf("[mesh] allarme fulmini score=%.2f delta=%lu @%ukm\n",
                (double)score, (unsigned long)delta, (unsigned)g_ui.lastDistKm);
}
#endif // MESH_LIGHTNING_TEXT

// ---------------------------------------------------------------------------
// Bollettino meteo testuale composto dai dati recenti in g_ui + astro.
// ---------------------------------------------------------------------------
static void meshSendWeatherText(uint8_t chanIdx) {
  uint32_t now = millis();
  char msg[240];
  size_t n = 0;
  bool hasData = false;
  n += snprintf(msg + n, sizeof(msg) - n, MESH_LONG_NAME "\n");

  if (uiFieldFresh(g_ui.tempC, now)) {
    n += snprintf(msg + n, sizeof(msg) - n, "🌡️ %.1f°C", g_ui.tempC.val);
    if (uiFieldFresh(g_ui.humidity, now))
      n += snprintf(msg + n, sizeof(msg) - n, "  💧 %u%%", (unsigned)(g_ui.humidity.val + 0.5f));
    n += snprintf(msg + n, sizeof(msg) - n, "\n");
    hasData = true;
  }
  if (uiFieldFresh(g_ui.pressureHpa, now)) {
    n += snprintf(msg + n, sizeof(msg) - n, "🔆 %.0f hPa\n", g_ui.pressureHpa.val);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.rainMm, now)) {
    float r1  = history.rainDeltaMm(g_ui.rainMm.val, 3600UL * 1000UL);
    float r24 = history.rainDeltaMm(g_ui.rainMm.val, 24UL * 3600UL * 1000UL);
    n += snprintf(msg + n, sizeof(msg) - n, "🌧️ 1h %.1fmm  24h %.1fmm\n", r1, r24);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.windAvg, now)) {
    n += snprintf(msg + n, sizeof(msg) - n, "💨 %.1f m/s\n", g_ui.windAvg.val);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.uv, now)) {
    n += snprintf(msg + n, sizeof(msg) - n, "☀️ UV %.1f\n", g_ui.uv.val);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.soil, now)) {
    n += snprintf(msg + n, sizeof(msg) - n, "🌱 %u%%\n", (unsigned)(g_ui.soil.val + 0.5f));
    hasData = true;
  }
  if (uiFieldFresh(g_ui.pm25, now) || uiFieldFresh(g_ui.co2, now)) {
    n += snprintf(msg + n, sizeof(msg) - n, "🏭");
    if (uiFieldFresh(g_ui.pm25, now)) n += snprintf(msg + n, sizeof(msg) - n, " PM2.5 %.0f", g_ui.pm25.val);
    if (uiFieldFresh(g_ui.co2, now))  n += snprintf(msg + n, sizeof(msg) - n, " CO2 %.0f", g_ui.co2.val);
    n += snprintf(msg + n, sizeof(msg) - n, "\n");
    hasData = true;
  }
  if (g_ui.haveLightning && g_ui.strikeTotal > 0) {
    if (g_ui.lastDistKm != 63)
      n += snprintf(msg + n, sizeof(msg) - n, "⚡ %lu fulmini  ~%u km\n",
                    (unsigned long)g_ui.strikeTotal, g_ui.lastDistKm);
    else
      n += snprintf(msg + n, sizeof(msg) - n, "⚡ %lu fulmini\n",
                    (unsigned long)g_ui.strikeTotal);
    hasData = true;
  }
  if (timeSyncValid()) {
    SunTimes sun;
    if (astroGetSunTimes(sun))
      n += snprintf(msg + n, sizeof(msg) - n, "🌅 %02d:%02d  🌇 %02d:%02d\n",
                    sun.riseH, sun.riseM, sun.setH, sun.setM);
    int illum = (int)(astroMoonIllum() * 100.0f + 0.5f);
    n += snprintf(msg + n, sizeof(msg) - n, "%s %s  %d%%",
                  astroMoonPhaseEmoji(), astroMoonPhaseName(), illum);
    hasData = true;
  }
  n += snprintf(msg + n, sizeof(msg) - n,
                "\n📡 Vuoi la tua stazione meteo sulla mesh? urly.it/31g1b7");
  if (!hasData) return;
  meshSendText(msg, chanIdx);
}

// ---------------------------------------------------------------------------
// Scheduler astronomico: alba+1h, mezzogiorno, tramonto-1h -> bollettino su ch0
// ---------------------------------------------------------------------------
namespace {
  struct AstroSchedule { time_t t[3]; bool sent[3]; int lastDay; };
  AstroSchedule g_asch = { {0,0,0}, {false,false,false}, -1 };
}
static void updateAstroSchedule(const struct tm &lt) {
  if (lt.tm_yday == g_asch.lastDay) return;
  g_asch.lastDay = lt.tm_yday;
  g_asch.sent[0] = g_asch.sent[1] = g_asch.sent[2] = false;

  SunTimes sun;
  if (!astroGetSunTimes(sun)) { g_asch.t[0] = g_asch.t[1] = g_asch.t[2] = 0; return; }

  struct tm base = lt;
  base.tm_hour = 0; base.tm_min = 0; base.tm_sec = 0;
  time_t midnight = mktime(&base);
  g_asch.t[0] = midnight + (time_t)(sun.riseH * 3600 + sun.riseM * 60) + 3600; // alba+1h
  g_asch.t[1] = midnight + 12 * 3600;                                           // mezzogiorno
  g_asch.t[2] = midnight + (time_t)(sun.setH * 3600 + sun.setM * 60) - 3600;    // tramonto-1h

  time_t nowT = time(nullptr);
  for (int i = 0; i < 3; i++)
    if (g_asch.t[i] > 0 && nowT >= g_asch.t[i]) g_asch.sent[i] = true;
}
static void checkAstroSend() {
  if (!timeSyncValid()) return;
  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);
  updateAstroSchedule(lt);
  for (int i = 0; i < 3; i++) {
    if (!g_asch.sent[i] && g_asch.t[i] > 0 && now >= g_asch.t[i]) {
      g_asch.sent[i] = true;
      meshSendWeatherText(0);   // canale principale (MediumFast)
      Serial.println("[astro] bollettino inviato su ch0");
      break;
    }
  }
}
#endif // MESH_ENABLED

// ---------------------------------------------------------------------------
static void dispatchFrame(const uint8_t *buf, size_t len, float rssi) {
  for (size_t i = 0; i < g_sensorDriverCount; i++) {
    const SensorDriver *d = g_sensorDrivers[i];
    SensorReading r;                       // fresh per tentativo (no contaminazione)
    if (d->match(buf, len) && d->parse(buf, len, rssi, r)) {
      uiSubmit(r);
#if MESH_ENABLED
      meshSubmit(r);
#if MESH_LIGHTNING_TEXT
      if (r.caps & CAP_LIGHTNING) checkLightningAlert(millis());
#endif
#endif
      g_fskOk++;
      return;
    }
  }
  g_fskBad++;
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\nWeatherStation2Meshtastic v%s - %s\n", FW_VERSION, BOARD_NAME);
  Serial.printf("Gruppo radio: %d bps, sync 0x%04X, %u driver attivi\n",
                RADIO_BITRATE_BPS, RADIO_SYNCWORD, (unsigned)g_sensorDriverCount);

  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, VEXT_ON_LEVEL);
  delay(50);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  gfxInit();
  uiInit();
  gfxClear();
  gfxText(0, 12, "Init SX1262...");
  gfxFlush();

  radioInit();

#if MESH_ENABLED
  meshInit();
  uint32_t firstTx  = millis() + TSYNC_WINDOW_MS + 30000UL;
  g_nextNodeInfoMs  = firstTx;
  g_nextTelemetryMs = firstTx + 30000UL;
  g_nextPositionMs  = firstTx + 60000UL;
#endif

  timeSyncBegin();    // radio -> LoRa RX per 5 min
  uiDraw();
  Serial.println("Finestra time-sync aperta (5 min) - premi PRG per navigare");
}

// ---------------------------------------------------------------------------
void loop() {
  uint32_t now = millis();

  // --- Fase sincronizzazione orario (radio in LoRa RX) --------------------
  if (!timeSyncDone()) {
    timeSyncTick();

    static bool btnPrev = HIGH; static uint32_t btnLast = 0;
    bool btn = digitalRead(PIN_BUTTON);
    if (btn != btnPrev && now - btnLast > 60) {
      btnLast = now; btnPrev = btn;
      if (btn == LOW) { uiNextScreen(); uiDraw(); }
    }
    static uint32_t lastDraw = 0;
    if (now - lastDraw > 1000) { lastDraw = now; uiDraw(); }
    return;
  }

  // --- Prima uscita dalla sync: avvia la ricezione FSK sensori ------------
  static bool fskStarted = false;
  if (!fskStarted) {
    fskStarted = true;
    radioStartFSK();
    Serial.printf("In ascolto FSK su %.2f MHz @ %.2f kbps (gruppo %d bps)\n",
                  (double)RX_FREQ_MHZ, (double)(RADIO_BITRATE_BPS / 1000.0f), RADIO_BITRATE_BPS);
  }

  // --- Ricezione FSK ------------------------------------------------------
  if (rxFlag) {
    rxFlag = false;
    uint8_t buf[RADIO_FSK_PKT_LEN];
    float rssi = 0;
    int st = radioReadFSK(buf, &rssi);
    if (st == RADIOLIB_ERR_NONE) {
      Serial.print("[RX] ");
      for (int i = 0; i < RADIO_FSK_PKT_LEN; i++) Serial.printf("%02X ", buf[i]);
      Serial.printf(" (RSSI %.0f dBm)\n", rssi);
      dispatchFrame(buf, RADIO_FSK_PKT_LEN, rssi);
    }
    uiDraw();
  }

  // --- Tasto PRG ----------------------------------------------------------
  static bool btnPrev = HIGH; static uint32_t btnLast = 0;
  bool btn = digitalRead(PIN_BUTTON);
  if (btn != btnPrev && now - btnLast > 60) {
    btnLast = now; btnPrev = btn;
    if (btn == LOW) { uiNextScreen(); uiDraw(); }
  }

  // --- Campionatura storico ----------------------------------------------
  static uint32_t nextSample = 0;
  if (now >= nextSample) {
    nextSample = now + (uint32_t)HISTORY_SAMPLE_MIN * 60000UL;
    sampleHistory();
  }

#if MESH_ENABLED
  if (g_nextNodeInfoMs && (int32_t)(now - g_nextNodeInfoMs) >= 0) {
    g_nextNodeInfoMs += (uint32_t)MESH_SEND_INTERVAL_MIN * 60000UL;
    meshSendNodeInfo();
  }
  if (g_nextTelemetryMs && (int32_t)(now - g_nextTelemetryMs) >= 0) {
    g_nextTelemetryMs += (uint32_t)MESH_SEND_INTERVAL_MIN * 60000UL;
    meshPeriodicSend();
  }
#if MESH_POS_ENABLED
  if (g_nextPositionMs && (int32_t)(now - g_nextPositionMs) >= 0) {
    g_nextPositionMs += (uint32_t)MESH_SEND_INTERVAL_MIN * 60000UL;
    meshSendPosition();
  }
#endif

  checkAstroSend();   // bollettini astronomici su ch0

#if MESH_TEXT_INTERVAL_MIN > 0
  static uint32_t nextTextMs = (uint32_t)MESH_TEXT_INTERVAL_MIN * 60000UL;
  if ((int32_t)(now - nextTextMs) >= 0) {
    nextTextMs = now + (uint32_t)MESH_TEXT_INTERVAL_MIN * 60000UL;
    meshSendWeatherText(1);   // canale testo
  }
#endif
#endif // MESH_ENABLED

  // --- Refresh display ----------------------------------------------------
  static uint32_t lastDraw = 0;
  if (now - lastDraw > 1000) { lastDraw = now; uiDraw(); }
}
