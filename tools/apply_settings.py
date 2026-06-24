#!/usr/bin/env python3
# ============================================================================
# apply_settings.py -- genera include/user_config.h a partire da settings.ini
# ============================================================================
import base64
import configparser
import os
import sys

try:
    Import("env")  # noqa: F821
    PROJECT_DIR = env.subst("$PROJECT_DIR")  # noqa: F821
except NameError:
    PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SETTINGS = os.path.join(PROJECT_DIR, "settings.ini")
OUTPUT   = os.path.join(PROJECT_DIR, "include", "user_config.h")

PRESETS = {
    "shortturbo":   (500.0, 7,  5),
    "shortfast":    (250.0, 7,  5),
    "shortslow":    (250.0, 8,  5),
    "mediumfast":   (250.0, 9,  5),
    "mediumslow":   (250.0, 10, 5),
    "longfast":     (250.0, 11, 5),
    "longmoderate": (125.0, 11, 8),
    "longslow":     (125.0, 12, 8),
}
PRESET_NAME = {
    "shortturbo": "ShortTurbo", "shortfast": "ShortFast",
    "shortslow": "ShortSlow",   "mediumfast": "MediumFast",
    "mediumslow": "MediumSlow", "longfast": "LongFast",
    "longmoderate": "LongModerate", "longslow": "LongSlow",
}

MESHTASTIC_DEFAULT_PSK = bytes([
    0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59,
    0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01
])


def die(msg):
    sys.stderr.write("\n*** settings.ini: " + msg + "\n\n")
    sys.exit(1)


def parse_channel_key(b64_value, field_name):
    """Decodifica chiave base64 -> (bytes, size).
    Accetta: AQ== (default pubblica, 16 b), 16 byte (AES-128), 32 byte (AES-256).
    """
    try:
        raw = base64.b64decode(b64_value)
    except Exception:
        die(field_name + " non e' una stringa base64 valida")
    if len(raw) == 1 and raw[0] == 0x01:
        return MESHTASTIC_DEFAULT_PSK, 16
    if len(raw) == 16:
        return raw, 16
    if len(raw) == 32:
        return raw, 32
    die(field_name + " deve essere AQ== (default), 16 byte (AES-128) o 32 byte (AES-256)"
        " -- ricevuti " + str(len(raw)) + " byte")


def get(cfg, sect, key, default=None):
    try:
        return cfg.get(sect, key).strip()
    except (configparser.NoSectionError, configparser.NoOptionError):
        if default is None:
            die("manca '" + key + "' nella sezione [" + sect + "]")
        return default


