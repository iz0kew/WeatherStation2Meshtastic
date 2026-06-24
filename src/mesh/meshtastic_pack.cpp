// ============================================================================
// meshtastic_pack.cpp — stack Meshtastic hand-made (riuso da EcoWitt).
//   AES128/256-CTR + mini-encoder protobuf + due canali. La trasmissione LoRa
//   e' delegata a radioTransmitMeshtastic() (radio/radio.cpp), unico gestore
//   dello switch di modalita' della radio.
// Riferimenti: meshtastic/firmware {RadioInterface,CryptoEngine,Channels},
//   meshtastic/protobufs {mesh,telemetry,portnums}.proto
// ============================================================================
#include "meshtastic_pack.h"
#include "../radio/radio.h"
#include "../timesync.h"
#include "../history.h"
#include "user_config.h"
#include <Crypto.h>
#include <AES.h>
#include <CTR.h>
#include <esp_system.h>
#include <esp_mac.h>
#include <time.h>

// ---------------------------------------------------------------------------
// PSK dei canali (da settings.ini -> user_config.h). 16 (AES-128) o 32 (AES-256).
static const uint8_t s_psk0[MESH_CHANNEL_KEY_SIZE]      = MESH_CHANNEL_KEY;
static const uint8_t s_psk1[MESH_TEXT_CHANNEL_KEY_SIZE] = MESH_TEXT_CHANNEL_KEY;

static const uint32_t BROADCAST_ADDR = 0xFFFFFFFF;
// portnum (portnums.proto)
static const uint8_t PORT_TEXT      = 1;
static const uint8_t PORT_POSITION  = 3;
static const uint8_t PORT_NODEINFO  = 4;
static const uint8_t PORT_TELEMETRY = 67;

static uint32_t s_nodeId    = 0;
static uint32_t s_pktSent   = 0;
static uint8_t  s_chanHash0 = 0;
static uint8_t  s_chanHash1 = 0;
static const size_t s_pskLen0 = MESH_CHANNEL_KEY_SIZE;
static const size_t s_pskLen1 = MESH_TEXT_CHANNEL_KEY_SIZE;
static char     s_shortName[5] = MESH_SHORT_NAME;

// ---------------------------------------------------------------------------
// Mini-encoder protobuf
// ---------------------------------------------------------------------------
static size_t pbVarint(uint8_t *buf, size_t p, uint64_t v) {
  while (v >= 0x80) { buf[p++] = (uint8_t)(v | 0x80); v >>= 7; }
  buf[p++] = (uint8_t)v;
  return p;
}
static size_t pbTag(uint8_t *buf, size_t p, uint32_t field, uint8_t wire) {
  return pbVarint(buf, p, ((uint64_t)field << 3) | wire);
}
static size_t pbVarintField(uint8_t *buf, size_t p, uint32_t field, uint64_t v) {
  p = pbTag(buf, p, field, 0);
  return pbVarint(buf, p, v);
}
static size_t pbFloatField(uint8_t *buf, size_t p, uint32_t field, float f) {
  p = pbTag(buf, p, field, 5);
  memcpy(buf + p, &f, 4);
  return p + 4;
}
static size_t pbFixed32Field(uint8_t *buf, size_t p, uint32_t field, uint32_t v) {
  p = pbTag(buf, p, field, 5);
  memcpy(buf + p, &v, 4);
  return p + 4;
}
static size_t pbBytesField(uint8_t *buf, size_t p, uint32_t field,
                           const uint8_t *data, size_t len) {
  p = pbTag(buf, p, field, 2);
  p = pbVarint(buf, p, len);
  memcpy(buf + p, data, len);
  return p + len;
}

// ---------------------------------------------------------------------------
static uint8_t xorHash(const uint8_t *p, size_t len) {
  uint8_t code = 0;
  for (size_t i = 0; i < len; i++) code ^= p[i];
  return code;
}

