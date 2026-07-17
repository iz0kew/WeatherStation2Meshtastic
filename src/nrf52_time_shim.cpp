// ============================================================================
// nrf52_time_shim.cpp — il core Arduino nRF52 (Adafruit/Seeed, senza RTC di
//   sistema) non implementa settimeofday()/gettimeofday(): newlib "nosys" non
//   fornisce nulla di funzionante per queste due syscall. timesync.cpp pero'
//   si aspetta settimeofday() per applicare l'orario ricevuto dalla rete
//   Meshtastic, e time()/localtime_r() (usati ovunque: main.cpp, astro.cpp,
//   ui.cpp) passano da _gettimeofday(). Qui si implementano entrambe sullo
//   stesso riferimento: un offset epoch fissato al momento della sync e
//   sommato a millis()/1000. Heltec/ESP32 non serve (libc gia' completa).
// ============================================================================
#if defined(ARDUINO_ARCH_NRF52)

#include <Arduino.h>
#include <sys/time.h>

static uint32_t s_epochAtMillis0 = 0;   // epoch corrispondente a millis()==0

extern "C" int settimeofday(const struct timeval *tv, const struct timezone *tz) {
  (void)tz;
  if (!tv) return -1;
  s_epochAtMillis0 = (uint32_t)tv->tv_sec - millis() / 1000;
  return 0;
}

extern "C" int _gettimeofday(struct timeval *tv, void *tz) {
  (void)tz;
  if (!tv) return -1;
  uint32_t ms = millis();
  tv->tv_sec  = s_epochAtMillis0 + ms / 1000;
  tv->tv_usec = (ms % 1000) * 1000;
  return 0;
}

#endif // ARDUINO_ARCH_NRF52
