// ============================================================================
// astro.cpp — calcoli astronomici offline (porting EcoWitt)
//   Alba/tramonto: algoritmo NOAA (Meeus). Fase lunare: luna sinodica.
//   La conversione UTC->locale usa l'offset gia' impostato da tzset().
// ============================================================================
#include "astro.h"
#include "user_config.h"
#include <math.h>
#include <time.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DEG (M_PI / 180.0)
#define RAD (180.0 / M_PI)

static const double LAT = (double)MESH_LAT_I / 1e7;
static const double LON = (double)MESH_LON_I / 1e7;

static int tzOffsetMin() {
    time_t t = time(nullptr);
    struct tm lt = *localtime(&t);
    struct tm ut = *gmtime(&t);
    int off = (lt.tm_hour - ut.tm_hour) * 60 + (lt.tm_min - ut.tm_min);
    int dd = lt.tm_mday - ut.tm_mday;
    if (dd >  1) dd = -1;
    if (dd < -1) dd =  1;
    off += dd * 24 * 60;
    return off;
}

static double julianDay(int y, int m, int d) {
    if (m <= 2) { y--; m += 12; }
    int A = y / 100, B = 2 - A + A / 4;
    return (int)(365.25 * (y + 4716)) + (int)(30.6001 * (m + 1)) + d + B - 1524.5;
}

static int sunEvent(int year, int month, int day, bool rising) {
    double JD    = julianDay(year, month, day) + 0.5;
    double n     = JD - 2451545.0;
    double Jstar = n - LON / 360.0;
    double M     = fmod(357.5291 + 0.98560028 * Jstar, 360.0);
    double C     = 1.9148 * sin(M * DEG) + 0.0200 * sin(2.0 * M * DEG)
                 + 0.0003 * sin(3.0 * M * DEG);
    double lam   = fmod(M + C + 180.0 + 102.9372, 360.0);
    double Jtr   = 2451545.0 + Jstar + 0.0053 * sin(M * DEG) - 0.0069 * sin(2.0 * lam * DEG);
    double sinDec = sin(lam * DEG) * sin(23.4397 * DEG);
    double cosDec = cos(asin(sinDec));
    const double zenith = 90.833 * DEG;
    double cosOmega = (cos(zenith) - sin(LAT * DEG) * sinDec) / (cos(LAT * DEG) * cosDec);
    if (cosOmega >  1.0) return -1;   // mai sorge
    if (cosOmega < -1.0) return -2;   // mai tramonta
    double omega  = acos(cosOmega) * RAD;
    double Jev    = rising ? (Jtr - omega / 360.0) : (Jtr + omega / 360.0);
    double frac   = Jev - floor(Jev);
    double utcH   = fmod((frac + 0.5) * 24.0, 24.0);
    return (int)(utcH * 60.0 + 0.5) % (24 * 60);
}

bool astroGetSunTimes(SunTimes &out) {
    out.valid = false;
#if !MESH_POS_ENABLED
    return false;
#else
    time_t now = time(nullptr);
    if (now < 1000000UL) return false;
    struct tm ut = *gmtime(&now);
    int riseUtc = sunEvent(ut.tm_year + 1900, ut.tm_mon + 1, ut.tm_mday, true);
    int setUtc  = sunEvent(ut.tm_year + 1900, ut.tm_mon + 1, ut.tm_mday, false);
    if (riseUtc < 0 || setUtc < 0) return false;
    int tzOff   = tzOffsetMin();
    int riseLoc = ((riseUtc + tzOff) % (24 * 60) + 24 * 60) % (24 * 60);
    int setLoc  = ((setUtc  + tzOff) % (24 * 60) + 24 * 60) % (24 * 60);
    out.riseH = riseLoc / 60;  out.riseM = riseLoc % 60;
    out.setH  = setLoc  / 60;  out.setM  = setLoc  % 60;
    out.valid = true;
    return true;
#endif
}

float astroMoonAge() {
    time_t now = time(nullptr);
    if (now < 1000000UL) return 0;
    struct tm ut = *gmtime(&now);
    double JD = julianDay(ut.tm_year + 1900, ut.tm_mon + 1, ut.tm_mday)
              + ut.tm_hour / 24.0 + ut.tm_min / 1440.0;
    const double REF = 2451550.259, SYNODIC = 29.53058867;
    double age = fmod(JD - REF, SYNODIC);
    if (age < 0) age += SYNODIC;
    return (float)age;
}

float astroMoonIllum() {
    float age = astroMoonAge();
    const float S = 29.53058867f;
    return (1.0f - cosf(2.0f * (float)M_PI * age / S)) * 0.5f;
}

static int moonIdx() {
    float age = astroMoonAge();
    const float S = 29.53058867f;
    if (age < S * 0.0625f || age >= S * 0.9375f) return 0;
    if (age < S * 0.1875f) return 1;
    if (age < S * 0.3125f) return 2;
    if (age < S * 0.4375f) return 3;
    if (age < S * 0.5625f) return 4;
    if (age < S * 0.6875f) return 5;
    if (age < S * 0.8125f) return 6;
    return 7;
}

static const char *NAMES_FULL[]  = {
    "Luna nuova", "Falce crescente", "Primo quarto", "Gibbosa crescente",
    "Luna piena", "Gibbosa calante", "Ultimo quarto", "Falce calante" };
static const char *NAMES_SHORT[] = {
    "Nuova", "F.Cresc.", "1.Quarto", "Gibb.+", "Piena", "Gibb.-", "Ult.Q.", "F.Cal." };
static const char *EMOJI[] = { "🌑","🌒","🌓","🌔","🌕","🌖","🌗","🌘" };

const char *astroMoonPhaseName()  { return NAMES_FULL[moonIdx()];  }
const char *astroMoonPhaseShort() { return NAMES_SHORT[moonIdx()]; }
const char *astroMoonPhaseEmoji() { return EMOJI[moonIdx()];       }

// Tendenza: fasi 1-3 (dopo la nuova) crescente, 5-7 (dopo la piena) calante.
const char *astroMoonTrend() {
    int i = moonIdx();
    if (i >= 1 && i <= 3) return "Crescente";
    if (i >= 5 && i <= 7) return "Calante";
    return "";   // 0 = nuova, 4 = piena
}
