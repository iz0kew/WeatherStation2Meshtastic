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
#include "led_status.h"
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
  char msg[128];
  size_t mn = snprintf(msg, sizeof(msg), "⚡ " MESH_LONG_NAME "\n%lu fulmini  ~%u km",
                       (unsigned long)delta, (unsigned)g_ui.lastDistKm);
  if (timeSyncValid() && mn < sizeof(msg)) {
    time_t tt = time(nullptr);
    struct tm lt; localtime_r(&tt, &lt);
    snprintf(msg + mn, sizeof(msg) - mn, "\n📅 %02d/%02d/%04d  🕒 %02d:%02d",
             lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900, lt.tm_hour, lt.tm_min);
  }
  meshSendText(msg);   // canale testo (default)
  Serial.printf("[mesh] allarme fulmini score=%.2f delta=%lu @%ukm\n",
                (double)score, (unsigned long)delta, (unsigned)g_ui.lastDistKm);
}
#endif // MESH_LIGHTNING_TEXT

// ---------------------------------------------------------------------------
// Il pacchetto Meshtastic aggiunge al testo l'overhead del wrapper protobuf
// (portnum + bytes-field + eventuale bit ok_to_mqtt: fino a ~7 byte, vedi
// sendData()/meshTransmit() in meshtastic_pack.cpp, che scartano in
// silenzio tutto cio' che supera i 200 byte). Restiamo sotto questo margine
// con un cuscinetto di sicurezza.
#define WEATHER_MSG_BUDGET 190

// Aggiunge una riga gia' formattata al bollettino SOLO se rientra nel
// budget residuo; se non ci sta la scarta per intero (mai spezzata a
// meta'). Con molti sensori attivi insieme non tutto entra in un unico
// pacchetto di testo: a corto di spazio si sacrificano per prime le righe
// meno essenziali (astro, poi data/ora, poi il link informativo — vedi
// l'ordine delle chiamate sotto), mai i dati dei sensori.
static void appendIfFits(char *msg, size_t &n, const char *line) {
  size_t len = strlen(line);
  if (n + len > WEATHER_MSG_BUDGET) return;
  memcpy(msg + n, line, len);
  n += len;
}

