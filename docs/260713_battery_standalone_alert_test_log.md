# 2026-07-13 Battery Standalone Alert Test Log

Project: Heat Risk Alert Wristband  
Purpose: Verify the prototype can run the alert flow from Li-Po battery power, including sensor measurement, SIM7080G boot/control, GNSS acquisition, and SMS transmission.

## 1. Scope

This log records the work performed after the earlier LTE/SMS integration tests.

Main goals:

- Confirm ESP32-C3 SuperMini can operate from external power while USB debug remains usable.
- Confirm SIM7080G can boot from Li-Po/battery-side power.
- Confirm SIM7080G LTE + GNSS + SMS works while powered from the battery path.
- Confirm the full prototype can run from battery power without relying on the laptop for operation.
- Add a standalone trigger flow with buzzer feedback for field demonstration.
- Re-check MAX30102 heart-rate and TMP117 temperature data after the standalone firmware changes.

## 2. Hardware Used

### MCU

ESP32-C3 SuperMini.

Observed / used points:

- USB-C used for firmware upload and serial debug.
- `5V` pin was used as the external input during full battery testing.
- `3V3` pin output was measured at approximately 3.3 V when the Li-Po battery was connected to the `5V` pin.
- The ESP32-C3 SuperMini was not planned to be integrated as a bare MCU on the first PCB. For the first prototype, it remains a mounted development board/module.

### Cellular / GNSS

SIM7080G developer board/module.

Used points:

- `VDD` and `GND` for module power during battery tests.
- UART connected to ESP32-C3 SuperMini.
- `PWRK` connected to ESP32-C3 GPIO5.
- LTE/GNSS antennas connected before GNSS/SMS tests.

Important observed behavior:

- The module did not always boot immediately after power application.
- User confirmed that applying the PWRK signal for about 1 second is enough to start the module.
- Practical boot time after PWRK was observed at about 7 seconds.

### Sensors

TMP117 precision temperature sensor:

- I2C address used: `0x48`.
- Connected to the shared I2C bus.
- Verified temperature read after firmware changes.

MAX30102 heart-rate / PPG sensor:

- I2C address used: `0x57`.
- Connected to the shared I2C bus.
- Finger placement test performed during `MEASURE`.
- Verified heart-rate estimation after firmware changes.

### Buzzer

3 V active buzzer.

- Connected to ESP32-C3 GPIO10.
- Used for standalone demo feedback.
- The buzzer initially stayed active in some error states; firmware was changed so the normal state-machine buzzer is disabled for the standalone demo build, and only explicit event beeps are used.

### Battery

Li-Po TW-602035 battery.

Known product data:

- Nominal voltage: 3.7 V
- Capacity: 380 mAh
- Energy marking: 1.406 Wh
- Approximate size: 20 mm x 35 mm x 6 mm
- Connector: 1.25 mm 2-pin type

Measured before battery tests:

```text
Battery voltage: about 3.989 V
After SIM7080G boot: about 3.97 V
```

## 3. Final Working Pin Map

Firmware environment: `esp32c3_supermini`

```text
I2C SDA: ESP32-C3 GPIO8
I2C SCL: ESP32-C3 GPIO9

SIM7080G UART:
  ESP32-C3 RX = GPIO20
  ESP32-C3 TX = GPIO21

Buzzer:
  ESP32-C3 GPIO10

SIM7080G PWRK:
  ESP32-C3 GPIO5

Standalone trigger:
  ESP32-C3 GPIO4
  Active-low trigger to GND
```

Important UART note:

- The firmware uses ESP RX = GPIO20 and ESP TX = GPIO21.
- The physical UART wiring only worked after swapping the two signal wires compared with the earlier non-working wiring.
- Before the swap, measured idle voltages were:

```text
SIM7080G TX: about 3.25 V
SIM7080G RX: about 3.07 V
ESP GPIO20: about 3.1 V
ESP GPIO21: about 3.265 V
```

Conclusion: preserve the currently working physical UART wiring. Do not rely only on the printed TX/RX labels without validating AT response.

## 4. Test 3: ESP32-C3 External 3.3 V + USB Debug

Purpose:

- Check whether the ESP32-C3 SuperMini can be externally powered while USB remains connected for debug/upload.

Setup:

```text
PSU connected to ESP32-C3 3V3 and GND
ESP32-C3 USB connected to laptop
Other wiring kept in place
```

Observed:

```text
Current draw: about 0.02 A to 0.03 A
USB debug/upload remained available
No immediate conflict observed
```

Result:

```text
PASS
```

Interpretation:

- External 3.3 V with USB debug did not immediately cause an operational issue.
- This was useful for bench testing, but not the final battery architecture.
- For full battery operation, the test moved to Li-Po input through the ESP32-C3 `5V` pin.

## 5. SIM7080G Battery Power Test

Purpose:

- Confirm SIM7080G can boot and perform LTE/GNSS/SMS when powered from the Li-Po/battery path.

Setup:

```text
ESP32-C3: USB connected for debug
Sensors: powered from ESP32-C3 3V3
SIM7080G: Li-Po connected to VDD and GND
All grounds common
```

Observed:

```text
Battery before/around test: about 3.989 V
After SIM7080G green LED came on: about 3.97 V
Green LED: on/blinking after PWRK action
```

The user confirmed:

```text
SMS received
Voltage drop was negligible for this test
```

Result:

```text
PASS
```

Interpretation:

- SIM7080G was able to boot from the battery-side power path.
- LTE/GNSS/SMS operation did not create an obvious voltage-collapse problem in this test.
- The first PCB still needs proper power design, because the dev board's `VDD` behavior is not identical to a final bare-module `VBAT` design.