static void aesEncrypt(uint8_t *out, const uint8_t *in, size_t len,
                       const uint8_t *key, size_t keyLen, const uint8_t *iv) {
  if (keyLen == 32) {
    CTR<AES256> ctr; ctr.setKey(key, 32); ctr.setIV(iv, 16); ctr.encrypt(out, in, len);
  } else {
    CTR<AES128> ctr; ctr.setKey(key, 16); ctr.setIV(iv, 16); ctr.encrypt(out, in, len);
  }
}

// ---------------------------------------------------------------------------
void meshInit() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  s_nodeId = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) |
             ((uint32_t)mac[4] << 8)  |  (uint32_t)mac[5];
  if (s_nodeId == 0 || s_nodeId == BROADCAST_ADDR) s_nodeId = 0x12345678;

#if MESH_SHORT_NAME_AUTO
  snprintf(s_shortName, sizeof(s_shortName), "%04lX", (unsigned long)(s_nodeId & 0xFFFF));
#endif

  s_chanHash0 = xorHash((const uint8_t *)MESH_CHANNEL_NAME, strlen(MESH_CHANNEL_NAME))
              ^ xorHash(s_psk0, sizeof(s_psk0));
  s_chanHash1 = xorHash((const uint8_t *)MESH_TEXT_CHANNEL_NAME, strlen(MESH_TEXT_CHANNEL_NAME))
              ^ xorHash(s_psk1, sizeof(s_psk1));

  Serial.printf("[mesh] nodo !%08lx  freq %.3f SF%d BW%g CR4/%d\n",
                (unsigned long)s_nodeId,
                (double)MESH_FREQ_MHZ, MESH_SF, (double)MESH_BW_KHZ, MESH_CR);
  Serial.printf("[mesh] canale principale '%s' hash 0x%02x\n", MESH_CHANNEL_NAME, s_chanHash0);
}

uint32_t meshNodeId()      { return s_nodeId; }
uint32_t meshPacketsSent() { return s_pktSent; }
const char *meshShortName() { return s_shortName; }

// ---------------------------------------------------------------------------
// Cifra il payload e trasmette header+payload (TX delegato al gestore radio).
//   nonce[0..7] = packetId (LE, 64 bit), nonce[8..11] = fromNode (LE)
// ---------------------------------------------------------------------------
static bool meshTransmit(const uint8_t *plain, size_t plainLen, uint8_t chanIdx = 0) {
  if (plainLen > 200) return false;

  const uint8_t *psk      = (chanIdx == 1) ? s_psk1      : s_psk0;
  size_t         pskLen   = (chanIdx == 1) ? s_pskLen1   : s_pskLen0;
  uint8_t        chanHash = (chanIdx == 1) ? s_chanHash1 : s_chanHash0;

  uint32_t pktId = esp_random();
  if (pktId == 0) pktId = 1;

  uint8_t nonce[16] = {0};
  uint64_t id64 = pktId;
  memcpy(nonce, &id64, 8);
  memcpy(nonce + 8, &s_nodeId, 4);

  uint8_t cipher[208];
  aesEncrypt(cipher, plain, plainLen, psk, pskLen, nonce);

  uint8_t pkt[224];
  uint32_t dest = BROADCAST_ADDR;
  memcpy(pkt + 0,  &dest,     4);
  memcpy(pkt + 4,  &s_nodeId, 4);
  memcpy(pkt + 8,  &pktId,    4);
  const uint8_t hopLimit = MESH_HOP_LIMIT, hopStart = MESH_HOP_LIMIT;
  pkt[12] = (hopLimit & 0x07) | (uint8_t)(hopStart << 5);   // flags
  pkt[13] = chanHash;                                        // channel hash
  pkt[14] = 0;                                               // next_hop
  pkt[15] = (uint8_t)(s_nodeId & 0xFF);                      // relay_node
  memcpy(pkt + 16, cipher, plainLen);

  if (radioTransmitMeshtastic(pkt, 16 + plainLen)) {
    s_pktSent++;
    Serial.printf("[mesh] TX ok ch%d id=0x%08lx %u byte\n",
                  chanIdx, (unsigned long)pktId, (unsigned)(16 + plainLen));
    return true;
  }
  Serial.printf("[mesh] TX fallita\n");
  return false;
}

