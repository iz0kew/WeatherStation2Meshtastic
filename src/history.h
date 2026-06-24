// ============================================================================
// history.h — storico campionature per grafici e finestre pioggia 1h/24h
// Un campione ogni HISTORY_SAMPLE_MIN minuti, buffer circolare da 24 ore.
// ============================================================================
#pragma once
#include <Arduino.h>

#define HIST_MAX 288   // 24h a passi di 5 min (a 10 min ne servono 144)

struct HistSample {
  uint32_t ms;        // millis() del campione
  int16_t  t10;       // temperatura *10, INT16_MIN = non valido
  int16_t  rh;        // umidita' %, -1 = non valido
  int32_t  rain10;    // contatore pioggia *10 mm, -1 = non valido
  int32_t  strikes;   // fulmini totali, -1 = non valido
};

class History {
public:
  void add(const HistSample &s) {
    buf[head] = s;
    head = (head + 1) % HIST_MAX;
    if (n < HIST_MAX) n++;
  }
  uint16_t count() const { return n; }

  // i = 0 -> campione piu' vecchio
  const HistSample &get(uint16_t i) const {
    uint16_t idx = (uint16_t)((head + HIST_MAX - n + i) % HIST_MAX);
    return buf[idx];
  }

  // pioggia caduta nella finestra [now-windowMs, now] rispetto al contatore
  // cumulativo corrente; gestisce il reset del contatore (cambio batterie)
  float rainDeltaMm(float curMm, uint32_t windowMs) const {
    if (n == 0 || curMm < 0) return 0;
    uint32_t now = millis();
    int32_t oldRain10 = -1;
    for (uint16_t i = 0; i < n; i++) {        // dal piu' vecchio
      const HistSample &s = get(i);
      if ((uint32_t)(now - s.ms) <= windowMs && s.rain10 >= 0) {
        oldRain10 = s.rain10;
        break;
      }
    }
    if (oldRain10 < 0) return 0;
    float delta = curMm - oldRain10 * 0.1f;
    if (delta < 0) delta = curMm;             // contatore azzerato nel frattempo
    return delta;
  }

private:
  HistSample buf[HIST_MAX];
  uint16_t head = 0, n = 0;
};

extern History history;
