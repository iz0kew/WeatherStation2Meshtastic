// ============================================================================
// variant.cpp — mappatura pin IDENTITA' per schede DIY nRF52840 in formato
//   Pro-Micro/NiceNano + HT-RA62 (MASN, FakeTec — vedi variant.h).
//   g_ADigitalPinMap[N] == N per ogni N: il numero di "pin Arduino" passato a
//   pinMode()/digitalWrite()/analogRead() corrisponde esattamente alla GPIO
//   fisica nRF52 raw (porta*32+pin). Vedi variant.h per i dettagli.
// ============================================================================
#include "variant.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "nrf.h"

const uint32_t g_ADigitalPinMap[] =
{
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31,   // P0.00 .. P0.31
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47,                   // P1.00 .. P1.15
};

void initVariant()
{
  // Nessuna inizializzazione specifica: nessuna delle board che usano questa
  // variant attiva PIN_BUTTON/HAS_STATUS_LED (vedi board_config.h), quindi
  // non c'e' hardware onboard da abilitare qui in questa prima versione.
}
