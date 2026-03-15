#pragma once

#include <stdint.h>

#ifndef LPP_SPEED
#define LPP_SPEED 129      // 2 bytes, 0.01m/s unsigned
#endif

#ifndef LPP_SPEED_SIZE
#define LPP_SPEED_SIZE 2
#endif

#ifndef LPP_SPEED_MULT
#define LPP_SPEED_MULT 100
#endif

#ifndef LPP_GUST
#define LPP_GUST 137       // 2 bytes, 0.01m/s unsigned
#endif

#ifndef LPP_GUST_SIZE
#define LPP_GUST_SIZE 2
#endif

#ifndef LPP_GUST_MULT
#define LPP_GUST_MULT 100
#endif

#ifndef LPP_DEWPOINT
#define LPP_DEWPOINT 138   // 2 bytes, 0.1C signed
#endif

#ifndef LPP_DEWPOINT_SIZE
#define LPP_DEWPOINT_SIZE 2
#endif

#ifndef LPP_DEWPOINT_MULT
#define LPP_DEWPOINT_MULT 10
#endif

#ifndef LPP_RAIN
#define LPP_RAIN 139       // 2 bytes, 0.1mm unsigned
#endif

#ifndef LPP_RAIN_SIZE
#define LPP_RAIN_SIZE 2
#endif

#ifndef LPP_RAIN_MULT
#define LPP_RAIN_MULT 10
#endif

#ifndef LPP_BINARY_BOOL
#define LPP_BINARY_BOOL 143
#endif

#ifndef LPP_BINARY_POWER_SWITCH
#define LPP_BINARY_POWER_SWITCH 144
#endif

#ifndef LPP_BINARY_OPEN
#define LPP_BINARY_OPEN 145
#endif

#ifndef LPP_BINARY_BATTERY_LOW
#define LPP_BINARY_BATTERY_LOW 146
#endif

#ifndef LPP_BINARY_CHARGING
#define LPP_BINARY_CHARGING 147
#endif

#ifndef LPP_BINARY_CARBON_MONOXIDE
#define LPP_BINARY_CARBON_MONOXIDE 148
#endif

#ifndef LPP_BINARY_COLD
#define LPP_BINARY_COLD 149
#endif

#ifndef LPP_BINARY_CONNECTIVITY
#define LPP_BINARY_CONNECTIVITY 150
#endif

#ifndef LPP_BINARY_DOOR
#define LPP_BINARY_DOOR 151
#endif

#ifndef LPP_BINARY_GARAGE_DOOR
#define LPP_BINARY_GARAGE_DOOR 152
#endif

#ifndef LPP_BINARY_GAS
#define LPP_BINARY_GAS 153
#endif

#ifndef LPP_BINARY_HEAT
#define LPP_BINARY_HEAT 154
#endif

#ifndef LPP_BINARY_LIGHT
#define LPP_BINARY_LIGHT 155
#endif

#ifndef LPP_BINARY_LOCK
#define LPP_BINARY_LOCK 156
#endif

#ifndef LPP_BINARY_MOISTURE
#define LPP_BINARY_MOISTURE 157
#endif

#ifndef LPP_BINARY_MOTION
#define LPP_BINARY_MOTION 158
#endif

#ifndef LPP_BINARY_MOVING
#define LPP_BINARY_MOVING 159
#endif

#ifndef LPP_BINARY_OCCUPANCY
#define LPP_BINARY_OCCUPANCY 160
#endif

#ifndef LPP_BINARY_PLUG
#define LPP_BINARY_PLUG 161
#endif

#ifndef LPP_BINARY_PRESENCE
#define LPP_BINARY_PRESENCE 162
#endif

#ifndef LPP_BINARY_PROBLEM
#define LPP_BINARY_PROBLEM 163
#endif

#ifndef LPP_BINARY_RUNNING
#define LPP_BINARY_RUNNING 164
#endif

#ifndef LPP_BINARY_SAFETY
#define LPP_BINARY_SAFETY 165
#endif

#ifndef LPP_BINARY_SMOKE
#define LPP_BINARY_SMOKE 166
#endif

#ifndef LPP_BINARY_SOUND
#define LPP_BINARY_SOUND 167
#endif

#ifndef LPP_BINARY_TAMPER
#define LPP_BINARY_TAMPER 168
#endif

#ifndef LPP_BINARY_VIBRATION
#define LPP_BINARY_VIBRATION 169
#endif

#ifndef LPP_BINARY_WINDOW
#define LPP_BINARY_WINDOW 170
#endif

