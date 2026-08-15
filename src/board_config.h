// ============================================================================
// board_config.h — definizione pin per le schede supportate
//   - Heltec WiFi LoRa 32 V3  (ESP32-S3 + SX1262 + OLED SSD1306)
//   - Heltec WiFi LoRa 32 V4  (pin-compatibile con la V3, PA piu' potente)
//   - Seeed XIAO nRF52840 + Wio-SX1262 Kit (SKU 102010710, pin-header)
//     Nessun display, nessun tasto utente: schermate/grafici/menu invio
//     manuale non sono disponibili (vedi display/display_none.cpp e i guard
//     su PIN_BUTTON/HAS_OLED in main.cpp). Feedback time-sync sul LED RGB
//     onboard (vedi led_status.cpp, HAS_STATUS_LED).
//   - MASN — NiceNano nRF52840 + modulo LoRa HT-RA62 (nodo solare open-source,
//     danielpcostas.dev/masn). Nessun display, nessun PIN_BUTTON/HAS_STATUS_LED
//     attivo (la scheda ha 2 pulsanti + 2 switch e possibilmente un LED, ma il
//     pin esatto non e' confermato). Richiede la board/variant PlatformIO
//     locale boards/nrf52_promicro_diy_htra62.json +
//     variants/nrf52_promicro_diy_htra62/ (nessuna board "nicenano" nel
//     platform ufficiale nordicnrf52).
//   - FakeTec — NRF52840 Pro Micro + modulo LoRa HT-RA62 (PCB DIY, v3/v4/v5 —
//     https://adrelien.com/diy-meshtastic-how-to-build-your-own-meshtastic-device-with-faketec-pcb-nrf52840/,
//     repo gargomoma/fakeTec_pcb). Nessun display; pinout LoRa/batteria
//     CONFERMATO dallo schematico ufficiale v5 (vedi blocco BOARD_FAKETEC
//     piu' sotto), pulsante utente esiste ma il suo GPIO raw non e' ancora
//     mappato -> PIN_BUTTON/HAS_STATUS_LED non attivi in questa prima
//     versione. Usa la stessa board/variant locale condivisa con MASN
//     (stesso schema community "nRF52 Pro-micro DIY" per il modulo HT-RA62).
// ============================================================================
#pragma once

#if defined(BOARD_HELTEC_V3) || defined(BOARD_HELTEC_V4)

  #define HAS_OLED 1
  #if defined(BOARD_HELTEC_V4)
    #define BOARD_NAME "Heltec V4"
  #else
    #define BOARD_NAME "Heltec V3"
  #endif

  // SX1262
  #define PIN_LORA_NSS   8
  #define PIN_LORA_SCK   9
  #define PIN_LORA_MOSI  10
  #define PIN_LORA_MISO  11
  #define PIN_LORA_RST   12
  #define PIN_LORA_BUSY  13
  #define PIN_LORA_DIO1  14

  // OLED SSD1306 (I2C)
  #define PIN_OLED_SDA   17
  #define PIN_OLED_SCL   18
  #define PIN_OLED_RST   21

  // Vext: alimenta l'OLED, attivo BASSO
  #define PIN_VEXT       36
  #define VEXT_ON_LEVEL  LOW

  #define PIN_BUTTON     0   // tasto PRG

  // Lettura batteria: partitore 390k/100k su VBAT, abilitato da ADC_Ctrl.
  // Sulle revisioni V3.2 l'ADC_Ctrl e' attivo ALTO: se con batteria collegata
  // la lettura resta ~0V, cambiare VBAT_CTRL_ON in HIGH.
  #define PIN_VBAT_READ  1    // GPIO1 = ADC1_CH0
  #define PIN_ADC_CTRL   37
  #define VBAT_CTRL_ON   LOW
  #define VBAT_MULTIPLIER 4.9f   // (390k+100k)/100k

  #define MESH_HW_MODEL  43   // HELTEC_V3 (hardware.proto HardwareModel)

