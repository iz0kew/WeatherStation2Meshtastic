// ============================================================================
// ws68.cpp — Ecowitt WS68 anemometro + luce (lux) + UV
//   (rif. rtl_433 ambientweather_wh31e.c, ramo msg_type 0x68). 16+ byte.
//   CRC-8/0x31 di b[0..13]==b[14] (crc8(b,15)==0) ; SUM8(b[0..14])==b[15]
//   wind: MSB in b[7] (0x10/0x20/0x40), LSB in b[10]/b[11]/b[12]
//   lux = ((b[4]<<8)|b[5])*10 ; uv = b[13]*0.1
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool ws68_match(const uint8_t *b, size_t len) {
  return len >= 16 && b[0] == 0x68;
}

static bool ws68_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 16) return false;
  if (crc8_0x31(b, 15) != 0)    return false;   // b[14] = CRC di b[0..13]
  if (sum8(b, 15)      != b[15]) return false;   // b[15] = SUM di b[0..14]

  o.model = "WS68";
  o.id    = (uint16_t)((b[2] << 8) | b[3]);
  o.rssi  = rssi;
  o.caps  = 0;

  int wspeed = ((b[7] & 0x10) << 4) | b[10];
  int wdir   = ((b[7] & 0x20) << 3) | b[11];
  int wgust  = ((b[7] & 0x40) << 2) | b[12];
  if (wspeed != 0x1ff || wgust != 0x1ff || wdir != 0x1ff) {
    if (wspeed != 0x1ff) o.windAvgMs  = wspeed * 0.1f;
    if (wgust  != 0x1ff) o.windGustMs = wgust * 0.1f;
    if (wdir   != 0x1ff) o.windDirDeg = wdir;
    o.caps |= CAP_WIND;
  }

  o.uvIndex  = b[13] * 0.1f;
  o.lightLux = ((b[4] << 8) | b[5]) * 10.0f;
  o.caps    |= CAP_UV;

  o.battLow  = (b[6] <= 0x20);
  return o.caps != 0;
}

extern const SensorDriver DRIVER_WS68 = { "WS68", CAP_WIND | CAP_UV, ws68_match, ws68_parse };
