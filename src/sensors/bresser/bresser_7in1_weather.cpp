// ============================================================================
// bresser_7in1_weather.cpp — Bresser 7-in-1 stazione METEO
//   (rif. rtl_433 bresser_7in1.c, varianti s_type WEATHER=1, WEATHER3=12,
//   WEATHER8=13). Stesso protocollo del 7-in-1 Air Quality ma s_type diverso:
//   qui si decodificano temp/umid/vento/pioggia/UV/luce.
//
//   Dopo il sync 0x2DD4 seguono 25 byte. s_type = msg_raw[6]>>4 (pre-whitening).
//   Whitening: msg[i]^=0xaa. Integrita': digest LFSR-16 (gen 0x8810, key 0xba95)
//   di msg[2..24];  (chk ^ digest) == 0x6df1
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

#define BR7W_WEATHER   1
#define BR7W_WEATHER3 12
#define BR7W_WEATHER8 13

static bool bresser7w_match(const uint8_t *b, size_t len) {
  if (len < 25) return false;
  int s_type = b[6] >> 4;                 // dal byte grezzo (pre-whitening)
  return (s_type == BR7W_WEATHER || s_type == BR7W_WEATHER3 || s_type == BR7W_WEATHER8);
}

static bool bresser7w_parse(const uint8_t *buf, size_t len, float rssi, SensorReading &o) {
  if (len < 25) return false;
  int s_type = buf[6] >> 4;
  if (s_type != BR7W_WEATHER && s_type != BR7W_WEATHER3 && s_type != BR7W_WEATHER8)
    return false;

  uint8_t m[25];
  for (unsigned i = 0; i < 25; i++) m[i] = buf[i] ^ 0xaa;   // data whitening

  uint16_t chk    = (uint16_t)((m[0] << 8) | m[1]);
  uint16_t digest = lfsr_digest16(&m[2], 23, 0x8810, 0xba95);
  if ((uint16_t)(chk ^ digest) != 0x6df1) return false;

  int flags        = (m[15] & 0x0f);
  bool battery_low = (flags & 0x06) == 0x06;
  bool wind_light_ok = (s_type != BR7W_WEATHER3);

  o.model   = "BRESSER_7IN1";
  o.id      = (m[2] << 8) | m[3];
  o.battLow = battery_low;
  o.rssi    = rssi;
  o.caps    = 0;

  // --- temperatura / umidita' ---
  int temp_raw = (m[14] >> 4) * 100 + (m[14] & 0x0f) * 10 + (m[15] >> 4);
  float temp_c = temp_raw * 0.1f;
  if (temp_raw > 600) temp_c = (temp_raw - 1000) * 0.1f;
  int humidity = (m[16] >> 4) * 10 + (m[16] & 0x0f);
  if (temp_c >= -50.0f && temp_c <= 70.0f) { o.tempC = temp_c; o.caps |= CAP_TEMP; }
  if (humidity <= 100)                     { o.humidity = humidity; o.caps |= CAP_HUM; }

  // --- pioggia (contatore cumulativo, 6 cifre BCD) ---
  long rain_raw = (long)(m[10] >> 4) * 100000 + (m[10] & 0x0f) * 10000
                + (m[11] >> 4) * 1000 + (m[11] & 0x0f) * 100
                + (m[12] >> 4) * 10 + (m[12] & 0x0f);
  o.rainMm = rain_raw * 0.1f;
  o.caps  |= CAP_RAIN;

  if (wind_light_ok) {
    int wdir     = (m[4] >> 4) * 100 + (m[4] & 0x0f) * 10 + (m[5] >> 4);
    int wgst_raw = (m[7] >> 4) * 100 + (m[7] & 0x0f) * 10 + (m[8] >> 4);
    int wavg_raw = (m[8] & 0x0f) * 100 + (m[9] >> 4) * 10 + (m[9] & 0x0f);
    o.windAvgMs  = wavg_raw * 0.1f;
    o.windGustMs = wgst_raw * 0.1f;
    o.windDirDeg = wdir;
    o.caps |= CAP_WIND;

    int uv_raw = (m[20] >> 4) * 100 + (m[20] & 0x0f) * 10 + (m[21] >> 4);
    o.uvIndex  = uv_raw * 0.1f;
    o.caps    |= CAP_UV;
  }
  return o.caps != 0;
}

extern const SensorDriver DRIVER_BRESSER_7IN1 = { "BRESSER_7IN1",
    CAP_TEMP | CAP_HUM | CAP_WIND | CAP_RAIN | CAP_UV,
    bresser7w_match, bresser7w_parse };
