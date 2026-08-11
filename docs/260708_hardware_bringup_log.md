# 2026-07-08 Hardware Bring-Up Log

Project: Heat Risk Alert Wristband  
Workspace: GCDP 하계(Korea)  
Purpose: Initial firmware environment setup and hardware bring-up for MCU, sensors, buzzer, and SIM7080G LTE module.

## 1. Development Environment

### Temporary Firmware Workspace

The firmware development workspace was moved from the cloud-synced `N:` drive to a local temporary directory to avoid PlatformIO build/cache issues.

Active firmware path:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware
```

### Tooling

Installed and verified:

- PlatformIO Core
- VS Code PlatformIO IDE extension
- VS Code C/C++ extension
- Espressif32 PlatformIO platform
- ESP32 and ESP32-C3 toolchains

### PlatformIO Environments

The following PlatformIO environments were created and build-tested:

- `esp32dev`
- `xiao_esp32c3`
- `esp32c3_supermini`

All three environments compiled successfully.

## 2. MCU Bring-Up

### ESP32-C3 Board

The USB-C ESP32-C3 board was detected by Windows and PlatformIO as:

```text
COM6
USB VID:PID=303A:1001
ESP32-C3
USB-Serial/JTAG
MAC: 44:b1:76:19:d5:f0
```

`esptool.py chip_id` confirmed:

```text
Chip is ESP32-C3 AZ (QFN32) revision v1.1
Embedded Flash 4MB
USB mode: USB-Serial/JTAG
```

Firmware upload to the ESP32-C3 board succeeded.

### ESP32 DevKitC WROOM-32U

The micro USB ESP32 DevKitC was detected as a CP2102 USB-UART device, but Windows reported a driver installation failure.

Observed device:

```text
CP2102 USB to UART Bridge Controller
Status: Error
Problem: CM_PROB_FAILED_INSTALL
```

The board was not used further during this session.

## 3. Firmware Structure

Initial PlatformIO firmware files were created under:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware
```

Main files:

- `platformio.ini`
- `src/main.cpp`
- `include/pins.h`
- `README.md`

Initial firmware functions implemented during the session:

- I2C scan
- TMP117 temperature read
- MAX30102 part ID read
- MAX30102 raw RED/IR sampling
- Buzzer GPIO control
- SIM7080G UART AT diagnostic scanning

## 4. I2C Bus Configuration

ESP32-C3 I2C pin configuration:

```text
SDA = GPIO6
SCL = GPIO7
```

Both sensors were connected on the same I2C bus.

Expected I2C addresses:

```text
TMP117   = 0x48
MAX30102 = 0x57
```

## 5. MAX30102 Heart-Rate Sensor Test

### Wiring

MAX30102 pins used:

```text
VIN -> ESP32-C3 3V3
GND -> ESP32-C3 GND
SDA -> ESP32-C3 GPIO6
SCL -> ESP32-C3 GPIO7
```

Unused MAX30102 pins:

```text
D
IRO
INT
extra GND
```

### I2C and Part ID Test

The sensor was detected on the I2C bus:

```text
[i2c] found 0x57
```

MAX30102 part ID read succeeded:

```text
[max30102] part id 0x15
```

### Raw Data Test

MAX30102 RED/IR raw data was successfully read.

Example observed values with finger on sensor:

```text
red = 104552
ir  = 130099
```

Later continuous sampling produced approximately:

```text
Sample rate: 49.98 Hz
RED range: 107248 to 107786
IR range: 133719 to 134472
```

