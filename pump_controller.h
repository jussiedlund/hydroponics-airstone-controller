#pragma once
#include <cmath>
#include <cstdint>

namespace air_stone {
// No network or wall clock dependency. Unsigned elapsed time survives millis rollover.
class PumpController {
 public:
  bool running = false;
  bool fault = false;
  float resting_voltage = NAN;
  float loaded_min_voltage = NAN;
  float voltage_sag() const {
    if (!std::isfinite(loaded_min_voltage)) return NAN;
    return std::fmax(0.0f, resting_voltage - loaded_min_voltage);
  }

  // Called once per complete INA219 sample, never once per scheduler tick.
  void sample(uint32_t now, float voltage) {
    if (!std::isfinite(voltage) || voltage <= 0.0f || voltage > 4.35f) return;
    if (running && now - started_ >= 250U) {
      loaded_min_voltage = std::fmin(loaded_min_voltage, voltage);
    } else if (sampling_) {
      baseline_sum_ += voltage;
      ++baseline_count_;
    }
  }

  const char *status = "Waiting for INA219";

  bool pending() const { return manual_requested_ || sampling_; }

  void schedule_now() { has_stopped_ = false; }
  void request_run() { manual_requested_ = true; }
  void stop(uint32_t now) {
    running = false;
    manual_requested_ = false;
    sampling_ = false;
    last_stop_ = now;
    has_stopped_ = true;
  }
  // Cancel a queued start too, even if a second press arrives before tick().
  bool toggle(uint32_t now) {
    if (running || manual_requested_ || sampling_) {
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
            bool fresh, float voltage, float current, float cutoff, float max_current, float start_cutoff = 0.0f) {
    const bool healthy = fresh && std::isfinite(voltage) && voltage >= (running ? cutoff : std::fmax(cutoff, start_cutoff)) &&
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
      sampling_ = false;
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
      if (!sampling_) {
        sampling_ = true;
        baseline_started_ = now;
        baseline_count_ = 0;
        baseline_sum_ = 0.0f;
      }
      if (now - baseline_started_ < 3000U || baseline_count_ < 3) {
        status = "Measuring resting battery voltage";
        return false;
      }
      resting_voltage = baseline_sum_ / baseline_count_;
      loaded_min_voltage = NAN;
      sampling_ = false;
      manual_requested_ = false;
      running = true;
      started_ = now;
      // Absolute 30-minute software ceiling, including manual operation.
      duration_ms_ = run_ms > 1800000U ? 1800000U : run_ms;
      status = "Pump running";
    } else {
      sampling_ = false;
      status = automatic ? "Resting" : "Schedule disabled";
    }
    return running;
  }

 private:
  bool sampling_ = false;
  uint32_t baseline_started_ = 0;
  uint32_t baseline_count_ = 0;
  float baseline_sum_ = 0.0f;
  bool manual_requested_ = false;
  bool has_stopped_ = false;
  uint32_t started_ = 0;
  uint32_t last_stop_ = 0;
  uint32_t duration_ms_ = 0;
};
}  // namespace air_stone
