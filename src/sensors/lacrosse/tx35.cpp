// ============================================================================
// tx35.cpp — LaCrosse TX35DTH-IT / TX29-IT / TFA 30.3155 / 30.3159 (temp/umid.)
//   (rif. rtl_433 lacrosse_tx35.c)
//
// Dopo il sync word 0x2DD4 il primo nibble e' il codice modello (9), quindi:
//   b[0] = 9<<4 | id_hi ; 5 byte totali, b[4] = CRC-8/0x31 di b[0..3]
//   temp_c = 10*(b[1]&0x0f) + ((b[2]>>4)&0x0f) + 0.1*(b[2]&0x0f) - 40
//   humidity = b[3] & 0x7f  (0x6a = nessun sensore umidita')
//
// NB: condivide il nibble alto 0x9 con il WS90 (family 0x90). La disambiguazione
//     e' affidata alla validazione CRC in parse() (WS90 viene provato prima).
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

#define TX35_NO_HUMIDITY 0x6a
#define TX35_PROBE_FLAG  0x7d

static bool tx35_match(const uint8_t *b, size_t len) {
  if (len < 5) return false;
  if ((b[0] & 0xF0) != 0x90) return false;
  return crc8_0x31(b, 4) == b[4];          // valida subito per non collidere col WS90
}

static bool tx35_parse(const uint8_t *b, size_t len, float rssi, SensorReading &o) {
  if (len < 5) return false;
  if (crc8_0x31(b, 4) != b[4]) return false;

  float tempC = 10.0f * (b[1] & 0x0f)
              + 1.0f  * ((b[2] >> 4) & 0x0f)
              + 0.1f  * (b[2] & 0x0f) - 40.0f;
  if (tempC < -45.0f || tempC > 70.0f) return false;

  o.model   = "TX35";
  o.id      = ((b[0] & 0x0f) << 2) | (b[1] >> 6);
  o.tempC   = tempC;
  o.battLow = (b[3] >> 7) != 0;
  o.rssi    = rssi;
  o.caps    = CAP_TEMP;

  uint8_t hum = b[3] & 0x7f;
  if (hum != TX35_NO_HUMIDITY && hum != TX35_PROBE_FLAG && hum <= 100) {
    o.humidity = hum;
    o.caps |= CAP_HUM;
  }
  return true;
}

extern const SensorDriver DRIVER_TX35 = { "TX35", CAP_TEMP | CAP_HUM, tx35_match, tx35_parse };
