#include "LocationAdvertiser.h"
#include <math.h>

// Earth's radius in meters
#define EARTH_RADIUS_M 6371000.0

LocationAdvertiser::LocationAdvertiser()
    : last_advert_lat(0.0),
      last_advert_lon(0.0),
      has_last_position(false),
      last_advert_time(0),
      last_guaranteed_check(0),
      distance_threshold(nullptr),
      frequency(nullptr),
      guaranteed_interval(nullptr),
      accuracy_threshold(nullptr),
      enabled(nullptr) {
}

void LocationAdvertiser::init(uint8_t* dist_thresh, uint8_t* freq, uint8_t* guaranteed, uint8_t* accuracy_thresh, uint8_t* en) {
    distance_threshold = dist_thresh;
    frequency = freq;
    guaranteed_interval = guaranteed;
    accuracy_threshold = accuracy_thresh;
    enabled = en;
    reset();
}

void LocationAdvertiser::reset() {
    has_last_position = false;
    last_advert_lat = 0.0;
    last_advert_lon = 0.0;
    last_advert_time = 0;
    last_guaranteed_check = millis();
}

double LocationAdvertiser::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    // Haversine formula for calculating distance between two GPS coordinates
    // Convert degrees to radians
    double lat1_rad = lat1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double delta_lat = (lat2 - lat1) * M_PI / 180.0;
    double delta_lon = (lon2 - lon1) * M_PI / 180.0;

    // Haversine formula
    double a = sin(delta_lat / 2.0) * sin(delta_lat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(delta_lon / 2.0) * sin(delta_lon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    double distance = EARTH_RADIUS_M * c;

    return distance;
}

unsigned long LocationAdvertiser::getGuaranteedIntervalMs() {
    if (!guaranteed_interval) return 60000; // Default 1 minute

    switch (*guaranteed_interval) {
        case 0: return 60000;      // 1 minute
        case 1: return 300000;     // 5 minutes
        case 2: return 900000;     // 15 minutes
        default: return 60000;     // Default to 1 minute
    }
}

bool LocationAdvertiser::shouldAdvertise(double lat, double lon, double accuracy) {
    // Feature disabled
    if (!enabled || *enabled == 0) {
        return false;
    }

    // Invalid position
    if (lat == 0.0 && lon == 0.0) {
        return false;
    }

    // Check GPS accuracy if threshold is set and accuracy is provided
    if (accuracy_threshold && accuracy > 0.0) {
        if (accuracy > *accuracy_threshold) {
            // GPS accuracy not good enough (higher number = worse accuracy)
            return false;
        }
    }

    unsigned long now = millis();
    bool should_send = false;

    // First position - always send
    if (!has_last_position) {
        return true;
    }

    // Calculate distance moved since last advertisement
    double distance_moved = calculateDistance(last_advert_lat, last_advert_lon, lat, lon);

    // Check distance threshold trigger
    if (distance_threshold && distance_moved >= *distance_threshold) {
        // Check frequency limiter
        unsigned long freq_ms = frequency ? (*frequency * 10 * 1000) : 30000; // Default 30s
        if (now - last_advert_time >= freq_ms) {
            should_send = true;
        }
    }

    // Check guaranteed interval (only if we haven't sent recently due to distance/frequency)
    if (!should_send) {
        unsigned long guaranteed_ms = getGuaranteedIntervalMs();
        if (now - last_guaranteed_check >= guaranteed_ms) {
            // Skip if recently sent due to distance trigger (within last frequency period)
            unsigned long freq_ms = frequency ? (*frequency * 10 * 1000) : 30000;
            if (now - last_advert_time >= freq_ms) {
                should_send = true;
            }
            // Update guaranteed check regardless (so we check again in next interval)
            last_guaranteed_check = now;
        }
    }

    return should_send;
}

void LocationAdvertiser::recordAdvertisement(double lat, double lon) {
    last_advert_lat = lat;
    last_advert_lon = lon;
    has_last_position = true;
    last_advert_time = millis();
}

bool LocationAdvertiser::getLastAdvertisedPosition(double& lat, double& lon) {
    if (!has_last_position) {
        return false;
    }
    lat = last_advert_lat;
    lon = last_advert_lon;
    return true;
}
