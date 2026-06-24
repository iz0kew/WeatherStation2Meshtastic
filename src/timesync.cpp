// ============================================================================
// timesync.cpp — Sincronizzazione orario da traffico Meshtastic (porting EcoWitt)
//
//   1. radioStartLoRaRX() mette l'SX1262 in ricezione LoRa con i parametri TX.
//   2. Ogni pacchetto: filtro channel hash -> decifra AES-CTR -> scansiona il
//      protobuf cercando un timestamp unix plausibile (2020-2050).
//   3. State machine a conferme multiple (immune al "poison first sample").
//   A fine sync la radio torna in FSK: lo gestisce main.cpp (blocco fskStarted)
//   non appena timeSyncDone() ritorna true.
// ============================================================================
#include "timesync.h"
#include "radio/radio.h"     // SX1262 radio, rxFlag, radioStartLoRaRX()
#include "user_config.h"
#include <Crypto.h>
#include <AES.h>
#include <CTR.h>
#include <time.h>
#include <sys/time.h>
#include <cstring>

// PSK canale principale e testo (da user_config.h)
static const uint8_t s_psk0[MESH_CHANNEL_KEY_SIZE]      = MESH_CHANNEL_KEY;
static const uint8_t s_psk1[MESH_TEXT_CHANNEL_KEY_SIZE] = MESH_TEXT_CHANNEL_KEY;

static TimeSyncState s_state        = TS_WAITING;
static uint32_t      s_firstEpoch   = 0;
static uint8_t       s_confirms     = 0;
static uint32_t      s_altEpoch     = 0;
static uint8_t       s_altConfirms  = 0;
static uint32_t      s_windowEndMs  = 0;
static uint8_t       s_chanHash0    = 0;
static uint8_t       s_chanHash1    = 0;

static uint8_t xorHash(const uint8_t *p, size_t len) {
  uint8_t h = 0;
  for (size_t i = 0; i < len; i++) h ^= p[i];
  return h;
}

static void aesCrypt(uint8_t *out, const uint8_t *in, size_t len,
                     const uint8_t *key, size_t keyLen, const uint8_t *iv) {
  if (keyLen == 32) {
    CTR<AES256> ctr; ctr.setKey(key, 32); ctr.setIV(iv, 16); ctr.encrypt(out, in, len);
  } else {
    CTR<AES128> ctr; ctr.setKey(key, 16); ctr.setIV(iv, 16); ctr.encrypt(out, in, len);
  }
}

// --- Finestra timestamp valida derivata dalla data di BUILD ----------------
// Il firmware non puo' girare prima di essere stato compilato: la data di build
// e' un limite inferiore legittimo che restringe la finestra e riduce i falsi
// positivi nello scanner protobuf. __DATE__/__TIME__ sono in ora LOCALE del PC,
// quindi sottraiamo un margine (2 giorni) per assorbire fuso e scarto orologio.
static int _bY() {
  return (__DATE__[7]-'0')*1000 + (__DATE__[8]-'0')*100 + (__DATE__[9]-'0')*10 + (__DATE__[10]-'0');
}
static int _bM() {
  return (__DATE__[0]=='J' && __DATE__[1]=='a') ? 1 :
         (__DATE__[0]=='F')                     ? 2 :
         (__DATE__[0]=='M' && __DATE__[2]=='r') ? 3 :
         (__DATE__[0]=='A' && __DATE__[1]=='p') ? 4 :
         (__DATE__[0]=='M')                     ? 5 :
         (__DATE__[0]=='J' && __DATE__[2]=='n') ? 6 :
         (__DATE__[0]=='J')                     ? 7 :
         (__DATE__[0]=='A')                     ? 8 :
         (__DATE__[0]=='S')                     ? 9 :
         (__DATE__[0]=='O')                     ? 10 :
         (__DATE__[0]=='N')                     ? 11 : 12;
}
static int _bD() {
  return (__DATE__[4]==' ' ? 0 : (__DATE__[4]-'0'))*10 + (__DATE__[5]-'0');
}
// Giorni dall'epoch (algoritmo "days from civil", H. Hinnant)
static long _civil(int y, int m, int d) {
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}
static const uint32_t BUILD_EPOCH = (uint32_t)(
    _civil(_bY(), _bM(), _bD()) * 86400L
  + ((__TIME__[0]-'0')*10 + (__TIME__[1]-'0')) * 3600
  + ((__TIME__[3]-'0')*10 + (__TIME__[4]-'0')) * 60
  + ((__TIME__[6]-'0')*10 + (__TIME__[7]-'0')));

static const uint32_t EPOCH_MIN = BUILD_EPOCH - 2UL * 86400UL;        // build - 2 giorni
static const uint32_t EPOCH_MAX = BUILD_EPOCH + 10UL * 365UL * 86400UL; // ~build + 10 anni

