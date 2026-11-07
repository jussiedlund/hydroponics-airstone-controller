#pragma once
#include <cmath>
#include <cstdint>

namespace air_stone {
// No network or wall clock dependency. Unsigned elapsed time survives millis rollover.
class PumpController {
 public:
  bool running = false;
  bool fault = false;
  const char *status = "Waiting for INA219";

  void schedule_now() { has_stopped_ = false; }
  void request_run() { manual_requested_ = true; }
  void stop(uint32_t now) {
    running = false;
    manual_requested_ = false;
    last_stop_ = now;
    has_stopped_ = true;
  }
  // Cancel a queued start too, even if a second press arrives before tick().
  bool toggle(uint32_t now) {
    if (running || manual_requested_) {
      stop(now);
      return false;
    }
    request_run();
    return true;
  }
  bool clear_fault(bool healthy, uint32_t now) {
    if (!healthy) return false;
    fault = false;
    stop(now);
    return true;
  }
  bool tick(uint32_t now, bool automatic, uint32_t run_ms, uint32_t rest_ms,
            bool fresh, float voltage, float current, float cutoff, float max_current) {
    const bool healthy = fresh && std::isfinite(voltage) && voltage >= cutoff &&
                         voltage <= 4.35f && std::isfinite(current) &&
                         std::fabs(current) < max_current;
    if (running && !healthy) {
      stop(now);
      fault = true;
    }
    if (fault) {
      manual_requested_ = false;
      status = "Safety trip - inspect battery / wiring, then clear";
      return false;
    }
    if (!healthy) {
      manual_requested_ = false;
      status = "Inhibited - low battery / invalid or stale INA219 / current limit";
      return false;
    }
    if (running) {
      // Snapshot the duration on start; later edits apply to the next run.
      if (now - started_ >= duration_ms_) {
        stop(now);
        status = "Resting";
      } else {
        status = "Pump running";
      }
      return running;
    }
    const bool due = automatic && (!has_stopped_ || now - last_stop_ >= rest_ms);
    if (manual_requested_ || due) {
      manual_requested_ = false;
      running = true;
      started_ = now;
      // Absolute 30-minute software ceiling, including manual operation.
      duration_ms_ = run_ms > 1800000U ? 1800000U : run_ms;
      status = "Pump running";
    } else {
      status = automatic ? "Resting" : "Schedule disabled";
    }
    return running;
  }

 private:
  bool manual_requested_ = false;
  bool has_stopped_ = false;
  uint32_t started_ = 0;
  uint32_t last_stop_ = 0;
  uint32_t duration_ms_ = 0;
};
}  // namespace air_stone
