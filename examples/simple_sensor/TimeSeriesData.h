#pragma once

#include <Arduino.h>
#include <Mesh.h>

struct MinMaxAvg {
  float _min, _max, _avg;
  uint8_t _lpp_type, _channel;
};

class TimeSeriesData {
  float* data;
  int num_slots, next, num_filled;
  uint32_t last_timestamp;
  uint32_t bucket_start_timestamp;
  uint32_t interval_secs;
  float pending_total;
  uint16_t pending_count;

public:
  TimeSeriesData(float* array, int num, uint32_t secs)
      : num_slots(num), data(array), next(0), num_filled(0), last_timestamp(0), bucket_start_timestamp(0), interval_secs(secs),
        pending_total(0.0f), pending_count(0) {
    memset(data, 0, sizeof(float)*num);
  }
  TimeSeriesData(int num, uint32_t secs)
      : num_slots(num), next(0), num_filled(0), last_timestamp(0), bucket_start_timestamp(0), interval_secs(secs),
        pending_total(0.0f), pending_count(0) {
    data = new float[num];
    memset(data, 0, sizeof(float)*num);
  }

  void clear();
  void recordData(mesh::RTCClock* clock, float value);
  void calcMinMaxAvg(mesh::RTCClock* clock, uint32_t start_secs_ago, uint32_t end_secs_ago, MinMaxAvg* dest, uint8_t channel, uint8_t lpp_type) const;
  bool calcFirstLast(mesh::RTCClock* clock, uint32_t start_secs_ago, uint32_t end_secs_ago, float& first, float& last) const;
  int copyChronological(float* dest, int max_values) const;
};
