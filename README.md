# WeatherStation2Meshtastic

Gateway modulare che riceve sensori meteo wireless a **868 MHz** (GFSK) e li
ritrasmette sulla rete **Meshtastic** (LoRa), con display OLED, sincronizzazione
oraria dalla rete e bollettini meteo/astronomici automatici.
Hardware target: **Heltec WiFi LoRa 32 V3 / V4** (ESP32‑S3 + Semtech **SX1262**).

*Modular gateway that receives 868 MHz wireless weather sensors (GFSK) and bridges
them onto the **Meshtastic** LoRa network, with an OLED display, network time‑sync
and automatic weather/astronomy bulletins. Target hardware: **Heltec WiFi LoRa 32
V3 / V4** (ESP32‑S3 + Semtech **SX1262**).*

> Riscrittura modulare di / Modular rewrite of
> [`iz0kew/EcoWittStation2Meshtastic`](https://github.com/iz0kew/EcoWittStation2Meshtastic):
> lo stack Meshtastic, il time‑sync, le effemeridi e i parser WH32/WH40/WH57 sono
> riusati da lì; il resto è nuovo. / the Meshtastic stack, time‑sync, ephemeris and
> the WH32/WH40/WH57 parsers are reused from there; the rest is new.

---

## 🇮🇹 Italiano

### Funzionalità

- **Multi‑sensore selezionabile**: scegli quali sensori decodificare; il firmware
  compila **solo** i parser e le schermate necessari (build modulare).
- **14 sensori supportati** su due gruppi radio (vedi tabella sotto).
- **Gateway Meshtastic hand‑made**: AES‑128/256‑CTR, protobuf, due canali
  (telemetria + testo), NodeInfo, posizione fissa sulla mappa.
- **Telemetria nativa**: `EnvironmentMetrics` + `AirQualityMetrics` (temperatura,
  umidità, pressione, vento, pioggia 1h/24h, luce, suolo, PM2.5/PM10, CO₂).
- **Sincronizzazione oraria senza RTC né GPS**: estrae l'ora dal traffico
  Meshtastic all'avvio, con algoritmo a conferme multiple (immune al "poison
  first sample") e finestra valida derivata dalla **data di build**.
- **Bollettini automatici**: 3 bollettini astronomici al giorno (alba+1h,
  mezzogiorno, tramonto−1h) sul canale principale + bollettino a intervallo fisso
  sul canale testo, con emoji.
- **Avvisi fulmini** sul canale testo con soglia configurabile.
- **Effemeridi offline**: alba/tramonto, fase lunare (crescente/calante).
- **Display OLED** con schermate a rotazione guidate dalle capability:
  panoramica, temp/umidità, pressione, pioggia, vento, UV/luce, fulmini, qualità
  aria, suolo, perdita acqua, **grafici 24h** (temperatura, umidità, pioggia,
  fulmini), orario+data, astro, stato Meshtastic (short name + countdown al
  prossimo invio). Ogni schermata mostra **modello e ID** del sensore ricevuto
  (utile a distinguere la propria stazione da quella di un vicino).
- **Configuratore web** build‑time (`tools/configurator/index.html`) per generare
  l'intero `settings.ini`.

### Vincolo fondamentale: un SX1262 = un bitrate

I sensori 868 si dividono in due gruppi per bitrate, entrambi con sync word `0x2DD4`.
Un solo SX1262 tiene **una sola configurazione alla volta**: i due gruppi **non**
sono ricevibili insieme. La scelta del gruppo è una scelta di **build** (un
environment per gruppo); lo script di pre‑build **rifiuta** una selezione mista.

| Gruppo | Bitrate | Sensori |
|--------|---------|---------|
| **A** | ~17.241 kbps | Fine Offset / Ecowitt + LaCrosse IT+/TFA |
| **B** | ~8.06 kbps | Bresser |

### Sensori supportati

**Gruppo A** (`groupA_*`)

| id | Modello | Misure |
|----|---------|--------|
| `WH32`  | Fine Offset WH32 / WH25 / WH32B | temp / umidità / pressione |
| `WH31E` | Ambient WH31E / WH31B | temp / umidità |
| `WH40`  | Ecowitt WH40 | pioggia |
| `WH57`  | Ecowitt WH57 / WH31L | fulmini |
| `WH51`  | Ecowitt WH51 / WN31 / SM23 | umidità suolo |
| `WH45`  | Ecowitt WH45 | PM2.5 / PM10 / CO₂ / temp / umidità |
| `WH46`  | Ecowitt WH46 | PM1/2.5/4/10 / CO₂ / temp / umidità |
| `WN34`  | Fine Offset WN34S/L/D | sonda temperatura (suolo/acqua) |
| `WS68`  | Ecowitt WS68 | vento / luce / UV |
| `WS80`  | Fine Offset WS80 | temp / umidità / vento / UV / luce |
| `WS85`  | Fine Offset WS85 | vento / pioggia |
| `WS90`  | Fine Offset WS90 (Wittboy) | come WS80 + pioggia |
| `WH65B` | Fine Offset WH24 / WH65B / WS69 | stazione all‑in‑one (temp / umid / vento / pioggia / UV) |
| `TX35`  | LaCrosse TX35 / TX29 / TFA 30.3155/30.3159 | temp / umidità |

**Gruppo B** (`groupB_*`)

| id | Modello | Misure |
|----|---------|--------|
| `BRESSER_5IN1`      | Bresser 5‑in‑1 | temp / umidità / vento / pioggia |
| `BRESSER_6IN1`      | Bresser 6‑in‑1 (+ new 5‑in‑1 / 3‑in‑1 wind / soil) | temp / umidità / vento / pioggia / UV / suolo |
| `BRESSER_7IN1`      | Bresser 7‑in‑1 meteo | temp / umidità / vento / pioggia / UV |
| `BRESSER_7IN1_AQ`   | Bresser 7‑in‑1 Air Quality | PM2.5 / PM10 / CO₂ |
| `BRESSER_LIGHTNING` | Bresser Lightning | fulmini |
| `BRESSER_LEAKAGE`   | Bresser Leakage | perdita acqua |

### Configurazione (`settings.ini`)

Scelta dei sensori (devono appartenere allo **stesso gruppo**):

```ini
[sensors]
enabled = WH32, WH40, WH57
```

La sezione `[meshtastic]` (frequenza, preset, intervalli, nomi/chiavi dei canali,
posizione, fuso orario, soglie fulmini) usa le stesse chiavi di EcoWitt. Puoi
generare l'intero file con `tools/configurator/index.html` (pagina statica che
gira sul PC e valida il vincolo di gruppo dal vivo).

