# GCDP Heat Risk Alert Wristband - Schematic Draft v0.1

Date: 2026-07-09

Purpose: 최신 실험 로그와 현재 firmware 상태를 반영해 1차 PCB schematic 초안을 sheet별로 확정한다. 이 문서는 KiCad 회로도 작성 전의 회로 설계 기준이다.

## 0. 최신 실험 반영사항

최신 로그에서 확인된 사실:

- 센서 착용 상태에서 `worn=1`, TMP117 온도, MAX30102 RED/IR 값 정상 확인.
- SIM7080G 계열 보드에서 SKTelecom 등록, `CGATT: 1`, `CSQ 24~25` 확인.
- GNSS 좌표 포함 emergency-format SMS 전송 성공.
- `ALERTSMS 01000000000` 형태처럼 전화번호는 하이픈 없이 digits only로 넣어야 `AT+CMGS` prompt가 정상적으로 나온다.
- 하이픈 포함 번호는 `AT+CMGS` 단계에서 `ERROR`가 발생했다.
- GNSS 응답 예:

```text
+CGNSINF: 1,,,36.050000,127.330003,-19.819,,,1,,0.1,0.1,0.1,,,,191841.6,6000.0
```

- SMS 성공 응답:

```text
+CMGS: 8
OK
```

중요 해석:

- 최종 제품 기능 흐름인 sensor + GNSS + SMS는 firmware/모듈 레벨에서 검증됐다.
- 현재 PWRK 실험은 developer kit/CoreBoard 레벨의 PWRK 동작이다. 최종 bare SIM7080G module schematic은 반드시 SIM7080G Hardware Design의 PWRKEY reference를 따른다.
- 현재 최신 빌드 산출물은 `xiao_esp32c3` 기준이지만, 1차 PCB 목표 MCU는 ESP32-C3 SuperMini다. 회로도는 SuperMini pinout 기준으로 작성한다.

## 1. 1차 PCB 설계 목표 재정의

1차 PCB는 완성형 wearable이 아니라 다음을 검증하는 bring-up carrier board다.

```text
1. SIM7080G bare module 직접 실장 성공
2. Li-Po 기반 VBAT 전원에서 LTE/SMS/GNSS 안정 동작
3. ESP32-C3 SuperMini + SIM7080G UART/PWRKEY 제어
4. 기존 TMP117/MAX30102 sensor module 4-wire 연동
5. buzzer 경고 출력
6. RF/SIM/power debug 가능한 test point 확보
```

1차에서 하지 않는 것:

```text
충전회로
USB-C 충전
ESP32-C3 bare chip 설계
MAX30102/TMP117 IC 직접 실장
PCB antenna
eSIM/MFF2 SIM
상용 방수/최종 착용감 최적화
자동 SMS 무제한 활성화
```

## 2. Schematic Sheet 구성

```text
Sheet 1  SYS_NOTES
Sheet 2  BATTERY_POWER
Sheet 3  ESP32C3_SUPERMINI
Sheet 4  SIM7080G_MODEM
Sheet 5  SIM_CARD
Sheet 6  RF_LTE_GNSS
Sheet 7  SENSOR_4WIRE
Sheet 8  BUZZER
Sheet 9  TEST_DEBUG
```

## 3. Sheet 1 - SYS_NOTES

회로도 첫 장에 다음 설계 note를 넣는다.

```text
1st PCB is a bring-up carrier board.
MCU is ESP32-C3 SuperMini module, not bare ESP32-C3.
Modem is bare SIM7080G SMT module, not developer kit/CoreBoard.
No charging circuit on this PCB.
Use protected 1S Li-Po pack only.
SMS automatic sending remains disabled unless persistent rate-limit is implemented.
Phone number for AT+CMGS must be digits only, no hyphen.
```

전압 domain:

```text
VBAT_SYS       1S Li-Po raw battery rail, 3.0~4.2 V practical range
VBAT_MODEM     SIM7080G high-current rail, directly from VBAT_SYS
3V3            ESP32-C3 SuperMini, sensors, buzzer logic
1V8_MODEM      SIM7080G logic domain / VDD_EXT reference, do not load heavily
RF_LTE         50 ohm RF path
RF_GNSS        50 ohm RF path
```

