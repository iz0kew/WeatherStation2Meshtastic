// ============================================================================
// wh40.cpp — Ecowitt WH40 pluviometro
// Formato 9 byte: 40 00 II II FV RR RR CC SS  (rif. rtl_433 ambientweather_wh31e.c)
//   CRC-8/0x31 sui primi 8 byte (= 0), poi SUM8(b,8)==b[8]
//   rainRaw in passi da 0.1 mm (contatore cumulativo)
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool wh40_match(const uint8_t *b, size_t len) {
  return len >= 9 && b[0] == 0x40;
}

static bool wh40_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 9) return false;
  if (crc8_0x31(b, 8) != 0) return false;
  if (sum8(b, 8) != b[8])   return false;

  uint8_t  battRaw = b[4] & 0x1F;                  // tensione in passi da 0.1 V
  uint16_t rainRaw = (uint16_t)((b[5] << 8) | b[6]);

  o.model  = "WH40";
  o.id     = (uint16_t)((b[2] << 8) | b[3]);
  o.rainMm = rainRaw * 0.1f;
  o.battV  = battRaw * 0.1f;
  o.battLow= (o.battV > 0 && o.battV < 1.2f);
  o.rssi   = rssi;
  o.caps   = CAP_RAIN;
  return true;
}

extern const SensorDriver DRIVER_WH40 = { "WH40", CAP_RAIN, wh40_match, wh40_parse };