A ogni build due script di pre‑build aggiornano la configurazione:
`apply_settings.py` (rete → `include/user_config.h`) e `configure_sensors.py`
(sensori → `src/config/generated_config.h` + `build_src_filter`).

### Compilazione

```bash
pio run -e groupA_heltec_v3 -t upload     # Gruppo A, Heltec V3
pio run -e groupA_heltec_v4 -t upload     # Gruppo A, Heltec V4
pio run -e groupB_heltec_v3 -t upload     # Gruppo B, Heltec V3
pio run -e groupB_heltec_v4 -t upload     # Gruppo B, Heltec V4
```

### How‑to: flashare la scheda Heltec

**Prerequisiti**
- [PlatformIO](https://platformio.org/) (estensione VS Code o `pip install platformio`).
- Cavo **USB‑C dati** (non solo carica) e una Heltec WiFi LoRa 32 **V3** o **V4**.
- Driver USB‑seriale: le Heltec recenti usano il **CP2102/CH9102**; su Windows
  installa il driver del produttore se la porta COM non compare.

**Passi**
1. Collega la scheda via USB‑C.
2. Modifica `settings.ini` (sensori + parametri Meshtastic), oppure generalo con
   il configuratore web.
3. Compila e carica con l'environment giusto per gruppo e scheda:
   ```bash
   pio run -e groupA_heltec_v4 -t upload
   ```
   PlatformIO rileva da solo la porta; per forzarla aggiungi
   `--upload-port COM5` (Windows) o `--upload-port /dev/ttyUSB0` (Linux/macOS).
4. Apri il monitor seriale per verificare:
   ```bash
   pio device monitor -b 115200
   ```
5. **Modalità boot (solo se l'upload fallisce):** tieni premuto il tasto **BOOT
   (PRG)**, premi e rilascia **RST**, poi rilascia **BOOT**; rilancia l'upload.
   La V3/V4 di solito entra in download mode da sola, quindi serve di rado.

All'avvio la scheda apre una finestra di ~5 minuti per sincronizzare l'orario
dalla rete Meshtastic; premi **PRG** per scorrere le schermate.

### Matrice hardware

| | Heltec V3 | Heltec V4 |
|--|--|--|
| MCU / radio | ESP32‑S3 + SX1262 | ESP32‑S3 + SX1262 |
| Display | OLED SSD1306 128×64 | OLED SSD1306 128×64 |
| Gruppo A | `groupA_heltec_v3` | `groupA_heltec_v4` |
| Gruppo B | `groupB_heltec_v3` | `groupB_heltec_v4` |

### Note

- L'SX1262 è *packet‑oriented*: si configura in **GFSK packet mode** (preamble
  detect + sync word) e il parsing del payload avviene nel firmware (niente
  `rtl_433_ESP`, che richiede SX127x in continuous mode).
- La radio è **una sola**, condivisa fra tre modalità (RX‑GFSK sensori, TX‑LoRa
  Meshtastic, RX‑LoRa time‑sync) gestite da un unico owner, senza interrompere il
  timing dei pacchetti Meshtastic.
- I parser seguono i formati di riferimento di
  [`merbanan/rtl_433`](https://github.com/merbanan/rtl_433).
- Per il **Gruppo B (Bresser)** deviazione e banda RX restano i valori comuni di
  default: la ricezione funziona, ma per la massima sensibilità andranno affinati
  sull'hardware reale.

---

## 🇬🇧 English

### Features

- **Selectable multi‑sensor**: pick which sensors to decode; the firmware compiles
  **only** the needed parsers and screens (modular build).
- **14 supported sensors** across two radio groups (see table below).
- **Hand‑made Meshtastic gateway**: AES‑128/256‑CTR, protobuf, two channels
  (telemetry + text), NodeInfo, fixed map position.
- **Native telemetry**: `EnvironmentMetrics` + `AirQualityMetrics` (temperature,
  humidity, pressure, wind, rainfall 1h/24h, lux, soil, PM2.5/PM10, CO₂).
- **Time‑sync without RTC or GPS**: derives the clock from Meshtastic traffic at
  boot, multi‑confirmation algorithm, with the valid window derived from the
  **build date**.
- **Automatic bulletins**: 3 daily astronomy bulletins (sunrise+1h, noon,
  sunset−1h) on the primary channel + a fixed‑interval bulletin on the text
  channel, with emoji.
- **Lightning alerts** on the text channel with a configurable threshold.
- **Offline ephemeris**: sunrise/sunset, moon phase (waxing/waning).
- **OLED display** with capability‑driven rotating screens: overview,
  temp/humidity, pressure, rain, wind, UV/light, lightning, air quality, soil,
  leak, **24h graphs** (temperature, humidity, rain, lightning), time+date,
  astro, Meshtastic status (short name + countdown to next send). Each data screen
  shows the **model and ID** of the received sensor (handy to tell your own
  station apart from a neighbour's).
- **Build‑time web configurator** (`tools/configurator/index.html`) to generate
  the whole `settings.ini`.

### Key constraint: one SX1262 = one bitrate

868 MHz sensors split into two bitrate groups, both using sync word `0x2DD4`. A
single SX1262 holds **one configuration at a time**: the two groups **cannot** be
received together. The group is a **build‑time** choice (one environment per
group); the pre‑build script **rejects** a mixed selection.

| Group | Bitrate | Sensors |
|-------|---------|---------|
| **A** | ~17.241 kbps | Fine Offset / Ecowitt + LaCrosse IT+/TFA |
| **B** | ~8.06 kbps | Bresser |

### Supported sensors

**Group A** (`groupA_*`)

| id | Model | Measures |
|----|-------|----------|
| `WH32`  | Fine Offset WH32 / WH25 / WH32B | temp / humidity / pressure |
| `WH31E` | Ambient WH31E / WH31B | temp / humidity |
| `WH40`  | Ecowitt WH40 | rain |
| `WH57`  | Ecowitt WH57 / WH31L | lightning |
| `WH51`  | Ecowitt WH51 / WN31 / SM23 | soil moisture |
| `WH45`  | Ecowitt WH45 | PM2.5 / PM10 / CO₂ / temp / humidity |
| `WH46`  | Ecowitt WH46 | PM1/2.5/4/10 / CO₂ / temp / humidity |
| `WN34`  | Fine Offset WN34S/L/D | temperature probe (soil/water) |
| `WS68`  | Ecowitt WS68 | wind / light / UV |
| `WS80`  | Fine Offset WS80 | temp / humidity / wind / UV / light |
| `WS85`  | Fine Offset WS85 | wind / rain |
| `WS90`  | Fine Offset WS90 (Wittboy) | like WS80 + rain |
| `WH65B` | Fine Offset WH24 / WH65B / WS69 | all‑in‑one station (temp / humidity / wind / rain / UV) |
| `TX35`  | LaCrosse TX35 / TX29 / TFA 30.3155/30.3159 | temp / humidity |

**Group B** (`groupB_*`)

| id | Model | Measures |
|----|-------|----------|
| `BRESSER_5IN1`      | Bresser 5‑in‑1 | temp / humidity / wind / rain |
| `BRESSER_6IN1`      | Bresser 6‑in‑1 (+ new 5‑in‑1 / 3‑in‑1 wind / soil) | temp / humidity / wind / rain / UV / soil |
| `BRESSER_7IN1`      | Bresser 7‑in‑1 weather | temp / humidity / wind / rain / UV |
| `BRESSER_7IN1_AQ`   | Bresser 7‑in‑1 Air Quality | PM2.5 / PM10 / CO₂ |
| `BRESSER_LIGHTNING` | Bresser Lightning | lightning |
| `BRESSER_LEAKAGE`   | Bresser Leakage | water leak |

### Configuration & build

Pick sensors (same group) in `settings.ini`:

```ini
[sensors]
enabled = WH32, WH40, WH57
```

The `[meshtastic]` section mirrors EcoWitt's keys. Two pre‑build scripts run on
every build: `apply_settings.py` (network → `include/user_config.h`) and
`configure_sensors.py` (sensors → `src/config/generated_config.h` + `build_src_filter`).

```bash
pio run -e groupA_heltec_v4 -t upload    # or _v3 / groupB_*
```

### How‑to: flashing the Heltec board

1. Install [PlatformIO](https://platformio.org/) and connect the Heltec
   (**V3/V4**) over a **data** USB‑C cable. Install the CP2102/CH9102 USB‑serial
   driver if no COM port shows up.
2. Edit `settings.ini` (or generate it with the web configurator).
3. Build & upload with the right environment:
   ```bash
   pio run -e groupA_heltec_v4 -t upload
   ```
   Add `--upload-port COMx` / `/dev/ttyUSBx` to force the port.
4. Open the serial monitor: `pio device monitor -b 115200`.
5. **Boot mode (only if upload fails):** hold **BOOT (PRG)**, tap **RST**, release
   **BOOT**, then re‑run the upload. The V3/V4 usually enters download mode on its
   own, so this is rarely needed.

At boot the board opens a ~5‑minute window to sync time from the Meshtastic
network; press **PRG** to cycle through the screens.

---

## Struttura del progetto / Project layout

```
platformio.ini              # env: groupA/B × heltec_v3/v4
settings.ini                # [meshtastic] (parità EcoWitt) + [sensors]
sensors_catalog.json        # FONTE DI VERITÀ: sensori, gruppi, capability, sorgenti
tools/
  apply_settings.py         # [meshtastic] -> include/user_config.h (MESH_*, RX_*)
  configure_sensors.py      # [sensors] + catalogo -> generated_config.h + src_filter
  configurator/index.html   # configuratore web (genera tutto il settings.ini)
include/user_config.h        # generato (rete/nodo)
src/
  main.cpp                  # dispatch: match -> parse -> uiSubmit -> meshSubmit
  config/generated_config.h # generato (RADIO_*, ENABLE_*, SCREEN_*)
  radio/                    # gestore unico SX1262 a 3 modalità
  sensors/                  # sensor_types.h, registry, sensor_util.h, fineoffset/ lacrosse/ bresser/
  display/                  # display.h, ui.{h,cpp}, screens/ (incl. grafici 24h)
  mesh/                     # meshtastic_pack.{h,cpp}
  timesync.{h,cpp}  astro.{h,cpp}  history.h
```

## Licenza / License

Codice riusato da EcoWittStation2Meshtastic (MIT). Vedi i singoli file per i
riferimenti. *Reused code from EcoWittStation2Meshtastic (MIT); see individual
files for attribution.*
