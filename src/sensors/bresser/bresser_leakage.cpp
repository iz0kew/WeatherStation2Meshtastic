// ============================================================================
// bresser_leakage.cpp — Bresser sensore perdita acqua (water leakage)
//   (rif. rtl_433 bresser_leakage.c)
//
// Dopo il sync 0x2DD4 seguono 18 byte (niente whitening). Integrita':
//   CRC-16/CCITT (poly 0x1021, init 0x0000) di msg[2..6] == (msg[0]<<8|msg[1])
//   alarm = (msg[7]&0x80)>>7 ; no_alarm = (msg[7]&0x40)>>6 ; battery_ok = msg[7]&0x30
//   sanity: alarm deve essere diverso da no_alarm
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool brleak_match(const uint8_t *b, size_t len) {
  return len >= 18;
}

static bool brleak_parse(const uint8_t *m, size_t len, float rssi, SensorReading &o) {
  if (len < 18) return false;

  uint16_t crc_calc = crc16_msb(&m[2], 5, 0x1021, 0x0000);
  uint16_t crc_recv = (uint16_t)((m[0] << 8) | m[1]);
  if (crc_calc != crc_recv) return false;

  int alarm    = (m[7] & 0x80) >> 7;
  int no_alarm = (m[7] & 0x40) >> 6;
  if (alarm == no_alarm) return false;          // sanity

  o.model   = "BRESSER_LEAKAGE";
  o.id      = ((uint32_t)m[2] << 24) | ((uint32_t)m[3] << 16) | ((uint32_t)m[4] << 8) | m[5];
  o.battLow = ((m[7] & 0x30) == 0x00);
  o.rssi    = rssi;
  o.leak    = (alarm != 0);
  o.caps    = CAP_LEAK;
  return true;
}

extern const SensorDriver DRIVER_BRESSER_LEAKAGE = { "BRESSER_LEAKAGE", CAP_LEAK,
                                              brleak_match, brleak_parse };
