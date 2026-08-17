#include "current_sensor.h"

#include <ADS1X15.h>
#include <Arduino.h>

#include <math.h>

namespace
{
constexpr unsigned long SAMPLE_INTERVAL_MS = 10000;
constexpr unsigned long RMS_WINDOW_MS = 250;
constexpr unsigned long CONVERSION_TIME_US = 1200;
constexpr uint32_t MINIMUM_RMS_SAMPLES = 20;

ADS1115 *adc = nullptr;
float probe_amps_per_volt = 0.0f;
bool available = false;
bool collecting = false;
unsigned long last_sample_ms = 0;
unsigned long window_started_ms = 0;
unsigned long next_conversion_check_us = 0;
double voltage_sum = 0.0;
double voltage_square_sum = 0.0;
uint32_t sample_count = 0;

/// @brief Start one asynchronous differential conversion on AIN0-AIN1.
static void request_conversion()
{
  adc->requestADC_Differential_0_1();
  next_conversion_check_us = micros() + CONVERSION_TIME_US;
}

/// @brief Start a fresh RMS accumulation window.
/// @param now_ms Current `millis()` value.
static void start_window(unsigned long now_ms)
{
  collecting = true;
  window_started_ms = now_ms;
  voltage_sum = 0.0;
  voltage_square_sum = 0.0;
  sample_count = 0;
  request_conversion();
}
} // namespace

bool current_sensor_begin(uint8_t i2c_address, float amps_per_volt)
{
  if (amps_per_volt <= 0.0f)
    return false;

  static ADS1115 configured_adc(i2c_address);
  adc = &configured_adc;
  probe_amps_per_volt = amps_per_volt;
  available = adc->begin();
  if (!available)
    return false;

  adc->setGain(ADS1X15_GAIN_2048MV);
  adc->setDataRate(ADS1115_860_SPS);
  adc->setMode(ADS1X15_MODE_SINGLE);
  last_sample_ms = millis() - SAMPLE_INTERVAL_MS;
  return true;
}

bool current_sensor_poll(unsigned long now_ms, float &current_a)
{
  if (!available)
    return false;

  if (!collecting)
  {
    if (now_ms - last_sample_ms < SAMPLE_INTERVAL_MS)
      return false;
    start_window(now_ms);
    return false;
  }

  if ((int32_t)(micros() - next_conversion_check_us) < 0 || !adc->isReady())
    return false;

  float voltage = adc->toVoltage(adc->getValue());
  voltage_sum += voltage;
  voltage_square_sum += (double)voltage * voltage;
  sample_count++;

  if (now_ms - window_started_ms < RMS_WINDOW_MS || sample_count < MINIMUM_RMS_SAMPLES)
  {
    request_conversion();
    return false;
  }

  double mean = voltage_sum / sample_count;
  double variance = voltage_square_sum / sample_count - mean * mean;
  if (variance < 0.0)
    variance = 0.0;
  current_a = (float)(sqrt(variance) * probe_amps_per_volt);
  collecting = false;
  last_sample_ms = now_ms;
  return true;
}
