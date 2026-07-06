// ============================================================================
// board_config.h — definizione pin per le schede supportate
//   - Heltec WiFi LoRa 32 V3  (ESP32-S3 + SX1262 + OLED SSD1306)
//   - Heltec WiFi LoRa 32 V4  (pin-compatibile con la V3, PA piu' potente)
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

#else
  #error "Definisci BOARD_HELTEC_V3 o BOARD_HELTEC_V4 nei build_flags"
#endif
