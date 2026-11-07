#include "../pump_controller.h"
#include <cassert>
#include <limits>
using air_stone::PumpController;
static bool tick(PumpController &c, uint32_t now, bool automatic = true,
                 bool fresh = true, float voltage = 3.9f, float current = 0.2f,
                 uint32_t duration = 1000) {
  return c.tick(now, automatic, duration, 2000, fresh, voltage, current, 3.3f, 2.5f);
}
int main() {
  PumpController c;
  assert(!tick(c, 0, false));  // Fresh install cannot start automatically.
  assert(!tick(c, 1, true, false));
  assert(tick(c, 2));
  assert(tick(c, 1001));
  assert(!tick(c, 1002));
  assert(!tick(c, 3001));
  assert(tick(c, 3002));  // Rest measured from stop, not previous start.
  assert(!tick(c, 3003, true, true, 3.2f));
  assert(c.fault);
  assert(!tick(c, 9999));  // Voltage recovery cannot silently restart the pump.
  c.request_run();
  assert(!tick(c, 10000, false));
  assert(!c.clear_fault(false, 10001));
  assert(c.clear_fault(true, 10002));
  c.request_run();
  assert(tick(c, 10003, false));
  assert(!tick(c, 10004, false, false));  // Lost sensor trips during manual run too.
  assert(c.fault);

  PumpController overcurrent;
  assert(tick(overcurrent, 0));
  assert(!tick(overcurrent, 1, true, true, 3.9f, 2.5f));
  assert(overcurrent.fault);
  PumpController invalid;
  invalid.request_run();
  assert(!tick(invalid, 0, false, true, std::numeric_limits<float>::quiet_NaN()));
  assert(!tick(invalid, 1, false));  // Rejected request cannot fire later.
  assert(!tick(invalid, 2, true, true, 5.0f));
  assert(!tick(invalid, 3, true, true, 3.9f, -2.6f));

  PumpController changed;
  assert(tick(changed, 0));
  assert(!tick(changed, 1000, true, true, 3.9f, 0.2f, 1800000));
  changed.request_run();
  assert(tick(changed, 1001, false));
  changed.stop(1002);
  assert(!tick(changed, 1003, false));

  PumpController rollover;
  assert(tick(rollover, UINT32_MAX - 500));
  assert(tick(rollover, 498));
  assert(!tick(rollover, 499));
  assert(!tick(rollover, 2498));
  assert(tick(rollover, 2499));

  PumpController enabled;
  enabled.stop(0);
  assert(!tick(enabled, 1));
  enabled.schedule_now();
  assert(tick(enabled, 2));

  PumpController physical;
  assert(physical.toggle(0));
  assert(!physical.toggle(1));  // Second press cancels a queued start.
  assert(!tick(physical, 2, false));
  assert(physical.toggle(3));
  assert(tick(physical, 4, false));
  assert(!physical.toggle(5));  // Second press stops a running pump.
  assert(!tick(physical, 6, false));
  assert(physical.toggle(7));
  assert(!tick(physical, 8, false, true, 3.2f)); // Physical input respects cutoff.
  assert(!tick(physical, 9, false));

  PumpController ceiling;
  assert(tick(ceiling, 0, false) == false);
  ceiling.request_run();
  assert(tick(ceiling, 1, false, true, 3.9f, 0.2f, UINT32_MAX));
  assert(!tick(ceiling, 1800001, false));
}
