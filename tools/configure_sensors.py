#!/usr/bin/env python3
"""
configure_sensors.py — script di pre-build PlatformIO per WeatherStation2Meshtastic.

Legge:
  - settings.ini          (scelta utente: sensori, stazione, mesh)
  - sensors_catalog.json  (fonte di verità: gruppo, capability, sorgenti, librerie)

Produce:
  - src/config/generated_config.h   (radio derivato, ENABLE_*, SCREEN_*, mesh)
  - imposta build_src_filter         (compila solo i parser abilitati)

Garantisce: tutti i sensori scelti appartengono allo STESSO gruppo (vincolo SX1262).

Uso in platformio.ini:
  extra_scripts = pre:tools/configure_sensors.py

Dry-run standalone (senza PlatformIO):
  python tools/configure_sensors.py --project-dir . [--settings settings.ini]
"""

import json
import os
import sys
import configparser

FAMILY_DIRS = ["sensors/fineoffset/", "sensors/lacrosse/", "sensors/bresser/"]


# ----------------------------------------------------------------------------- helpers
def _die(msg):
    sys.stderr.write("\n[configure_sensors] ERRORE: %s\n\n" % msg)
    sys.exit(1)


def _cstr(s):
    """Stringa C escapata."""
    return '"' + str(s).replace("\\", "\\\\").replace('"', '\\"') + '"'


def _load_catalog(project_dir):
    path = os.path.join(project_dir, "sensors_catalog.json")
    if not os.path.isfile(path):
        _die("sensors_catalog.json non trovato in %s" % project_dir)
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def _load_settings(project_dir, settings_name="settings.ini"):
    path = os.path.join(project_dir, settings_name)
    if not os.path.isfile(path):
        _die("%s non trovato in %s" % (settings_name, project_dir))
    cfg = configparser.ConfigParser(inline_comment_prefixes=(";",))
    cfg.read(path, encoding="utf-8")
    return cfg


# ----------------------------------------------------------------------------- core
def resolve(catalog, settings):
    by_id = {s["id"]: s for s in catalog["sensors"]}
    caps_map = catalog["capabilities"]
    groups = catalog["groups"]

    raw = settings.get("sensors", "enabled", fallback="").strip()
    enabled = [x.strip() for x in raw.split(",") if x.strip()]
    if not enabled:
        _die("nessun sensore in [sensors].enabled di settings.ini")

    unknown = [e for e in enabled if e not in by_id]
    if unknown:
        _die("sensori sconosciuti nel catalogo: %s" % ", ".join(unknown))

    chosen = [by_id[e] for e in enabled]

    # vincolo fondamentale: un solo gruppo per build (un SX1262 = un bitrate)
    used_groups = sorted({s["group"] for s in chosen})
    if len(used_groups) > 1:
        members = {g: [s["id"] for s in chosen if s["group"] == g] for g in used_groups}
        _die(
            "sensori di gruppi diversi non sono ricevibili insieme su un solo SX1262.\n"
            "         %s\n"
            "         Separa le build (un environment per gruppo)."
            % "  ".join("[%s] %s" % (g, ", ".join(ids)) for g, ids in members.items())
        )

    group = used_groups[0]
    gdef = groups[group]

    caps, screens, sources, libs = set(), set(), [], set()
    for s in chosen:
        for c in s["caps"]:
            caps.add(c)
            scr = caps_map.get(c, {}).get("screen")
            if scr:
                screens.add(scr)
        for src in s["sources"]:
            if src not in sources:
                sources.append(src)
        for lib in s.get("libs", []):
            libs.add(lib)

    return {
        "enabled": enabled,
        "chosen": chosen,
        "group": group,
        "bitrate": int(gdef["bitrate_bps"]),
        "syncword": gdef["syncword"],
        "caps": sorted(caps),
        "screens": sorted(screens),
        "sources": sources,
        "libs": sorted(libs),
    }


def render_header(data, settings):
    g = data["group"]
    L = []
    L.append("// ============================================================")
    L.append("// AUTO-GENERATO da configure_sensors.py - NON MODIFICARE A MANO")
    L.append("// Parte SENSORI. La config di rete/nodo Meshtastic (MESH_*) e'")
    L.append("// prodotta da apply_settings.py (EcoWitt) in include/user_config.h.")
    L.append("// ============================================================")
    L.append("#pragma once")
    L.append("")
    L.append("// ---- Radio RX (derivato dal gruppo dei sensori) ----")
    L.append("#define RADIO_GROUP_%s" % g)
    L.append("#define RADIO_BITRATE_BPS %d" % data["bitrate"])
    L.append("#define RADIO_SYNCWORD    %s" % data["syncword"])
    L.append("")
    L.append("// ---- Sensori abilitati ----")
    for s in data["chosen"]:
        L.append("#define ENABLE_%s" % s["id"])
    L.append("")
    L.append("// ---- Schermate derivate (OR delle capability) ----")
    for scr in data["screens"]:
        L.append("#define %s" % scr)
    L.append("")
    return "\n".join(L)


def src_filter(data):
    flt = ["+<*>"]
    flt += ["-<%s>" % d for d in FAMILY_DIRS]      # via tutti i parser di famiglia
    flt += ["+<%s>" % s for s in data["sources"]]  # rientrano solo gli abilitati
    return flt


def write_header(project_dir, text):
    out_dir = os.path.join(project_dir, "src", "config")
    os.makedirs(out_dir, exist_ok=True)
    out = os.path.join(out_dir, "generated_config.h")
    with open(out, "w", encoding="utf-8") as f:
        f.write(text)
    return out


def summary(data, header_path):
    print("[configure_sensors] gruppo %s  ·  %d bps  ·  sync %s"
          % (data["group"], data["bitrate"], data["syncword"]))
    print("[configure_sensors] sensori : %s" % ", ".join(data["enabled"]))
    print("[configure_sensors] schermate: %s" % (", ".join(data["screens"]) or "—"))
    print("[configure_sensors] sorgenti : %d parser compilati" % len(data["sources"]))
    if data["libs"]:
        print("[configure_sensors] NB librerie extra richieste (verifica lib_deps): %s"
              % ", ".join(data["libs"]))
    print("[configure_sensors] header   : %s" % header_path)


def run(project_dir, settings_name="settings.ini"):
    catalog = _load_catalog(project_dir)
    settings = _load_settings(project_dir, settings_name)
    data = resolve(catalog, settings)
    header_path = write_header(project_dir, render_header(data, settings))
    summary(data, header_path)
    data["src_filter"] = src_filter(data)
    return data


# ----------------------------------------------------------------------------- entrypoints
try:
    Import("env")  # iniettato da PlatformIO/SCons
    _PIO = True
except Exception:
    _PIO = False

if _PIO:
    project_dir = env["PROJECT_DIR"]
    data = run(project_dir)
    env.Replace(SRC_FILTER=data["src_filter"])

elif __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="Dry-run di configure_sensors.py")
    ap.add_argument("--project-dir", default=".")
    ap.add_argument("--settings", default="settings.ini")
    a = ap.parse_args()
    d = run(a.project_dir, a.settings)
    print("\n[configure_sensors] src_filter:")
    for f in d["src_filter"]:
        print("   ", f)
