// ============================================================================
// astro.h — calcoli astronomici offline: alba/tramonto e fase lunare
// Tutto calcolato da data/ora di sistema + coordinate (MESH_LAT_I/MESH_LON_I).
// ============================================================================
#pragma once
#include <time.h>

struct SunTimes {
  int  riseH, riseM;
  int  setH,  setM;
  bool valid;
};

bool        astroGetSunTimes(SunTimes &out);
float       astroMoonAge();
float       astroMoonIllum();
const char *astroMoonPhaseName();
const char *astroMoonPhaseShort();
const char *astroMoonPhaseEmoji();

// "Crescente" / "Calante" / "" (vuoto per luna nuova o piena)
const char *astroMoonTrend();
