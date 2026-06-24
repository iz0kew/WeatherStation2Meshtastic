// ============================================================================
// registry.h — elenco dei driver sensori attivi (popolato a compile-time).
// L'array reale e' costruito in registry.cpp con blocchi #ifdef ENABLE_<id>
// derivati da src/config/generated_config.h (prodotto da configure_sensors.py).
// ============================================================================
#pragma once
#include "sensor_types.h"

// Array dei driver compilati in questa build (solo i sensori abilitati).
extern const SensorDriver *const g_sensorDrivers[];
extern const size_t              g_sensorDriverCount;