static uint32_t pbScanTimestamp(const uint8_t *buf, size_t len) {
  size_t i = 0;
  while (i < len) {
    uint64_t tag = 0; uint8_t shift = 0;
    while (i < len && shift < 64) {
      uint8_t b = buf[i++];
      tag |= (uint64_t)(b & 0x7F) << shift; shift += 7;
      if (!(b & 0x80)) break;
    }
    if (shift == 0) break;
    uint8_t wire = (uint8_t)(tag & 0x07);
    switch (wire) {
      case 0: {  // varint
        uint64_t val = 0; uint8_t sh = 0;
        while (i < len && sh < 64) {
          uint8_t b = buf[i++];
          val |= (uint64_t)(b & 0x7F) << sh; sh += 7;
          if (!(b & 0x80)) break;
        }
        if (val >= EPOCH_MIN && val <= EPOCH_MAX) return (uint32_t)val;
        break;
      }
      case 1: if (i + 8 > len) return 0; i += 8; break;
      case 2: {  // length-delimited: ricorre nel sotto-messaggio
        uint64_t l = 0; uint8_t sh = 0;
        while (i < len && sh < 64) {
          uint8_t b = buf[i++];
          l |= (uint64_t)(b & 0x7F) << sh; sh += 7;
          if (!(b & 0x80)) break;
        }
        if (l > len - i) return 0;
        uint32_t ts = pbScanTimestamp(buf + i, (size_t)l);
        if (ts) return ts;
        i += (size_t)l;
        break;
      }
      case 5: {  // 32-bit fixed
        if (i + 4 > len) return 0;
        uint32_t val; memcpy(&val, buf + i, 4); i += 4;
        if (val >= EPOCH_MIN && val <= EPOCH_MAX) return val;
        break;
      }
      default: return 0;
    }
  }
  return 0;
}

static uint32_t tryExtractTimestamp(const uint8_t *pkt, size_t len) {
  if (len < 17 || len > 256) return 0;
  uint8_t chanHash = pkt[13];

  uint32_t fromNode, pktId;
  memcpy(&fromNode, pkt + 4, 4);
  memcpy(&pktId,    pkt + 8, 4);

  uint8_t nonce[16] = {0};
  uint64_t id64 = pktId;
  memcpy(nonce, &id64, 8);
  memcpy(nonce + 8, &fromNode, 4);

  const uint8_t *cipher = pkt + 16;
  size_t          cLen  = len - 16;
  if (cLen == 0 || cLen > 208) return 0;

  struct { const uint8_t *psk; size_t pskLen; uint8_t hash; } channels[] = {
    { s_psk0, sizeof(s_psk0), s_chanHash0 },
    { s_psk1, sizeof(s_psk1), s_chanHash1 },
  };
  const int nChannels = (s_chanHash0 == s_chanHash1) ? 1 : 2;

  for (int ch = 0; ch < nChannels; ch++) {
    if (chanHash != channels[ch].hash) continue;
    uint8_t plain[208];
    aesCrypt(plain, cipher, cLen, channels[ch].psk, channels[ch].pskLen, nonce);
    uint8_t firstField = plain[0] >> 3;
    uint8_t firstWire  = plain[0] & 0x07;
    if (firstField < 1 || firstField > 9) continue;
    if (firstWire != 0 && firstWire != 2 && firstWire != 5) continue;
    uint32_t ts = pbScanTimestamp(plain, cLen);
    if (ts) {
      Serial.printf("[tsync] pkt da !%08lx  ch%d  len=%u  ts=%lu\n",
                    (unsigned long)fromNode, ch, (unsigned)len, (unsigned long)ts);
      return ts;
    }
  }
  return 0;
}

static void applyEpoch(uint32_t epoch) {
  struct timeval tv = { (time_t)epoch, 0 };
  settimeofday(&tv, nullptr);
}

// ---------------------------------------------------------------------------
void timeSyncBegin() {
  s_state       = TS_WAITING;
  s_firstEpoch  = 0; s_confirms = 0;
  s_altEpoch    = 0; s_altConfirms = 0;
  s_windowEndMs = millis() + TSYNC_WINDOW_MS;

  s_chanHash0 = xorHash((const uint8_t *)MESH_CHANNEL_NAME, strlen(MESH_CHANNEL_NAME))
              ^ xorHash(s_psk0, sizeof(s_psk0));
  s_chanHash1 = xorHash((const uint8_t *)MESH_TEXT_CHANNEL_NAME, strlen(MESH_TEXT_CHANNEL_NAME))
              ^ xorHash(s_psk1, sizeof(s_psk1));

  setenv("TZ", MESH_TIMEZONE, 1);
  tzset();
  Serial.printf("[tsync] timezone: '%s'\n", MESH_TIMEZONE);
  Serial.printf("[tsync] avvio — finestra %lu s, hash ch0=0x%02x ch1=0x%02x\n",
                (unsigned long)(TSYNC_WINDOW_MS / 1000), s_chanHash0, s_chanHash1);

  radioStartLoRaRX();
}

