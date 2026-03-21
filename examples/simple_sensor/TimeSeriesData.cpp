#include "TimeSeriesData.h"

void TimeSeriesData::clear() {
  memset(data, 0, sizeof(float) * num_slots);
  last_timestamp = 0;
  bucket_start_timestamp = 0;
  next = 0;
  num_filled = 0;
  pending_total = 0.0f;
  pending_count = 0;
}

void TimeSeriesData::recordData(mesh::RTCClock* clock, float value) {
  uint32_t now = clock->getCurrentTime();
  if (pending_count == 0) {
    bucket_start_timestamp = now;
    pending_total = value;
    pending_count = 1;
    return;
  }

  if (now < bucket_start_timestamp + interval_secs) {
    pending_total += value;
    pending_count++;
    return;
  }

  last_timestamp = bucket_start_timestamp + interval_secs;
  data[next] = pending_total / pending_count;
  next = (next + 1) % num_slots;
  if (num_filled < num_slots) {
    num_filled++;
  }

  bucket_start_timestamp = now;
  pending_total = value;
  pending_count = 1;
}

void TimeSeriesData::calcMinMaxAvg(mesh::RTCClock* clock, uint32_t start_secs_ago, uint32_t end_secs_ago, MinMaxAvg* dest, uint8_t channel, uint8_t lpp_type) const {
  int i = next, n = num_filled;
  uint32_t ago = clock->getCurrentTime() - last_timestamp;
  int num_values = 0;
  float total = 0.0f;

  dest->_channel = channel;
  dest->_lpp_type = lpp_type;

  // start at most recet recording, back-track through to oldest
  while (n > 0) {
    n--;
    i = (i + num_slots - 1) % num_slots;  // go back by one
    if (ago >= end_secs_ago && ago < start_secs_ago) {   // filter by the desired time range
      float v = data[i];
      num_values++;
      total += v;
      if (num_values == 1) {
        dest->_max = dest->_min = v;
      } else {
        if (v < dest->_min) dest->_min = v;
        if (v > dest->_max) dest->_max = v;
      }
    }
    ago += interval_secs;
  }
  // calc average
  if (num_values > 0) {
    dest->_avg = total / num_values;
  } else {
    dest->_max = dest->_min = dest->_avg = NAN;
  }
}

bool TimeSeriesData::calcFirstLast(mesh::RTCClock* clock, uint32_t start_secs_ago, uint32_t end_secs_ago, float& first, float& last) const {
  int i = next, n = num_filled;
  uint32_t ago = clock->getCurrentTime() - last_timestamp;
  bool found = false;

  while (n > 0) {
    n--;
    i = (i + num_slots - 1) % num_slots;  // go back by one
    if (ago >= end_secs_ago && ago < start_secs_ago) {
      float v = data[i];
      if (!found) {
        last = v;
      }
      first = v;
      found = true;
    }
    ago += interval_secs;
  }

  return found;
}

int TimeSeriesData::copyChronological(float* dest, int max_values) const {
  int count = min(num_filled, max_values);
  int start = (next + num_slots - count) % num_slots;

  for (int i = 0; i < count; i++) {
    dest[i] = data[(start + i) % num_slots];
  }

  return count;
}
