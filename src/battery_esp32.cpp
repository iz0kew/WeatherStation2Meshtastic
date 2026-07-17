// ============================================================================
// battery_esp32.cpp — lettura VBAT via ADC con partitore commutato da
//   ADC_Ctrl (Heltec WiFi LoRa 32 V3/V4, ESP32-S3).
// ============================================================================
#include "battery.h"
#include "board_config.h"
#if defined(BOARD_HELTEC_V3) || defined(BOARD_HELTEC_V4)

#define BATT_REFRESH_MS 5000UL   // la UI ridisegna ogni secondo: cache lettura

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
  pinMode(PIN_ADC_CTRL, OUTPUT);
  digitalWrite(PIN_ADC_CTRL, VBAT_CTRL_ON);
  delay(10);                                  // settle del partitore
  uint32_t mv = 0;
  for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(PIN_VBAT_READ);
  digitalWrite(PIN_ADC_CTRL, VBAT_CTRL_ON == LOW ? HIGH : LOW);
  return (mv / 8) * VBAT_MULTIPLIER / 1000.0f;
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
  return cachedV >= 3.0f;   // sotto 3V il partitore e' scollegato: niente batteria
}

#endif // BOARD_HELTEC_V3 || BOARD_HELTEC_V4
