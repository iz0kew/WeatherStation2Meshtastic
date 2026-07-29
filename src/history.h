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
  // cumulativo corrente. Somma gli incrementi campione-per-campione invece di
  // confrontare solo gli estremi: cosi' un reset del contatore del sensore
  // (cambio batterie, o gli auto-reset periodici tipici dei pluviometri
  // piezo Fine Offset come WS85/WS90) sporca solo il segmento in cui avviene
  // -- non l'intera finestra. Con il vecchio confronto singolo, un reset
  // avvenuto in un punto qualsiasi della finestra faceva collassare il
  // risultato sull'intero contatore corrente (es. il totale "24h" restava
  // pari all'intero accumulo dal reset per le successive 24h). Gestisce
  // anche piu' reset nella stessa finestra.
  float rainDeltaMm(float curMm, uint32_t windowMs) const {
    if (n == 0 || curMm < 0) return 0;
    uint32_t now = millis();
    float    total = 0;
    bool     haveBaseline = false;
    int32_t  prevRain10 = 0;
    for (uint16_t i = 0; i < n; i++) {        // dal piu' vecchio
      const HistSample &s = get(i);
      if ((uint32_t)(now - s.ms) > windowMs || s.rain10 < 0) continue;
      if (!haveBaseline) { prevRain10 = s.rain10; haveBaseline = true; continue; }
      int32_t diff10 = s.rain10 - prevRain10;
      total += (diff10 >= 0) ? diff10 * 0.1f : s.rain10 * 0.1f;  // negativo = reset
      prevRain10 = s.rain10;
    }
    if (!haveBaseline) return 0;              // nessun campione utile nella finestra
    float diffCur = curMm * 10.0f - prevRain10;
    total += (diffCur >= 0) ? diffCur * 0.1f : curMm;            // negativo = reset
    return total;
  }

  // fulmini caduti nella finestra [now-windowMs, now], stessa logica
  // anti-reset di rainDeltaMm (somma incrementi campione-per-campione
  // cosi' un reset del contatore del sensore sporca solo il segmento in
  // cui avviene, non l'intera finestra).
  uint32_t strikeDelta(uint32_t curStrikes, uint32_t windowMs) const {
    if (n == 0) return 0;
    uint32_t now = millis();
    uint32_t total = 0;
    bool     haveBaseline = false;
    int32_t  prevStrikes = 0;
    for (uint16_t i = 0; i < n; i++) {        // dal piu' vecchio
      const HistSample &s = get(i);
      if ((uint32_t)(now - s.ms) > windowMs || s.strikes < 0) continue;
      if (!haveBaseline) { prevStrikes = s.strikes; haveBaseline = true; continue; }
      int32_t diff = s.strikes - prevStrikes;
      total += (uint32_t)((diff >= 0) ? diff : s.strikes);       // negativo = reset
      prevStrikes = s.strikes;
    }
    if (!haveBaseline) return 0;              // nessun campione utile nella finestra
    int32_t diffCur = (int32_t)curStrikes - prevStrikes;
    total += (uint32_t)((diffCur >= 0) ? diffCur : (int32_t)curStrikes);  // negativo = reset
    return total;
  }

private:
  HistSample buf[HIST_MAX];
  uint16_t head = 0, n = 0;
};

extern History history;
