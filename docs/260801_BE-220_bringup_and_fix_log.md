# BE-220 GPS Bring-Up and Fix Test Log

Date: 2026-08-01

## 1. Objective

Complete the deferred incoming inspection, physical measurement, UART bring-up,
NMEA validation, and outdoor position-fix test for the delivered Beitian BE-220
GPS module before resuming the wearable enclosure work.

## 2. Hardware and Wiring

- MCU: ESP32-C3 SuperMini
- GPS: Beitian BE-220
- MCU power/debug: USB from notebook
- GPS supply: ESP32-C3 `5V` and `GND`
- GPS UART: 3.3 V TTL

```text
BE-220 GND -> ESP32-C3 GND
BE-220 TX  -> ESP32-C3 GPIO6 (GPS RX)
BE-220 RX  -> ESP32-C3 GPIO7 (GPS TX)
BE-220 VCC -> ESP32-C3 5V
```

The module pin order in the saved vendor reference is `GND, TX, RX, VCC` from
pin 1 through pin 4. The reference image is stored at:

```text
03_references/260801_BE-220/260801_BE-220_pinout_and_led_reference.webp
```

## 3. Physical Inspection

Measured maximum envelope:

```text
20 x 22 x 6.25 mm
```

The nominal model height may be treated as 6 mm, but 6.25 mm is the measured
worst-case height to use for fit validation. The ceramic antenna and receiver
occupy nearly the complete board area.

## 4. Baud-Rate Diagnosis

The product reference stated a default UART rate of 38400 baud. Initial reads at
38400 produced incoming bytes but not readable NMEA text. A firmware `GPSRAW`
command exposed the malformed stream, so the earlier assumption that any byte
count implied valid NMEA was rejected.

The added `GPSPROBE` command tested common rates. Relevant results:

```text
38400 baud: 62% printable, no NMEA sentence delimiters
115200 baud: 100% printable, 31 '$' delimiters, 31 complete lines
```

Conclusion: this physical BE-220 unit is configured for 115200 baud despite the
38400-baud vendor listing. The `esp32c3_supermini` firmware configuration was
changed to `GPS_BAUD=115200`.

## 5. NMEA and Outdoor Fix Results

At 115200 baud the receiver emitted valid NMEA sentences including `GNRMC`,
`GNGGA`, `GNGSA`, `GPGSV`, `GAGSV`, `GBGSV`, and `GNGLL`.

Representative outdoor result:

```text
RMC status: A (valid)
GGA fix quality: 1
GSA mode: 3 (3D)
Satellites used: 6
HDOP: 2.32
Altitude: approximately 68 m
Latitude: 35.821526
Longitude: 128.755417
NMEA checksum failures: 0
```

The integrated TinyGPSPlus parser also returned a valid latitude and longitude.
The parser originally returned immediately after the RMC location field and
therefore printed satellite count and HDOP as zero before the following GGA
sentence arrived. The success condition was updated to wait for valid location,
satellite, and HDOP fields.

After the successful outdoor fix, the setup was moved back indoors. A subsequent
check then returned zero satellites and no fix. This was an expected indoor signal
loss, not intermittent module behavior or a regression caused by the firmware
upload.

## 6. Firmware Changes

- Corrected the SuperMini external GPS baud rate from 38400 to 115200.
- Added `GPSRAW [seconds]` for direct NMEA inspection.
- Added `GPSPROBE` for UART baud-rate detection.
- Changed GPS success logging to wait for GGA quality data.
- Extended no-fix logging with satellite count and HDOP.

## 7. Result

```text
Physical inspection: PASS
5 V power: PASS
UART electrical connection: PASS
Baud-rate identification: PASS (115200)
Readable NMEA stream: PASS
Outdoor 3D position fix: PASS
Firmware coordinate parsing: PASS
PPS LED visual confirmation: not recorded during this test
```

The temporary `ERROR` state visible in some serial logs was caused by the other
I2C sensors not being connected during this GPS-only test. It is unrelated to the
BE-220 result.