// ---------------------------------------------------------------------------
// Bollettino meteo testuale composto dai dati recenti in g_ui + astro.
// Non-static: richiamata anche dal menu di invio manuale (display/ui.cpp).
// Ritorna true se il messaggio e' stato effettivamente inviato.
// ---------------------------------------------------------------------------
bool meshSendWeatherText(uint8_t chanIdx) {
  uint32_t now = millis();
  char msg[240];
  size_t n = 0;
  bool hasData = false;
  n += snprintf(msg + n, sizeof(msg) - n, MESH_LONG_NAME "\n");

  char line[96];
  size_t ln;

  if (uiFieldFresh(g_ui.tempC, now)) {
    ln = (size_t)snprintf(line, sizeof(line), "🌡️ %.1f°C", g_ui.tempC.val);
    if (uiFieldFresh(g_ui.humidity, now))
      ln += (size_t)snprintf(line + ln, sizeof(line) - ln, "  💧 %u%%", (unsigned)(g_ui.humidity.val + 0.5f));
    snprintf(line + ln, sizeof(line) - ln, "\n");
    appendIfFits(msg, n, line);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.pressureHpa, now)) {
    snprintf(line, sizeof(line), "🔆 %.0f hPa\n", g_ui.pressureHpa.val);
    appendIfFits(msg, n, line);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.rainMm, now)) {
    float r1  = history.rainDeltaMm(g_ui.rainMm.val, 3600UL * 1000UL);
    float r24 = history.rainDeltaMm(g_ui.rainMm.val, 24UL * 3600UL * 1000UL);
    snprintf(line, sizeof(line), "🌧️ 1h %.1fmm  24h %.1fmm\n", r1, r24);
    appendIfFits(msg, n, line);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.windAvg, now)) {
    snprintf(line, sizeof(line), "💨 %.1f m/s\n", g_ui.windAvg.val);
    appendIfFits(msg, n, line);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.uv, now)) {
    snprintf(line, sizeof(line), "☀️ UV %.1f\n", g_ui.uv.val);
    appendIfFits(msg, n, line);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.soil, now)) {
    snprintf(line, sizeof(line), "🌱 %u%%\n", (unsigned)(g_ui.soil.val + 0.5f));
    appendIfFits(msg, n, line);
    hasData = true;
  }
  if (uiFieldFresh(g_ui.pm25, now) || uiFieldFresh(g_ui.co2, now)) {
    ln = (size_t)snprintf(line, sizeof(line), "🏭");
    if (uiFieldFresh(g_ui.pm25, now)) ln += (size_t)snprintf(line + ln, sizeof(line) - ln, " PM2.5 %.0f", g_ui.pm25.val);
    if (uiFieldFresh(g_ui.co2, now))  ln += (size_t)snprintf(line + ln, sizeof(line) - ln, " CO2 %.0f", g_ui.co2.val);
    snprintf(line + ln, sizeof(line) - ln, "\n");
    appendIfFits(msg, n, line);
    hasData = true;
  }
  if (g_ui.haveLightning && g_ui.strikeTotal > 0) {
    if (g_ui.lastDistKm != 63)
      snprintf(line, sizeof(line), "⚡ %lu fulmini  ~%u km\n",
               (unsigned long)g_ui.strikeTotal, g_ui.lastDistKm);
    else
      snprintf(line, sizeof(line), "⚡ %lu fulmini\n", (unsigned long)g_ui.strikeTotal);
    appendIfFits(msg, n, line);
    hasData = true;
  }
  // Astro e data/ora: informativi ma meno essenziali dei dati sensore, per
  // questo appesi in fondo all'ordine di priorita' del budget. Fase lunare
  // ridotta a emoji+percentuale (niente nome esteso) per restare compatta.
  if (timeSyncValid()) {
    ln = 0;
    SunTimes sun;
    if (astroGetSunTimes(sun))
      ln += (size_t)snprintf(line + ln, sizeof(line) - ln, "🌅 %02d:%02d  🌇 %02d:%02d\n",
                              sun.riseH, sun.riseM, sun.setH, sun.setM);
    int illum = (int)(astroMoonIllum() * 100.0f + 0.5f);
    snprintf(line + ln, sizeof(line) - ln, "%s %d%%\n", astroMoonPhaseEmoji(), illum);
    appendIfFits(msg, n, line);
    hasData = true;
  }
  // Data e orario locale di invio: il primo a saltare se lo spazio scarseggia.
  if (timeSyncValid()) {
    time_t tt = time(nullptr);
    struct tm lt; localtime_r(&tt, &lt);
    snprintf(line, sizeof(line), "📅 %02d/%02d  🕒 %02d:%02d\n",
             lt.tm_mday, lt.tm_mon + 1, lt.tm_hour, lt.tm_min);
    appendIfFits(msg, n, line);
  }
  snprintf(line, sizeof(line), "\n📡 urly.it/31g1b7");
  appendIfFits(msg, n, line);

  if (!hasData) return false;
  return meshSendText(msg, chanIdx);
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
// Tasto PRG: pressione breve = schermata successiva / navigazione menu;
// oltre 700 ms = apertura/selezione nel menu (scatta senza attendere il
// rilascio). Debounce: rilasci sotto i 60 ms vengono ignorati.
// Disponibile solo sulle schede che definiscono PIN_BUTTON (Heltec V3/V4):
// il kit XIAO non espone un tasto utente generico, quindi la navigazione
// schermate e il menu di invio manuale (irrilevanti senza display) restano
// semplicemente irraggiungibili, senza bisogno di toccare ui.cpp.
// ---------------------------------------------------------------------------
#ifdef PIN_BUTTON
static void pollButton(uint32_t now) {
  static bool     prev      = HIGH;
  static uint32_t tPress    = 0;
  static bool     longFired = false;
  bool b = digitalRead(PIN_BUTTON);
  if (b == LOW) {
    if (prev == HIGH) { tPress = now; longFired = false; }
    else if (!longFired && now - tPress >= 700) {
      longFired = true;
      uiButton(true); uiDraw();
    }
  } else if (prev == LOW) {
    if (!longFired && now - tPress >= 60) { uiButton(false); uiDraw(); }
  }
  prev = b;
}
#endif

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

#ifdef PIN_VEXT
  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, VEXT_ON_LEVEL);
  delay(50);
#endif
#ifdef PIN_BUTTON
  pinMode(PIN_BUTTON, INPUT_PULLUP);
#endif
#ifdef HAS_STATUS_LED
  ledStatusInit();
#endif

  gfxInit();
  uiInit();
#ifdef HAS_OLED
  uiSplash();
  delay(5000);
  gfxClear();
  gfxText(0, 12, "Init SX1262...");
  gfxFlush();
#endif

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
#ifdef PIN_BUTTON
  Serial.println("Finestra time-sync aperta (5 min) - premi PRG per navigare");
#else
  Serial.println("Finestra time-sync aperta (5 min)");
#endif
}

// ---------------------------------------------------------------------------
void loop() {
  uint32_t now = millis();

#ifdef HAS_STATUS_LED
  // Chiamata ad ogni giro (non solo durante la sync): il modulo gestisce da
  // solo lo spegnimento qualche secondo dopo l'esito, quindi deve poter
  // essere richiamato anche a sync ormai conclusa.
  ledStatusTick();
#endif

  // --- Fase sincronizzazione orario (radio in LoRa RX) --------------------
  if (!timeSyncDone()) {
    timeSyncTick();

#ifdef PIN_BUTTON
    pollButton(now);
#endif
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
#ifdef PIN_BUTTON
  pollButton(now);
#endif

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
