// ============================================================================
// sensor_types.h — tipi condivisi del livello sensori
//
// Disaccoppiamento a tre livelli:
//   parser  ->  SensorReading (struct tipizzata con bitmask di capability)  ->
//   consumatori (display + mesh)
//
// Aggiungere un sensore  = un file parser che espone un SensorDriver.
// Aggiungere una misura  = un bit di capability + una schermata.
// I due non si toccano.
// ============================================================================
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Capability: una misura = un bit. La UI sceglie le schermate dall'OR dei bit
// dei dati recenti; la mesh sceglie quali metriche incapsulare.
// I nomi combaciano con sensors_catalog.json -> "capabilities".
// ---------------------------------------------------------------------------
enum SensorCap : uint16_t {
  CAP_TEMP      = 1u << 0,
  CAP_HUM       = 1u << 1,
  CAP_PRESSURE  = 1u << 2,
  CAP_RAIN      = 1u << 3,
  CAP_WIND      = 1u << 4,
  CAP_UV        = 1u << 5,
  CAP_LIGHTNING = 1u << 6,
  CAP_PM        = 1u << 7,
  CAP_CO2       = 1u << 8,
  CAP_SOIL      = 1u << 9,
  CAP_LEAK      = 1u << 10,
};

// ---------------------------------------------------------------------------
// SensorReading — risultato tipizzato di una decodifica.
// Il parser setta SOLO i campi che conosce e i corrispondenti bit in 'caps'.
// I consumatori leggono esclusivamente i campi i cui bit sono settati.
// ---------------------------------------------------------------------------
struct SensorReading {
  uint16_t    caps = 0;          // OR dei SensorCap valorizzati
  uint32_t    id   = 0;          // id del singolo sensore (per distinguere piu' unita')
  const char *model = "";        // stringa statica del modello (es. "WH32")

  // --- termo/igro/baro ---
  float tempC       = 0;         // CAP_TEMP
  float humidity    = 0;         // CAP_HUM   (%)
  float pressureHpa = 0;         // CAP_PRESSURE

  // --- pioggia ---
  float rainMm      = 0;         // CAP_RAIN  (contatore cumulativo, passi sensore)

  // --- vento ---
  float windAvgMs   = 0;         // CAP_WIND  (m/s)
  float windGustMs  = 0;         // CAP_WIND  (m/s)
  float windDirDeg  = 0;         // CAP_WIND  (gradi, 0=N)

  // --- luce / UV ---
  float uvIndex     = 0;         // CAP_UV
  float lightLux    = 0;         // CAP_UV    (lux, opzionale)

  // --- fulmini ---
  uint8_t  lightningDistKm = 63; // CAP_LIGHTNING (63 = nessun fulmine)
  uint32_t lightningCount  = 0;  // CAP_LIGHTNING (conteggio grezzo nel pacchetto)
  bool     lightningCountValid = false;

  // --- qualita' aria ---
  float    pm25 = 0;             // CAP_PM (ug/m3)
  float    pm10 = 0;             // CAP_PM (ug/m3)
  uint16_t co2  = 0;             // CAP_CO2 (ppm)

  // --- suolo / perdita ---
  uint8_t soilMoisture = 0;      // CAP_SOIL (%)
  bool    leak         = false;  // CAP_LEAK

  // --- comuni ---
  bool  battLow = false;
  float battV   = 0;
  float rssi    = 0;
};

// ---------------------------------------------------------------------------
// Interfaccia uniforme: free-function pointer (niente vtable, footprint minimo).
//   match() : true se il buffer appartiene a questo sensore (controllo rapido)
//   parse() : decodifica + valida (CRC/checksum); riempie 'out' e ritorna true
// ---------------------------------------------------------------------------
typedef bool (*SensorMatchFn)(const uint8_t *buf, size_t len);
typedef bool (*SensorParseFn)(const uint8_t *buf, size_t len, float rssi,
                              SensorReading &out);

struct SensorDriver {
  const char    *model;
  uint16_t       caps;     // capability dichiarate dal driver (per documentazione/UI)
  SensorMatchFn  match;
  SensorParseFn  parse;
};