def esc_c(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


def hex_arr(b):
    return ", ".join("0x{:02x}".format(x) for x in b)


def coords_to_tz(lat, lon):
    """
    Deriva una stringa TZ POSIX dalle coordinate geografiche usando bounding box.
    Restituisce (tz_posix_string, descrizione_leggibile).

    La corrispondenza e' approssimata (bounding box rettangolari, non confini
    politici esatti), ma copre correttamente la grande maggioranza dei casi.
    Nota: la convenzione POSIX e' OPPOSTA all'offset UTC standard:
      UTC+1 (est di Greenwich) -> 'CET-1'   (segno negativo)
      UTC-5 (ovest)            -> 'EST5'    (segno positivo)
    """

    # ---- EUROPA -------------------------------------------------------
    if 34 <= lat <= 72 and -30 <= lon <= 45:
        # Islanda (UTC+0, nessun DST)
        if lon <= -12 and lat >= 62:
            return "GMT0", "Islanda (UTC+0)"
        # Azzorre (UTC-1, DST europeo)
        if lon <= -22 and lat >= 36:
            return "AZOT1AZOST,M3.5.0,M10.5.0/2", "Azzorre (UTC-1/0)"
        # UK e Irlanda
        if lon <= 2 and lat >= 49:
            return "GMT0BST,M3.5.0/1,M10.5.0", "UK/Irlanda (GMT/BST)"
        # Portogallo continentale
        if lon <= -6 and lat <= 42:
            return "WET0WEST,M3.5.0/1,M10.5.0", "Portogallo (WET/WEST)"
        # Europa Orientale: Grecia, Romania, Bulgaria, Finlandia, Estonia, ecc.
        if lon >= 22:
            # Turchia (UTC+3, nessun DST dal 2016)
            if lat <= 43 and lon >= 26:
                return "TRT-3", "Turchia (UTC+3)"
            return "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europa Orientale (EET/EEST, UTC+2/+3)"
        # Europa Centrale e Occidentale: Italia, Francia, Germania, Spagna, ecc.
        return "CET-1CEST,M3.5.0,M10.5.0/3", "Europa Centrale (CET/CEST, UTC+1/+2)"

    # ---- RUSSIA e EX-URSS ---------------------------------------------
    if lat >= 40 and 27 <= lon <= 180:
        if lon < 45:  return "MSK-3",             "Russia/Medio Oriente (UTC+3)"
        if lon < 55:  return "SAMT-4",             "Russia Samara (UTC+4)"
        if lon < 65:  return "YEKT-5",             "Russia Ekaterinburg (UTC+5)"
        if lon < 80:  return "OMST-6",             "Russia Omsk (UTC+6)"
        if lon < 98:  return "KRAT-7",             "Russia Krasnojarsk (UTC+7)"
        if lon < 115: return "IRKT-8",             "Russia Irkutsk (UTC+8)"
        if lon < 125: return "YAKT-9",             "Russia Jakutsk (UTC+9)"
        if lon < 140: return "VLAT-10",            "Russia Vladivostok (UTC+10)"
        if lon < 155: return "MAGT-11",            "Russia Magadan (UTC+11)"
        return "PETT-12",                          "Russia Kamchatka (UTC+12)"

    # ---- MEDIO ORIENTE e ASIA MERIDIONALE -----------------------------
    if 10 <= lat <= 42 and 27 <= lon <= 90:
        if lon < 37:  return "EET-2",              "Medio Oriente Ovest (UTC+2)"
        if lon < 42:  return "EAT-3",              "Medio Oriente Est (UTC+3)"
        if lon < 50:  return "IRST-3:30IRDT,M3.3.0/0,M9.3.0/0", "Iran (UTC+3:30/+4:30)"
        if lon < 57:  return "GST-4",              "Golfo Persico (UTC+4)"
        if lon < 67:  return "PKT-5",              "Pakistan (UTC+5)"
        if lon < 80:  return "IST-5:30",           "India (IST, UTC+5:30)"
        return "NPT-5:45",                         "Nepal (UTC+5:45)"

    # ---- ASIA ORIENTALE e SUD-EST -------------------------------------
    if -15 <= lat <= 55 and 90 <= lon <= 180:
        if lon < 100: return "ICT-7",              "Indocina (UTC+7)"
        if lon < 110: return "CST-8",              "Cina Ovest/Myanmar (UTC+8)"
        if lon < 125:
            if lat >= 30: return "CST-8",          "Cina/Taiwan (UTC+8)"
            return "WITA-8",                       "Indonesia Centrale (UTC+8)"
        if lon < 135: return "JST-9",              "Giappone/Corea (UTC+9)"
        if lon < 150: return "AEST-10",            "Papua/Pacifico Ovest (UTC+10)"
        return "NZST-12NZDT,M9.5.0,M4.1.0/3",     "Nuova Zelanda (NZST/NZDT)"

    # ---- OCEANIA ------------------------------------------------------
    if lat <= 0 and 100 <= lon <= 180:
        if lon < 128: return "AWST-8",             "Australia Ovest (UTC+8)"
        if lon < 138: return "ACST-9:30ACDT,M10.1.0,M4.1.0/3", "Australia Centrale (UTC+9:30/+10:30)"
        if lon < 155: return "AEST-10AEDT,M10.1.0,M4.1.0/3",   "Australia Est (UTC+10/+11)"
        return "SBT-11",                           "Isole Salomone (UTC+11)"

    # ---- AMERICHE -----------------------------------------------------
    if -60 <= lat <= 72 and -180 <= lon <= -30:
        # Alaska
        if lon < -140 and lat >= 54:
            return "AKST9AKDT,M3.2.0,M11.1.0", "Alaska (AKST/AKDT, UTC-9/-8)"
        # Hawaii (nessun DST)
        if lon < -140:
            return "HST10", "Hawaii (UTC-10)"
        # Canada/USA Pacifico
        if lon < -115 and lat >= 30:
            return "PST8PDT,M3.2.0,M11.1.0", "US/Canada Pacifico (PST/PDT)"
        # Canada/USA Mountain
        if lon < -100 and lat >= 25:
            return "MST7MDT,M3.2.0,M11.1.0", "US/Canada Mountain (MST/MDT)"
        # Canada/USA Centrale
        if lon < -85 and lat >= 25:
            return "CST6CDT,M3.2.0,M11.1.0", "US/Canada Centrale (CST/CDT)"
        # Canada/USA Orientale
        if lon < -67 and lat >= 25:
            return "EST5EDT,M3.2.0,M11.1.0", "US/Canada Orientale (EST/EDT)"
        # Canada Atlantico
        if lat >= 43:
            return "AST4ADT,M3.2.0,M11.1.0", "Canada Atlantico (AST/ADT)"
        # Caraibi / America Centrale
        if lat >= 10:
            return "EST5", "Caraibi/America Centrale (UTC-5)"
        # Sud America Ovest
        if lon < -68:
            return "PET5", "Sud America Ovest (UTC-5)"
        # Brasile (UTC-3, la maggior parte)
        if lon < -34:
            return "BRT3", "Brasile (BRT, UTC-3)"
        return "ART3", "Argentina/Uruguay (UTC-3)"

    # ---- AFRICA -------------------------------------------------------
    if -35 <= lat <= 38 and -20 <= lon <= 55:
        if lon < 0:   return "GMT0",               "Africa Atlantica (UTC+0)"
        if lon < 15:  return "WAT-1",              "Africa Occidentale (UTC+1)"
        if lon < 35:  return "CAT-2",              "Africa Centrale (UTC+2)"
        return "EAT-3",                            "Africa Orientale (UTC+3)"

    # ---- PACIFICO e ATLANTICO -----------------------------------------
    if lon < -30 and lat < 10:
        return "GST3", "Atlantico Sud (UTC-3)"
    if lon > 155:
        return "TOT-13", "Pacifico Est (UTC+13)"

    # ---- FALLBACK: solo offset da longitudine, nessun DST -------------
    offset_h = int(round(lon / 15.0))
    offset_h = max(-12, min(14, offset_h))
    if offset_h == 0:
        return "UTC0", "UTC (stimato da longitudine)"
    elif offset_h > 0:
        return "UTC-{}".format(offset_h), "UTC+{} (stimato da longitudine, nessun DST)".format(offset_h)
    else:
        return "UTC+{}".format(-offset_h), "UTC{} (stimato da longitudine, nessun DST)".format(offset_h)


def main():
    if not os.path.isfile(SETTINGS):
        die("file non trovato: " + SETTINGS)

    cfg = configparser.ConfigParser(inline_comment_prefixes=("#", ";"))
    cfg.read(SETTINGS, encoding="utf-8")

    # [meshtastic]
    enabled    = get(cfg, "meshtastic", "enabled", "true").lower() in ("1","true","yes","si")
    freq       = float(get(cfg, "meshtastic", "frequency_mhz", "869.525"))
    preset_raw = get(cfg, "meshtastic", "preset", "MediumFast")
    preset_key = preset_raw.replace(" ","").replace("_","").lower()
    if preset_key not in PRESETS:
        die("preset '" + preset_raw + "' sconosciuto. Validi: " + ", ".join(PRESET_NAME.values()))
    bw, sf, cr = PRESETS[preset_key]
    chan_name  = PRESET_NAME[preset_key]

    interval = int(get(cfg, "meshtastic", "send_interval_min", "10"))
    if interval < 1:
        die("send_interval_min deve essere >= 1")

    text_interval = int(get(cfg, "meshtastic", "text_interval_min", "0"))
    if text_interval < 0:
        die("text_interval_min non puo' essere negativo (0 = disabilitato)")

    short_name      = get(cfg, "meshtastic", "short_name", "WX")
    short_name_auto = (short_name == "")
    if not short_name_auto and len(short_name) > 4:
        print("[apply_settings] short_name '" + short_name + "' troncato a 4 caratteri")
        short_name = short_name[:4]
    long_name = get(cfg, "meshtastic", "long_name", "Stazione Meteo")[:39]

    txpwr = int(get(cfg, "meshtastic", "tx_power_dbm", "14"))
    if not -9 <= txpwr <= 22:
        die("tx_power_dbm fuori range (-9..22)")

    hop_limit = int(get(cfg, "meshtastic", "hop_limit", "3"))
    if not 0 <= hop_limit <= 7:
        die("hop_limit deve essere tra 0 e 7")

    ok_to_mqtt = get(cfg, "meshtastic", "ok_to_mqtt", "true").lower() in ("1","true","yes","si")

    chan_name_override = get(cfg, "meshtastic", "channel_name", "").strip()
    if chan_name_override:
        if len(chan_name_override) > 39:
            die("channel_name troppo lungo (max 39 caratteri)")
        chan_name = chan_name_override

    channel_key_b64 = get(cfg, "meshtastic", "channel_key", "AQ==").strip()
    psk_bytes, psk_size = parse_channel_key(channel_key_b64, "channel_key")

    text_chan_name    = get(cfg, "meshtastic", "text_channel_name", "").strip()
    text_chan_key_b64 = get(cfg, "meshtastic", "text_channel_key",  "").strip()
    text_chan_enabled = bool(text_chan_name or text_chan_key_b64)
    if text_chan_enabled:
        if not text_chan_name:
            die("text_channel_name vuoto ma text_channel_key e' valorizzato")
        if len(text_chan_name) > 39:
            die("text_channel_name troppo lungo (max 39 caratteri)")
        if not text_chan_key_b64:
            die("text_channel_key vuoto ma text_channel_name e' valorizzato")
        text_psk_bytes, text_psk_size = parse_channel_key(text_chan_key_b64, "text_channel_key")
    else:
        text_chan_name  = chan_name
        text_psk_bytes = psk_bytes
        text_psk_size  = psk_size

    lat = float(get(cfg, "meshtastic", "latitude",   "0") or "0")
    lon = float(get(cfg, "meshtastic", "longitude",  "0") or "0")
    alt = int(float(get(cfg, "meshtastic", "altitude_m", "0") or "0"))
    pos_enabled = (lat != 0.0 or lon != 0.0)
    if pos_enabled:
        if not -90  <= lat <= 90:  die("latitude fuori range")
        if not -180 <= lon <= 180: die("longitude fuori range")
        if not -450 <= alt <= 9000: die("altitude_m non plausibile")
    lat_i = int(round(lat * 1e7))
    lon_i = int(round(lon * 1e7))

    # --- Fuso orario ---
    # 'auto' (default) -> deriva TZ dalle coordinate geografiche.
    # Stringa POSIX esplicita -> usata direttamente (override manuale).
    tz_raw = get(cfg, "meshtastic", "timezone", "auto").strip()
    tz_auto = (tz_raw == "" or tz_raw.lower() == "auto")
    if tz_auto:
        if pos_enabled:
            tz_string, tz_desc = coords_to_tz(lat, lon)
            tz_source = "auto da ({:.4f}, {:.4f}) -> {}".format(lat, lon, tz_desc)
        else:
            tz_string = "UTC0"
            tz_desc   = "UTC (nessuna coordinata configurata)"
            tz_source = "auto -> UTC0 (coordinate non configurate)"
    else:
        if len(tz_raw) > 63:
            die("timezone: stringa TZ POSIX troppo lunga (max 63 caratteri)")
        tz_string = tz_raw
        tz_source = "esplicito"

    lightning           = get(cfg, "meshtastic", "lightning_text",     "true").lower() in ("1","true","yes","si")
    lightning_window    = int(get(cfg, "meshtastic", "lightning_window_min", "5"))
    if lightning_window < 1: die("lightning_window_min deve essere >= 1")
    lightning_threshold = float(get(cfg, "meshtastic", "lightning_threshold", "0.5"))
    if lightning_threshold <= 0: die("lightning_threshold deve essere > 0")

    if enabled and not (869.4 <= freq <= 869.65) and 868.0 <= freq <= 870.0:
        print("[apply_settings] ATTENZIONE: " + str(freq) + " MHz fuori dalla sotto-banda"
              " 869.4-869.65 (duty cycle 10%)")

    # [ricezione]
    rx_freq = float(get(cfg, "ricezione", "freq_mhz",       "868.35"))
    rx_br   = float(get(cfg, "ricezione", "bitrate_kbps",   "17.24"))
    rx_dev  = float(get(cfg, "ricezione", "freq_dev_khz",   "60.0"))
    rx_bw   = float(get(cfg, "ricezione", "rx_bw_khz",      "234.3"))

    # [storico]
    sample_min = int(get(cfg, "storico", "sample_interval_min", "10"))
    if sample_min < 5:
        print("[apply_settings] sample_interval_min < 5: buffer < 24h")
        sample_min = max(1, sample_min)

    # --- Costruisci user_config.h riga per riga ---
    FW_VERSION = "1.0.0"

    L = []
    def w(s=""):
        L.append(s)

    w("// ============================================================================")
    w("// user_config.h -- GENERATO AUTOMATICAMENTE da tools/apply_settings.py")
    w("// NON modificare a mano: edita settings.ini e ricompila.")
    w("// ============================================================================")
    w("#pragma once")
    w()
    w("// --- Firmware ---")
    w('#define FW_VERSION             "' + FW_VERSION + '"')
    w()
    w("// --- Meshtastic (canale principale: telemetria, NodeInfo, posizione) ---")
    w("#define MESH_ENABLED           " + str(1 if enabled else 0))
    w("#define MESH_FREQ_MHZ          " + str(freq) + "f")
    w("#define MESH_BW_KHZ            " + str(bw)   + "f")
    w("#define MESH_SF                " + str(sf))
    w("#define MESH_CR                " + str(cr))
    w('#define MESH_CHANNEL_NAME      "' + esc_c(chan_name) + '"')
    w("#define MESH_CHANNEL_KEY       {" + hex_arr(psk_bytes) + "}")
    w("#define MESH_CHANNEL_KEY_SIZE  " + str(psk_size))
    w("#define MESH_SEND_INTERVAL_MIN " + str(interval))
    w("#define MESH_TEXT_INTERVAL_MIN " + str(text_interval))
    w('#define MESH_SHORT_NAME        "' + esc_c(short_name) + '"')
    w("#define MESH_SHORT_NAME_AUTO   " + str(1 if short_name_auto else 0))
    w('#define MESH_LONG_NAME         "' + esc_c(long_name) + '"')
    w("#define MESH_TX_POWER_DBM      " + str(txpwr))
    w("#define MESH_HOP_LIMIT         " + str(hop_limit))
    w("#define MESH_OK_TO_MQTT        " + str(1 if ok_to_mqtt else 0))
    w("#define MESH_LIGHTNING_TEXT    " + str(1 if lightning else 0))
    w("#define MESH_LIGHTNING_WINDOW_MIN " + str(lightning_window))
    w("#define MESH_LIGHTNING_THRESHOLD  " + str(lightning_threshold) + "f")
    w()
    w("// --- Secondo canale (testi meteo + avvisi fulmini) ---")
    w("// Se MESH_TEXT_CHANNEL_ENABLED==0, testi e fulmini usano il canale principale.")
    w("#define MESH_TEXT_CHANNEL_ENABLED  " + str(1 if text_chan_enabled else 0))
    w('#define MESH_TEXT_CHANNEL_NAME     "' + esc_c(text_chan_name) + '"')
    w("#define MESH_TEXT_CHANNEL_KEY      {" + hex_arr(text_psk_bytes) + "}")
    w("#define MESH_TEXT_CHANNEL_KEY_SIZE " + str(text_psk_size))
    w()
    w("// --- Posizione fissa (interi a 1e-7 gradi, quota in m s.l.m.) ---")
    w("#define MESH_POS_ENABLED       " + str(1 if pos_enabled else 0))
    w("#define MESH_LAT_I             " + str(lat_i))
    w("#define MESH_LON_I             " + str(lon_i))
    w("#define MESH_ALT_M             " + str(alt))
    w()
    w("// --- Fuso orario (stringa TZ POSIX per setenv/tzset su ESP32) ---")
    w("// Derivato automaticamente dalle coordinate; override con timezone=... in settings.ini")
    w('#define MESH_TIMEZONE          "' + esc_c(tz_string) + '"')
    w()
    w("// --- Ricezione FSK sensori ---")
    w("// NB: il BITRATE e il SYNC WORD effettivi NON sono qui: derivano dal GRUPPO")
    w("//     dei sensori e stanno in src/config/generated_config.h")
    w("//     (RADIO_BITRATE_BPS / RADIO_SYNCWORD, generati da configure_sensors.py).")
    w("//     Frequenza, deviazione e banda RX qui sotto sono comuni a tutti i gruppi.")
    w("#define RX_FREQ_MHZ            " + str(rx_freq) + "f")
    w("#define RX_BITRATE_KBPS        " + str(rx_br)   + "f   // INFORMATIVO: non usato dal firmware (il bitrate viene dal gruppo)")
    w("#define RX_FREQ_DEV_KHZ        " + str(rx_dev)  + "f")
    w("#define RX_BW_KHZ              " + str(rx_bw)   + "f")
    w()
    w("// --- Storico ---")
    w("#define HISTORY_SAMPLE_MIN     " + str(sample_min))
    w()

    hdr = "\n".join(L)

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    old = None
    if os.path.isfile(OUTPUT):
        with open(OUTPUT, "r", encoding="utf-8") as fh:
            old = fh.read()
    if old != hdr:
        with open(OUTPUT, "w", encoding="utf-8") as fh:
            fh.write(hdr)
        print("[apply_settings] generato " + OUTPUT)
        key_lbl = "default (AQ==)" if psk_bytes == MESHTASTIC_DEFAULT_PSK \
                  else "custom AES-" + str(psk_size * 8)
        print("[apply_settings] preset {}: BW {} kHz, SF{}, CR 4/{}, {} MHz, "
              "invio ogni {} min, hop {}".format(preset_raw, bw, sf, cr, freq, interval, hop_limit))
        print("[apply_settings] canale principale '{}', key={}".format(chan_name, key_lbl))
        if text_chan_enabled:
            txt_lbl = "default (AQ==)" if text_psk_bytes == MESHTASTIC_DEFAULT_PSK \
                      else "custom AES-" + str(text_psk_size * 8)
            print("[apply_settings] canale testo '{}', key={}".format(text_chan_name, txt_lbl))
        else:
            print("[apply_settings] canale testo: stesso del principale")
        if pos_enabled:
            print("[apply_settings] posizione fissa: {}, {}, {} m s.l.m.".format(lat, lon, alt))
        else:
            print("[apply_settings] posizione fissa: disabilitata")
        print("[apply_settings] timezone: {}  ->  '{}'".format(tz_source, tz_string))
    else:
        print("[apply_settings] user_config.h gia' aggiornato")


main()
