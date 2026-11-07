#include "../pump_controller.h"
#include <cassert>
#include <limits>

using air_stone::PumpController;
static bool tick(PumpController &c, uint32_t now, bool automatic = true,
                 bool fresh = true, float voltage = 3.9f, float current = 0.2f,
                 uint32_t duration = 1000, uint32_t rest = 2000) {
  return c.tick(now, automatic, duration, rest, fresh, voltage, current, 3.3f, 2.5f);
}

static void start(PumpController &c, uint32_t now, bool automatic = true,
                  uint32_t duration = 1000) {
  assert(!tick(c, now, automatic));
  c.sample(now + 100, 4.00f);
  c.sample(now + 1100, 3.99f);
  c.sample(now + 2100, 3.98f);
  assert(!tick(c, now + 2999, automatic));
  assert(tick(c, now + 3000, automatic, true, 3.98f, 0.2f, duration));
}

int main() {
  PumpController c;
  assert(!tick(c, 0, false));
  assert(!tick(c, 1, true, false));
  start(c, 2);
  assert(std::fabs(c.resting_voltage - 3.99f) < 0.0001f);
  c.sample(3010, 4.02f);  // Conversion may predate GPIO assertion.
  assert(std::isnan(c.loaded_min_voltage));
  c.sample(3302, 3.78f);
  c.sample(3602, 3.85f);
  assert(std::fabs(c.voltage_sag() - 0.21f) < 0.0001f);
  assert(tick(c, 4001));
  assert(!tick(c, 4002));
  c.sample(4202, 3.60f);  // Recovery does not change loaded minimum.
  assert(std::fabs(c.loaded_min_voltage - 3.78f) < 0.0001f);
  assert(!tick(c, 6001));  // Rest measured from stop.
  start(c, 6002);
  c.sample(9302, 3.2f);
  assert(!tick(c, 9302, true, true, 3.2f));
  assert(c.fault);
  assert(c.loaded_min_voltage == 3.2f);
  assert(!tick(c, 9999));
  c.request_run();
  assert(!tick(c, 10000, false));
  assert(!c.clear_fault(false, 10001));
  assert(c.clear_fault(true, 10002));
  c.request_run();
  start(c, 10003, false);
  assert(!tick(c, 13004, false, false));  // Lost monitor trips manual runs.
  assert(c.fault);

  PumpController samples;
  assert(!tick(samples, 0));
  samples.sample(1000, 4.0f);
  for (uint32_t i = 1250; i <= 3500; i += 250) assert(!tick(samples, i));
  samples.sample(3600, 4.0f);
  samples.sample(4600, 4.0f);
  assert(tick(samples, 4600));  // Three distinct conversions, not repeated ticks.

  PumpController physical;
  assert(physical.toggle(0));
  assert(!tick(physical, 0, false));
  assert(!physical.toggle(1));  // Cancels pre-start sampling.
  assert(!tick(physical, 3001, false));
  assert(physical.toggle(3002));
  start(physical, 3002, false);
  assert(!physical.toggle(6003));
  assert(!tick(physical, 6004, false));
  assert(physical.toggle(6005));
  assert(!tick(physical, 6006, false, true, 3.2f));
  assert(!tick(physical, 9006, false));  // Rejected request does not fire later.

  PumpController interrupted;
  interrupted.request_run();
  assert(!tick(interrupted, 0, false));
  interrupted.sample(1000, 4.0f);
  assert(!tick(interrupted, 1100, false, false));
  assert(!tick(interrupted, 4000, false));

  PumpController overcurrent;
  start(overcurrent, 0);
  assert(!tick(overcurrent, 3001, true, true, 3.9f, 2.5f));
  assert(overcurrent.fault);
  PumpController invalid;
  invalid.request_run();
  assert(!tick(invalid, 0, false, true, std::numeric_limits<float>::quiet_NaN()));
  assert(!tick(invalid, 1, false));
  assert(!tick(invalid, 2, true, true, 5.0f));
  assert(!tick(invalid, 3, true, true, 3.9f, -2.6f));

  PumpController changed;
  start(changed, 0);
  assert(!tick(changed, 4000, true, true, 3.9f, 0.2f, 1800000));

  PumpController rollover;
  start(rollover, UINT32_MAX - 2000);
  assert(tick(rollover, 1998));
  assert(!tick(rollover, 1999));
  assert(!tick(rollover, 3998));
  start(rollover, 3999);

  PumpController enabled;
  enabled.stop(0);
  assert(!tick(enabled, 1));
  enabled.schedule_now();
  start(enabled, 2);

  PumpController ceiling;
  ceiling.request_run();
  start(ceiling, 1, false, UINT32_MAX);
  assert(!tick(ceiling, 1803001, false));
}