static bool sendData(uint8_t portnum, const uint8_t *payload, size_t len,
                     uint8_t chanIdx = 0) {
  uint8_t data[220];
  size_t p = 0;
  p = pbVarintField(data, p, 1, portnum);
  p = pbBytesField(data, p, 2, payload, len);
#if MESH_OK_TO_MQTT
  p = pbVarintField(data, p, 9, 1);   // bitfield campo 9: bit 0 = ok_to_mqtt
#endif
  return meshTransmit(data, p, chanIdx);
}

// ===========================================================================
// Accumulatore sensori -> telemetria
// ===========================================================================
namespace {
  struct Accum {
    double tSum = 0; uint32_t tN = 0;
    double hSum = 0; uint32_t hN = 0;
    double pSum = 0; uint32_t pN = 0;
    bool   haveWind = false; float windAvg = 0, windGust = 0; uint16_t windDir = 0;
    bool   haveRain = false; float rainMm = 0;
    bool   haveSoil = false; uint8_t soil = 0;
    bool   haveLux  = false; float lux = 0;
    bool   havePM   = false; float pm25 = 0, pm10 = 0;
    bool   haveCO2  = false; uint16_t co2 = 0;
  };
  Accum g_acc;
}

void meshSubmit(const SensorReading &r) {
  if (r.caps & CAP_TEMP)     { g_acc.tSum += r.tempC;    g_acc.tN++; }
  if (r.caps & CAP_HUM)      { g_acc.hSum += r.humidity; g_acc.hN++; }
  if (r.caps & CAP_PRESSURE) { g_acc.pSum += r.pressureHpa; g_acc.pN++; }
  if (r.caps & CAP_WIND) {
    g_acc.haveWind = true;
    g_acc.windAvg  = r.windAvgMs; g_acc.windGust = r.windGustMs;
    g_acc.windDir  = (uint16_t)(r.windDirDeg + 0.5f);
  }
  if (r.caps & CAP_RAIN) { g_acc.haveRain = true; g_acc.rainMm = r.rainMm; }
  if (r.caps & CAP_SOIL) { g_acc.haveSoil = true; g_acc.soil = r.soilMoisture; }
  if (r.caps & CAP_UV)   { g_acc.haveLux  = true; g_acc.lux = r.lightLux; }
  if (r.caps & CAP_PM)   { g_acc.havePM   = true; g_acc.pm25 = r.pm25; g_acc.pm10 = r.pm10; }
  if (r.caps & CAP_CO2)  { g_acc.haveCO2  = true; g_acc.co2 = r.co2; }
}

bool meshHaveData() {
  return g_acc.tN || g_acc.hN || g_acc.pN || g_acc.haveWind || g_acc.haveRain ||
         g_acc.haveSoil || g_acc.haveLux || g_acc.havePM || g_acc.haveCO2;
}