## 4. Sheet 2 - BATTERY_POWER

### 4.1 Battery Input

Connector:

```text
J_BAT: 1.25 mm 2-pin Li-Po connector
Pin 1: VBAT_IN
Pin 2: GND
Battery target: TW-602035 Li-Po 380 mAh, protected pack preferred
```

Protection/minimum debug:

```text
F1: optional polyfuse or 0 ohm current-measure link
D1: optional reverse protection footprint, DNP if battery connector polarity is keyed and controlled
TP_VBAT_IN
TP_GND near battery connector
```

Decision:

- 물리 power switch는 1차에서 제외 가능.
- 단, current measurement를 위해 `0 ohm link` 또는 solder jumper는 반드시 둔다.
- 배터리 커넥터 탈착으로 hard power cycle 가능하게 한다.

### 4.2 Modem VBAT Rail

Net:

```text
VBAT_IN -> F1/current link -> VBAT_SYS -> VBAT_MODEM
```

SIM7080G VBAT decoupling, modem pin 근처:

```text
C_MODEM_BULK1: 100 uF low-ESR SMD, >=6.3 V
C_MODEM_10U1: 10 uF ceramic, >=6.3 V
C_MODEM_1U1: 1 uF ceramic
C_MODEM_100N1: 100 nF ceramic
C_MODEM_BACKUP: large electrolytic/polymer backup footprint, DNP default
  footprint should accept available 470 uF / 1000 uF / 2200 uF leaded capacitor by hand solder if needed
TP_VBAT_MODEM near SIM7080G VBAT pins
```

Layout note:

```text
VBAT_MODEM trace/polygon must be short and wide.
Place bulk capacitors as close as possible to SIM7080G VBAT pins.
Provide many GND vias around modem power capacitors.
```

### 4.3 3.3 V Rail

Regulator:

```text
U_3V3: 3.3 V LDO, >=500 mA, low dropout, not AMS1117
Candidate class: AP2112K-3.3 / ME6211-3.3 500mA / TLV75533 or equivalent
```

Generic schematic:

```text
VBAT_SYS -> U_3V3 VIN
U_3V3 VOUT -> 3V3
U_3V3 GND -> GND
U_3V3 EN -> VBAT_SYS through 100 k or tied to VIN if no EN sequencing needed
C_LDO_IN: 10 uF near VIN
C_LDO_OUT: 10 uF near VOUT
C_3V3_100N: 100 nF near each IC/module load
TP_3V3
```

Caution:

- SuperMini 3V3 pin 직접 공급은 USB 연결 시 역전류 가능성을 실제 보드에서 확인해야 한다.
- 회로에는 `R_SUPERMINI_3V3_LINK = 0 ohm`을 넣어 SuperMini 3V3 rail을 분리 가능하게 한다.

## 5. Sheet 3 - ESP32C3_SUPERMINI

Module:

```text
U_MCU: ESP32-C3 SuperMini module, soldered directly to carrier PCB
```

Power:

```text
3V3 -> R_SUPERMINI_3V3_LINK 0 ohm -> SuperMini 3V3
GND -> SuperMini GND
SuperMini 5V/VIN: NC by default
USB-C on SuperMini remains accessible for upload/debug
```

Final SuperMini pin map:

| Function | SuperMini GPIO | Net | Note |
| --- | --- | --- | --- |
| I2C SDA | GPIO8 | I2C_SDA_3V3 | SuperMini final PCB 기준 |
| I2C SCL | GPIO9 | I2C_SCL_3V3 | SuperMini final PCB 기준 |
| LTE UART RX | GPIO20 | MCU_RX_LTE_TX_3V3 | modem TXD -> MCU RX through level shifter |
| LTE UART TX | GPIO21 | MCU_TX_LTE_RX_3V3 | MCU TX -> modem RXD through level shifter |
| Modem PWRKEY control | GPIO5 | LTE_PWRKEY_CTL_3V3 | drives MOSFET gate |
| Buzzer | GPIO10 | BUZZER_CTL | MOSFET driver input |
| Optional spare | GPIO4 | TP_SPARE_GPIO4 | DNP/test pad |
| Optional spare | GPIO3 | TP_SPARE_GPIO3 | DNP/test pad |

