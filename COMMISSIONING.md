# Commissioning — 7 November 2026

Initial installation over `/dev/tty.usbmodem1101`:

- USB identified ESP32-C6FH4 revision v0.2, 4 MB embedded flash.
- ESPHome 2026.9.1 / ESP-IDF 5.5.5 compiled and uploaded successfully; flash
  contents verified by esptool.
- Wi-Fi connected after initial authentication retries.
- Encrypted native API connection verified at `192.168.86.237` (DHCP address;
  use `air-stone-controller.local` or reserve the address in the router).
- Read back Automatic Schedule = OFF, Pump = OFF, run = 60 s, rest = 15 min.
- INA219 initialization failed; Battery Voltage and Battery Current are NaN,
  Power Monitor Healthy is false, and pump status is inhibited.
- Final diagnostic build: I2C bus recovery succeeded, but the scan found no
  devices on GPIO4/GPIO5. A brownout reset was observed during startup with
  the pump disabled; check USB/regulator/battery supply stability. Brownout
  protection remains enabled.
- No manual pump run or automatic cycle was commanded during commissioning.
- Home Assistant integration and history have not yet been verified.

Current wiring (moved after the initial scan): INA219 VCC → 3V3,
GND → ESP32 GND, SDA → GPIO6, SCL → GPIO7. Check these connections
and its address straps. VIN+/VIN- are the measured power path, not a substitute
for the sensor's VCC/GND connections. Reboot after correcting wiring: a sensor
marked failed during startup needs reinitialization. Startup logs include a
bus scan to distinguish an alternate address from no detected I2C devices.

Before enabling a schedule, verify the R100 shunt assumption, MOSFET position,
LOW = pump off, gate pulldown, and acceptable pump-start voltage/current and
MOSFET heating. Hardware pump behavior has not been exercised yet.

## GPIO6/GPIO7 follow-up — 7 November 2026, 14:20 SGT

The user moved SDA to GPIO6 and SCL to GPIO7. Firmware was updated, validated,
compiled and uploaded over USB. The scan now finds `0x40`, INA219 initializes
successfully, and the encrypted API reports Power Monitor Healthy = true.

A live snapshot with the pump and Automatic Schedule both OFF:

- Battery voltage: approximately 3.960 V.
- Signed battery current: approximately -0.118 A.
- Signed battery power: approximately -0.465 W.
- IP: 192.168.86.237.

Negative current can indicate charging or reversed VIN+/VIN- orientation; USB
was connected, so verify direction under a known battery-only discharge load
before interpreting discharge-only Energy Used. No pump run was commanded.
A startup brownout occurred again before Wi-Fi connected. Changing I2C pins
resolved detection but did not eliminate the observed supply/reset issue.

## Firmware 0.2.0 — brownout setting and GPIO0 button

At the owner's explicit request, disabled `CONFIG_ESP_BROWNOUT_DET` for this
module. Verified the generated sdkconfig has the option unset. This does not
fix supply instability; software battery/current checks remain enabled.

Added a normally-open GPIO0-to-GND tactile input with an internal pull-up and
40 ms debounce. Each press toggles a timed manual run, including cancellation
of a queued start. Holding the input does not repeat. Automatic Schedule stays
OFF; manual runs use the configured duration and existing safety checks.

Host tests and ESPHome validation passed; firmware compiled and was flashed
successfully. The encrypted API reports project version 0.2.0 and a released
Pump Button input. A controlled test temporarily selected a five-second run:
Pump changed OFF → ON → OFF, with exactly five seconds between the ON and OFF
state reports. No safety trip or API disconnect occurred during the test.
The original 60-second run duration was restored and confirmed via the API.

The sample remained about 3.964 V and -0.117 A throughout; there was no clear
battery-side load increase. GPIO output commands alone do not establish
USB-A output voltage or actual pump operation. Physical button/pump operation
and a USB-A voltage-under-load check are pending the owner's observations.

## Scheduled operation — 7 November 2026

The owner confirmed 5 V at the USB-A output after the physical test. The
controller rejoined Wi-Fi after a USB restart; the encrypted ESPHome API
reported a healthy INA219 and no safety trip. Enabled Automatic Schedule
with 60 seconds on and 15 minutes off. Verified the switch, both durations,
and the first scheduled pump-on state via the API. A later read confirmed
Pump = OFF, Pump Status = Resting, and no safety trip. The schedule runs
on the controller independently of Home Assistant.

Home Assistant discovered Air Stone Controller as an ESPHome device. The
integration setup reached the encryption-key prompt; the key is in the
local, untracked `secrets.yaml`. Keep it out of commissioning logs and Git.
