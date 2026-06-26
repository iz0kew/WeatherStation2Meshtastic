// ============================================================================
// wh46.cpp — Fine Offset / Ecowitt WH46 qualita' aria (PM1/2.5/4/10 + CO2 + T/H)
// Formato 21 byte (rif. rtl_433 fineoffset_wh46.c). Estende il WH45 con PM1/PM4.
//   YY=0x46; CRC-8/0x31 di b[0..18]==b[19]; SUM8(b[0..19])==b[20]
//   temp = ((b[4]&0x07)<<8|b[5]) offset 400 *0.1 ; hum=b[6]
//   PM2.5 = ((b[7]&0x3f)<<8|b[8])*0.1 ; PM10 = ((b[9]&0x3f)<<8|b[10])*0.1
//   CO2   = (b[11]<<8)|b[12]
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool wh46_match(const uint8_t *b, size_t len) {
  return len >= 21 && b[0] == 0x46;
}

static bool wh46_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 21) return false;
  if (crc8_0x31(b, 19) != b[19]) return false;
  if (sum8(b, 20)      != b[20]) return false;

  int   rawT  = ((b[4] & 0x07) << 8) | b[5];
  float tempC = (rawT - 400) * 0.1f;
  int   hum   = b[6];
  if (hum > 100 || tempC < -45.0f || tempC > 70.0f) return false;

  o.model    = "WH46";
  o.id       = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.tempC    = tempC;
  o.humidity = hum;
  o.pm25     = (((b[7] & 0x3f) << 8) | b[8])  * 0.1f;
  o.pm10     = (((b[9] & 0x3f) << 8) | b[10]) * 0.1f;
  o.co2      = (uint16_t)((b[11] << 8) | b[12]);
  int bars   = ((b[7] & 0x40) >> 4) | ((b[9] & 0xC0) >> 6);
  o.battLow  = (bars <= 1);
  o.rssi     = rssi;
  o.caps     = CAP_TEMP | CAP_HUM | CAP_PM | CAP_CO2;
  return true;
}

extern const SensorDriver DRIVER_WH46 = { "WH46", CAP_TEMP | CAP_HUM | CAP_PM | CAP_CO2,
                                          wh46_match, wh46_parse };
