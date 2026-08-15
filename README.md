# WeatherStation2Meshtastic

Gateway modulare che riceve sensori meteo wireless a **868 MHz** (GFSK) e li
ritrasmette sulla rete **Meshtastic** (LoRa), con display OLED, sincronizzazione
oraria dalla rete e bollettini meteo/astronomici automatici.
Hardware supportato: **Heltec WiFi LoRa 32 V3 / V4** (ESP32‑S3 + Semtech
**SX1262**, con display OLED), **Seeed XIAO nRF52840 + Wio‑SX1262 Kit**
(nRF52840 + SX1262, **senza display**: nessuna schermata/grafico/menu di invio
manuale, feedback della sincronizzazione oraria sul LED RGB onboard) e, in via
**sperimentale** (pinout non ancora verificato su hardware reale), **MASN
NiceNano nRF52840 + HT‑RA62** e **FakeTec NRF52840 Pro Micro + HT‑RA62**
(entrambe nRF52840 + SX1262, senza display).

*Modular gateway that receives 868 MHz wireless weather sensors (GFSK) and bridges
them onto the **Meshtastic** LoRa network, with network time‑sync and automatic
weather/astronomy bulletins. Supported hardware: **Heltec WiFi LoRa 32 V3 / V4**
(ESP32‑S3 + Semtech **SX1262**, with OLED display), **Seeed XIAO nRF52840 +
Wio‑SX1262 Kit** (nRF52840 + SX1262, **no display**: no screens/graphs/manual‑send
menu, time‑sync feedback on the onboard RGB LED instead) and, **experimentally**
(pinout not yet verified on real hardware), **MASN NiceNano nRF52840 + HT‑RA62**
and **FakeTec NRF52840 Pro Micro + HT‑RA62** (both nRF52840 + SX1262, no display).*

**Versione attuale: v1.2.1**

**Novità in questa release:**
- Fix: il bollettino meteo mostra i fulmini accumulati nelle **ultime 24h**
  invece del totale cumulativo del sensore (mai azzerato).
- Fix: pioggia 1h/24h non collassa più sull'intero contatore dopo un reset
  del sensore (es. cambio batterie sui pluviometri piezo).
- Fix: il bollettino meteo non veniva più troncato in coda al messaggio.

*Current version: v1.2.1*

*What's new in this release:*
- *Fix: the weather bulletin now shows lightning strikes accumulated in the
  **last 24h** instead of the sensor's cumulative (never‑reset) total.*
- *Fix: 1h/24h rainfall no longer collapses onto the full counter after a
  sensor reset (e.g. battery change on piezo rain gauges).*
- *Fix: the weather bulletin was no longer being truncated at the end of the
  message.*
- *Weather bulletins never exceed the Meshtastic 200‑byte text limit anymore —
  when space runs short, the least essential fields (astro, date/time, link)
  are dropped whole and in priority order, never truncating a value mid‑field.*

---

## 🇮🇹 Italiano

### Funzionalità

- **Multi‑sensore selezionabile**: scegli quali sensori decodificare; il firmware
  compila **solo** i parser e le schermate necessari (build modulare).
- **20 sensori supportati** su due gruppi radio (vedi tabella sotto).
- **Gateway Meshtastic hand‑made**: AES‑128/256‑CTR, protobuf, due canali
  (telemetria + testo), NodeInfo, posizione fissa sulla mappa.
- **Telemetria nativa**: `EnvironmentMetrics` + `AirQualityMetrics` (temperatura,
  umidità, pressione, vento, pioggia 1h/24h, luce, suolo, PM2.5/PM10, CO₂).
- **Sincronizzazione oraria senza RTC né GPS**: estrae l'ora dal traffico
  Meshtastic all'avvio, con algoritmo a conferme multiple (immune al "poison
  first sample") e finestra valida derivata dalla **data di build**.
- **Bollettini automatici**: 3 bollettini astronomici al giorno (alba+1h,
  mezzogiorno, tramonto−1h) sul canale principale + bollettino a intervallo fisso
  sul canale testo, con emoji e **data e orario locale di invio** (📅 🕒). Il
  testo non supera mai il limite Meshtastic di 200 byte: a corto di spazio si
  scartano per intero (mai a metà) prima i campi meno essenziali — astro,
  data/ora, link — mantenendo sempre i dati sensore.
