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

#else
  #error "Definisci BOARD_HELTEC_V3 o BOARD_HELTEC_V4 nei build_flags"
#endif
