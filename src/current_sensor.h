#pragma once

#include <stdint.h>

/// @file current_sensor.h
/// @brief Non-blocking RMS current measurement through the board's ADS1115.

/// @brief Initialize the ADS1115 differential AIN0-AIN1 input at I2C address
///        @p i2c_address for a current-transformer probe.
/// @param i2c_address Seven-bit ADS1115 I2C address configured by the board wiring.
/// @param amps_per_volt Probe calibration in RMS amperes per RMS volt; must be positive.
/// @return True when the ADS1115 acknowledges initialization.
bool current_sensor_begin(uint8_t i2c_address, float amps_per_volt);

/// @brief Advance the asynchronous RMS sampling state machine.
/// @param now_ms Current `millis()` value used to schedule measurement windows.
/// @param current_a Receives the completed RMS current in amperes when true is returned.
/// @return True exactly once for each completed measurement window.
bool current_sensor_poll(unsigned long now_ms, float &current_a);