bool meshPeriodicSend() {
  if (!meshHaveData()) {
    Serial.println("[mesh] nessun dato da inviare, salto il giro");
    return false;
  }
  bool sent = false;
  uint32_t now = timeSyncValid() ? (uint32_t)time(nullptr) : 0;

  // --- EnvironmentMetrics (telemetry.proto) ---
  uint8_t env[96];
  size_t e = 0;
  if (g_acc.tN) e = pbFloatField(env, e, 1, (float)(g_acc.tSum / g_acc.tN));
  if (g_acc.hN) e = pbFloatField(env, e, 2, (float)(g_acc.hSum / g_acc.hN));
  if (g_acc.pN) e = pbFloatField(env, e, 3, (float)(g_acc.pSum / g_acc.pN));
  if (g_acc.haveLux)  e = pbFloatField(env, e, 9, g_acc.lux);
  if (g_acc.haveWind) {
    e = pbVarintField(env, e, 13, g_acc.windDir);   // wind_direction (uint32)
    e = pbFloatField (env, e, 14, g_acc.windAvg);    // wind_speed
    e = pbFloatField (env, e, 16, g_acc.windGust);   // wind_gust
  }
  if (g_acc.haveRain) {
    float r1  = history.rainDeltaMm(g_acc.rainMm, 3600UL * 1000UL);
    float r24 = history.rainDeltaMm(g_acc.rainMm, 24UL * 3600UL * 1000UL);
    e = pbFloatField(env, e, 19, r1);                // rainfall_1h
    e = pbFloatField(env, e, 20, r24);               // rainfall_24h
  }
  if (g_acc.haveSoil) e = pbVarintField(env, e, 21, g_acc.soil);  // soil_moisture (uint32)

  if (e > 0) {
    uint8_t tel[112];
    size_t t = 0;
    t = pbFixed32Field(tel, t, 1, now);              // time (0 se non sync)
    t = pbBytesField(tel, t, 3, env, e);             // environment_metrics
    sent |= sendData(PORT_TELEMETRY, tel, t, 0);
  }

  // --- AirQualityMetrics (pacchetto Telemetry separato) ---
  if (g_acc.havePM || g_acc.haveCO2) {
    uint8_t aq[48];
    size_t a = 0;
    if (g_acc.havePM) {
      a = pbVarintField(aq, a, 1, (uint32_t)(g_acc.pm10 + 0.5f));   // pm10_standard
      a = pbVarintField(aq, a, 2, (uint32_t)(g_acc.pm25 + 0.5f));   // pm25_standard
    }
    if (g_acc.haveCO2) a = pbVarintField(aq, a, 10, g_acc.co2);     // co2

    uint8_t tel[64];
    size_t t = 0;
    t = pbFixed32Field(tel, t, 1, now);
    t = pbBytesField(tel, t, 4, aq, a);              // air_quality_metrics
    sent |= sendData(PORT_TELEMETRY, tel, t, 0);
  }

  g_acc = Accum();   // reset
  return sent;
}

// ---------------------------------------------------------------------------
bool meshSendNodeInfo() {
  char nodeIdStr[12];
  snprintf(nodeIdStr, sizeof(nodeIdStr), "!%08lx", (unsigned long)s_nodeId);

  uint8_t user[120];
  size_t u = 0;
  u = pbBytesField(user, u, 1, (const uint8_t *)nodeIdStr,        strlen(nodeIdStr));
  u = pbBytesField(user, u, 2, (const uint8_t *)MESH_LONG_NAME,   strlen(MESH_LONG_NAME));
  u = pbBytesField(user, u, 3, (const uint8_t *)s_shortName,      strlen(s_shortName));
  u = pbVarintField(user, u, 5, 43);   // hw_model: HELTEC_V3 = 43
  u = pbVarintField(user, u, 9, 1);    // is_unmessagable: nodo TX-only
  return sendData(PORT_NODEINFO, user, u, 0);
}

// ---------------------------------------------------------------------------
bool meshSendPosition() {
#if !MESH_POS_ENABLED
  return false;
#else
  uint8_t pos[40];
  size_t p = 0;
  p = pbFixed32Field(pos, p, 1, (uint32_t)(int32_t)MESH_LAT_I);   // latitude_i (sfixed32)
  p = pbFixed32Field(pos, p, 2, (uint32_t)(int32_t)MESH_LON_I);   // longitude_i
  p = pbVarintField(pos, p, 3, (uint64_t)(uint32_t)MESH_ALT_M);   // altitude
  if (timeSyncValid())
    p = pbFixed32Field(pos, p, 4, (uint32_t)time(nullptr));        // time (field 4!)
  p = pbVarintField(pos, p, 21, 32);                               // precision_bits
  return sendData(PORT_POSITION, pos, p, 0);
#endif
}

// ---------------------------------------------------------------------------
bool meshSendText(const char *txt, uint8_t chanIdx) {
  size_t len = strlen(txt);
  if (len == 0 || len > 200) return false;
  return sendData(PORT_TEXT, (const uint8_t *)txt, len, chanIdx);
}
