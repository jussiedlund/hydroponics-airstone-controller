#pragma once
#include <cmath>
#include <cstdint>

namespace air_stone {
constexpr float SOLAR_MIN_VOLTAGE = 3.50f;
constexpr float SOLAR_DAY_VOLTAGE = 3.70f;

struct SolarSleep {
  uint32_t minutes;
  const char *reason;
};

inline SolarSleep select_sleep(float resting_voltage, bool fault, int local_hour,
                               uint32_t day_minutes) {
  if (fault) return {60, "Safety trip"};
  if (!std::isfinite(resting_voltage) || resting_voltage <= 0.0f || resting_voltage > 4.35f)
    return {60, "No valid battery reading"};
  if (resting_voltage < SOLAR_MIN_VOLTAGE) return {60, "Low battery"};
  const auto extended = day_minutes > 30 ? day_minutes : 30;
  if (resting_voltage < SOLAR_DAY_VOLTAGE) return {extended, "Battery reserve"};
  if (local_hour < 0) return {extended, "Clock unavailable"};
  if (local_hour < 7 || local_hour >= 19) return {extended, "Night"};
  return {day_minutes, "Day"};
}
}  // namespace air_stone
