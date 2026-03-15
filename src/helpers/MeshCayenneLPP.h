#pragma once

#include <CayenneLPP.h>
#include "sensors/MeshLPPTypes.h"

class MeshCayenneLPP : public CayenneLPP {
public:
  explicit MeshCayenneLPP(uint8_t size) : CayenneLPP(size) {}

  uint8_t addSpeed(uint8_t channel, float value) {
    return addScaledU16(channel, LPP_SPEED, LPP_SPEED_SIZE, LPP_SPEED_MULT, value);
  }

  uint8_t addGust(uint8_t channel, float value) {
    return addScaledU16(channel, LPP_GUST, LPP_GUST_SIZE, LPP_GUST_MULT, value);
  }

private:
  uint8_t addScaledU16(uint8_t channel, uint8_t type, uint8_t size, uint16_t multiplier, float value) {
    if ((_cursor + size + 2) > _maxsize) {
      _error = LPP_ERROR_OVERFLOW;
      return 0;
    }
    if (value < 0) {
      value = 0;
    }

    uint16_t encoded = value * multiplier;
    _buffer[_cursor++] = channel;
    _buffer[_cursor++] = type;
    _buffer[_cursor++] = encoded >> 8;
    _buffer[_cursor++] = encoded & 0xFF;
    return _cursor;
  }
};