#elif defined(BOARD_XIAO_WIOSX1262)

  // Nessun HAS_OLED: display/display_none.cpp fornisce le gfx* no-op.
  // Nessun PIN_BUTTON: il kit non espone un tasto utente generico -> il
  // menu di invio manuale (main.cpp/ui.cpp) resta compilato ma irraggiungibile.
  #define BOARD_NAME "XIAO nRF52840 + Wio-SX1262"

  // Wio-SX1262 Kit per XIAO, connessione a pin-header (SKU 102010710 /
  // 113010003). Pinout confermato dal variant.h ufficiale Meshtastic
  // (variants/nrf52840/seeed_xiao_nrf52840_kit), blocco "Default" (non B2B).
  #define PIN_LORA_NSS   D4
  #define PIN_LORA_SCK   D8
  #define PIN_LORA_MOSI  D10
  #define PIN_LORA_MISO  D9
  #define PIN_LORA_RST   D2
  #define PIN_LORA_BUSY  D3
  #define PIN_LORA_DIO1  D1
  #define PIN_LORA_RXEN  D5     // via radio.setRfSwitchPins(), non un RF-switch DIO2 puro
  #define RADIO_TCXO_VOLTAGE 1.8f

  // LED RGB onboard, anodo comune (attivo BASSO).
  #define PIN_LED_RED    LED_RED
  #define PIN_LED_GREEN  LED_GREEN
  #define PIN_LED_BLUE   LED_BLUE
  #define LED_ACTIVE_LOW 1
  #define HAS_STATUS_LED 1

  // Batteria: divisore onboard su AIN7 (PIN_VBAT), abilitato da VBAT_ENABLE
  // (macro fornite dal board package Seeed). Lettura in battery_nrf52.cpp.
  #define PIN_VBAT_READ    PIN_VBAT
  #define PIN_VBAT_ENABLE  VBAT_ENABLE

  #define MESH_HW_MODEL  88   // XIAO_NRF52_KIT (hardware.proto HardwareModel)

#elif defined(BOARD_MASN_HTRA62)

  // MASN (danielpcostas.dev/masn) — NiceNano nRF52840 + modulo LoRa HT-RA62.
  // Nessun HAS_OLED (display/display_none.cpp fornisce le gfx* no-op).
  // Nessun PIN_BUTTON / HAS_STATUS_LED in questa prima versione: la scheda ha
  // fisicamente 2 pulsanti + 2 switch (e forse un LED), ma non e' confermato
  // quale GPIO corrisponda al "tasto utente" ne' se esista un LED pilotabile
  // via firmware -> menu di invio manuale e feedback LED restano disattivati
  // finche' non si verifica lo schematico reale (vedi nota sotto).
  #define BOARD_NAME "MASN (NiceNano nRF52840 + HT-RA62)"

  // ==========================================================================
  // ATTENZIONE — PINOUT NON VERIFICATO SULL'HARDWARE REALE.
  // La documentazione MASN indica di selezionare il profilo "NRF52 Pro-micro
  // DIY" su flasher.meshtastic.org (nessun variant.h dedicato a MASN esiste
  // nel firmware Meshtastic ufficiale): questo suggerisce che il PCB sia
  // instradato secondo lo schema pubblico "nRF52 ProMicro DIY + TCXO" gia'
  // noto alla community (variant nrf52840/diy/nrf52_promicro_diy_tcxo del
  // firmware Meshtastic), da cui sono stati derivati i numeri pin sotto
  // (notazione raw nRF52 "porta*32+pin", es. P1.13 = 1*32+13 = 45). Sono gli
  // STESSI pin del blocco BOARD_FAKETEC piu' sotto, per lo stesso motivo.
  // NON E' PERO' CONFERMATO dallo schematico MASN (Schematic_masn-ht-ra62.pdf)
  // ne' testato su un modulo HT-RA62 reale. Verificare con multimetro/
  // schematico PRIMA del primo upload: un pinout SPI errato puo' danneggiare
  // il modulo HT-RA62 o il NiceNano.
  // ==========================================================================
  #define PIN_LORA_MISO  2          // P0.02
  #define PIN_LORA_MOSI  47         // P1.15  (1*32+15)
  #define PIN_LORA_SCK   43         // P1.11  (1*32+11)
  #define PIN_LORA_NSS   45         // P1.13  (1*32+13)
  #define PIN_LORA_DIO1  10         // P0.10
  #define PIN_LORA_BUSY  29         // P0.29
  #define PIN_LORA_RST   9          // P0.09
  #define PIN_LORA_RXEN  17         // P0.17  (DIO2 usato come RF-switch dal modulo)
  #define RADIO_TCXO_VOLTAGE 1.8f

  // Batteria: lettura ADC nRF52 generica (vedi battery_nrf52_promicro_diy.cpp).
  // Pin VBAT desunto dallo stesso schema DIY di riferimento (P0.31); nessun
  // pin di enable dedicato noto per MASN (a differenza della XIAO). Coefficienti
  // di conversione mV/percentuale in battery_nrf52_promicro_diy.cpp sono
  // PLACEHOLDER da calibrare con un multimetro sull'hardware reale.
  #define PIN_VBAT_READ  31         // P0.31

  #define MESH_HW_MODEL  255  // PRIVATE_HW: nessun HardwareModel Meshtastic dedicato a MASN

