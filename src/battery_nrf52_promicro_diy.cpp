// ============================================================================
// battery_nrf52_promicro_diy.cpp — lettura VBAT su schede DIY nRF52840 in
//   formato Pro-Micro/NiceNano + HT-RA62 (MASN, FakeTec — vedi board_config.h).
//   Il partitore VBAT (VBAT_DIVIDER_COMP) e' confermato per FakeTec dallo
//   schematico ufficiale v5 (R4=10M/R5=10M, vedi board_config.h), resta
//   invece un PLACEHOLDER non verificato per MASN (nessuno schematico reale
//   controllato) — vanno comunque ricalibrati/confermati con un multimetro
//   sull'hardware reale prima di fidarsi ciecamente della percentuale
//   riportata (la curva LiPo sotto e' comunque solo un'approssimazione).
// ============================================================================
#include "battery.h"
#include "board_config.h"
#if defined(BOARD_MASN_HTRA62) || defined(BOARD_FAKETEC)

#define BATT_REFRESH_MS 5000UL

// mV per LSB con ADC a 12 bit e fondo scala 3.0V (riferimento interno
// AR_INTERNAL_3_0): 3000/4096 = 0.732421875.
#define VBAT_MV_PER_LSB   0.732421875f
// Compensazione partitore: FakeTec la definisce in board_config.h (R4=10M/
// R5=10M -> 2.0f, confermato dallo schematico v5). Per MASN resta un
// PLACEHOLDER 1:1 non verificato (nessuno schematico reale controllato):
// da correggere dopo verifica hardware.
#ifndef VBAT_DIVIDER_COMP
#define VBAT_DIVIDER_COMP (1.0f)
#endif

// Curva LiPo semplificata (scarica a riposo), lineare a tratti.
static const struct { float v; int pct; } CURVE[] = {
  {3.30f, 0}, {3.50f, 10}, {3.65f, 25}, {3.75f, 50},
  {3.90f, 75}, {4.05f, 90}, {4.20f, 100},
};
static const int N_CURVE = sizeof(CURVE) / sizeof(CURVE[0]);

static int voltsToPercent(float v) {
  if (v <= CURVE[0].v) return 0;
  if (v >= CURVE[N_CURVE - 1].v) return 100;
  for (int i = 1; i < N_CURVE; i++) {
    if (v < CURVE[i].v) {
      float t = (v - CURVE[i - 1].v) / (CURVE[i].v - CURVE[i - 1].v);
      return CURVE[i - 1].pct + (int)(t * (CURVE[i].pct - CURVE[i - 1].pct) + 0.5f);
    }
  }
  return 100;
}

static float readVolts() {
  static bool inited = false;
  if (!inited) {
    inited = true;
    analogReadResolution(12);
    analogReference(AR_INTERNAL_3_0);
    delay(10);
  }
  uint32_t raw = 0;
  for (int i = 0; i < 8; i++) raw += analogRead(PIN_VBAT_READ);
  return (raw / 8) * VBAT_MV_PER_LSB * VBAT_DIVIDER_COMP / 1000.0f;
}

bool batteryRead(float &volts, int &percent) {
  static uint32_t lastMs = 0;
  static float    cachedV = 0;
  static bool     first = true;

  uint32_t now = millis();
  if (first || (uint32_t)(now - lastMs) >= BATT_REFRESH_MS) {
    first = false; lastMs = now;
    cachedV = readVolts();
  }
  volts = cachedV;
  percent = voltsToPercent(cachedV);
  return cachedV >= 3.0f;   // sotto 3V: nessuna batteria collegata (solo USB)
}

#endif // BOARD_MASN_HTRA62 || BOARD_FAKETEC