- **Avvisi fulmini** sul canale testo con soglia configurabile.
- **Invio manuale dei bollettini**: pressione **prolungata** del tasto PRG →
  menu a finestra per scegliere il canale (Ch0/Ch1) → conferma invio; ogni
  sottomenu ha la voce "Indietro" e si chiude da solo dopo 10 s di inattività.
- **Effemeridi offline**: alba/tramonto, fase lunare (crescente/calante).
- **Display OLED** con schermate a rotazione guidate dalle capability:
  panoramica, temp/umidità, pressione, pioggia, vento, UV/luce, fulmini, qualità
  aria, suolo, perdita acqua, **grafici 24h** (temperatura, umidità, pioggia,
  fulmini), orario+data, astro, stato Meshtastic (short name + countdown al
  prossimo invio + **batteria %/alimentazione USB**). Ogni schermata mostra
  **modello e ID** del sensore ricevuto (utile a distinguere la propria stazione
  da quella di un vicino).
- **Splash screen all'avvio** (5 s) con logo, nome del progetto e versione
  firmware.
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

La sezione `[meshtastic]` configura frequenza, preset, intervalli, nomi/chiavi
dei canali, posizione, fuso orario e soglie fulmini. Puoi generare l'intero
file con `tools/configurator/index.html` (pagina statica che gira sul PC e
valida il vincolo di gruppo dal vivo).

A ogni build due script di pre‑build aggiornano la configurazione:
`apply_settings.py` (rete → `include/user_config.h`) e `configure_sensors.py`
(sensori → `src/config/generated_config.h` + `build_src_filter`).

### Compilazione

