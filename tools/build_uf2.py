"""
build_uf2.py — post-build step per le board nRF52 DIY (MASN, FakeTec).

Il bootloader Adafruit UF2 su questi cloni Pro-Micro/NiceNano nRF52840 si
presenta, al doppio-tap di RST, come una chiavetta USB (mass storage, es.
"NICENANOBOOT") su cui trascinare un file .uf2 — NON come porta seriale per
un upload automatico via nrfutil (che infatti in questo progetto non viene
usato per il flashing, solo dichiarato nel board.json per compatibilita').

PlatformIO/nordicnrf52 non genera un .uf2 di default (solo .hex/.zip): questo
script lo produce a fine build usando uf2conv.py, gia' incluso nel framework
Adafruit nRF52 scaricato da PlatformIO, cosi' il file da copiare sulla
chiavetta e' pronto in .pio/build/<env>/firmware.uf2 dopo ogni `pio run`.
"""
Import("env")

import os


def _after_build(source, target, env):
    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoadafruitnrf52")
    if not framework_dir:
        print("[build_uf2] framework-arduinoadafruitnrf52 non trovato, salto la generazione .uf2")
        return

    uf2conv = os.path.join(framework_dir, "tools", "uf2conv", "uf2conv.py")
    build_dir = env.subst("$BUILD_DIR")
    hex_path = os.path.join(build_dir, "firmware.hex")
    uf2_path = os.path.join(build_dir, "firmware.uf2")

    if not os.path.isfile(uf2conv):
        print("[build_uf2] uf2conv.py non trovato in %s, salto la generazione .uf2" % uf2conv)
        return
    if not os.path.isfile(hex_path):
        print("[build_uf2] firmware.hex non trovato in %s, salto la generazione .uf2" % hex_path)
        return

    env.Execute(
        env.VerboseAction(
            ' '.join([
                '"$PYTHONEXE"', '"%s"' % uf2conv,
                "-f", "NRF52",
                "-c",
                "-o", '"%s"' % uf2_path,
                '"%s"' % hex_path,
            ]),
            "Building %s" % uf2_path,
        )
    )


# Solo per le board che usano il bootloader UF2 mass-storage (nordicnrf52
# con la variant locale nrf52_promicro_diy_htra62); non tocca Heltec/XIAO.
if env.get("PIOPLATFORM") == "nordicnrf52":
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", _after_build)
