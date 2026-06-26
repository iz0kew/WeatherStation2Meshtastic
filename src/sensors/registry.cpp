// ============================================================================
// registry.cpp — costruzione a compile-time della tabella dei driver attivi.
//
// Ogni file parser definisce un  `const SensorDriver DRIVER_<id>`  ed e'
// compilato SOLO se abilitato (build_src_filter, impostato da
// configure_sensors.py). Qui includiamo generated_config.h e referenziamo i
// driver sotto lo stesso #ifdef ENABLE_<id>, così l'array contiene esattamente
// i sensori scelti in settings.ini.
//
// Aggiungere un sensore = una voce nel catalogo + un file parser + due righe
// qui (extern + puntatore, entrambe sotto il suo #ifdef). Nient'altro.
// ============================================================================
#include "registry.h"
#include "../config/generated_config.h"

// ---- dichiarazioni extern (una per sensore abilitato) ----------------------
#ifdef ENABLE_WH32
extern const SensorDriver DRIVER_WH32;
#endif
#ifdef ENABLE_WH31E
extern const SensorDriver DRIVER_WH31E;
#endif
#ifdef ENABLE_WH40
extern const SensorDriver DRIVER_WH40;
#endif
#ifdef ENABLE_WH57
extern const SensorDriver DRIVER_WH57;
#endif
#ifdef ENABLE_WH51
extern const SensorDriver DRIVER_WH51;
#endif
#ifdef ENABLE_WH45
extern const SensorDriver DRIVER_WH45;
#endif
#ifdef ENABLE_WH46
extern const SensorDriver DRIVER_WH46;
#endif
#ifdef ENABLE_WN34
extern const SensorDriver DRIVER_WN34;
#endif
#ifdef ENABLE_WS68
extern const SensorDriver DRIVER_WS68;
#endif
#ifdef ENABLE_WH65B
extern const SensorDriver DRIVER_WH65B;
#endif
#ifdef ENABLE_WS80
extern const SensorDriver DRIVER_WS80;
#endif
#ifdef ENABLE_WS90
extern const SensorDriver DRIVER_WS90;
#endif
#ifdef ENABLE_WS85
extern const SensorDriver DRIVER_WS85;
#endif
#ifdef ENABLE_TX35
extern const SensorDriver DRIVER_TX35;
#endif
#ifdef ENABLE_BRESSER_5IN1
extern const SensorDriver DRIVER_BRESSER_5IN1;
#endif
#ifdef ENABLE_BRESSER_6IN1
extern const SensorDriver DRIVER_BRESSER_6IN1;
#endif
#ifdef ENABLE_BRESSER_7IN1_AQ
extern const SensorDriver DRIVER_BRESSER_7IN1_AQ;
#endif
#ifdef ENABLE_BRESSER_7IN1
extern const SensorDriver DRIVER_BRESSER_7IN1;
#endif
#ifdef ENABLE_BRESSER_LIGHTNING
extern const SensorDriver DRIVER_BRESSER_LIGHTNING;
#endif
#ifdef ENABLE_BRESSER_LEAKAGE
extern const SensorDriver DRIVER_BRESSER_LEAKAGE;
#endif

// ---- tabella dei driver attivi ---------------------------------------------
const SensorDriver *const g_sensorDrivers[] = {
#ifdef ENABLE_WH32
  &DRIVER_WH32,
#endif
#ifdef ENABLE_WH31E
  &DRIVER_WH31E,
#endif
#ifdef ENABLE_WH40
  &DRIVER_WH40,
#endif
#ifdef ENABLE_WH57
  &DRIVER_WH57,
#endif
#ifdef ENABLE_WH51
  &DRIVER_WH51,
#endif
#ifdef ENABLE_WH45
  &DRIVER_WH45,
#endif
#ifdef ENABLE_WH46
  &DRIVER_WH46,
#endif
#ifdef ENABLE_WN34
  &DRIVER_WN34,
#endif
#ifdef ENABLE_WS68
  &DRIVER_WS68,
#endif
#ifdef ENABLE_WH65B
  &DRIVER_WH65B,
#endif
#ifdef ENABLE_WS80
  &DRIVER_WS80,
#endif
#ifdef ENABLE_WS90
  &DRIVER_WS90,
#endif
#ifdef ENABLE_WS85
  &DRIVER_WS85,
#endif
#ifdef ENABLE_TX35
  &DRIVER_TX35,
#endif
#ifdef ENABLE_BRESSER_5IN1
  &DRIVER_BRESSER_5IN1,
#endif
#ifdef ENABLE_BRESSER_6IN1
  &DRIVER_BRESSER_6IN1,
#endif
#ifdef ENABLE_BRESSER_7IN1_AQ
  &DRIVER_BRESSER_7IN1_AQ,
#endif
#ifdef ENABLE_BRESSER_7IN1
  &DRIVER_BRESSER_7IN1,
#endif
#ifdef ENABLE_BRESSER_LIGHTNING
  &DRIVER_BRESSER_LIGHTNING,
#endif
#ifdef ENABLE_BRESSER_LEAKAGE
  &DRIVER_BRESSER_LEAKAGE,
#endif
  nullptr   // sentinella: garantisce un array non vuoto anche in casi limite
};

const size_t g_sensorDriverCount =
    (sizeof(g_sensorDrivers) / sizeof(g_sensorDrivers[0])) - 1;  // esclude la sentinella