Important:

- 현재 실험 firmware의 최신 빌드는 `xiao_esp32c3`일 수 있다. 하지만 PCB schematic은 SuperMini 최종 탑재 기준이므로 I2C는 GPIO8/GPIO9로 잡는다.
- 회로도에 `XIAO test wiring used GPIO6/GPIO7; final SuperMini uses GPIO8/GPIO9` note를 남긴다.

## 6. Sheet 4 - SIM7080G_MODEM

Module:

```text
U_MODEM: SIM7080G LCC+LGA 77-pin SMT module
```

### 6.1 Power Pins

```text
VBAT pins -> VBAT_MODEM
All GND pins -> solid GND plane
VDD_EXT -> TP_VDD_EXT only, do not use as power source for external loads except level reference if reference design allows
```

### 6.2 PWRKEY

Final bare module circuit:

```text
ESP32 GPIO5 / LTE_PWRKEY_CTL_3V3
  -> R_GATE_PWRKEY 100 ohm~1 k
  -> Q_PWRKEY gate
Q_PWRKEY: small N-MOSFET, source=GND, drain=SIM7080G_PWRKEY
R_GATE_PD: 100 k from gate to GND
SIM7080G_PWRKEY has no external pull-up
TP_PWRKEY on SIM7080G_PWRKEY net
```

Firmware polarity for final bare module:

```text
GPIO HIGH -> MOSFET ON -> PWRKEY pulled LOW
GPIO LOW/input -> PWRKEY released
```

Timing:

```text
Power on: pull PWRKEY LOW for about 1.0~2.0 s
Power off: prefer AT+CPOWD=1; PWRKEY low pulse may be used as fallback
```

Critical note:

- Developer kit/CoreBoard에서 GPIO5에 3.3 V를 1~2초 인가하면 켜지는 동작은 board-level PWRK behavior다.
- Bare SIM7080G module의 PWRKEY는 1.8 V internal pull-up active-low input으로 설계한다.
- 최종 schematic에서는 GPIO가 PWRKEY에 3.3 V를 직접 밀어 넣지 않는다.

### 6.3 UART

SIM7080G UART domain:

```text
SIM7080G TXD/RXD: 1.8 V logic domain
ESP32-C3 UART: 3.3 V logic domain
```

Required circuit:

```text
MCU_TX_LTE_RX_3V3 -> level shift down -> LTE_RXD_1V8 -> SIM7080G RXD
SIM7080G TXD -> LTE_TXD_1V8 -> level shift up -> MCU_RX_LTE_TX_3V3
```

Schematic implementation:

- Use a 2-channel unidirectional level translator or discrete reference transistor circuit.
- Add small series resistors if recommended by selected translator:

```text
R_UART_MCU_TX: 33~100 ohm optional
R_UART_MCU_RX: 33~100 ohm optional
TP_MCU_TX_3V3
TP_MCU_RX_3V3
TP_LTE_TXD_1V8
TP_LTE_RXD_1V8
```

Do not:

- Do not copy developer kit UART electrical level blindly.
- Do not connect ESP32 3.3 V TX directly into SIM7080G RXD unless the exact target module documentation proves tolerance.

### 6.4 Optional Modem Pins

Connect as test pads only:

```text
STATUS -> TP_STATUS
NETLIGHT -> TP_NETLIGHT
DTR -> TP_DTR
1PPS -> TP_1PPS
USB_DP -> TP_USB_DP
USB_DM -> TP_USB_DM
USB_VBUS -> TP_USB_VBUS or NC if routing is too tight
BOOT_CFG -> TP_BOOT_CFG, keep open for normal boot
RESET_N -> NC or TP_RESET_N only, no MCU control in v0.1
```

Rationale:

- SIM7080G 직접 실장 후 debug 접근성이 매우 중요하다.
- 하지만 STATUS/DTR/NETLIGHT까지 MCU에 연결하면 firmware/schematic 복잡도가 늘어난다.

