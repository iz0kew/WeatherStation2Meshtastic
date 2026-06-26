// ============================================================================
// ws85.cpp — Fine Offset WS85 (anemometro ultrasonico + pluviometro piezo)
//   (rif. rtl_433 fineoffset_ws85.c). E' un WS90 senza temp/umid/UV/luce:
//   misura SOLO vento e pioggia. Family code 0x85.
//   CRC-8/0x31 di b[0..25] == b[26] ; SUM8(b[0..26]) == b[27]
//   vento: MSB in b[5] (0x10/0x20/0x40), LSB in b[7]/b[8]/b[9]
//   pioggia totale: (b[15]<<8)|b[16], passi 0.1 mm
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool ws85_match(const uint8_t *b, size_t len) {
  return len >= 28 && b[0] == 0x85;
}

static bool ws85_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 28) return false;
  if (crc8_0x31(b, 26) != b[26]) return false;   // CRC di b[0..25]
  if (sum8(b, 27)      != b[27]) return false;    // SUM di b[0..26]

  o.model = "WS85";
  o.id    = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.rssi  = rssi;
  o.caps  = 0;

  int wind_avg = ((b[5] & 0x10) << 4) | b[7];
  int wind_dir = ((b[5] & 0x20) << 3) | b[8];
  int wind_max = ((b[5] & 0x40) << 2) | b[9];
  if (wind_avg != 0x1ff || wind_max != 0x1ff || wind_dir != 0x1ff) {
    if (wind_avg != 0x1ff) o.windAvgMs  = wind_avg * 0.1f;
    if (wind_max != 0x1ff) o.windGustMs = wind_max * 0.1f;
    if (wind_dir != 0x1ff) o.windDirDeg = wind_dir;
    o.caps |= CAP_WIND;
  }

  // pioggia totale (contatore cumulativo)
  int rain_raw = (b[15] << 8) | b[16];
  o.rainMm = rain_raw * 0.1f;
  o.caps  |= CAP_RAIN;

  int batt_mv = b[4] * 20;
  o.battV   = batt_mv / 1000.0f;
  o.battLow = (batt_mv > 0 && batt_mv < 1400);
  return o.caps != 0;
}

extern const SensorDriver DRIVER_WS85 = { "WS85", CAP_WIND | CAP_RAIN,
                                          ws85_match, ws85_parse };
