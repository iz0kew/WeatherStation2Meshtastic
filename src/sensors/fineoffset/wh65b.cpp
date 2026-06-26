// ============================================================================
// wh65b.cpp — Fine Offset WH24 / WH65B / WS69 stazione all-in-one
//   temp/umid/vento/pioggia/UV/luce (rif. rtl_433 fineoffset.c, famiglia 0x24).
//   17 byte; CRC-8/0x31 di b[0..14]==b[15] ; SUM8(b[0..15])==b[16]
//
//   NB: WH24 e WH65B/WS69 condividono lo stesso family code 0x24 e si
//   distinguono solo dalla lunghezza del postambolo (non disponibile via
//   SX1262). Qui si assume WH65B/WS69 (anemometro 0.51, bilanciere 0.254 mm),
//   il modello attualmente in commercio. Per un vecchio WH24 i fattori vento
//   (1.12) e pioggia (0.3) vanno cambiati.
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool wh65b_match(const uint8_t *b, size_t len) {
  return len >= 17 && b[0] == 0x24;
}

static bool wh65b_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 17) return false;
  if (crc8_0x31(b, 15) != b[15]) return false;
  if (sum8(b, 16)      != b[16]) return false;

  const float WIND_FACTOR = 0.51f;   // WH65B/WS69 (WH24 = 1.12)
  const float RAIN_CUP    = 0.254f;  // WH65B/WS69 (WH24 = 0.3)

  o.model = "WH65B";
  o.id    = b[1];
  o.rssi  = rssi;
  o.battLow = ((b[3] & 0x08) >> 3) != 0;
  o.caps  = 0;

  int temp_raw = ((b[3] & 0x07) << 8) | b[4];
  if (temp_raw != 0x7ff) { o.tempC = (temp_raw - 400) * 0.1f; o.caps |= CAP_TEMP; }
  if (b[5] != 0xff)      { o.humidity = b[5];                 o.caps |= CAP_HUM;  }

  int wind_dir   = b[2] | ((b[3] & 0x80) << 1);
  int wspeed_raw = b[6] | ((b[3] & 0x10) << 4);
  int gust_raw   = b[7];
  if (wspeed_raw != 0x1ff || gust_raw != 0xff || wind_dir != 0x1ff) {
    if (wspeed_raw != 0x1ff) o.windAvgMs  = wspeed_raw * 0.125f * WIND_FACTOR;
    if (gust_raw   != 0xff)  o.windGustMs = gust_raw * WIND_FACTOR;
    if (wind_dir   != 0x1ff) o.windDirDeg = wind_dir;
    o.caps |= CAP_WIND;
  }

  int rain_raw = (b[8] << 8) | b[9];
  o.rainMm = rain_raw * RAIN_CUP;     // contatore cumulativo
  o.caps  |= CAP_RAIN;

  int uv_raw = (b[10] << 8) | b[11];
  if (uv_raw != 0xffff) {
    static const int uvi_upper[] = {432,851,1210,1570,2017,2450,2761,3100,3512,3918,4277,4650,5029};
    int uvi = 0;
    while (uvi < 13 && uvi_upper[uvi] < uv_raw) ++uvi;
    o.uvIndex  = (float)uvi;
    int light_raw = (b[12] << 16) | (b[13] << 8) | b[14];
    if (light_raw != 0xffffff) o.lightLux = light_raw * 0.1f;
    o.caps |= CAP_UV;
  }
  return o.caps != 0;
}

extern const SensorDriver DRIVER_WH65B = { "WH65B",
    CAP_TEMP | CAP_HUM | CAP_WIND | CAP_RAIN | CAP_UV, wh65b_match, wh65b_parse };
