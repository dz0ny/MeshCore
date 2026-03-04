#ifndef LOCATION_ADVERTISER_H
#define LOCATION_ADVERTISER_H

#include <Arduino.h>

/**
 * LocationAdvertiser manages GPS-based location advertisement triggering
 * based on configurable parameters:
 * - Distance moved threshold (1-20m)
 * - Frequency limiter (30s-5min minimum between updates)
 * - Guaranteed interval timer (1/5/15min, skips if recently sent)
 */
class LocationAdvertiser {
  private:
    // Last advertised position
    double last_advert_lat;
    double last_advert_lon;
    bool has_last_position;

    // Timing tracking
    unsigned long last_advert_time;
    unsigned long last_guaranteed_check;

    // Configuration (pointers to NodePrefs values)
    uint8_t* distance_threshold;    // meters (1-20)
    uint8_t* frequency;              // seconds/10 (30s-300s stored as 3-30)
    uint8_t* guaranteed_interval;    // 0=1min, 1=5min, 2=15min
    uint8_t* accuracy_threshold;     // meters (3-20, GPS accuracy must be better than this)
    uint8_t* enabled;                // 0=off, 1=on

    /**
     * Calculate distance between two GPS coordinates using Haversine formula
     * @param lat1 Latitude of first point (degrees)
     * @param lon1 Longitude of first point (degrees)
     * @param lat2 Latitude of second point (degrees)
     * @param lon2 Longitude of second point (degrees)
     * @return Distance in meters
     */
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);

    /**
     * Get guaranteed interval in milliseconds based on config
     * @return Interval in milliseconds
     */
    unsigned long getGuaranteedIntervalMs();

  public:
    LocationAdvertiser();

    /**
     * Initialize with pointers to configuration values
     * @param dist_thresh Pointer to distance threshold config
     * @param freq Pointer to frequency config
     * @param guaranteed Pointer to guaranteed interval config
     * @param accuracy_thresh Pointer to accuracy threshold config
     * @param en Pointer to enabled flag
     */
    void init(uint8_t* dist_thresh, uint8_t* freq, uint8_t* guaranteed, uint8_t* accuracy_thresh, uint8_t* en);

    /**
     * Check if location should be advertised based on current position
     * @param lat Current latitude
     * @param lon Current longitude
     * @param accuracy GPS accuracy in meters (0 if not available)
     * @return true if advertisement should be sent
     */
    bool shouldAdvertise(double lat, double lon, double accuracy = 0.0);

    /**
     * Record that an advertisement was sent with given position
     * Updates tracking state
     * @param lat Advertised latitude
     * @param lon Advertised longitude
     */
    void recordAdvertisement(double lat, double lon);

    /**
     * Reset all tracking state (useful when GPS is re-enabled)
     */
    void reset();

    /**
     * Get last advertised position
     * @param lat Output parameter for latitude
     * @param lon Output parameter for longitude
     * @return true if last position is valid
     */
    bool getLastAdvertisedPosition(double& lat, double& lon);
};

#endif // LOCATION_ADVERTISER_H
