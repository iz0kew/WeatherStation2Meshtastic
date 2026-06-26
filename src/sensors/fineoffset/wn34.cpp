// ============================================================================
// wn34.cpp — Fine Offset WN34S/L/D (sonda temperatura suolo/acqua/alimenti)
//   (rif. rtl_433 fineoffset_wn34.c). Famiglia 0x34, 9 byte.
//   CRC-8/0x31 di b[0..6]==b[7] ; SUM8(b[0..7])==b[8]
//   temp_raw = sign-extend((b[4]&0x0F)<<12 | b[5]<<4) >> 4
//   sub_type 4 = WN34D (niente offset); altri = offset 40
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool wn34_match(const uint8_t *b, size_t len) {
  return len >= 9 && b[0] == 0x34;
}

static bool wn34_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 9) return false;
  if (crc8_0x31(b, 7) != b[7]) return false;
  if (sum8(b, 8)      != b[8]) return false;

  int16_t temp_raw = (int16_t)(((b[4] & 0x0F) << 12) | (b[5] << 4));   // sign extend
  int sub_type     = (b[4] & 0xF0) >> 4;
  float tempC = (sub_type == 4) ? (temp_raw >> 4) * 0.1f
                                : ((temp_raw >> 4) * 0.1f) - 40.0f;
  if (tempC < -50.0f || tempC > 85.0f) return false;

  o.model   = "WN34";
  o.id      = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.tempC   = tempC;
  int batt_mv = (b[6] & 0x7f) * 20;
  o.battV   = batt_mv / 1000.0f;
  o.battLow = (batt_mv > 0 && batt_mv < 1200);
  o.rssi    = rssi;
  o.caps    = CAP_TEMP;
  return true;
}

extern const SensorDriver DRIVER_WN34 = { "WN34", CAP_TEMP, wn34_match, wn34_parse };
