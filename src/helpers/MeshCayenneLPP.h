#pragma once

#include <CayenneLPP.h>
#include "sensors/MeshLPPTypes.h"

class MeshCayenneLPP : public CayenneLPP {
public:
  explicit MeshCayenneLPP(uint8_t size) : CayenneLPP(size) {}

  uint8_t addSpeed(uint8_t channel, float value) {
    if ((_cursor + LPP_SPEED_SIZE + 2) > _maxsize) {
      _error = LPP_ERROR_OVERFLOW;
      return 0;
    }
    if (value < 0) {
      value = 0;
    }

    uint16_t encoded = value * LPP_SPEED_MULT;
    _buffer[_cursor++] = channel;
    _buffer[_cursor++] = LPP_SPEED;
    _buffer[_cursor++] = encoded >> 8;
    _buffer[_cursor++] = encoded & 0xFF;
    return _cursor;
  }
};