```bash
pio run -e groupA_heltec_v3 -t upload         # Gruppo A, Heltec V3
pio run -e groupA_heltec_v4 -t upload         # Gruppo A, Heltec V4
pio run -e groupA_xiao_wiosx1262 -t upload    # Gruppo A, XIAO nRF52840 + Wio-SX1262
pio run -e groupA_masn_htra62 -t upload       # Gruppo A, MASN NiceNano + HT-RA62 (sperim.)
pio run -e groupA_faketec_htra62 -t upload    # Gruppo A, FakeTec Pro Micro + HT-RA62 (sperim.)
pio run -e groupB_heltec_v3 -t upload         # Gruppo B, Heltec V3
pio run -e groupB_heltec_v4 -t upload         # Gruppo B, Heltec V4
pio run -e groupB_xiao_wiosx1262 -t upload    # Gruppo B, XIAO nRF52840 + Wio-SX1262
pio run -e groupB_masn_htra62 -t upload       # Gruppo B, MASN NiceNano + HT-RA62 (sperim.)
pio run -e groupB_faketec_htra62 -t upload    # Gruppo B, FakeTec Pro Micro + HT-RA62 (sperim.)
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

All'avvio la scheda mostra la splash screen (5 s) e apre una finestra di ~5
minuti per sincronizzare l'orario dalla rete Meshtastic; premi **PRG** per
scorrere le schermate. A sincronizzazione conclusa, una **pressione prolungata**
di PRG apre il menu di invio manuale (breve = scorri voce, lunga = seleziona).

### How‑to: flashare la scheda XIAO nRF52840 + Wio‑SX1262

**Prerequisiti**
- [PlatformIO](https://platformio.org/).
- Kit **Seeed XIAO nRF52840 + Wio‑SX1262** (SKU 102010710/113010003, modulo
  radio a pin‑header, non il connettore B2B della variante ESP32‑S3).
- Cavo USB‑C dati.

**Passi**
1. Collega la scheda via USB‑C.
2. Modifica `settings.ini` come per la Heltec.
3. Compila e carica:
   ```bash
   pio run -e groupA_xiao_wiosx1262 -t upload
   ```
   Se l'upload non parte, entra manualmente in bootloader UF2 con un
   **doppio‑tap** del tasto **RST** (comportamento standard delle schede XIAO),
   poi rilancia l'upload.
4. Apri il monitor seriale (`pio device monitor -b 115200`) per verificare
   l'inizializzazione radio e la ricezione dei sensori.

Nessun display, nessuna schermata/grafico e nessun tasto utente generico su
questa scheda: la navigazione schermate e il menu di invio manuale non sono
disponibili. Durante i ~5 minuti di sincronizzazione oraria il **LED RGB
onboard** dà il feedback al posto dello schermo: **blu lampeggiante** = in
ascolto, **verde fisso** (~3 s) = orario confermato, **rosso lampeggiante** =
finestra scaduta senza conferma.

### How‑to: flashare la scheda MASN (NiceNano nRF52840 + HT‑RA62)

> ⚠️ **Supporto sperimentale, pinout non verificato.** I pin LoRa/batteria in
> `src/board_config.h` (blocco `BOARD_MASN_HTRA62`) sono dedotti dal profilo
> pubblico "NRF52 Pro‑micro DIY" indicato dalla stessa documentazione MASN per
> il flasher Meshtastic ufficiale, **non** dallo schematico reale della
> scheda. Confronta `Schematic_masn-ht-ra62.pdf` (repo
> [`danielcharrua/masn-meshtastic-autonomous-solar-node`](https://github.com/danielcharrua/masn-meshtastic-autonomous-solar-node))
> con i pin in `board_config.h` **prima** del primo upload: un pinout SPI
> errato può danneggiare il modulo HT‑RA62 o il NiceNano.

**Prerequisiti**
- [PlatformIO](https://platformio.org/).
- Scheda **MASN** variante **HT‑RA62** (NiceNano nRF52840).
- Cavo USB‑C dati.

**Passi**
1. Collega la scheda via USB‑C.
2. Modifica `settings.ini` come per la Heltec.
3. Compila e carica:
   ```bash
   pio run -e groupA_masn_htra62 -t upload
   ```
   Se l'upload non parte, entra manualmente in bootloader UF2 con un
   **doppio‑tap** del tasto **RST** (comportamento standard del bootloader
   Adafruit nRF52 usato dal NiceNano), poi rilancia l'upload.
4. Apri il monitor seriale (`pio device monitor -b 115200`) per verificare
   l'inizializzazione radio e la ricezione dei sensori.

Nessun display, nessun tasto utente e nessun feedback LED su questa scheda in
questa release (la scheda ha fisicamente 2 pulsanti + 2 switch, ma il pin del
"tasto utente" non è confermato): la navigazione schermate, il menu di invio
manuale e il feedback di sincronizzazione oraria non sono disponibili. Il
BME280 e l'INA3221 onboard **non** sono letti da questo firmware (solo bridge
LoRa, come la XIAO).

### How‑to: flashare la scheda FakeTec (NRF52840 Pro Micro + HT‑RA62)

> ⚠️ **Supporto sperimentale, mai testato su un dispositivo reale.** I pin
> LoRa/batteria in `src/board_config.h` (blocco `BOARD_FAKETEC`) sono
> **confermati dallo schematico ufficiale FakeTec v5**
> (`design_files/ShimonHoranek_fakeTecv5schematics.pdf` nel repo
> [`gargomoma/fakeTec_pcb`](https://github.com/gargomoma/fakeTec_pcb)),
> incrociato con lo schema di riferimento ufficiale Meshtastic "Pro‑micro
> Pinouts" (`nrf52_promicro_diy_tcxo`) — le due fonti indipendenti
> combaciano pin per pin. Non è stato però controllato su un modulo HT‑RA62
> né su una FakeTec fisica, e solo la revisione v5 è stata verificata a
> schematico (v3/v4 dovrebbero condividere lo stesso routing LoRa, cambia
> solo il circuito di ricarica batteria). Un rapido controllo di continuità
> col multimetro prima del primo upload resta comunque buona norma. Vedi
> anche l'articolo
> [adrelien.com](https://adrelien.com/diy-meshtastic-how-to-build-your-own-meshtastic-device-with-faketec-pcb-nrf52840/).

**Prerequisiti**
- [PlatformIO](https://platformio.org/).
- Scheda **FakeTec PCB** (v3/v4/v5) con **NRF52840 Pro Micro** + modulo LoRa
  **HT‑RA62** montati.
- Cavo USB‑C dati.

**Passi**
1. Collega la scheda via USB‑C.
2. Modifica `settings.ini` come per la Heltec.
3. Compila e carica:
   ```bash
   pio run -e groupA_faketec_htra62 -t upload
   ```
   Se l'upload non parte, entra manualmente in bootloader UF2 con un
   **doppio‑tap** del tasto **RST** (comportamento standard del bootloader
   Adafruit nRF52 usato dal Pro Micro), poi rilancia l'upload.
4. Apri il monitor seriale (`pio device monitor -b 115200`) per verificare
   l'inizializzazione radio e la ricezione dei sensori.

Nessun display, nessun tasto utente e nessun feedback LED su questa scheda in
questa release (le revisioni v3+ hanno fisicamente 2 pulsanti sul PCB, ma il
pin del "tasto utente" non è confermato): la navigazione schermate, il menu di
invio manuale e il feedback di sincronizzazione oraria non sono disponibili.

### Matrice hardware

| | Heltec V3 | Heltec V4 | XIAO nRF52840 + Wio‑SX1262 | MASN NiceNano + HT‑RA62 | FakeTec Pro Micro + HT‑RA62 |
|--|--|--|--|--|--|
| MCU / radio | ESP32‑S3 + SX1262 | ESP32‑S3 + SX1262 | nRF52840 + SX1262 | nRF52840 + SX1262 (sperim.) | nRF52840 + SX1262 (sperim.) |
| Display | OLED SSD1306 128×64 | OLED SSD1306 128×64 | assente (feedback su LED RGB) | assente (nessun feedback LED) | assente (nessun feedback LED) |
| Tasto utente / menu invio manuale | sì (PRG) | sì (PRG) | non disponibile | non disponibile | non disponibile |
| Gruppo A | `groupA_heltec_v3` | `groupA_heltec_v4` | `groupA_xiao_wiosx1262` | `groupA_masn_htra62` | `groupA_faketec_htra62` |
| Gruppo B | `groupB_heltec_v3` | `groupB_heltec_v4` | `groupB_xiao_wiosx1262` | `groupB_masn_htra62` | `groupB_faketec_htra62` |

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
- **20 supported sensors** across two radio groups (see table below).
- **Hand‑made Meshtastic gateway**: AES‑128/256‑CTR, protobuf, two channels
  (telemetry + text), NodeInfo, fixed map position.
- **Native telemetry**: `EnvironmentMetrics` + `AirQualityMetrics` (temperature,
  humidity, pressure, wind, rainfall 1h/24h, lux, soil, PM2.5/PM10, CO₂).
- **Time‑sync without RTC or GPS**: derives the clock from Meshtastic traffic at
  boot, multi‑confirmation algorithm, with the valid window derived from the
  **build date**.
- **Automatic bulletins**: 3 daily astronomy bulletins (sunrise+1h, noon,
  sunset−1h) on the primary channel + a fixed‑interval bulletin on the text
  channel, with emoji and the **local send date and time** (📅 🕒). The text
  never exceeds the Meshtastic 200‑byte limit: when space runs short, the
  least essential fields — astro, date/time, link — are dropped whole (never
  mid‑field), always keeping the actual sensor data.
- **Lightning alerts** on the text channel with a configurable threshold.
- **Manual bulletin send**: **long‑press** the PRG button → windowed menu to
  pick the channel (Ch0/Ch1) → confirm; every submenu has a "Back" entry and
  auto‑closes after 10 s of inactivity.
- **Offline ephemeris**: sunrise/sunset, moon phase (waxing/waning).
- **OLED display** with capability‑driven rotating screens: overview,
  temp/humidity, pressure, rain, wind, UV/light, lightning, air quality, soil,
  leak, **24h graphs** (temperature, humidity, rain, lightning), time+date,
  astro, Meshtastic status (short name + countdown to next send + **battery
  %/USB power**). Each data screen shows the **model and ID** of the received
  sensor (handy to tell your own station apart from a neighbour's).
- **Boot splash screen** (5 s) with logo, project name and firmware version.
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

The `[meshtastic]` section configures frequency, preset, intervals, channel
names/keys, position, timezone and lightning thresholds. Two pre‑build scripts
run on every build: `apply_settings.py` (network → `include/user_config.h`)
and `configure_sensors.py` (sensors → `src/config/generated_config.h` +
`build_src_filter`).

```bash
pio run -e groupA_heltec_v4 -t upload         # or _v3 / groupB_*
pio run -e groupA_xiao_wiosx1262 -t upload    # Seeed XIAO nRF52840 + Wio-SX1262
pio run -e groupA_masn_htra62 -t upload       # MASN NiceNano nRF52840 + HT-RA62 (experimental)
pio run -e groupA_faketec_htra62 -t upload    # FakeTec NRF52840 Pro Micro + HT-RA62 (experimental)
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

