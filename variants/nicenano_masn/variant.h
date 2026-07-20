// ============================================================================
// variant.h — variant PlatformIO locale e minimale per NiceNano nRF52840
//   (scheda MASN, modulo LoRa HT-RA62). Scritta da zero per questo progetto
//   (licenza MIT), NON copiata dal firmware Meshtastic (GPL-3.0) ne' da altri
//   pacchetti board di terzi.
//
//   Design volutamente "raw": g_ADigitalPinMap[] in variant.cpp e' una
//   mappatura IDENTITA' (pin Arduino N == GPIO fisica nRF52 N, con N =
//   porta*32+pin, es. P1.13 = 1*32+13 = 45). In questo modo le macro
//   PIN_LORA_*/PIN_VBAT_READ definite in src/board_config.h (blocco
//   BOARD_MASN_HTRA62) indirizzano esattamente la GPIO fisica voluta, senza
//   passare da un alias "Dx" specifico di una scheda di terzi che potrebbe
//   non corrispondere al pinout reale di MASN.
//
//   ATTENZIONE: i valori delle macro PIN_SPI_*/PIN_WIRE_* qui sotto DEVONO
//   restare allineati alle macro PIN_LORA_* in board_config.h, perche'
//   radio.cpp chiama SPI.begin() senza argomenti su nRF52 (usa quindi il bus
//   SPI hardware di default definito qui). Il pinout stesso NON e' verificato
//   sull'hardware MASN reale — vedi il commento in board_config.h.
// ============================================================================
#ifndef _NICENANO_MASN_VARIANT_H_
#define _NICENANO_MASN_VARIANT_H_

/** Master clock frequency */
#define VARIANT_MCK       (64000000ul)

// Molti cloni "ProMicro/NiceNano" non hanno il quarzo da 32.768 kHz montato
// (a differenza di Feather/XIAO): si usa l'RC interno per il clock LF per
// evitare un blocco all'avvio del SoftDevice se il quarzo non e' presente.
// Se il modulo NiceNano usato su MASN monta invece un quarzo LF, cambiare in
// USE_LFXO.
#define USE_LFRC

#include "WVariant.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// Copre l'intero spazio GPIO nRF52840 (P0.00..P0.31 = 0..31, P1.00..P1.15 =
// 32..47) con mappatura identita' in g_ADigitalPinMap (variant.cpp).
#define PINS_COUNT              (48)
#define NUM_DIGITAL_PINS        (48)
#define NUM_ANALOG_INPUTS       (1)
#define NUM_ANALOG_OUTPUTS      (0)
#define ADC_RESOLUTION          (12)

// Nessun LED/pulsante generico noto e verificato per MASN in questa board
// definition: main.cpp/led_status.cpp restano disattivati per questa scheda
// (nessun PIN_BUTTON/HAS_STATUS_LED in board_config.h).
#define LED_BUILTIN              (13)   // placeholder, non usato dal firmware
#define PIN_LED                  (LED_BUILTIN)
#define LED_STATE_ON              (1)   // richiesto dal core (wiring_digital.c), non usato dal firmware

// SPI hardware di default: DEVE combaciare con PIN_LORA_NSS/SCK/MOSI/MISO in
// board_config.h (BOARD_MASN_HTRA62), perche' radio.cpp usa SPI.begin() senza
// argomenti su nRF52.
#define SPI_INTERFACES_COUNT    (1)
#define PIN_SPI_MISO             (2)    // P0.02 — vedi PIN_LORA_MISO
#define PIN_SPI_MOSI             (47)   // P1.15 — vedi PIN_LORA_MOSI
#define PIN_SPI_SCK              (43)   // P1.11 — vedi PIN_LORA_SCK
static const uint8_t SS   = 45;         // P1.13 — vedi PIN_LORA_NSS
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

// Wire (I2C) non usato in questa build (nessun display, U8g2 escluso via
// lib_ignore): pin placeholder solo per soddisfare il core Arduino.
#define WIRE_INTERFACES_COUNT   (1)
#define PIN_WIRE_SDA             (4)    // P0.04, non cablato/usato
#define PIN_WIRE_SCL             (5)    // P0.05, non cablato/usato
static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

// Seriale USB-CDC standard del core Adafruit nRF52 (nessun UART hardware
// usato da questo firmware).
#define PIN_SERIAL1_RX            (33)  // P1.01, non cablato/usato
#define PIN_SERIAL1_TX            (34)  // P1.02, non cablato/usato
static const uint8_t RX = PIN_SERIAL1_RX;
static const uint8_t TX = PIN_SERIAL1_TX;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _NICENANO_MASN_VARIANT_H_
