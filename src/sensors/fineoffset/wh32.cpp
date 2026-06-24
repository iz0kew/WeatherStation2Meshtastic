// ============================================================================
// wh32.cpp — Fine Offset WH32 / WH25 / WH32B / WH65B (temp / umidita' / press.)
// Formato 8 byte: MI IT TT HH PP PP CC XX   (rif. rtl_433 fineoffset.c)
//   nibble alto b[0] = tipo messaggio (0xD0/0xE0), checksum = sum8(b,6)==b[6]
//   WH25/WH32B includono la pressione (campo PP), WH32 base no -> CAP_PRESSURE
//   viene settata solo se la pressione e' plausibile.
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool wh32_match(const uint8_t *b, size_t len) {
  if (len < 7) return false;
  uint8_t t = b[0] & 0xF0;
  return (t == 0xD0 || t == 0xE0);
}

static bool wh32_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 7) return false;
  if (sum8(b, 6) != b[6]) return false;

  bool     invalid = (b[1] & 0x04) != 0;
  uint16_t rawT    = (uint16_t)(((b[1] & 0x03) << 8) | b[2]);
  float    tempC   = (rawT - 400) * 0.1f;
  uint8_t  hum     = b[3];
  if (invalid || rawT == 0x7FF) return false;
  if (hum > 100 || tempC < -45.0f || tempC > 70.0f) return false;

  o.model   = "WH32";
  o.id      = (uint8_t)(((b[0] & 0x0F) << 4) | (b[1] >> 4));
  o.tempC   = tempC;
  o.humidity= hum;
  o.battLow = (b[1] & 0x08) != 0;
  o.rssi    = rssi;
  o.caps    = CAP_TEMP | CAP_HUM;

  // Pressione (WH25/WH32B): campo a 16 bit in hPa*10. 0 o fuori range = assente.
  uint16_t rawP = (uint16_t)((b[4] << 8) | b[5]);
  float    hpa  = rawP * 0.1f;
  if (rawP != 0 && hpa >= 800.0f && hpa <= 1100.0f) {
    o.pressureHpa = hpa;
    o.caps |= CAP_PRESSURE;
  }
  return true;
}

extern const SensorDriver DRIVER_WH32 = { "WH32", CAP_TEMP | CAP_HUM | CAP_PRESSURE,
                                   wh32_match, wh32_parse };