#ifndef LPP_BUTTON_EVENT
#define LPP_BUTTON_EVENT 171
#endif

#ifndef LPP_DIMMER
#define LPP_DIMMER 172
#endif

#ifndef LPP_UV
#define LPP_UV 173
#endif

#ifndef LPP_LIGHT_LEVEL
#define LPP_LIGHT_LEVEL 174
#endif

#ifndef LPP_PM25
#define LPP_PM25 175
#endif

#ifndef LPP_PM10
#define LPP_PM10 176
#endif

#ifndef LPP_CO2
#define LPP_CO2 177
#endif

#ifndef LPP_TVOC
#define LPP_TVOC 178
#endif

#ifndef LPP_RPM
#define LPP_RPM 179
#endif

#ifndef LPP_CONDUCTIVITY
#define LPP_CONDUCTIVITY 180
#endif

#ifndef LPP_ROTATION
#define LPP_ROTATION 181
#endif

#ifndef LPP_DURATION
#define LPP_DURATION 182
#endif

#ifndef LPP_ACCELERATION
#define LPP_ACCELERATION 183
#endif

#ifndef LPP_GYRO_RATE
#define LPP_GYRO_RATE 184
#endif

#ifndef LPP_VOLUME
#define LPP_VOLUME 185
#endif

#ifndef LPP_FLOW_RATE
#define LPP_FLOW_RATE 186
#endif

#ifndef LPP_VOLUME_STORAGE
#define LPP_VOLUME_STORAGE 187
#endif

#ifndef LPP_WATER
#define LPP_WATER 188
#endif

#ifndef LPP_GAS_VOLUME
#define LPP_GAS_VOLUME 189
#endif

#ifndef LPP_MASS
#define LPP_MASS 190
#endif

#ifndef LPP_SIGNED_SPEED
#define LPP_SIGNED_SPEED 191
#endif

#ifndef LPP_SIGNED_POWER
#define LPP_SIGNED_POWER 192
#endif

#ifndef LPP_SIGNED_CURRENT
#define LPP_SIGNED_CURRENT 193
#endif

#ifndef LPP_BINARY_FIRST
#define LPP_BINARY_FIRST LPP_BINARY_BOOL
#endif

#ifndef LPP_BINARY_LAST
#define LPP_BINARY_LAST LPP_BINARY_WINDOW
#endif

#ifndef LPP_BINARY_SIZE
#define LPP_BINARY_SIZE 1
#endif

#ifndef LPP_BUTTON_EVENT_SIZE
#define LPP_BUTTON_EVENT_SIZE 1
#endif

#ifndef LPP_DIMMER_SIZE
#define LPP_DIMMER_SIZE 1
#endif

#ifndef LPP_UV_SIZE
#define LPP_UV_SIZE 1
#endif

#ifndef LPP_LIGHT_LEVEL_SIZE
#define LPP_LIGHT_LEVEL_SIZE 1
#endif

#ifndef LPP_PM25_SIZE
#define LPP_PM25_SIZE 2
#endif

#ifndef LPP_PM10_SIZE
#define LPP_PM10_SIZE 2
#endif

#ifndef LPP_CO2_SIZE
#define LPP_CO2_SIZE 2
#endif

#ifndef LPP_TVOC_SIZE
#define LPP_TVOC_SIZE 2
#endif

#ifndef LPP_RPM_SIZE
#define LPP_RPM_SIZE 2
#endif

#ifndef LPP_CONDUCTIVITY_SIZE
#define LPP_CONDUCTIVITY_SIZE 2
#endif

#ifndef LPP_ROTATION_SIZE
#define LPP_ROTATION_SIZE 2
#endif

#ifndef LPP_DURATION_SIZE
#define LPP_DURATION_SIZE 4
#endif

#ifndef LPP_ACCELERATION_SIZE
#define LPP_ACCELERATION_SIZE 4
#endif

#ifndef LPP_GYRO_RATE_SIZE
#define LPP_GYRO_RATE_SIZE 4
#endif

#ifndef LPP_VOLUME_SIZE
#define LPP_VOLUME_SIZE 4
#endif

#ifndef LPP_FLOW_RATE_SIZE
#define LPP_FLOW_RATE_SIZE 4
#endif

#ifndef LPP_VOLUME_STORAGE_SIZE
#define LPP_VOLUME_STORAGE_SIZE 4
#endif

#ifndef LPP_WATER_SIZE
#define LPP_WATER_SIZE 4
#endif

