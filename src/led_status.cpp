// ============================================================================
// led_status.cpp — pilota il LED RGB onboard (XIAO nRF52840) per mostrare lo
//   stato del time-sync al posto della schermata "Orario" (assente: niente
//   display su questa scheda). Solo per #ifdef HAS_STATUS_LED.
//
//   Blu lampeggiante  -> in ascolto, nessun campione confermato ancora.
//   Verde fisso (~3s) -> orario confermato, poi si spegne.
//   Rosso lampeggiante -> finestra scaduta senza conferma (nessun nodo
//                         Meshtastic in ascolto/portata durante la finestra).
// ============================================================================
#include "led_status.h"
#include "board_config.h"
#ifdef HAS_STATUS_LED

#include "timesync.h"
#include <Arduino.h>

#define LED_BLINK_MS         400UL
#define LED_SUCCESS_HOLD_MS  3000UL

static inline void setLed(bool r, bool g, bool b) {
#if LED_ACTIVE_LOW
  digitalWrite(PIN_LED_RED,   r ? LOW : HIGH);
  digitalWrite(PIN_LED_GREEN, g ? LOW : HIGH);
  digitalWrite(PIN_LED_BLUE,  b ? LOW : HIGH);
#else
  digitalWrite(PIN_LED_RED,   r ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, g ? HIGH : LOW);
  digitalWrite(PIN_LED_BLUE,  b ? HIGH : LOW);
#endif
}

void ledStatusInit() {
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  setLed(false, false, false);
}

void ledStatusTick() {
  static uint32_t confirmedAtMs = 0;
  static bool     latched       = false;

  TimeSyncStatus s   = timeSyncGetStatus();
  uint32_t       now = millis();
  bool blinkPhase = (now / LED_BLINK_MS) % 2 == 0;

  switch (s.state) {
    case TS_WAITING:
    case TS_UNCONFIRMED:
      latched = false;
      setLed(false, false, blinkPhase);
      break;
    case TS_CONFIRMED:
      if (!latched) { latched = true; confirmedAtMs = now; }
      if (now - confirmedAtMs < LED_SUCCESS_HOLD_MS) setLed(false, true, false);
      else                                            setLed(false, false, false);
      break;
    case TS_TIMEOUT:
      setLed(blinkPhase, false, false);
      break;
  }
}

#endif // HAS_STATUS_LED
