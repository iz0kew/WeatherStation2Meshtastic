// ============================================================================
// bresser_5in1.cpp — Bresser 5-in-1 (temp/umid/vento/pioggia)
//   (rif. rtl_433 bresser_5in1.c)
//
// Dopo il sync 0x2DD4 seguono 26 byte. I primi 13 sono il complemento a 1 dei
// successivi 13: msg[i] ^ msg[i+13] == 0xFF (integrity check). I dati utili
// stanno in msg[14..25], in BCD.
// ============================================================================
#include "../sensor_types.h"

static bool bresser5_match(const uint8_t *b, size_t len) {
  return len >= 26;
}

static bool bresser5_parse(const uint8_t *m, size_t len, float rssi, SensorReading &o) {
  if (len < 26) return false;
  // integrity: i primi 13 byte sono il complemento dei 13 successivi
  for (unsigned i = 0; i < 13; i++)
    if ((uint8_t)(m[i] ^ m[i + 13]) != 0xff) return false;

  o.model = "BRESSER_5IN1";
  o.id    = m[14];
  o.rssi  = rssi;
  o.battLow = (m[25] & 0x80) != 0;
  o.caps  = 0;

  bool temp_ok = (m[20] & 0x0f) <= 9;
  int  temp_raw = (m[20] & 0x0f) + ((m[20] & 0xf0) >> 4) * 10 + (m[21] & 0x0f) * 100;
  if (m[25] & 0x0f) temp_raw = -temp_raw;
  if (temp_ok) { o.tempC = temp_raw * 0.1f; o.caps |= CAP_TEMP; }

  bool hum_ok = (m[22] & 0x0f) <= 9;
  int  hum    = (m[22] & 0x0f) + ((m[22] & 0xf0) >> 4) * 10;
  if (hum_ok && hum <= 100) { o.humidity = hum; o.caps |= CAP_HUM; }

  int gust_raw = ((m[17] & 0x0f) << 8) + m[16];
  int wind_raw = (m[18] & 0x0f) + ((m[18] & 0xf0) >> 4) * 10 + (m[19] & 0x0f) * 100;
  o.windGustMs = gust_raw * 0.1f;
  o.windAvgMs  = wind_raw * 0.1f;
  o.windDirDeg = ((m[17] & 0xf0) >> 4) * 22.5f;
  o.caps |= CAP_WIND;

  int rain_raw = (m[23] & 0x0f) + ((m[23] & 0xf0) >> 4) * 10
               + (m[24] & 0x0f) * 100 + ((m[24] & 0xf0) >> 4) * 1000;
  o.rainMm = rain_raw * 0.1f;
  o.caps |= CAP_RAIN;
  return true;
}

extern const SensorDriver DRIVER_BRESSER_5IN1 = { "BRESSER_5IN1",
    CAP_TEMP | CAP_HUM | CAP_WIND | CAP_RAIN, bresser5_match, bresser5_parse };
