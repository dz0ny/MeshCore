#pragma once

#include <CayenneLPP.h>
#include <math.h>
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

  uint8_t addDewPoint(uint8_t channel, float value) {
    return addScaledS16(channel, LPP_DEWPOINT, LPP_DEWPOINT_SIZE, LPP_DEWPOINT_MULT, value);
  }

  uint8_t addRain(uint8_t channel, float value) {
    return addScaledU16(channel, LPP_RAIN, LPP_RAIN_SIZE, LPP_RAIN_MULT, value);
  }

  uint8_t addCustomU8(uint8_t channel, uint8_t type, uint8_t value) {
    return addUnsigned(channel, type, 1, value);
  }

  uint8_t addCustomS8(uint8_t channel, uint8_t type, int8_t value) {
    return addSigned(channel, type, 1, value);
  }

  uint8_t addCustomScaledU8(uint8_t channel, uint8_t type, uint32_t multiplier, float value) {
    return addScaledUnsigned(channel, type, 1, multiplier, value);
  }

  uint8_t addCustomScaledS8(uint8_t channel, uint8_t type, uint32_t multiplier, float value) {
    return addScaledSigned(channel, type, 1, multiplier, value);
  }

  uint8_t addCustomScaledU16(uint8_t channel, uint8_t type, uint32_t multiplier, float value) {
    return addScaledUnsigned(channel, type, 2, multiplier, value);
  }

  uint8_t addCustomScaledS16(uint8_t channel, uint8_t type, uint32_t multiplier, float value) {
    return addScaledSigned(channel, type, 2, multiplier, value);
  }

  uint8_t addCustomScaledU32(uint8_t channel, uint8_t type, uint32_t multiplier, float value) {
    return addScaledUnsigned(channel, type, 4, multiplier, value);
  }

  uint8_t addCustomScaledS32(uint8_t channel, uint8_t type, uint32_t multiplier, float value) {
    return addScaledSigned(channel, type, 4, multiplier, value);
  }

private:
  uint8_t addScaledU16(uint8_t channel, uint8_t type, uint8_t size, uint16_t multiplier, float value) {
    return addScaledUnsigned(channel, type, size, multiplier, value);
  }

  uint8_t addScaledS16(uint8_t channel, uint8_t type, uint8_t size, uint16_t multiplier, float value) {
    return addScaledSigned(channel, type, size, multiplier, value);
  }

  uint8_t addScaledUnsigned(uint8_t channel, uint8_t type, uint8_t size, uint32_t multiplier, float value) {
    if (value < 0.0f) {
      value = 0.0f;
    }
    uint64_t encoded = llroundf(value * multiplier);
    return addUnsigned(channel, type, size, encoded);
  }

  uint8_t addScaledSigned(uint8_t channel, uint8_t type, uint8_t size, uint32_t multiplier, float value) {
    int64_t encoded = llroundf(value * multiplier);
    return addSigned(channel, type, size, encoded);
  }

  uint8_t addUnsigned(uint8_t channel, uint8_t type, uint8_t size, uint64_t value) {
    if ((_cursor + size + 2) > _maxsize) {
      _error = LPP_ERROR_OVERFLOW;
      return 0;
    }

    uint64_t max_value = (size >= 8) ? UINT64_MAX : ((1ULL << (size * 8)) - 1ULL);
    if (value > max_value) {
      return 0;
    }

    _buffer[_cursor++] = channel;
    _buffer[_cursor++] = type;
    for (int8_t i = size - 1; i >= 0; i--) {
      _buffer[_cursor++] = (value >> (i * 8)) & 0xFF;
    }
    return _cursor;
  }

  uint8_t addSigned(uint8_t channel, uint8_t type, uint8_t size, int64_t value) {
    if ((_cursor + size + 2) > _maxsize) {
      _error = LPP_ERROR_OVERFLOW;
      return 0;
    }

    int64_t min_value = -(1LL << ((size * 8) - 1));
    int64_t max_value = (1LL << ((size * 8) - 1)) - 1;
    if (value < min_value || value > max_value) {
      return 0;
    }

    uint64_t encoded = (uint64_t) value & ((1ULL << (size * 8)) - 1ULL);
    _buffer[_cursor++] = channel;
    _buffer[_cursor++] = type;
    for (int8_t i = size - 1; i >= 0; i--) {
      _buffer[_cursor++] = (encoded >> (i * 8)) & 0xFF;
    }
    return _cursor;
  }
};
