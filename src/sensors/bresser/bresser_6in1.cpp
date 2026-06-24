// ============================================================================
// bresser_6in1.cpp — Bresser 6-in-1 / 7-in-1 (temp/umid/vento/pioggia/UV/suolo)
//   (rif. rtl_433 bresser_6in1.c)
//
// Dopo il sync 0x2DD4 seguono 18 byte. Integrita':
//   digest LFSR-16 (gen 0x8810, init 0x5412) di msg[2..16] == (msg[0]<<8|msg[1])
//   checksum: add_bytes(msg[2..17]) & 0xff == 0xff
// Alcuni byte sono condivisi (temp/umid vs pioggia) e altri invertiti.
// ============================================================================
#include "../sensor_types.h"
#include "../sensor_util.h"

static bool bresser6_match(const uint8_t *b, size_t len) {
  return len >= 18;
}

static bool bresser6_parse(const uint8_t *buf, size_t len, float rssi, SensorReading &o) {
  if (len < 18) return false;
  uint8_t m[18];
  memcpy(m, buf, 18);

  uint16_t chkdgst = (uint16_t)((m[0] << 8) | m[1]);
  if (chkdgst != lfsr_digest16(&m[2], 15, 0x8810, 0x5412)) return false;
  if ((sum8(&m[2], 16)) != 0xff) return false;        // add_bytes(msg[2..17]) & 0xff

  static const int moisture_map[] = {0,7,13,20,27,33,40,47,53,60,67,73,80,87,93,99};

  o.model = "BRESSER_6IN1";
  o.id    = ((uint32_t)m[2] << 24) | ((uint32_t)m[3] << 16) | ((uint32_t)m[4] << 8) | m[5];
  o.rssi  = rssi;
  o.battLow = !(((m[13] >> 1) & 1));
  o.caps  = 0;

  int s_type = (m[6] >> 4);

  // --- temperatura / umidita' (byte 12-14, BCD non invertito) ---
  bool temp_ok  = m[12] <= 0x99 && (m[13] & 0xf0) <= 0x90;
  int  temp_raw = (m[12] >> 4) * 100 + (m[12] & 0x0f) * 10 + (m[13] >> 4);
  float temp_c  = temp_raw * 0.1f;
  if ((m[13] >> 3) & 1) temp_c = (temp_raw - 1000) * 0.1f;
  int humidity  = (m[14] >> 4) * 10 + (m[14] & 0x0f);

  // --- UV (byte 15-16 invertiti) ---
  bool uv_ok  = (m[16] & 0x0f) == 0 && (uint8_t)(~m[15]) <= 0x99 && (uint8_t)(~m[16] & 0xf0) <= 0x90;
  int  uv_raw = ((~m[15] & 0xf0) >> 4) * 100 + (~m[15] & 0x0f) * 10 + ((~m[16] & 0xf0) >> 4);

  // --- vento (byte 7-9 invertiti) ---
  m[7] ^= 0xff; m[8] ^= 0xff; m[9] ^= 0xff;
  bool wind_ok = (m[7] <= 0x99) && (m[8] <= 0x99) && (m[9] <= 0x99);
  int  gust_raw = (m[7] >> 4) * 100 + (m[7] & 0x0f) * 10 + (m[8] >> 4);
  int  wavg_raw = (m[9] >> 4) * 100 + (m[9] & 0x0f) * 10 + (m[8] & 0x0f);
  int  wind_dir = ((m[10] & 0xf0) >> 4) * 100 + (m[10] & 0x0f) * 10 + ((m[11] & 0xf0) >> 4);

  // --- pioggia (byte 12-14 invertiti, condivisi con temp/umid) ---
  bool rain_ok = (m[16] & 1);
  uint8_t r12 = m[12] ^ 0xff, r13 = m[13] ^ 0xff, r14 = m[14] ^ 0xff;
  long rain_raw = (long)(r12 >> 4) * 100000 + (r12 & 0x0f) * 10000
                + (r13 >> 4) * 1000 + (r13 & 0x0f) * 100
                + (r14 >> 4) * 10 + (r14 & 0x0f);

  // il sensore thermo/hygro e il soil potrebbero presentare letture spurie
  int moisture = -1;
  if (s_type == 4 && temp_ok && humidity >= 1 && humidity <= 16) {
    moisture = moisture_map[humidity - 1];
    wind_ok = false; uv_ok = false;
  }

  if (rain_ok) {
    o.rainMm = rain_raw * 0.1f;
    o.caps |= CAP_RAIN;
  } else {
    if (temp_ok) { o.tempC = temp_c; o.caps |= CAP_TEMP; }
    if (temp_ok && moisture < 0 && humidity <= 100) { o.humidity = humidity; o.caps |= CAP_HUM; }
  }
  if (moisture >= 0) { o.soilMoisture = (uint8_t)moisture; o.caps |= CAP_SOIL; }
  if (wind_ok) {
    o.windGustMs = gust_raw * 0.1f;
    o.windAvgMs  = wavg_raw * 0.1f;
    o.windDirDeg = wind_dir;
    o.caps |= CAP_WIND;
  }
  if (uv_ok) { o.uvIndex = uv_raw * 0.1f; o.caps |= CAP_UV; }
  return o.caps != 0;
}

extern const SensorDriver DRIVER_BRESSER_6IN1 = { "BRESSER_6IN1",
    CAP_TEMP | CAP_HUM | CAP_WIND | CAP_RAIN | CAP_UV | CAP_SOIL,
    bresser6_match, bresser6_parse };
