// ============================================================================
// wh57.cpp — Ecowitt WH57 / WH31L rilevatore fulmini
// Formato 9 byte: 57 SI II II FF KK CC XX AA  (rif. rtl_433 fineoffset_wh31l.c)
//   state = b[1]>>4 (8 = fulmine), distanza = b[5]&0x3F (63 = nessuno),
//   conteggio grezzo = b[6]. CRC-8/0x31 su 8 byte = 0, poi SUM8(b,8)==b[8].
// Il parser e' senza stato: riporta il conteggio grezzo e la distanza; la
// gestione del wrap a 8 bit e del totale cumulato e' compito del consumatore
// (ui.cpp / meshtastic_pack.cpp).
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool wh57_match(const uint8_t *b, size_t len) {
  return len >= 9 && b[0] == 0x57;
}

static bool wh57_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 9) return false;
  if (crc8_0x31(b, 8) != 0) return false;
  if (sum8(b, 8) != b[8])   return false;

  uint8_t state = b[1] >> 4;
  uint8_t dist  = b[5] & 0x3F;

  o.model    = "WH57";
  o.id       = ((uint32_t)(b[1] & 0x0F) << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.battLow  = (((b[4] >> 1) & 0x03) == 0);
  o.rssi     = rssi;
  o.caps     = CAP_LIGHTNING;
  o.lightningCount      = b[6];
  o.lightningCountValid = (state != 0);          // state 0 = avvio: conteggio non affidabile
  o.lightningDistKm     = (state == 8 && dist != 63) ? dist : 63;
  return true;
}

extern const SensorDriver DRIVER_WH57 = { "WH57", CAP_LIGHTNING, wh57_match, wh57_parse };
