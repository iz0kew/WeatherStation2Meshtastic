// ============================================================================
// radio.h — gestore unico dell'SX1262 con TRE modalita' su una sola radio:
//
//   RADIO_FSK_RX   ricezione GFSK dei sensori (bitrate/sync dal GRUPPO,
//                  derivati in src/config/generated_config.h: RADIO_*)
//   RADIO_LORA_TX  trasmissione Meshtastic (parametri MESH_* da user_config.h);
//                  e' transitoria: dopo il TX la radio torna in RADIO_FSK_RX
//   RADIO_LORA_RX  ricezione LoRa temporanea usata dal timesync all'avvio
//                  (stessa frequenza/preset del TX Meshtastic)
//
// Un solo proprietario dello switch di modalita': il timing dei pacchetti
// Meshtastic non viene mai interrotto da accessi radio paralleli.
// ============================================================================
#pragma once
#include <Arduino.h>
#include <RadioLib.h>

// Lunghezza fissa del pacchetto FSK letto dopo il sync word. Deve coprire il
// frame piu' lungo del gruppo (WS90 = 32 byte). Frame piu' corti validano
// comunque solo i propri byte tramite CRC/checksum.
#define RADIO_FSK_PKT_LEN 32

enum RadioMode { RADIO_OFF, RADIO_FSK_RX, RADIO_LORA_RX, RADIO_LORA_TX };

// Oggetto radio e flag ISR (condivisi con timesync.cpp)
extern SX1262        radio;
extern volatile bool rxFlag;

// Inizializza SPI + modulo SX1262 (NON avvia la ricezione).
void radioInit();

// (Ri)configura la radio nelle tre modalita'.
void radioStartFSK();        // ricezione sensori (GFSK)
void radioStartLoRaRX();     // ricezione LoRa per timesync

// Trasmette un pacchetto Meshtastic gia' formattato (header+payload cifrato),
// poi ripristina automaticamente RADIO_FSK_RX. Ritorna true se TX ok.
bool radioTransmitMeshtastic(const uint8_t *pkt, size_t len);

// Legge il frame FSK ricevuto (RADIO_FSK_PKT_LEN byte) e riavvia la ricezione.
// Ritorna lo status RadioLib; *rssiOut riceve l'RSSI in dBm.
int  radioReadFSK(uint8_t *buf, float *rssiOut);

RadioMode radioGetMode();
