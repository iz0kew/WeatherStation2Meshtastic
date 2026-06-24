// ============================================================================
// sensor_util.h — utility di decodifica condivise dai parser (header-only)
//   CRC-8 Fine Offset, checksum a somma, digest LFSR Bresser, reflect bit.
// ============================================================================
#pragma once
#include <Arduino.h>

// CRC-8, polinomio 0x31, init 0x00, MSB-first (standard Fine Offset / Ecowitt)
static inline uint8_t crc8_0x31(const uint8_t *data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
  }
  return crc;
}

// CRC-8 generico (poly/init parametrici, MSB-first) — usato da alcuni Bresser
static inline uint8_t crc8_generic(const uint8_t *data, size_t len,
                                   uint8_t poly, uint8_t init) {
  uint8_t crc = init;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ poly) : (uint8_t)(crc << 1);
  }
  return crc;
}

// Somma a 8 bit
static inline uint8_t sum8(const uint8_t *data, size_t len) {
  uint8_t s = 0;
  for (size_t i = 0; i < len; i++) s += data[i];
  return s;
}

// XOR di tutti i byte (parita' Bresser 5in1)
static inline uint8_t xor8(const uint8_t *data, size_t len) {
  uint8_t x = 0;
  for (size_t i = 0; i < len; i++) x ^= data[i];
  return x;
}

// Somma dei nibble (checksum Bresser 5in1)
static inline uint8_t add_nibbles(const uint8_t *data, size_t len) {
  uint8_t s = 0;
  for (size_t i = 0; i < len; i++) { s += (data[i] >> 4); s += (data[i] & 0x0F); }
  return s;
}

// CRC-16 MSB-first (poly/init parametrici) — usato dal Bresser leakage (0x1021)
static inline uint16_t crc16_msb(const uint8_t *data, size_t len,
                                 uint16_t poly, uint16_t init) {
  uint16_t crc = init;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ poly) : (uint16_t)(crc << 1);
  }
  return crc;
}

// Digest LFSR a 16 bit (Bresser 6in1/7in1, vedi rtl_433 bresser_6in1.c)
static inline uint16_t lfsr_digest16(const uint8_t *data, size_t bytes,
                                     uint16_t gen, uint16_t key) {
  uint16_t sum = 0;
  for (size_t k = 0; k < bytes; k++) {
    uint8_t b = data[k];
    for (int i = 7; i >= 0; i--) {
      if ((b >> i) & 1) sum ^= key;
      if (key & 1) key = (uint16_t)((key >> 1) ^ gen);
      else         key = (uint16_t)(key >> 1);
    }
  }
  return sum;
}