## 6. Full Battery System Test

Purpose:

- Confirm the full prototype can run without laptop power.

Setup:

```text
Li-Po positive -> ESP32-C3 5V pin
Li-Po positive -> SIM7080G VDD
Li-Po negative -> common GND

ESP32-C3 3V3 -> TMP117 VIN
ESP32-C3 3V3 -> MAX30102 VIN
ESP32-C3 3V3 -> buzzer supply/control path
All grounds common
```

Additional observation:

```text
ESP32-C3 3V3 pin measured about 3.3 V while Li-Po was connected to ESP32-C3 5V pin.
```

User-confirmed result:

```text
SMS received
No meaningful voltage problem observed
```

Result:

```text
PASS for prototype demonstration
```

Engineering caution:

- Li-Po must not be connected directly to ESP32-C3 `3V3`.
- The tested path was Li-Po to the ESP32-C3 `5V` pin, then sensors from ESP32-C3 `3V3`.
- For the PCB, the power path should be explicitly designed instead of assuming the development board's regulator behavior.

## 7. Standalone Demo Flow

The standalone test mode was added so the device can be operated without serial commands.

Trigger:

```text
GPIO4 held LOW to GND for about 1.5 seconds
```

Standalone operation:

```text
1. Start beep
2. Wait about 5 seconds for finger/sensor placement
3. Measurement begins
4. Measurement complete -> two beeps
5. SIM7080G AT check
6. If needed, PWRK HIGH for 1000 ms
7. Wait about 7 seconds for SIM7080G boot
8. LTE registration
9. GNSS acquisition
10. SMS send
11. SMS success -> three beeps
```

Buzzer result codes:

```text
1 beep  : standalone test start
2 beeps : sensor measurement complete
3 beeps : SMS success
4 beeps : modem AT response failure
5 beeps : LTE registration failure
6 beeps : SMS send failure
```

Issue observed:

- The buzzer kept sounding in an earlier build.
- Measurement also felt too long because completion feedback was not heard.

Firmware changes made:

- Heart-rate measurement duration set to 5 seconds.
- Normal state-machine buzzer disabled for the demo build with `ENABLE_STATE_BUZZER=0`.
- Buzzer now mainly provides explicit event/result codes.
- SIM7080G PWRK timing changed to match observed behavior:

```text
PWRK HIGH: 1000 ms
Modem boot wait: 7000 ms
```

## 8. SIM7080G Boot / UART Issue

Problem:

- Standalone test sometimes failed with 4 beeps, meaning modem AT response failure.
- User confirmed SIM7080G could physically boot, but firmware could not always get AT response.

Actions:

- Rechecked UART pin definitions.
- Rechecked physical TX/RX wiring.
- Used LTE probe behavior to test the UART response.
- Changed standalone boot logic so it does not wait unnecessarily before applying PWRK.

Final behavior:

- SIM7080G boot takes about 7 seconds after PWRK.
- After the UART wiring correction and PWRK timing change, standalone eventually produced 3 beeps, meaning SMS success.

Result:

```text
PASS after UART/PWRK corrections
```

## 9. Sensor Measurement Recheck

Problem:

- After the standalone SMS path worked, the user noticed heart-rate and temperature were not visible in the received message.
- Sensor function was rechecked directly with the user placing a finger on the MAX30102.

Command/test:

```text
MEASURE
```

Observed successful measurement:

```text
MAX30102:
  samples = 94
  peaks = 6
  bpm = 76.6
  mean = 127970.5
  amplitude = 487.0

TMP117:
  temp_ok = 1
  temperature = 22.66 C

Wear detection:
  worn = 1

State:
  ERROR -> NORMAL after valid sensor data
```

Result:

```text
PASS
```

Interpretation:

- MAX30102 and TMP117 were both readable after the firmware changes.
- The lack of heart-rate/temperature in the SMS is likely a message-format or standalone data-handling issue, not a basic sensor hardware failure.
- Next firmware task is to ensure `STANDALONE` inserts the latest measured `bpm` and `tempC` into the SMS body every time.

## 10. Current Verified Status

Verified by direct test:

```text
ESP32-C3 SuperMini external power + USB debug: PASS
SIM7080G boot from battery-side power: PASS
SIM7080G LTE registration: PASS
SIM7080G GNSS coordinate acquisition: PASS
SIM7080G SMS transmission: PASS
Full prototype battery-powered SMS path: PASS
Standalone trigger and buzzer feedback: PASS after fixes
MAX30102 heart-rate measurement: PASS
TMP117 temperature measurement: PASS
```

Known working GNSS area from tests:

```text
Latitude: about 36.05
Longitude: about 127.33
```

## 11. Remaining Work Before Case / PCB Freeze

Firmware:

- Ensure standalone SMS body always includes:

```text
HR: <bpm>, TEMP: <temperature>, LOC: <lat>, <lon>
```

- Avoid repeated automatic SMS transmission. SMS must remain trigger-based.
- Keep the SMS destination configurable for demonstration.

Power:

- Repeat full battery test while watching voltage with oscilloscope if time permits.
- Confirm behavior at lower battery voltage, not only around 3.97 to 3.99 V.
- Add proper power architecture in PCB design instead of relying on development board regulator behavior.

Mechanical:

- Design a wearable enclosure that holds the sensor surfaces against the skin.
- Keep SIM7080G/LTE heat away from TMP117.
- Provide antenna clearance and cable strain relief.
- Provide USB/debug access during prototype phase.
- Provide a simple trigger button or exposed test pads for GPIO4-to-GND standalone start.