#elif defined(BOARD_FAKETEC)

  // FakeTec — NRF52840 Pro Micro + modulo LoRa HT-RA62 (PCB DIY community,
  // v3/v4/v5 — vedi repo gargomoma/fakeTec_pcb e l'articolo adrelien.com).
  // Nessun HAS_OLED (display/display_none.cpp fornisce le gfx* no-op).
  // Nessun PIN_BUTTON / HAS_STATUS_LED in questa prima versione. Lo
  // schematico v5 mostra 2 pulsanti: RST_BTN (SW1, reset hardware) e un BTN
  // "programmabile" (SW2), quest'ultimo sull'header destro del NiceNano con
  // l'annotazione "connected to D6" — quindi un tasto utente reale ESISTE,
  // ma il GPIO raw nRF52 (P0.xx/P1.xx) dietro quell'alias "D6" non e' stato
  // identificato in questa prima versione -> PIN_BUTTON resta non definito
  // finche' non si mappa quel pin (nessun LED pilotabile via firmware
  // individuato sullo schematico). Menu di invio manuale e feedback LED
  // restano quindi disattivati per ora (stessa scelta prudente gia' fatta
  // per BOARD_MASN_HTRA62 qui sopra).
  #define BOARD_NAME "FakeTec (NRF52840 Pro Micro + HT-RA62)"

  // ==========================================================================
  // PINOUT CONFERMATO dallo schematico ufficiale FakeTec v5
  // (design_files/ShimonHoranek_fakeTecv5schematics.pdf nel repo
  // gargomoma/fakeTec_pcb, KiCad, rev. 2025-03-19): il blocco "NicetNano
  // nrf52840 microcontroller" instrada l'header sinistro nell'esatta sequenza
  // RST, DIO1, SCK, CS, MOSI, MISO, BUSY, BattX verso il modulo HT-RA62/
  // RA-01SH, e RXEN e' presente come rete dedicata. Incrociato con lo schema
  // di riferimento ufficiale Meshtastic "Pro-micro Pinouts"
  // (variants/nrf52840/diy/nrf52_promicro_diy_tcxo/Schematic_Pro-micro_Pinouts_*.pdf),
  // che mostra la stessa identica sequenza posizionale sul connettore
  // standard PRO_MICRO_NRF52840_26P: pin14=P0.09(RST), pin15=P0.10(DIO1),
  // pin16=P1.11(SCK), pin17=P1.13(CS), pin18=P1.15(MOSI), pin19=P0.02(MISO),
  // pin20=P0.29(BUSY), pin21=P0.31(BATT), con RXEN=P0.17 collegato all'HT-RA62.
  // Le due fonti indipendenti combaciano esattamente (notazione raw nRF52
  // "porta*32+pin", es. P1.13 = 1*32+13 = 45) — sono gli STESSI numeri gia'
  // usati nel blocco BOARD_MASN_HTRA62 qui sopra (per MASN pero' lo
  // schematico reale non e' stato controllato, resta dedotto).
  // Verificato solo sulla revisione v5: le revisioni precedenti (v3/v4)
  // dovrebbero condividere lo stesso routing LoRa (cambia solo il circuito
  // di ricarica batteria), ma non e' stato controllato schematico alla mano.
  // Un rapido controllo di continuita' col multimetro prima del primo upload
  // resta comunque buona norma, non piu' per dubbio sul pinout ma come
  // verifica standard pre power-on.
  // ==========================================================================
  #define PIN_LORA_MISO  2          // P0.02
  #define PIN_LORA_MOSI  47         // P1.15  (1*32+15)
  #define PIN_LORA_SCK   43         // P1.11  (1*32+11)
  #define PIN_LORA_NSS   45         // P1.13  (1*32+13)
  #define PIN_LORA_DIO1  10         // P0.10
  #define PIN_LORA_BUSY  29         // P0.29
  #define PIN_LORA_RST   9          // P0.09
  #define PIN_LORA_RXEN  17         // P0.17  (DIO2 usato come RF-switch dal modulo)
  #define RADIO_TCXO_VOLTAGE 1.8f

  // Batteria: lettura ADC nRF52 generica (vedi battery_nrf52_promicro_diy.cpp).
  // Pin VBAT confermato dallo schematico FakeTec v5 (net "BattX", stessa
  // sequenza posizionale di RST/DIO1/SCK/CS/MOSI/MISO/BUSY -> P0.31, vedi
  // nota sopra). Nessun pin di enable dedicato: il partitore (R4=10M,
  // R5=10M verso GND, rapporto 1:2 -> VBAT_DIVIDER_COMP=2.0f sotto) resta
  // sempre collegato, corrente trascurabile (~165nA a 3.3V) quindi non serve
  // un pin di enable. Il jumper BOOST del circuito di carica FakeTec incide
  // sulla corrente di ricarica, non su questo partitore di lettura.
  #define PIN_VBAT_READ  31         // P0.31
  #define VBAT_DIVIDER_COMP 2.0f    // R4=10M / R5=10M verso GND (schematico v5)

  #define MESH_HW_MODEL  255  // PRIVATE_HW: nessun HardwareModel Meshtastic dedicato a FakeTec

#else
  #error "Definisci BOARD_HELTEC_V3, BOARD_HELTEC_V4, BOARD_XIAO_WIOSX1262, BOARD_MASN_HTRA62 o BOARD_FAKETEC nei build_flags"
#endif
