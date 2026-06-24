// ============================================================================
// ui.h — store aggregato dei dati recenti + gestione schermate per capability.
//
// uiSubmit() unisce ogni SensorReading nello store (per capability). La UI ruota
// SOLO le schermate per cui esiste un dato recente; le schermate di sistema
// (overview, mesh, orario, astro) sono sempre presenti.
//
// Ogni campo ricorda anche modello e ID del sensore che lo ha fornito: cosi'
// l'intestazione di ogni schermata mostra "Titolo MODELLO IDhex", utile per
// capire se si stanno ricevendo piu' sensori dello stesso tipo (es. la propria
// stazione + quella di un vicino: stesso modello, ID diverso).
// ============================================================================
#pragma once
#include "../sensors/sensor_types.h"
#include "../history.h"

#define UI_STALE_MS (15UL * 60UL * 1000UL)   // un dato piu' vecchio = "assente"

struct UiField {
  bool        valid    = false;
  float       val      = 0;
  uint32_t    lastSeen = 0;
  const char *model    = "";       // sigla del sensore che ha fornito il dato
  uint32_t    id       = 0;        // ID del sensore (distingue piu' unita')
};

struct UiStore {
  UiField tempC, humidity, pressureHpa, rainMm;
  UiField windAvg, windGust, windDir;
  UiField uv, lux;
  UiField soil;
  UiField pm25, pm10, co2;
  UiField leak;

  // fulmini (aggregati: il parser e' senza stato, il totale lo cumula la UI)
  bool        haveLightning      = false;
  bool        lightningHaveCount = false;
  uint32_t    lightningLastCount = 0;
  uint32_t    strikeTotal        = 0;
  uint8_t     lastDistKm         = 63;
  uint32_t    lightningLastSeen  = 0;
  const char *lightningModel     = "";
  uint32_t    lightningId        = 0;

  float       lastRssi = 0;
  bool        battLow  = false;
  uint32_t    lastAnySeen = 0;
  char        lastModel[16] = "";
};
extern UiStore g_ui;

// Descrittore di schermata (free-function pointer, niente vtable).
struct ScreenDef {
  const char *name;
  bool (*available)(uint32_t now);   // true se c'e' un dato recente da mostrare
  void (*draw)();
};

bool uiFieldFresh(const UiField &f, uint32_t now);

// Intestazioni: titolo a sinistra, tag sensore a destra + riga separatrice.
void uiHeader(const char *title, const char *tag);                       // tag libero
void uiHeaderSensor(const char *title, const char *model, uint32_t id);  // "MODELLO IDhex"

// --- Grafici 24h (leggono il buffer storico) -------------------------------
// Estrattore di una grandezza da un campione: ritorna il valore e setta ok.
typedef float (*HistGetFn)(const HistSample &s, bool &ok);
// Numero di campioni validi per quella grandezza (per decidere se mostrare il grafico).
int  uiGraphCount(HistGetFn get);
// Disegna un grafico a linee a tutta larghezza con auto-scala Y e min/max/corrente.
void uiDrawGraph(const char *title, const char *tag, HistGetFn get, const char *valFmt);

void uiInit();
void uiSubmit(const SensorReading &r);
void uiNextScreen();      // passa alla schermata successiva disponibile
void uiDraw();            // disegna la schermata corrente
