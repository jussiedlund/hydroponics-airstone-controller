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

Check INA219 VCC → 3V3, GND → ESP32 GND, SDA → GPIO4, SCL → GPIO5,
and its address straps. VIN+/VIN- are the measured power path, not a substitute
for the sensor's VCC/GND connections. Reboot after correcting wiring: a sensor
marked failed during startup needs reinitialization. Startup logs include a
bus scan to distinguish an alternate address from no detected I2C devices.

Before enabling a schedule, verify the R100 shunt assumption, MOSFET position,
LOW = pump off, gate pulldown, and acceptable pump-start voltage/current and
MOSFET heating. Hardware pump behavior has not been exercised yet.