At boot the board shows the splash screen (5 s), then opens a ~5‑minute window
to sync time from the Meshtastic network; press **PRG** to cycle through the
screens. Once time‑sync is done, **long‑press** PRG to open the manual send
menu (short press = next item, long press = select).

### How‑to: flashing the XIAO nRF52840 + Wio‑SX1262 board

1. Install [PlatformIO](https://platformio.org/) and connect the **Seeed XIAO
   nRF52840 + Wio‑SX1262 Kit** (SKU 102010710/113010003, pin‑header radio
   module, not the ESP32‑S3's B2B variant) over USB‑C.
2. Edit `settings.ini` as for the Heltec.
3. Build & upload:
   ```bash
   pio run -e groupA_xiao_wiosx1262 -t upload
   ```
   If the upload doesn't start, enter the UF2 bootloader manually with a
   **double‑tap** of the **RST** button (standard XIAO behaviour), then
   re‑run the upload.
4. Open the serial monitor (`pio device monitor -b 115200`) to check radio
   init and sensor reception.

No display, no screens/graphs and no general‑purpose user button on this
board: screen navigation and the manual‑send menu aren't available. During
the ~5‑minute time‑sync window the **onboard RGB LED** gives feedback
instead: **blinking blue** = listening, **solid green** (~3 s) = time
confirmed, **blinking red** = window expired without confirmation.

### How‑to: flashing the MASN board (NiceNano nRF52840 + HT‑RA62)

> ⚠️ **Experimental support, pinout not verified.** The LoRa/battery pins in
> `src/board_config.h` (`BOARD_MASN_HTRA62` block) are derived from the public
> "NRF52 Pro‑micro DIY" profile MASN's own docs point to for the official
> Meshtastic flasher, **not** from the board's real schematic. Compare
> `Schematic_masn-ht-ra62.pdf` (repo
> [`danielcharrua/masn-meshtastic-autonomous-solar-node`](https://github.com/danielcharrua/masn-meshtastic-autonomous-solar-node))
> against the pins in `board_config.h` **before** the first upload: a wrong
> SPI pinout can damage the HT‑RA62 module or the NiceNano.

**Prerequisites**
- [PlatformIO](https://platformio.org/).
- A **MASN** board, **HT‑RA62** variant (NiceNano nRF52840).
- Data‑capable USB‑C cable.

**Steps**
1. Connect the board over USB‑C.
2. Edit `settings.ini` as for the Heltec.
3. Build & upload:
   ```bash
   pio run -e groupA_masn_htra62 -t upload
   ```
   If the upload doesn't start, enter the UF2 bootloader manually with a
   **double‑tap** of the **RST** button (standard behaviour of the Adafruit
   nRF52 bootloader used by the NiceNano), then re‑run the upload.
4. Open the serial monitor (`pio device monitor -b 115200`) to check radio
   init and sensor reception.

No display, no user button and no LED feedback on this board in this release
(the board physically has 2 buttons + 2 switches, but the "user button" pin
isn't confirmed): screen navigation, the manual‑send menu and time‑sync
feedback aren't available. The onboard BME280 and INA3221 are **not** read by
this firmware (LoRa bridge only, same as the XIAO).

### How‑to: flashing the FakeTec board (NRF52840 Pro Micro + HT‑RA62)

> ⚠️ **Experimental support, never tested on real hardware.** The LoRa/
> battery pins in `src/board_config.h` (`BOARD_FAKETEC` block) are
> **confirmed by the official FakeTec v5 schematic**
> (`design_files/ShimonHoranek_fakeTecv5schematics.pdf` in the
> [`gargomoma/fakeTec_pcb`](https://github.com/gargomoma/fakeTec_pcb) repo),
> cross‑checked against the official Meshtastic "Pro‑micro Pinouts"
> reference (`nrf52_promicro_diy_tcxo`) — both independent sources match pin
> for pin. It hasn't been tested on a real HT‑RA62 module or a physical
> FakeTec board though, and only revision v5 was checked against a
> schematic (v3/v4 should share the same LoRa routing, only the charging
> circuit differs). A quick continuity check with a multimeter before the
> first upload is still good practice. See also the
> [adrelien.com article](https://adrelien.com/diy-meshtastic-how-to-build-your-own-meshtastic-device-with-faketec-pcb-nrf52840/).

**Prerequisites**
- [PlatformIO](https://platformio.org/).
- A **FakeTec PCB** (v3/v4/v5) with an **NRF52840 Pro Micro** + **HT‑RA62**
  LoRa module fitted.
- Data‑capable USB‑C cable.

**Steps**
1. Connect the board over USB‑C.
2. Edit `settings.ini` as for the Heltec.
3. Build & upload:
   ```bash
   pio run -e groupA_faketec_htra62 -t upload
   ```
   If the upload doesn't start, enter the UF2 bootloader manually with a
   **double‑tap** of the **RST** button (standard behaviour of the Adafruit
   nRF52 bootloader used by the Pro Micro), then re‑run the upload.
4. Open the serial monitor (`pio device monitor -b 115200`) to check radio
   init and sensor reception.

No display, no user button and no LED feedback on this board in this release
(v3+ revisions physically have 2 buttons on the PCB, but the "user button"
pin isn't confirmed): screen navigation, the manual‑send menu and time‑sync
feedback aren't available.

---

## Struttura del progetto / Project layout

```
platformio.ini              # env: groupA/B × heltec_v3/v4/xiao_wiosx1262/masn_htra62/faketec_htra62
boards/                     # board.json locale al progetto (nRF52 ProMicro DIY + HT-RA62)
variants/                   # variant.h/.cpp locale al progetto (idem, condiviso MASN+FakeTec)
settings.ini                # [meshtastic] (parità EcoWitt) + [sensors]
sensors_catalog.json        # FONTE DI VERITÀ: sensori, gruppi, capability, sorgenti
tools/
  apply_settings.py         # [meshtastic] -> include/user_config.h (MESH_*, RX_*)
  configure_sensors.py      # [sensors] + catalogo -> generated_config.h + src_filter
  configurator/index.html   # configuratore web (genera tutto il settings.ini)
include/user_config.h        # generato (rete/nodo)
src/
  main.cpp                  # dispatch: match -> parse -> uiSubmit -> meshSubmit
  board_config.h             # pin per scheda (Heltec V3/V4, XIAO+Wio-SX1262, MASN/FakeTec+HT-RA62)
  config/generated_config.h # generato (RADIO_*, ENABLE_*, SCREEN_*)
  radio/                    # gestore unico SX1262 a 3 modalità
  sensors/                  # sensor_types.h, registry, sensor_util.h, fineoffset/ lacrosse/ bresser/
  display/                  # display.h, ui.{h,cpp}, splash_logo.h, screens/ (incl. grafici 24h)
                             #   display_oled.cpp (HAS_OLED) / display_none.cpp (senza display)
  mesh/                     # meshtastic_pack.{h,cpp}
  timesync.{h,cpp}  astro.{h,cpp}  history.h
  battery.h  battery_esp32.cpp  battery_nrf52.cpp  battery_nrf52_promicro_diy.cpp
  led_status.{h,cpp}        # feedback time-sync su LED RGB (schede senza display)
```

## Autore / Author

**IZ0KEW**

## Licenza / License

Licenza MIT — vedi il file `LICENSE`. *MIT License — see the `LICENSE` file.*
