# 2026-07-09 LTE/SMS and Integrated Firmware Log

Project: Heat Risk Alert Wristband  
Purpose: Validate physical SIM, SKT network registration, SMS transmission, and transition from bring-up sketches to integrated firmware.

## 1. SIM and LTE Network Test

### SIM

A physical SIM was activated on the SKT network and inserted into the SIM7080G developer kit.

Sensitive identifiers are intentionally masked in this log.

### Modem and UART

The SIM7080G developer kit communicated with the ESP32-C3 over UART.

Confirmed UART configuration:

```text
ESP32-C3 RX = GPIO20
ESP32-C3 TX = GPIO21
Baud = 115200
```

The SIM7080G responded to AT commands.

Observed:

```text
AT -> OK
ATI -> R1951.07 / OK
AT+CPIN? -> +CPIN: READY / OK
```

### Antenna and Signal

Before antenna connection:

```text
AT+CSQ -> +CSQ: 99,99
AT+CEREG? -> +CEREG: 0,2
```

After antenna connection:

```text
AT+CSQ -> +CSQ: 23 to 26,99
```

Conclusion: antenna connection was required for practical network reception.

### Network Registration

The modem successfully registered on SKT.

Observed successful registration:

```text
AT+CEREG? -> +CEREG: 2,1,...
AT+COPS?  -> +COPS: 0,2,"45005",7
AT+CGATT? -> +CGATT: 1
```

Conclusion:

- SKT network registration succeeded.
- The working PLMN was `45005`.
- Manual attempt with `45012` failed with no network service.

### Modem Mode

The modem was set to Cat-M preferred mode:

```text
AT+CMNB=1
```

The modem reported:

```text
AT+CNMP? -> +CNMP: 38
AT+CBAND? -> +CBAND: ALL_MODE
```

## 2. SMS Transmission Test

### Test Message

A test SMS was sent from the SIM7080G module to a separate phone number.

Message body:

```text
GCDP SIM7080G SMS TEST
```

The receiving phone confirmed that the SMS was received.

### Safety Issue Found

The initial SMS test firmware used a one-shot flag stored only in RAM. Because the flag resets after firmware reset, the message could be sent again after re-upload or reset.

This caused duplicate test SMS reception.

### Safety Fix

Automatic SMS sending was disabled.

The integrated firmware now sends SMS only when a manual serial command is entered:

```text
SMS <number> <message>
```

No SMS is sent automatically during boot, reset, network registration, or state-machine operation.

## 3. Integrated Firmware

