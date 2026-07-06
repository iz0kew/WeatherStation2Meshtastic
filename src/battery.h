// ============================================================================
// battery.h — lettura tensione batteria (Heltec V3/V4, partitore su VBAT).
// batteryRead() ritorna false se la batteria non e' collegata (il partitore
// legge ~0V): in quel caso la scheda e' alimentata solo dal cavo USB.
// ============================================================================
#pragma once
#include <Arduino.h>

bool batteryRead(float &volts, int &percent);