#ifndef LPP_GAS_VOLUME_SIZE
#define LPP_GAS_VOLUME_SIZE 4
#endif

#ifndef LPP_MASS_SIZE
#define LPP_MASS_SIZE 4
#endif

#ifndef LPP_SIGNED_SPEED_SIZE
#define LPP_SIGNED_SPEED_SIZE 4
#endif

#ifndef LPP_SIGNED_POWER_SIZE
#define LPP_SIGNED_POWER_SIZE 4
#endif

#ifndef LPP_SIGNED_CURRENT_SIZE
#define LPP_SIGNED_CURRENT_SIZE 4
#endif

#ifndef LPP_BUTTON_EVENT_MULT
#define LPP_BUTTON_EVENT_MULT 1
#endif

#ifndef LPP_DIMMER_MULT
#define LPP_DIMMER_MULT 1
#endif

#ifndef LPP_UV_MULT
#define LPP_UV_MULT 10
#endif

#ifndef LPP_LIGHT_LEVEL_MULT
#define LPP_LIGHT_LEVEL_MULT 1
#endif

#ifndef LPP_PM25_MULT
#define LPP_PM25_MULT 1
#endif

#ifndef LPP_PM10_MULT
#define LPP_PM10_MULT 1
#endif

#ifndef LPP_CO2_MULT
#define LPP_CO2_MULT 1
#endif

#ifndef LPP_TVOC_MULT
#define LPP_TVOC_MULT 1
#endif

#ifndef LPP_RPM_MULT
#define LPP_RPM_MULT 1
#endif

#ifndef LPP_CONDUCTIVITY_MULT
#define LPP_CONDUCTIVITY_MULT 1
#endif

#ifndef LPP_ROTATION_MULT
#define LPP_ROTATION_MULT 10
#endif

#ifndef LPP_DURATION_MULT
#define LPP_DURATION_MULT 1000
#endif

#ifndef LPP_ACCELERATION_MULT
#define LPP_ACCELERATION_MULT 1000000
#endif

#ifndef LPP_GYRO_RATE_MULT
#define LPP_GYRO_RATE_MULT 1000
#endif

#ifndef LPP_VOLUME_MULT
#define LPP_VOLUME_MULT 1000
#endif

#ifndef LPP_FLOW_RATE_MULT
#define LPP_FLOW_RATE_MULT 1000
#endif

#ifndef LPP_VOLUME_STORAGE_MULT
#define LPP_VOLUME_STORAGE_MULT 1000
#endif

#ifndef LPP_WATER_MULT
#define LPP_WATER_MULT 1000
#endif

#ifndef LPP_GAS_VOLUME_MULT
#define LPP_GAS_VOLUME_MULT 1000
#endif

#ifndef LPP_MASS_MULT
#define LPP_MASS_MULT 1000
#endif

#ifndef LPP_SIGNED_SPEED_MULT
#define LPP_SIGNED_SPEED_MULT 1000000
#endif

#ifndef LPP_SIGNED_POWER_MULT
#define LPP_SIGNED_POWER_MULT 100
#endif

#ifndef LPP_SIGNED_CURRENT_MULT
#define LPP_SIGNED_CURRENT_MULT 1000
#endif

static inline bool isMeshBinaryType(uint8_t type) {
  return type >= LPP_BINARY_FIRST && type <= LPP_BINARY_LAST;
}

static inline bool isMeshCustom1ByteType(uint8_t type) {
  return isMeshBinaryType(type) || type == LPP_BUTTON_EVENT || type == LPP_DIMMER || type == LPP_UV ||
         type == LPP_LIGHT_LEVEL;
}

static inline bool isMeshCustom2ByteType(uint8_t type) {
  return type == LPP_PM25 || type == LPP_PM10 || type == LPP_CO2 || type == LPP_TVOC || type == LPP_RPM ||
         type == LPP_CONDUCTIVITY || type == LPP_ROTATION || type == LPP_DEWPOINT || type == LPP_RAIN;
}

static inline bool isMeshCustom4ByteType(uint8_t type) {
  return type == LPP_DURATION || type == LPP_ACCELERATION || type == LPP_GYRO_RATE || type == LPP_VOLUME ||
         type == LPP_FLOW_RATE || type == LPP_VOLUME_STORAGE || type == LPP_WATER || type == LPP_GAS_VOLUME ||
         type == LPP_MASS || type == LPP_SIGNED_SPEED || type == LPP_SIGNED_POWER || type == LPP_SIGNED_CURRENT;
}