The firmware was rewritten from separate diagnostic sketches into one integrated firmware file:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware\src\main.cpp
```

Implemented components:

- I2C sensor initialization and scanning
- TMP117 temperature reading
- MAX30102 RED/IR sampling
- IR-threshold-based wear detection
- active buzzer control
- LTE modem initialization
- LTE status polling
- manual SMS command
- basic state machine

## 4. State Machine

Implemented states:

```text
BOOT
NOT_WORN
NORMAL
SUSPECT
ALARM
EMERGENCY_READY
ERROR
```

Current behavior:

- No contact: `NOT_WORN`
- Contact detected by MAX30102 IR threshold: `NORMAL`
- Temperature above threshold for confirmation period: `SUSPECT`
- Sustained abnormal condition: `ALARM`
- Alarm duration elapsed: `EMERGENCY_READY`
- Sensor failure persistence: `ERROR`

The current emergency workflow does not automatically send SMS. SMS sending remains manual for safety during development.

## 5. Wear Detection

Wear detection uses MAX30102 IR intensity with hysteresis and consecutive-sample confirmation.

Thresholds:

```text
wear on  threshold: 80000
wear off threshold: 60000
confirmation count: 3
```

Observed:

```text
[wear] detected ...
[state] NOT_WORN -> NORMAL
[wear] removed ...
[state] NORMAL -> NOT_WORN
```

## 6. LTE Status in Integrated Firmware

The integrated firmware successfully maintained LTE status monitoring.

Observed:

```text
lte_reg = 1
csq = 22 to 23
CPIN READY
COPS 45005
CGATT 1
```

## 7. Sensor Stability Adjustment

During integrated testing, occasional I2C read failures occurred. The first integrated version treated transient I2C failures as immediate `ERROR` conditions, which caused state flapping.

Adjustment:

- TMP117 must fail repeatedly before `ERROR`.
- MAX30102 must fail repeatedly before `ERROR`.
- Last valid readings are retained through short transient failures.
- Implausible TMP117 readings outside the expected sensor range are rejected.

After adjustment, the firmware maintained stable `NOT_WORN` and `NORMAL` states during regular operation.

## 8. Current Confirmed Capabilities

Confirmed:

- ESP32-C3 integrated firmware upload
- TMP117 read
- MAX30102 read
- wear detection
- buzzer control
- SIM7080G AT communication
- SKT network registration
- packet attach
- actual SMS transmission
- manual-only SMS safety lock

Not yet completed:

- automatic emergency SMS workflow
- GNSS/GPS acquisition
- Li-Po battery operation
- LTE transmission power stability test
- PCB schematic
- final threshold tuning

## 9. Next Recommended Steps

1. Add a manual emergency simulation command:

```text
ALARM
CANCEL
```

2. Add a manual SMS command test only when intentionally needed:

```text
SMS <number> <message>
```

3. Implement GNSS test:

```text
GNSS power on
location query
timeout handling
last-known-location fallback
```

4. Prepare circuit-design notes:

- ESP32-C3 pin map
- I2C bus
- SIM7080G UART
- SIM7080G PWRK control
- buzzer pin
- LTE antenna requirement
- GNSS antenna requirement
- power rail requirements
- LTE current spike mitigation

## 10. GNSS Test

GNSS test commands were added to the integrated firmware.

Serial commands:

```text
GNSS
GNSSOFF
```

Observed GNSS command sequence:

```text
AT+CGNSPWR=1
OK
AT+CGNSINF
+CGNSINF: 1,,,36.050000,127.330003,-19.819,,,1,,0.1,0.1,0.1,,,,191841.8,6000.0
OK
[gnss] coordinate acquired
```

Result:

- SIM7080G GNSS command path worked.
- GNSS coordinate fields were returned.
- Firmware was adjusted to treat valid latitude/longitude fields as coordinate acquisition, because the module response did not always return the simple `+CGNSINF: 1,1` pattern.
- GNSS was turned off after the test:

```text
AT+CGNSPWR=0
OK
```

Note:

- GNSS reliability should be retested outdoors or near a window.
- The developer kit has multiple antenna connectors; the final board must clearly separate LTE antenna and GNSS antenna requirements.

## 11. Modem PWRK Control Preparation

Firmware support was added for MCU-controlled SIM7080G PWRK pulsing.

Temporary development pin assignment:

```text
ESP32-C3 GPIO5 -> SIM7080G DevKit PWRK
ESP32-C3 GND   -> SIM7080G DevKit GND
```

Serial command:

```text
MODEMPWR
```

Behavior:

- The firmware pulls `LTE_PWRK_PIN` LOW for about 1.8 seconds.
- After the pulse, the pin is released back to input/high-impedance mode.
- This mimics the manual PWRK-to-GND activation previously used.

Implemented build flag for the XIAO ESP32-C3 environment:

```text
LTE_PWRK_PIN=5
```

Result:

- Firmware built and uploaded successfully after adding PWRK control.
- Command list verified:

```text
HELP | STATUS | LTE | GNSS | GNSSOFF | MODEMPWR | ALARM | CANCEL | SMS <number> <message>
```

Pending:

- Physical PWRK wire test remains pending until `SIM7080G DevKit PWRK` is connected to `ESP32-C3 GPIO5`.
- Final PCB should not rely on a direct push-pull drive for PWRK. Use an open-drain style transistor/MOSFET interface or equivalent safe low-side pull-down circuit.

## 12. Current Pre-PCB Pin Map

Current validated development wiring:

```text
ESP32-C3 GPIO6  -> I2C SDA -> TMP117 SDA, MAX30102 SDA
ESP32-C3 GPIO7  -> I2C SCL -> TMP117 SCL, MAX30102 SCL
ESP32-C3 GPIO10 -> Buzzer signal
ESP32-C3 GPIO20 -> SIM7080G DevKit UTX
ESP32-C3 GPIO21 -> SIM7080G DevKit URX
ESP32-C3 GPIO5  -> SIM7080G DevKit PWRK (prepared, physical test pending)
Common GND      -> all modules
```

Current validated features:

- TMP117 temperature read
- MAX30102 red/IR read
- wear detection
- buzzer state feedback
- LTE AT command communication
- SKT network registration on `45005`
- packet attach
- manual SMS transmission
- GNSS coordinate query

Still required before PCB schematic freeze:

- PWRK GPIO physical test
- battery/power-path decision
- LTE peak-current supply test
- final antenna connector decision
- final SIM holder decision
- final enclosure-driven sensor placement decision

## 13. SIM7080G DevKit PWRK Follow-Up Test

Additional PWRK testing was performed after connecting the SIM7080G developer kit to an external USB power source through a micro-USB cable and keeping the PWRK line connected to ESP32-C3 GPIO5.

Observed:

- With the previous modem power-down state, the modem did not respond to AT commands.
- After external power was applied, the modem eventually became responsive again.
- LTE registration recovered:

```text
AT
OK
CPIN READY
CSQ 26,99
CEREG 0,1
COPS SKTelecom
CGATT 1
```

Firmware command added:

```text
PWRKHIGH [ms]
```

Test result:

- `PWRKHIGH 3000` drove GPIO5 HIGH for about 3 seconds.
- After the HIGH pulse, the modem later printed:

```text
NORMAL POWER DOWN
```

- After this event, AT commands returned no response.
- A second `PWRKHIGH 3000` from the powered-down state did not bring AT communication back within the observation window.

Interpretation:

- The developer kit PWRK behavior is not the same as a simple active-low push button on this board.
- GPIO5 HIGH pulsing appears capable of triggering modem power-down.
- MCU-driven modem power-on has not yet been confirmed.
- External power stability improved modem availability, so modem supply behavior must remain a pre-PCB design focus.

Current status:

- `MODEMOFF` / `AT+CPOWD=1`: confirmed to power down modem.
- `PWRKHIGH 3000`: confirmed to trigger power-down when modem is on.
- `MODEMPWR` LOW pulse: did not recover modem from power-down during the observed tests.
- `PWRKHIGH 3000` from powered-down state: did not recover modem during the observed tests.

Next check:

- Manually recover modem power using the developer kit method that previously worked.
- After recovery, identify whether the developer kit PWRK pin is intended as:
  - a direct modem PWRKEY signal,
  - an enable pin,
  - or a board-level power control input.
- Final PCB should not copy the developer kit PWRK behavior blindly; use the target SIM7080G module datasheet/reference circuit for the final PWRKEY interface.

Follow-up:

- The modem green LED was observed blinking even while the firmware status still showed `lte_reg=0`.
- After firmware re-upload/reset and UART probing, LTE UART communication recovered at `115200`.
- `LTEPROBE` was added to test multiple UART baud rates.

Observed:

```text
[lte-probe] trying baud=115200
[lte-probe] baud=115200 attempt=1 response=OK
[lte-probe] success baud=115200
CPIN READY
CSQ 25 to 26
CEREG 0,1
COPS SKTelecom
CGATT 1
```

Updated interpretation:

- The modem was alive when the green LED was blinking.
- The temporary no-response state was likely due to reset/timing/UART synchronization during the power-control experiments, not confirmed modem death.
- External power through the micro-USB cable improved modem stability.
- Continue treating PWRK behavior as development-kit-specific until verified against the target module reference circuit.

## 14. Manual Emergency-Format SMS Test

A manual emergency-format SMS test was performed using live sensor/GNSS readings.

Firmware command added:

```text
ALERTSMS <number>
```

Command behavior:

- Estimate heart rate from MAX30102 IR samples for about 20 seconds.
- Read TMP117 temperature.
- Enable GNSS and query `AT+CGNSINF`.
- Disable GNSS after coordinate acquisition.
- Build an emergency-format SMS message.
- Send exactly one SMS through the existing manual SMS path.

Observed measured values:

```text
HR:   48.0 bpm
TEMP: 33.17 C
LOC:  36.049999,127.330002
```

GNSS observation:

```text
AT+CGNSPWR=1
OK
AT+CGNSINF
+CGNSINF: 1,,,36.050000,127.330003,-19.819,,,1,,0.1,0.1,0.1,,,,191841.6,6000.0
OK
```

Message preview:

```text
SOS HELP REQUEST
HR: 48.0 bpm
TEMP: 33.17 C
LOC: 36.049999,127.330002
```

First send attempt:

- The first `ALERTSMS` attempt used a phone number string containing hyphens.
- `AT+CMGS` returned `ERROR` before the SMS prompt.
- Result:

```text
[sms] failed: no prompt
[alert] SMS result=failed
```

Second send attempt:

- The same measured values were sent with a digits-only phone number.
- Message body was converted to a compact single-line ASCII format:

```text
SOS HELP REQUEST | HR: 48.0 bpm | TEMP: 33.17 C | LOC: 36.049999,127.330002
```

Observed result:

```text
[sms] sending to 01000000000
>
+CMGS: 8
OK
[cmd] SMS result=success
```

Conclusion:

- Live sensor values and GNSS coordinates were successfully included in an emergency-format SMS.
- Phone numbers should be passed to `AT+CMGS` as digits only, without hyphens.
- The current heart-rate estimate is still a rough prototype calculation and should not be treated as medically validated.
- The emergency SMS workflow should keep manual/rate-limited safeguards until persistent duplicate-prevention is implemented.

## 15. Power Supply / UART Recovery Test

Date: 2026-07-10

Bench equipment available:

- Multimeter
- Bench power supply
- Oscilloscope

Battery voltage before testing:

```text
Li-Po open-circuit voltage: 3.991 V
```

SIM7080G board power header check:

- With the SIM7080G board powered through micro-USB, the header `VDD` pin measured about `5.022 V`.
- Therefore this board's exposed `VDD` header is treated as a 5 V board input/rail, not a direct Li-Po/VBAT input.
- Direct 3.7 to 4.2 V Li-Po input to this `VDD` pin is not treated as a valid final test for bare SIM7080G VBAT behavior.

Bench power supply test:

```text
PSU setting: 5.00 V
Current limit: initially 0.50 A, later increased to 1.00 A
Connection: PSU + -> SIM7080G VDD, PSU - -> SIM7080G GND
```

Observed:

- Red power LED turned on.
- Current initially stayed very low, around `0.005 A`.
- PWRK activation was required.
- On this developer board, applying voltage to PWRK rather than pulling it to GND was observed to trigger board power behavior.
- With PSU at 5 V and current limit at 1 A, current varied roughly around `0.02 A` to `0.05 A` while the green LED was blinking.

UART/GND issue:

- During the first PSU-powered attempts, the green LED blinked but UART returned no AT response.
- One SIM7080G-side GND point was not connected to the ESP32-side GND and was then connected.
- After correcting the GND/common reference and restoring the UART wiring, UART communication recovered.

Measured powered UART idle voltages:

```text
SIM7080G VDD -> GND: 4.993 V
SIM7080G UTX -> GND: 3.24 V
SIM7080G URX -> GND: 3.313 V
```

Working UART wiring after correction:

```text
SIM7080G UTX -> ESP GPIO20
SIM7080G URX -> ESP GPIO21
SIM7080G GND -> ESP GND
```

Recovered UART result:

```text
LTEPROBE:
baud 115200 attempt 2 response: AT / OK
success baud=115200
```

LTE result:

```text
AT -> OK
CPIN READY
CSQ 19 to 20
CEREG 0,1
COPS SKTelecom
CGATT 1
lte-status at=1 sim=1 reg=1 attach=1
```

GNSS result:

```text
AT+CGNSPWR=1 -> OK
AT+CGNSINF attempt 3:
+CGNSINF: 1,,,36.050000,127.330003,-19.819,,,1,,0.1,0.1,0.1,,,,191844.8,6000.0
[gnss] coordinate acquired
```

Conclusion:

- The SIM7080G developer board can run from a bench 5 V supply through the exposed `VDD/GND` header.
- Common GND between PSU/SIM7080G/ESP32 is mandatory for UART communication.
- UART idle voltages around 3.2 to 3.3 V are consistent with active 3.3 V TTL UART.
- LTE registration and GNSS coordinate acquisition were confirmed after the wiring/reference issue was corrected.
- The current developer board's exposed `VDD` should be treated as 5 V input, not as the final bare-module Li-Po VBAT node.

Follow-up with micro-USB removed:

- SIM7080G micro-USB power was removed.
- Bench PSU was re-enabled as the SIM7080G board supply through `VDD/GND`.
- All other signal wiring remained unchanged.

Observed:

```text
LTEPROBE:
baud 115200 attempt 2 response: AT / OK
success baud=115200

