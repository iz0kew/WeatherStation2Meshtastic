// ============================================================================
// bresser_7in1.cpp — Bresser 7-in-1 Air Quality (PM2.5 / PM10 / CO2)
//   (rif. rtl_433 bresser_7in1.c, varianti SENSOR_TYPE_AIR_PM=8 e CO2=10)
//
// Dopo il sync 0x2DD4 seguono 25 byte. s_type = msg_raw[6]>>4 (PRIMA del
// whitening). Whitening: msg[i] ^= 0xaa. Integrita': digest LFSR-16
//   (gen 0x8810, key 0xba95) di msg[2..24];  (chk ^ digest) == 0x6df1
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

#define BR7_TYPE_AIR_PM  8
#define BR7_TYPE_CO2    10

static bool bresser7_match(const uint8_t *b, size_t len) {
  if (len < 25) return false;
  int s_type = b[6] >> 4;                  // letto dal byte grezzo (pre-whitening)
  return (s_type == BR7_TYPE_AIR_PM || s_type == BR7_TYPE_CO2);
}

static bool bresser7_parse(const uint8_t *buf, size_t len, float rssi, SensorReading &o) {
  if (len < 25) return false;
  int s_type = buf[6] >> 4;
  if (s_type != BR7_TYPE_AIR_PM && s_type != BR7_TYPE_CO2) return false;

  uint8_t m[25];
  for (unsigned i = 0; i < 25; i++) m[i] = buf[i] ^ 0xaa;   // data whitening

  uint16_t chk    = (uint16_t)((m[0] << 8) | m[1]);
  uint16_t digest = lfsr_digest16(&m[2], 23, 0x8810, 0xba95);
  if ((uint16_t)(chk ^ digest) != 0x6df1) return false;

  int flags       = (m[15] & 0x0f);
  bool battery_low = (flags & 0x06) == 0x06;

  o.model   = "BRESSER_7IN1_AQ";
  o.id      = (m[2] << 8) | m[3];
  o.battLow = battery_low;
  o.rssi    = rssi;
  o.caps    = 0;

  if (s_type == BR7_TYPE_AIR_PM) {
    bool pm25_init = (m[10] & 0x0f) == 0x0f;
    bool pm10_init = (m[12] & 0x0f) == 0x0f;
    int pm_2_5 = (m[10] & 0x0f) * 1000 + (m[11] >> 4) * 100 + (m[11] & 0x0f) * 10 + (m[12] >> 4);
    int pm_10  = (m[12] & 0x0f) * 1000 + (m[13] >> 4) * 100 + (m[13] & 0x0f) * 10 + (m[14] >> 4);
    if (!pm25_init) o.pm25 = pm_2_5;
    if (!pm10_init) o.pm10 = pm_10;
    o.caps |= CAP_PM;
  } else {  // CO2
    bool co2_init = (m[5] & 0x0f) == 0x0f;
    int co2 = ((m[4] & 0xf0) >> 4) * 1000 + (m[4] & 0x0f) * 100
            + ((m[5] & 0xf0) >> 4) * 10 + (m[5] & 0x0f);
    if (!co2_init) o.co2 = (uint16_t)co2;
    o.caps |= CAP_CO2;
  }
  return o.caps != 0;
}

extern const SensorDriver DRIVER_BRESSER_7IN1_AQ = { "BRESSER_7IN1_AQ", CAP_PM | CAP_CO2,
                                              bresser7_match, bresser7_parse };
