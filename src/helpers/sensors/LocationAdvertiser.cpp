#include "LocationAdvertiser.h"
#include <math.h>
#include <MeshCore.h>

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

    MESH_DEBUG_PRINTLN("LocationAdvertiser INIT - enabled: %d, dist: %d m, freq: %d, guaranteed: %d, accuracy: %d m",
                      *en, *dist_thresh, *freq, *guaranteed, *accuracy_thresh);
    MESH_DEBUG_PRINTLN("LocationAdvertiser INIT - guaranteed interval: %lu ms", getGuaranteedIntervalMs());
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
        case 1: return 120000;     // 2 minutes
        case 2: return 300000;     // 5 minutes
        case 3: return 600000;     // 10 minutes
        case 4: return 900000;     // 15 minutes
        default: return 60000;     // Default to 1 minute
    }
}

bool LocationAdvertiser::shouldAdvertise(double lat, double lon, double accuracy) {
    // Feature disabled
    if (!enabled || *enabled == 0) {
        MESH_DEBUG_PRINTLN("LocationAdvertiser: Feature DISABLED");
        return false;
    }

    unsigned long now = millis();
    bool should_send = false;
    bool is_guaranteed_interval = false;

    // Check guaranteed interval FIRST (before any position validation)
    // Guaranteed interval works even with invalid GPS (uses last known position)
    // This ensures periodic updates even when GPS signal is lost
    unsigned long guaranteed_ms = getGuaranteedIntervalMs();
    unsigned long time_since_check = (now - last_guaranteed_check) / 1000;

    if (has_last_position && (now - last_guaranteed_check >= guaranteed_ms)) {
        // Update the guaranteed check timer NOW to maintain the guaranteed interval
        last_guaranteed_check = now;

        // Guaranteed interval always sends with last known position
        // Ignores current GPS validity, accuracy, and frequency limiter
        should_send = true;
        is_guaranteed_interval = true;
        MESH_DEBUG_PRINTLN("LocationAdvertiser: GUARANTEED INTERVAL TRIGGERED - %lu sec elapsed (threshold: %lu ms), sending last known position", time_since_check, guaranteed_ms);
        return true;  // Return immediately, bypass all other checks
    } else if (has_last_position) {
        unsigned long time_remaining = (guaranteed_ms - (now - last_guaranteed_check)) / 1000;
        MESH_DEBUG_PRINTLN("LocationAdvertiser: shouldAdvertise called - guaranteed interval NOT due yet (%lu sec elapsed, %lu sec remaining)", time_since_check, time_remaining);
    }

    // Invalid position - only matters for distance-based triggers
    if (lat == 0.0 && lon == 0.0) {
        MESH_DEBUG_PRINTLN("LocationAdvertiser: Invalid position (0,0) - skipping distance check");
        return false;
    }

    // First position - always send
    if (!has_last_position) {
        MESH_DEBUG_PRINTLN("LocationAdvertiser: FIRST POSITION - sending");
        return true;
    }

    // If not triggered by guaranteed interval, check distance-based trigger
    // (distance-based triggers require good accuracy)
    if (!should_send) {
        // Check GPS accuracy if threshold is set and accuracy is provided
        // NOTE: This check is ONLY for distance-based triggers
        if (accuracy_threshold && accuracy > 0.0 && accuracy > *accuracy_threshold) {
            // GPS accuracy not good enough for distance-based trigger
            return false;
        }

        // Calculate distance moved since last advertisement
        double distance_moved = calculateDistance(last_advert_lat, last_advert_lon, lat, lon);

        // Check distance threshold trigger
        if (distance_threshold && distance_moved >= *distance_threshold) {
            // Check frequency limiter
            unsigned long freq_ms = frequency ? (*frequency * 10 * 1000) : 30000; // Default 30s
            if (now - last_advert_time >= freq_ms) {
                should_send = true;
                MESH_DEBUG_PRINTLN("LocationAdvertiser: DISTANCE TRIGGER - moved %.1f m", distance_moved);
            }
        }
    }

    if (!should_send) {
        MESH_DEBUG_PRINTLN("LocationAdvertiser: No trigger conditions met - NOT sending");
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
