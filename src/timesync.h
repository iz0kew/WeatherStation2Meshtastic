// ============================================================================
// timesync.h — Sincronizzazione orario da rete Meshtastic (senza RTC ne' GPS)
//
// All'avvio la scheda resta in ascolto LoRa per TSYNC_WINDOW_MS (5 min).
// Ogni pacchetto Meshtastic ricevuto sul canale configurato viene decifrato e
// scansionato alla ricerca di un timestamp unix (campo time di Telemetry/Position).
// Algoritmo a conferme multiple immune al "poison first sample".
//
// PUNTO CRITICO architettura radio: a fine sync la radio torna in FSK per i
// sensori. timesync e' il terzo "modo" RX-LoRa del gestore radio (radio.cpp).
// ============================================================================
#pragma once
#include <Arduino.h>

#define TSYNC_WINDOW_MS    (5UL * 60UL * 1000UL)   // finestra ascolto all'avvio: 5 min
#define TSYNC_CONFIRM_MIN  3                          // conferme minime per accettare l'orario
#define TSYNC_MAX_DELTA_S  120                        // scarto max tra campioni (s)
#define TSYNC_ALT_MIN      2                          // pacchetti coerenti nel cluster alternativo

enum TimeSyncState {
  TS_WAITING,       // finestra attiva, nessun timestamp ancora
  TS_UNCONFIRMED,   // primo timestamp ricevuto, attesa conferme
  TS_CONFIRMED,     // orario confermato da TSYNC_CONFIRM_MIN campioni concordi
  TS_TIMEOUT        // finestra scaduta senza raggiungere la conferma
};

struct TimeSyncStatus {
  TimeSyncState state;
  uint32_t      firstEpoch;
  uint8_t       confirms;
  int32_t       secsLeft;
};

// Chiamare da setup() DOPO meshInit() e DOPO SPI/radio init.
void timeSyncBegin();
// true se concluso (confermato o timeout). A quel punto la radio e' gia' in FSK.
bool timeSyncDone();
// Da chiamare a ogni loop() finche' timeSyncDone()==false.
void timeSyncTick();
TimeSyncStatus timeSyncGetStatus();
bool timeSyncValid();
