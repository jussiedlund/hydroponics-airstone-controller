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
