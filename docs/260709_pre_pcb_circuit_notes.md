# Pre-PCB Circuit Notes

Date: 2026-07-09

Project: Heat Risk Alert Wristband

Purpose: summarize the validated development wiring and remaining circuit decisions before schematic capture.

## 1. Current Development Board

MCU:

- Seeed XIAO ESP32-C3

Firmware workspace:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware
```

Document/log workspace:

```text
N:\Own\01_YU\프로젝트 및 활동\2025-2_GCDP(Thailand-Korea)\GCDP_하계(Korea)\project
```

## 2. Validated Pin Map

```text
ESP32-C3 GPIO6  -> I2C SDA -> TMP117 SDA, MAX30102 SDA
ESP32-C3 GPIO7  -> I2C SCL -> TMP117 SCL, MAX30102 SCL
ESP32-C3 GPIO10 -> Buzzer signal
ESP32-C3 GPIO20 -> SIM7080G DevKit UTX
ESP32-C3 GPIO21 -> SIM7080G DevKit URX
ESP32-C3 GPIO5  -> SIM7080G DevKit PWRK (firmware prepared, physical test pending)
Common GND      -> all modules
```

Notes:

- TMP117 and MAX30102 share the same I2C bus.
- TMP117 address observed: `0x48`.
- MAX30102 address observed: `0x57`.
- SIM7080G UART baud rate used: `115200`.

## 3. Validated Functions

Confirmed during bring-up:

- ESP32-C3 firmware build/upload
- TMP117 temperature read
- MAX30102 red/IR read
- wear detection using MAX30102 IR threshold
- buzzer output on GPIO10
- SIM7080G AT command communication
- SKT registration through operator `45005`
- packet attach
- manual SMS transmission
- GNSS coordinate query through `AT+CGNSINF`

Current firmware commands:

```text
HELP
STATUS
LTE
GNSS
GNSSOFF
MODEMPWR
ALARM
CANCEL
SMS <number> <message>
```

Safety note:

- SMS is manual-only in the current firmware.
- Do not add automatic SMS sending until the emergency workflow and rate limiting are intentionally implemented.

## 4. SIM7080G Circuit Requirements

Required signals:

```text
UART TX/RX
PWRK
GND
Main power input
LTE antenna
GNSS antenna if location is used
SIM card interface or SIM holder
```

PWRK recommendation:

- Use an open-drain style low-side pull-down circuit for PWRK.
- MCU should pull PWRK LOW for about 1.8 to 2.0 seconds to toggle modem power.
- Avoid a final PCB design that forces a push-pull high level directly into PWRK.

Power recommendation:

- SIM7080G power must tolerate LTE current spikes.
- Add bulk capacitance close to the modem power input.
- Keep modem power traces short and wide.
- Verify whether the selected module requires direct Li-ion input, regulated 5 V, or regulated 3.8 to 4.2 V before schematic freeze.

## 5. Sensor Circuit Requirements

I2C bus:

- SDA: GPIO6
- SCL: GPIO7
- Confirm pull-up placement and values on the final PCB.
- Avoid duplicate overly-strong pull-ups if breakout modules already include pull-ups during prototype testing.

TMP117:

- VIN/GND/SDA/SCL required.
- INT and ADDR are currently unused.
- ADDR can remain default if address `0x48` is acceptable.

MAX30102:

- VIN/GND/SDA/SCL required.
- INT is currently unused.
- Final physical placement matters because optical contact quality affects wear detection.

## 6. Buzzer Circuit Requirements

Current prototype:

```text
ESP32-C3 GPIO10 -> buzzer signal
```

Final PCB decision:

- If using an active buzzer with low current, direct GPIO drive may be possible.
- If current is above safe GPIO limits, add a transistor driver and flyback/protection as appropriate.
- Decide final sound level based on wearable use, not just bench visibility.

## 7. Remaining Tests Before Schematic Freeze

Required:

- Connect `SIM7080G DevKit PWRK` to `ESP32-C3 GPIO5` and test `MODEMPWR`.
- Confirm modem starts from a fully off state using MCU PWRK control.
- Confirm LTE registration after MCU-driven modem power-up.
- Retest GNSS outdoors or near a window.
- Test operation from intended battery/power source.
- Measure or at least observe brownout/reset risk during LTE attach/SMS/GNSS.

Strongly recommended:

- Decide final SIM7080G form factor: developer kit is too large for final wearable; use a smaller module or custom carrier in PCB.
- Decide antenna connector/antenna type.
- Decide SIM holder location and accessibility.
- Decide charging path and power switch behavior.
- Decide enclosure and sensor window/contact geometry before finalizing MAX30102 placement.

## 8. Current PCB Readiness Status

Ready for schematic drafting:

- MCU pin allocation draft
- I2C sensor bus
- UART modem interface
- buzzer output
- manual SMS and GNSS firmware proof

Not ready to freeze:

- modem power rail
- PWRK hardware interface
- battery/charging circuit
- antenna mechanical placement
- final module footprint
- optical sensor mechanical placement