bool timeSyncDone() { return s_state == TS_CONFIRMED || s_state == TS_TIMEOUT; }

void timeSyncTick() {
  if (timeSyncDone()) return;
  uint32_t now = millis();

  if ((int32_t)(now - s_windowEndMs) >= 0) {
    Serial.printf("[tsync] timeout — stato: %s\n",
                  s_state == TS_UNCONFIRMED ? "non confermato" : "nessun campione");
    s_state = TS_TIMEOUT;
    return;
  }

  if (!rxFlag) return;
  rxFlag = false;

  size_t pktLen = (size_t)radio.getPacketLength();
  if (pktLen < 17 || pktLen > 240) { radio.startReceive(); return; }

  uint8_t buf[256];
  int st = radio.readData(buf, pktLen);
  radio.startReceive();
  if (st != RADIOLIB_ERR_NONE) return;

  uint32_t epoch = tryExtractTimestamp(buf, pktLen);
  if (epoch == 0) return;

  if (s_state == TS_WAITING) {
    s_firstEpoch = epoch; s_confirms = 1;
    s_altEpoch = 0; s_altConfirms = 0;
    s_state = TS_UNCONFIRMED;
    Serial.printf("[tsync] primo campione: %lu — in attesa di %d conferme\n",
                  (unsigned long)epoch, TSYNC_CONFIRM_MIN - 1);

  } else if (s_state == TS_UNCONFIRMED) {
    int32_t deltaS = (int32_t)epoch - (int32_t)s_firstEpoch;
    if (deltaS >= -TSYNC_MAX_DELTA_S && deltaS <= TSYNC_MAX_DELTA_S) {
      s_altEpoch = 0; s_altConfirms = 0;
      s_confirms++;
      Serial.printf("[tsync] conferma %d/%d (delta %+lds)\n",
                    s_confirms, TSYNC_CONFIRM_MIN, (long)deltaS);
      if (s_confirms >= TSYNC_CONFIRM_MIN) {
        applyEpoch(epoch);
        s_state = TS_CONFIRMED;
        Serial.printf("[tsync] orario confermato: %lu\n", (unsigned long)epoch);
      }
      return;
    }
    if (s_altEpoch == 0) {
      s_altEpoch = epoch; s_altConfirms = 1;
      Serial.printf("[tsync] campione rifiutato (delta %+lds): avvio cluster alt %lu [1/%d]\n",
                    (long)deltaS, (unsigned long)epoch, TSYNC_ALT_MIN);
    } else {
      int32_t deltaAlt = (int32_t)epoch - (int32_t)s_altEpoch;
      if (deltaAlt >= -TSYNC_MAX_DELTA_S && deltaAlt <= TSYNC_MAX_DELTA_S) {
        s_altConfirms++;
        Serial.printf("[tsync] cluster alt: conferma %d/%d (delta alt %+lds)\n",
                      s_altConfirms, TSYNC_ALT_MIN, (long)deltaAlt);
        if (s_altConfirms >= TSYNC_ALT_MIN) {
          Serial.printf("[tsync] riferimento errato — reset su cluster alt: ancora=%lu\n",
                        (unsigned long)s_altEpoch);
          s_firstEpoch = s_altEpoch; s_confirms = s_altConfirms;
          s_altEpoch = 0; s_altConfirms = 0;
          if (s_confirms >= TSYNC_CONFIRM_MIN) {
            applyEpoch(epoch);
            s_state = TS_CONFIRMED;
            Serial.printf("[tsync] orario confermato: %lu\n", (unsigned long)epoch);
          }
        }
      } else {
        Serial.printf("[tsync] cluster alt azzerato (delta alt %+lds) — nuovo anchor: %lu\n",
                      (long)deltaAlt, (unsigned long)epoch);
        s_altEpoch = epoch; s_altConfirms = 1;
      }
    }
  }
}

TimeSyncStatus timeSyncGetStatus() {
  uint32_t now = millis();
  int32_t secsLeft = (int32_t)(s_windowEndMs - now) / 1000;
  if (secsLeft < 0) secsLeft = 0;
  return { s_state, s_firstEpoch, s_confirms, secsLeft };
}

bool timeSyncValid() { return s_state == TS_CONFIRMED; }
