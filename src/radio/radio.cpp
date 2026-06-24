// ============================================================================
// radio.cpp — implementazione del gestore SX1262 a tre modalita'.
// Riusa lo switch TX-LoRa / RX-GFSK del progetto EcoWitt, estendendolo con il
// terzo modo RX-LoRa del timesync (un solo gestore di modalita').
// ============================================================================
#include "radio.h"
#include "../board_config.h"
#include "user_config.h"
#include "../config/generated_config.h"
#include <SPI.h>

// SX1262 onboard Heltec V3/V4
SX1262 radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY);

volatile bool rxFlag = false;
static void IRAM_ATTR onRxDone() { rxFlag = true; }

static RadioMode s_mode = RADIO_OFF;

// Parametri LoRa Meshtastic (specchio di meshtastic_pack.cpp)
static const uint8_t  LORA_SYNCWORD = 0x2B;
static const uint16_t LORA_PREAMBLE = 16;

RadioMode radioGetMode() { return s_mode; }

// ---------------------------------------------------------------------------
void radioInit() {
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
  s_mode = RADIO_OFF;
}

// ---------------------------------------------------------------------------
// Ricezione LoRa continua con i parametri Meshtastic (per il timesync).
// ---------------------------------------------------------------------------
void radioStartLoRaRX() {
  radio.standby();
  int st = radio.begin(MESH_FREQ_MHZ, MESH_BW_KHZ, MESH_SF, MESH_CR,
                       LORA_SYNCWORD, 10, LORA_PREAMBLE);   // pwr 10 dBm (RX), preamble 16
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] LoRa-RX begin fallito: %d\n", st);
    return;
  }
  radio.setDio2AsRfSwitch(true);
  radio.setCRC(true);
  radio.setDio1Action(onRxDone);
  radio.startReceive();
  s_mode = RADIO_LORA_RX;
}

// ---------------------------------------------------------------------------
// Ricezione GFSK dei sensori. Bitrate e sync word derivano dal GRUPPO
// (generated_config.h); frequenza/deviazione/banda dal front-end (user_config.h).
// ---------------------------------------------------------------------------
void radioStartFSK() {
  radio.standby();
  float bitrate_kbps = RADIO_BITRATE_BPS / 1000.0f;
  int st = radio.beginFSK(RX_FREQ_MHZ, bitrate_kbps, RX_FREQ_DEV_KHZ,
                          RX_BW_KHZ, 10, 16);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] beginFSK fallito: %d\n", st);
    return;
  }
  uint16_t sw = RADIO_SYNCWORD;
  uint8_t syncWord[] = { (uint8_t)((sw >> 8) & 0xFF), (uint8_t)(sw & 0xFF) };
  radio.setSyncWord(syncWord, sizeof(syncWord));
  radio.fixedPacketLengthMode(RADIO_FSK_PKT_LEN);
  radio.setCRC(0);                       // CRC verificato in software dai parser
  radio.setRxBoostedGainMode(true);
  radio.setDio2AsRfSwitch(true);
  radio.setDio1Action(onRxDone);
  radio.startReceive();
  s_mode = RADIO_FSK_RX;
}

// ---------------------------------------------------------------------------
int radioReadFSK(uint8_t *buf, float *rssiOut) {
  int st = radio.readData(buf, RADIO_FSK_PKT_LEN);
  if (rssiOut) *rssiOut = radio.getRSSI();
  radio.startReceive();
  return st;
}

// ---------------------------------------------------------------------------
// Trasmissione Meshtastic: standby -> begin LoRa TX -> transmit -> torna FSK.
// ---------------------------------------------------------------------------
bool radioTransmitMeshtastic(const uint8_t *pkt, size_t len) {
  radio.standby();
  s_mode = RADIO_LORA_TX;
  int st = radio.begin(MESH_FREQ_MHZ, MESH_BW_KHZ, MESH_SF, MESH_CR,
                       LORA_SYNCWORD, MESH_TX_POWER_DBM, LORA_PREAMBLE);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] LoRa-TX begin fallito: %d\n", st);
    radioStartFSK();
    return false;
  }
  radio.setDio2AsRfSwitch(true);
  radio.setCRC(true);

  st = radio.transmit((uint8_t *)pkt, len);
  radioStartFSK();                       // ripristina subito la ricezione sensori
  return st == RADIOLIB_ERR_NONE;
}
