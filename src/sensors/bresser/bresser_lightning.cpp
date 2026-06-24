// ============================================================================
// bresser_lightning.cpp — Bresser rilevatore fulmini
//   (rif. rtl_433 bresser_lightning.c)
//
// Dopo il sync 0x2DD4 seguono 10 byte. s_type/batt letti dai byte grezzi, poi
// whitening msg[i]^=0xaa. Integrita': digest LFSR-16 (gen 0x8810, key 0xabf9)
//   di msg[2..9]; (chk ^ digest) == 0x899e
//   distance_km = msg[7]; count (BCD) = (msg[4]>>4)*100+(msg[4]&0xf)*10+(msg[5]>>4)
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool brlight_match(const uint8_t *b, size_t len) {
  return len >= 10;
}

static bool brlight_parse(const uint8_t *buf, size_t len, float rssi, SensorReading &o) {
  if (len < 10) return false;
  bool battery_low = ((buf[5] & 0x08) >> 3) != 0;   // dal byte grezzo

  uint8_t m[10];
  for (unsigned i = 0; i < 10; i++) m[i] = buf[i] ^ 0xaa;

  uint16_t chk    = (uint16_t)((m[0] << 8) | m[1]);
  uint16_t digest = lfsr_digest16(&m[2], 8, 0x8810, 0xabf9);
  if ((uint16_t)(chk ^ digest) != 0x899e) return false;

  int distance_km = m[7];
  int count = (m[4] >> 4) * 100 + (m[4] & 0x0f) * 10 + (m[5] >> 4);

  o.model   = "BRESSER_LIGHTNING";
  o.id      = (m[2] << 8) | m[3];
  o.battLow = battery_low;
  o.rssi    = rssi;
  o.caps    = CAP_LIGHTNING;
  o.lightningCount      = (uint32_t)count;
  o.lightningCountValid = true;
  o.lightningDistKm     = (distance_km > 0 && distance_km < 63) ? (uint8_t)distance_km : 63;
  return true;
}

extern const SensorDriver DRIVER_BRESSER_LIGHTNING = { "BRESSER_LIGHTNING", CAP_LIGHTNING,
                                                brlight_match, brlight_parse };