## 7. Sheet 5 - SIM_CARD

SIM holder:

```text
J_SIM: internal nano-SIM holder
SIM_DET not used
```

Connections:

```text
SIM_VDD -> SIM holder VCC
SIM_DATA -> SIM holder IO
SIM_CLK -> SIM holder CLK
SIM_RST -> SIM holder RST
GND -> SIM holder GND/shield
```

Capacitor:

```text
C_SIM_VDD: 100 nF close to SIM holder VCC/GND
```

ESD:

```text
D_SIM_ESD: low-capacitance SIM ESD array on SIM_DATA/SIM_CLK/SIM_RST/SIM_VDD
```

Layout:

```text
SIM holder close to SIM7080G.
SIM_CLK short, no branch, no unnecessary test pad.
No SIM line test pads in v0.1 unless routing/debug absolutely requires them.
```

Note:

- SIM7080G documentation indicates 1.8 V SIM operation. Confirm physical SKT SIM compatibility remains acceptable.

## 8. Sheet 6 - RF_LTE_GNSS

### 8.1 LTE Antenna Path

```text
SIM7080G RF_ANT -> LTE PI matching -> J_LTE_UFL
```

PI matching default:

```text
R/L_SER_LTE: 0 ohm default
C_SHUNT_LTE_IN: DNP
C_SHUNT_LTE_OUT: DNP
D_LTE_ESD: low-capacitance RF ESD, DNP or installed depending part availability
```

### 8.2 GNSS Antenna Path

```text
SIM7080G GNSS_ANT -> GNSS PI matching -> J_GNSS_UFL
```

PI matching default:

```text
R/L_SER_GNSS: 0 ohm default
C_SHUNT_GNSS_IN: DNP
C_SHUNT_GNSS_OUT: DNP
D_GNSS_ESD: low-capacitance RF ESD, DNP or installed depending part availability
```

GNSS active antenna option:

```text
Reserve L/C/R bias tee footprint if active GNSS antenna is selected.
Default v0.1 can leave GNSS bias DNP until antenna part is selected.
TP_GNSS_BIAS if bias rail exists.
```

Connector:

```text
J_LTE_UFL: U.FL/IPEX/MHF compatible SMT connector
J_GNSS_UFL: same connector family as LTE if possible
Silkscreen: LTE, GNSS
```

Layout:

```text
50 ohm controlled impedance.
Prefer 4-layer PCB.
GND plane continuous under RF path.
Via fence near RF trace.
No right-angle RF routing.
Keep LTE and GNSS antenna connectors separated as much as board size allows.
```

## 9. Sheet 7 - SENSOR_4WIRE

Decision:

- 1차 PCB에서 MAX30102/TMP117 IC 직접 실장하지 않는다.
- 기존 sensor modules를 별도 skin-side sensor assembly로 유지한다.
- Main PCB에는 4-wire solder pads만 둔다.

Pads:

```text
J_SENSOR_PAD_1: GND
J_SENSOR_PAD_2: 3V3
J_SENSOR_PAD_3: I2C_SDA_3V3
J_SENSOR_PAD_4: I2C_SCL_3V3
```

I2C pull-up:

```text
R_I2C_SDA: 4.7 k to 3V3, DNP/install option
R_I2C_SCL: 4.7 k to 3V3, DNP/install option
```

Default assembly:

- If sensor modules already have pull-ups, leave R_I2C_SDA/R_I2C_SCL DNP first.
- If I2C rise time or cable length causes errors, install 4.7 k or adjust to 10 k / 2.2 k after measurement.

Test points:

```text
TP_SDA
TP_SCL
TP_SENSOR_3V3
TP_SENSOR_GND
```

Cable:

```text
4-wire soldered cable, target 5~8 cm, avoid >10 cm.
Use strain relief with glue/epoxy/UV resin after soldering.
```

Note:

- Live test showed I2C transient errors can occur during LTE/status operations, but firmware recovered. Schematic should preserve I2C test pads and optional pull-up footprints.

## 10. Sheet 8 - BUZZER

Decision:

- Use MOSFET low-side driver, not direct GPIO drive.

Circuit:

```text
GPIO10 / BUZZER_CTL -> R_BUZZ_GATE 100 ohm~1 k -> Q_BUZZ gate
R_BUZZ_PD 100 k gate to GND
Q_BUZZ source -> GND
Q_BUZZ drain -> buzzer negative
Buzzer positive -> 3V3 or VBAT_SYS depending buzzer rated voltage
```

Default:

- If using existing 3.3 V active buzzer, connect buzzer positive to 3V3.
- If buzzer is louder/works better from Li-Po and rated for 3~5 V, provide solder jumper option:

```text
JP_BUZZ_PWR selects 3V3 or VBAT_SYS
Default: 3V3
```

Protection:

- Active piezo buzzer usually does not need flyback diode.
- If magnetic buzzer is selected, add diode/clamp footprint.

Test:

```text
TP_BUZZER_CTL
```

## 11. Sheet 9 - TEST_DEBUG

Minimum test pads:

```text
TP_GND x multiple
TP_VBAT_IN
TP_VBAT_MODEM near modem
TP_3V3
TP_PWRKEY
TP_MCU_TX_3V3
TP_MCU_RX_3V3
TP_LTE_TXD_1V8
TP_LTE_RXD_1V8
TP_STATUS
TP_NETLIGHT
TP_DTR
TP_1PPS
TP_VDD_EXT
TP_SDA
TP_SCL
TP_SIM_VDD
TP_USB_DP
TP_USB_DM
```

Optional debug header:

```text
J_DEBUG 1x6 pads:
1 GND
2 3V3
3 MCU_TX
4 MCU_RX
5 I2C_SDA
6 I2C_SCL
```

Note:

- SuperMini USB-C must remain physically reachable. This is the primary upload/debug path.
- Do not rely on SIM7080G USB for normal bring-up; keep pads only.

## 12. PCB Layer Decision

Recommendation:

```text
Use 4-layer PCB if budget/manufacturing allows.
```

Reason:

- SIM7080G direct SMT + LTE/GNSS RF + high-current modem bursts are a lot for a first board.
- 4-layer gives a stable GND plane and easier RF/power return path.

Suggested stack:

```text
L1: components / signals / RF
L2: solid GND plane
L3: power + low-speed signals
L4: signals / GND pour
```

If forced to 2-layer:

- Still possible, but RF and modem power margin shrinks.
- Use large GND pours, aggressive via stitching, short RF traces, and keep modem power path very compact.

## 13. Schematic Freeze Checklist

Before KiCad schematic freeze:

```text
[ ] SIM7080G exact footprint verified from hardware design package
[ ] SMT vendor can assemble SIM7080G LCC+LGA 77-pin module
[ ] SuperMini 3V3 direct supply / USB back-power risk checked
[ ] 1.8 V UART level shifter circuit selected
[ ] 3.3 V LDO exact part selected
[ ] Nano-SIM holder exact part selected
[ ] SIM ESD array selected
[ ] U.FL/IPEX connector exact part selected
[ ] LTE antenna selected
[ ] GNSS antenna selected, active/passive/bias decided
[ ] 380 mAh battery pulse discharge capability checked
[ ] Buzzer rated voltage/current checked
[ ] 4-layer PCB cost/lead time accepted
```

## 14. Current Schematic Decision Summary

```text
Battery: TW-602035 1S Li-Po, no charger, no switch, current-link/test pads included
Power: VBAT direct to SIM7080G, separate 3.3 V LDO for MCU/sensors
MCU: ESP32-C3 SuperMini, soldered directly, USB accessible
Modem: SIM7080G bare SMT, PWRKEY through N-MOS open-drain pull-down
UART: 3.3 V <-> 1.8 V level shifting required
SIM: internal nano-SIM holder, SIM ESD, no SIM_DET
RF: LTE and GNSS U.FL/IPEX connectors, PI matching footprints
Sensors: external existing MAX30102/TMP117 modules via 4-wire solder pads
Buzzer: GPIO10 through MOSFET low-side driver
Debug: generous test pads for power, UART, PWRKEY, modem auxiliary pins, I2C
PCB: 4-layer preferred
```