The captured waveform file was saved at:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware\captures\max30102_waveform_20260708_173314.csv
```

### BPM Estimate

A crude peak detector estimated:

```text
Mean BPM:   115.2
Median BPM: 115.4
```

This was treated as a preliminary waveform validation, not a final heart-rate algorithm.

## 6. TMP117 Temperature Sensor Test

### Wiring

TMP117 pins used:

```text
VIN -> ESP32-C3 3V3
GND -> ESP32-C3 GND
SDA -> ESP32-C3 GPIO6
SCL -> ESP32-C3 GPIO7
```

Unused TMP117 pins:

```text
INT
ADDR
```

### Simultaneous I2C Test

With both TMP117 and MAX30102 connected, the I2C scan detected both sensors:

```text
[i2c] found 0x48
[i2c] found 0x57
```

TMP117 temperature readings were successfully observed:

```text
32.84 C
33.01 C
33.12 C
33.21 C
```

MAX30102 raw RED/IR data continued to work at the same time.

Conclusion: TMP117 and MAX30102 worked simultaneously on the same I2C bus.

## 7. Wear Detection and Buzzer Test

### Buzzer Wiring

The active buzzer was connected to:

```text
ESP32-C3 GPIO10
```

### Test Logic

Firmware was updated so that:

- Finger/contact detected: buzzer ON
- Finger/contact removed: buzzer OFF

Initial IR thresholds:

```text
wear on  threshold: 80000
wear off threshold: 60000
```

### Result

Wear detection worked with low perceived delay. The buzzer turned on/off when contact state changed.

Observed serial events included:

```text
# wear detected ir=127793
# wear removed ir=1013
# wear detected ir=128071
# wear removed ir=1023
```

The user confirmed the delay was acceptable for the current prototype stage.

## 8. SIM7080G Small Module Test

### Small Module Pins

The small SIM7080G CAT-M module had these labeled pins:

```text
5V
3V3
GND
Rf
Tx
Rx
```

### Initial Connection

Connection attempted:

```text
SIM7080G GND -> ESP32-C3 GND
SIM7080G 5V  -> 5V power
SIM7080G Tx/Rx -> ESP32-C3 GPIO20/GPIO21 combinations
```

`Rf` was not connected to the ESP32-C3.

### Result

The module LED turned on, but no AT response was received.

Firmware scanned:

- normal TX/RX direction
- swapped TX/RX direction
- baud rates: `115200`, `9600`, `57600`, `38400`, `19200`

Result:

```text
AT -> no response
```

Conclusion: The small module was not successfully brought up during this session.

Possible causes:

- module body not powered on
- hidden PWRKEY/boot requirement
- UART pins not active
- board-specific power or level-shifter condition

## 9. SIM7080G Developer Kit Test

### Developer Kit Pin Labels

The developer kit exposed these pins:

```text
DTR
GND
VDD
PWRK
UTX
URX
GND
```

It also had:

- micro USB connector
- 4-pin connector
- three antenna connectors

PCB markings observed:

```text
CAT-1-A76-80X
NB-SIM-7080
SIM-800/868
```

### USB Test

The developer kit was connected to the PC via micro USB.

Result:

- LED turned on.
- No new COM port appeared in Windows.
- Only the existing ESP32-C3 COM6 port was detected.

Possible cause:

- micro USB cable may have been power-only
- developer kit USB may not expose USB-UART
- driver not applicable or device not enumerating

### UART Pin Test Through ESP32-C3

The developer kit was then connected to the ESP32-C3 UART pins.

Working connection after diagnosis:

```text
DevKit UTX -> ESP32-C3 GPIO21
DevKit URX -> ESP32-C3 GPIO20
DevKit GND -> ESP32-C3 GND
```

The developer kit was powered separately via micro USB.

### Power Key

The developer kit required a power-key action.

Action performed:

```text
PWRK -> GND for approximately 1 to 2 seconds, then released
```

After this, a green LED began blinking.

### AT Command Success

UART diagnostic succeeded after PWRK activation.

Successful UART configuration:

```text
ESP32-C3 RX = GPIO21
ESP32-C3 TX = GPIO20
baud = 115200
```

Observed AT response:

```text
AT
OK
```

Observed ATI response:

```text
ATI
R1951.07
OK
```

Conclusion: SIM7080G developer kit UART bring-up succeeded.

## 10. SIM Status

No SIM card was available during this session.

The user noted that their phone uses eSIM, so no physical SIM was available for SMS or network registration testing.

SIM-dependent tests were not performed:

- LTE network registration
- signal quality with valid SIM
- SMS transmission
- emergency SMS workflow

SIM-independent tests that succeeded:

- SIM7080G developer kit power-on via PWRK
- UART communication
- AT command response
- firmware version query via ATI

## 11. Current Hardware Status

### Confirmed Working

- ESP32-C3 firmware upload and serial monitoring
- MAX30102 I2C communication
- MAX30102 raw RED/IR sampling
- TMP117 I2C communication
- TMP117 temperature reading
- simultaneous TMP117 + MAX30102 I2C operation
- buzzer output on GPIO10
- IR-threshold-based wear detection
- SIM7080G developer kit UART AT command communication

### Not Yet Completed

- CP2102 driver setup for ESP32 DevKitC WROOM-32U
- small SIM7080G module AT communication
- SIM card test
- LTE network registration
- SMS sending
- GNSS/GPS test
- battery/Li-Po power test
- integrated emergency state machine

## 12. Next Recommended Tests

1. Run SIM7080G developer kit command sequence:

```text
AT
ATI
AT+CGMM
AT+CGMR
AT+CPIN?
```

2. Obtain a physical SIM card that supports LTE Cat-M or compatible LTE service.

3. Test:

```text
AT+CPIN?
AT+CSQ
AT+CEREG?
```

4. Test SMS send workflow.

5. Test GNSS separately.

6. Restore or refactor firmware from diagnostic mode into modular bring-up modes:

- sensor mode
- wear detection mode
- LTE diagnostic mode
- integrated emergency workflow mode

