#ifndef OLEDDISPLAYFONTS_h
#define OLEDDISPLAYFONTS_h

#ifdef ARDUINO
#include <Arduino.h>
#elif __MBED__
#define PROGMEM
#endif

extern const uint8_t ArialMT_Plain_10[] PROGMEM;
extern const uint8_t ArialMT_Plain_16[] PROGMEM;
extern const uint8_t ArialMT_Plain_24[] PROGMEM;
extern const uint8_t ArialMT_Plain_7[] PROGMEM;  // Small font for top bar screen names
extern const uint8_t ArialMT_Plain_5[] PROGMEM;  // Extra small font for top bar screen names
#endif