LTE:
CPIN READY
CSQ 19 to 21
CEREG 0,1
COPS SKTelecom
CGATT 1
lte-status at=1 sim=1 reg=1 attach=1

GNSS:
AT+CGNSPWR=1 -> OK
AT+CGNSINF attempt 3 -> coordinate acquired
coordinate: 36.050000,127.330003
```

Conclusion:

- With the corrected common GND and UART wiring, the SIM7080G developer board operated successfully from the bench PSU via `VDD/GND` without micro-USB power.
- LTE registration and GNSS coordinate acquisition both passed under this PSU-powered condition.

## 16. PSU-Powered GNSS + SMS Load Test

Date: 2026-07-10

Purpose:

- Confirm whether the SIM7080G developer board remains stable during GNSS acquisition and SMS transmission while powered from a bench PSU through the board `VDD/GND` header.
- Heart-rate measurement was skipped because the user could not keep a finger on the MAX30102 while simultaneously holding measurement probes.

Firmware command added:

```text
POWERTESTSMS <number>
```

Command behavior:

- Skip heart-rate measurement.
- Read temperature if available.
- Enable GNSS.
- Acquire coordinate through `AT+CGNSINF`.
- Disable GNSS.
- Re-check LTE readiness.
- Send SMS with `HR: N/A`, temperature, and location.

Power setup:

```text
SIM7080G micro-USB: disconnected
PSU + -> SIM7080G VDD
PSU - -> SIM7080G GND
ESP32 USB -> PC for serial/debug
ESP32 GND -> SIM7080G GND common
PSU setting: 5.00 V
Current limit: 1.00 A
```

First run:

```text
GNSS coordinate acquired
LTE readiness recovered after GNSS off
SMS response: +CMGS: 9 / OK
[powertest] SMS result=success
```

Second run, with multimeter observation:

```text
GNSS coordinate acquired
LOC: 36.049999,127.330002
CSQ: 18
CEREG: 0,1
COPS: SKTelecom
CGATT: 1
SMS response: +CMGS: 10 / OK
[powertest] SMS result=success
```

User confirmed:

```text
SMS received.
Lowest observed voltage during the test: about 4.85 V
```

Conclusion:

- PSU-powered 5 V input through the SIM7080G developer board `VDD/GND` header passed GNSS + SMS load testing.
- Observed minimum voltage of about 4.85 V is acceptable for this 5 V board input test.
- No ESP32 reset was observed during the successful power-test SMS runs.
- No SIM7080G UART failure was observed during the successful power-test SMS runs.
- This confirms the developer board can operate from a stable 5 V rail, but does not prove bare SIM7080G direct Li-Po/VBAT operation because the exposed board `VDD` was measured as a 5 V rail.
