#ifndef BTTF_TYPES_H
#define BTTF_TYPES_H

#include <Arduino.h>

// ================== DEFINIZIONI TIPI PER BTTF ==================

// Struct per le date BTTF
struct BTTFDate {
  uint8_t month;
  uint8_t day;
  uint16_t year;
  uint8_t hour;
  uint8_t minute;
  const char* ampm;
};

// Struct per coordinate individuali dei campi
struct FieldCoords {
  int16_t x;
  int16_t y;
};

// Struct per coordinate di un pannello (6 campi)
struct PanelCoords {
  FieldCoords month;
  FieldCoords day;
  FieldCoords year;
  FieldCoords ampm;  // LED AM/PM
  FieldCoords hour;
  FieldCoords min;
};

#endif // BTTF_TYPES_H
