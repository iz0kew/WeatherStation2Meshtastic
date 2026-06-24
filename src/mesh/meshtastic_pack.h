// ============================================================================
// meshtastic_pack.h — invio pacchetti sulla rete Meshtastic.
// Implementa il protocollo radio Meshtastic "a mano": header 16 byte +
// payload protobuf (Data) cifrato AES128/256-CTR. Riusato e refattorizzato da
// iz0kew/EcoWittStation2Meshtastic (la trasmissione LoRa e' delegata al
// gestore radio in radio/radio.cpp).
// ============================================================================
#pragma once
#include <Arduino.h>
#include "../sensors/sensor_types.h"

void        meshInit();
uint32_t    meshNodeId();
uint32_t    meshPacketsSent();
const char *meshShortName();      // short name effettivo (auto = ultimi 4 hex del NodeID)

// --- Consumatore sensori: accumula le letture e invia la telemetria ---------
// Aggiunge una lettura all'accumulatore (medie T/H/P, ultimi valori vento/uv/
// pioggia/PM/CO2). Da chiamare nel dispatch per ogni SensorReading valida.
void meshSubmit(const SensorReading &r);

// Invia EnvironmentMetrics + AirQualityMetrics con i dati accumulati (canale 0)
// e azzera l'accumulatore. Ritorna true se almeno un pacchetto e' stato inviato.
bool meshPeriodicSend();

// Indica se l'accumulatore contiene almeno un dato da inviare.
bool meshHaveData();

// NodeInfo (short/long name) e posizione fissa: canale principale.
bool meshSendNodeInfo();
bool meshSendPosition();

// Messaggio di testo. chanIdx 0 = principale, 1 = canale testo (default).
bool meshSendText(const char *txt, uint8_t chanIdx = 1);
