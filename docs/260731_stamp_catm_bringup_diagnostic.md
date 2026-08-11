# 2026-07-31 Stamp CatM Bring-Up Diagnostic

## 1. Purpose

Verify whether the M5Stack Stamp CatM S003 can boot its SIM7080G modem and
respond to UART AT commands. Separate power, ESP32 UART, USB-UART, wiring, and
modem hardware causes using only available project parts.

## 2. Hardware

- M5Stack Stamp CatM S003
- Main ESP32-C3 SuperMini connected to the laptop
- Spare ESP32-C3 SuperMini used only as a USB 5 V breakout
- SEENGREAT FT232 USB-UART bridge, VCCIO switch set to 3.3 V
- INA219 high-side current sensor module
- USB charger or power-bank source for the spare SuperMini

## 3. Final Diagnostic Wiring

Power path:

```text
USB source
  -> spare SuperMini USB
  -> spare SuperMini 5V
  -> INA219 VIN+
  -> INA219 VIN-
  -> Stamp CatM 5V
```

UART path:

```text
Stamp TX -> FT232 RXD
Stamp RX <- FT232 TXD
Stamp GND <-> FT232 GND
```

INA219 logic:

```text
INA219 VCC -> main ESP32-C3 3.3V
INA219 GND -> common GND
INA219 SDA -> main ESP32-C3 GPIO8
INA219 SCL -> main ESP32-C3 GPIO9
```

All grounds were common. Stamp `3V3`, `Rf`, FT232 `VCCIO`, `RTS`, and `CTS`
were not connected.

## 4. Tests and Observations

### 4.1 ESP32 UART test

The Stamp was first connected to the main ESP32-C3 UART:

```text
Stamp TX -> ESP32-C3 GPIO20 (RX)
Stamp RX <- ESP32-C3 GPIO21 (TX)
```

Observed line states:

```text
GPIO20 / Stamp TX: LOW for all samples, no transitions
GPIO20 internal pull-up test: remained LOW
GPIO21 / Stamp RX: HIGH when driven by ESP32 UART
```

Removing the Stamp TX wire allowed GPIO20 to follow its internal pull-up and
pull-down. Therefore, the ESP32 GPIO was healthy and the Stamp side was holding
its TX line LOW.

AT probes at 115200, 9600, 19200, 38400, 57600, 74880, and 230400 baud
returned no response.

### 4.2 Dedicated USB 5 V source

The Stamp was powered separately through the 5 V pin using a spare SuperMini
connected to a USB source. The main ESP32 remained connected to the laptop,
with common ground.

Result:

```text
Stamp TX remained LOW
No UART AT response
No improvement over the original USB power arrangement
```

### 4.3 FT232 direct UART test

The main ESP32 UART was removed from the modem path. FT232 was connected
directly to the Stamp with 3.3 V UART logic.

Windows detected the bridge as:

```text
FT232: COM5
Main ESP32-C3: COM6
```

Direct AT probes were performed at:

```text
9600
19200
38400
57600
74880
115200
230400
460800
921600
```

Each baud was tested three times. No received byte was observed at any baud.

### 4.4 FT232 loopback verification

FT232 TXD and RXD were connected directly together. A 33-byte test payload was
transmitted.

```text
Transmitted bytes: 33
Received bytes: 33
Exact content match: true
```

This confirmed the laptop, COM5 driver, FT232 transmitter, and FT232 receiver
were functioning.

### 4.5 INA219 power measurement

INA219 was detected on the main ESP32-C3 I2C bus:

```text
I2C address: 0x40
```

The Stamp input was measured for 15 seconds while FT232 transmitted `AT`
twelve times.

Measured result:

```text
Samples: 149
Stamp load voltage minimum: 5.044 V
Stamp load voltage average: 5.083 V
Stamp load voltage maximum: 5.088 V

Input current minimum: 11.0 mA
Input current average: 12.3 mA
Input current maximum: 32.5 mA

FT232 AT commands sent: 12
FT232 received bytes: 0
INA219 overflow: none
```

The current calculation assumes the INA219 module uses an `R100` 0.1-ohm
shunt. The voltage result does not depend on this current-shunt assumption.

### 4.6 Component-level voltage diagnosis

The Stamp was re-tested with a multimeter and the ESP32-C3 UART.

External Stamp pins:

```text
5V: 5.27 V
3V3: 3.415 V
TX: 3.398 V
RX: 3.25 V
```

SIM7080 package-side points:

```text
VDD_EXT, pin 40: 1.803 V
STATUS, pin 42: 1.803 V
UART1_TXD, pin 1: 1.803 V
UART1_RXD, pin 2: 1.72 V
```

Grounding the external Stamp RX input pulled SIM7080 UART1_RXD to about
1.5 mV. This verified the Stamp RX level-shifter path.

### 4.7 UART dynamic tests

The ESP32-C3 firmware was extended with controlled UART line tests.

`UARTTXTEST` drove ESP32-C3 GPIO21 LOW and HIGH in two-second intervals.
The SIM7080 UART1_RXD package pin followed between approximately 0 V and
1.7-1.8 V.

`UARTRXTEST` counted transitions on ESP32-C3 GPIO20 while transmitting AT
commands at several baud rates. Results included:

```text
115200 baud:
  596 and 711 detected edges
  72 and 90 decoded bytes

Other tested baud rates:
  about 130 detected edges
  zero valid decoded bytes
```

`LTEPROBE` then received:

```text
AT   -> OK
ATE0 -> OK
```

This proved that the modem had booted and that both UART directions worked at
115200 baud. The earlier no-response result was not a modem hardware failure.

### 4.8 SIM orientation and detection

With the SIM in the initial orientation, network commands reported SIM
detection errors. The SIM was rotated 180 degrees and the modem was fully
restarted.

After restart:

```text
AT+CPIN? -> READY
AT+CCID  -> ICCID read successfully
AT+CIMI  -> IMSI read successfully
```

The current, rotated SIM orientation is the verified orientation. SIM identity
values are intentionally omitted from this shared record.

### 4.9 Antenna and network scan

The LTE antenna was connected to the Stamp IPEX connector before RF tests.

Confirmed modem configuration:

```text
Model response: SIMCOM_SIM7080
Firmware: 1951B17SIM7080
Initial network mode readback: automatic, CNMP=2
Preferred LPWA mode: Cat-M only
Cat-M bands: B1/B2/B3/B4/B5/B8/B12/B13/B14/B18/B19/B20/
             B25/B26/B27/B28/B66/B85
```

Observed signal strength ranged from `CSQ 22` to `CSQ 31`. The antenna and RF
receive path therefore worked.

An uninterrupted full operator scan returned:

```text
45005, SKT: available
45012, SKT IoT PLMN: available
45008, KT: forbidden
45006, LG U+: forbidden
45030: forbidden
```

### 4.10 Network registration attempts

Automatic registration remained unsuccessful:

```text
AT+COPS?  -> mode 0, automatic
AT+CEREG? -> 0,2 while searching
AT+CGATT? -> 0
AT+CPSI?  -> NO SERVICE
```

Manual registration attempts were then made:

```text
AT+COPS=1,2,"45005",7 -> CME ERROR: no network service
AT+COPS=1,2,"45012",7 -> CME ERROR: no network service
AT+CEREG?              -> 0,3, registration denied
```

The modem was returned to automatic operator selection:

```text
AT+COPS?  -> 0
AT+CEREG? -> 0,3
AT+CSQ    -> 23,99
```

The modem could see both SKT PLMNs with strong RF signal, but the initial
registration attempt was rejected. This was the pre-remediation result, not
the final result.

## 5. Reference Values

The official M5Stack Stamp CatM documentation specifies:

```text
Supply and standby: DC 5 V at 46 mA
Network access current: DC 5 V at 71 mA
UART: 115200 baud, 8N1
```

The official schematic shows:

```text
5 V input -> onboard GM9308/HM8089 converter -> 3.3 V rail
3.3 V rail -> SIM7080G VBAT
Red power LED -> 3.3 V rail
SIM7080G PWRKEY pin 39 linked to STATUS pin 42 through R4, 0 ohm
```

## 6. Final Result

```text
USB 5 V source: PASS
Stamp input voltage: PASS
Stamp onboard 3.3 V rail: PASS
INA219 measurement path: PASS
ESP32 UART GPIO: PASS
FT232 direct UART: PASS
FT232 loopback: PASS
Stamp UART level shifting: PASS
SIM7080 modem boot: PASS
UART AT communication at 115200 8N1: PASS
SIM detection after orientation correction: PASS
LTE antenna and RF reception: PASS
Korean operator scan: PASS
Initial SKT network registration: FAIL - registration denied
SKT network registration after stored-setting cleanup: PASS
```

The initial modem-fault conclusion was disproved by package-pin voltage checks,
controlled UART transition tests, and successful AT responses. The Stamp
hardware is operational.

`CEREG 0,3` was real, but the later successful registration proves that it did
not establish a carrier-account fault. The Stamp had stale or unsuitable
stored modem settings. A controlled cleanup and Cat-M configuration resolved
the registration without changing the SIM or asking the carrier to modify the
subscription.

## 7. Decision and Next Action

The Stamp CatM is electrically and operationally usable and remains the compact
LTE module candidate. Replacement is not justified by the test results.

Priority actions:

1. Preserve the verified Cat-M initialization settings in firmware.
2. Perform one controlled SMS transmission test with the Stamp.
3. Repeat battery-only operation with Stamp, ESP32-C3, sensors, and GPS.
4. Continue BE-220 GPS integration and enclosure work.
5. Contact the carrier only if registration fails again after the verified
   initialization sequence.

## 8. Firmware Changes Made for Diagnosis

The integrated firmware gained these manual diagnostic commands:

```text
I2CSCAN
POWERMON [seconds]
UARTTXTEST
UARTRXTEST
ATCMD <command>
ATLONG <seconds> <command>
```

`POWERMON` reads INA219 bus and shunt voltage, prints approximately 10 samples
per second, and reports minimum, average, and maximum load voltage and current.
`ATLONG` allows uninterrupted long-running modem commands such as full operator
scans. No SMS was transmitted during this diagnostic.

## 9. Milestone Status

```text
Stamp power bring-up: COMPLETE
Stamp UART bring-up: COMPLETE
SIM orientation/detection: COMPLETE
LTE antenna/RF validation: COMPLETE
Korean operator visibility: COMPLETE
SKT Cat-M registration on PLMN 45005: COMPLETE
Packet attachment: COMPLETE
Stamp SMS test: PENDING CONTROLLED SINGLE-SMS TEST
```

## 10. Comparison With the Previously Successful Board

The previously tested SIM7080 developer board was rechecked in the earlier
development records. It was not operating as a different Cat-1 radio:

```text
Previous developer board
ATI: R1951.07
AT+CNMP?: 38, LTE only
AT+CMNB?: 1, Cat-M only
Registered PLMN: 45005, SKT
AT+CEREG?: registered
AT+CGATT?: 1
SMS transmission: PASS

Current Stamp CatM
ATI: R1951.07
Initial AT+CNMP?: 2, automatic
AT+CMNB?: 1, Cat-M only
Visible PLMNs: 45005 and 45012
Initial AT+CEREG?: 0,3, registration denied
Initial PDP context 1 APN: literal placeholder "YourAPN"
After cleanup AT+CNMP?: 38, LTE only
After cleanup AT+CEREG?: 0,1, home network registered
After cleanup AT+CGATT?: 1
After cleanup AT+CPSI?: LTE CAT-M1, Online, 450-05, Band 5
SMS transmission: pending controlled single-SMS test
```

Therefore, no evidence currently shows a Cat-1 versus Cat-M protocol mismatch.
Both tests used the SIM7080 firmware family and Cat-M mode. The same SIM
registered on the Stamp after its local modem configuration was normalized.
The earlier IMEI/OMD/account hypothesis is therefore not supported by the final
test result.

Source records:

```text
260708_hardware_bringup_log.md
260709_lte_sms_integrated_firmware_log.md
```

## 11. Carrier Inquiry Checklist

Contact the company that actually issued the SIM. If the line is an SKT-network
MVNO line, its MVNO customer center must check the subscription and device
authorization first; the SKT retail customer center may not be able to modify
an MVNO account.

Prepare:

```text
Subscriber phone number
SIM ICCID from AT+CCID
SIM IMSI from AT+CIMI (starts with 45005)
Stamp modem IMEI from AT+CGSN
Module: M5Stack Stamp CatM, SIMCom SIM7080G
Mode: LTE Cat-M1, CNMP=38, CMNB=1
Supported Korean LTE bands used by the module: B1/B3/B5
```

Suggested Korean inquiry:

```text
SKT망으로 개통한 유심을 SIMCom SIM7080G 기반 LTE Cat-M1 모듈에서
사용하려고 합니다. 같은 유심은 기존 SIM7080 개발보드에서 SKT 45005에
등록되어 SMS 송신까지 성공했지만, 새 Stamp CatM 모듈에서는 CPIN READY,
CSQ 23~31, SKT 45005/45012 검색까지 정상인데 CEREG 0,3으로 등록이
거절됩니다.

새 모듈 IMEI를 OMD 또는 M2M/LTE 모뎀 단말로 등록해야 하는지,
유심보호·기기변경 제한이나 IMEI 화이트리스트가 걸려 있는지,
현재 요금제가 LTE Cat-M1 단말과 SMS를 허용하는지 확인 부탁드립니다.
망 로그에 남은 registration denied의 reject cause도 확인 가능하면
부탁드립니다. 이 회선은 45005와 45012 중 어느 PLMN으로 등록해야 하며,
등록 후 사용할 APN도 알려주세요.
```

Questions that require an explicit carrier answer:

1. Is LTE Cat-M1 service allowed on the current subscription and SIM?
2. Does the Stamp IMEI require OMD, M2M, or LTE-modem registration?
3. Is USIM protection, terminal-change blocking, or an IMEI allowlist active?
4. What network reject cause was recorded for `CEREG stat 3`?
5. Should the device register on PLMN `45005` or IoT PLMN `45012`?
6. Can the current line send SMS from a Cat-M1 modem?
7. Which APN should be used after successful registration?

## 12. SKT Contact Information Checked on 2026-07-31

```text
SKT mobile: 114, free
Other phones: 080-011-6000, free
General number: 1599-0011, paid
Human counselor: weekdays 09:00-18:00
Lunch 12:00-13:00: emergency specialist service only
AI/ARS: 24 hours
```

The official T world LTE-M pages state that Cat-M1 and Cat.1 special-purpose
devices are supported by LTE-M plans and that device/plan inquiries are handled
through SKT branches, the customer center, or an IoT/M2M specialist agency.

Official references:

- https://m.tworld.co.kr/customer/svc-info/service/detail?code=C00038
- https://m.tworld.co.kr/product/callplan?prod_id=NA00005661
- https://m.tworld.co.kr/customer/svc-info/service/detail?code=C00023

Because the tested SIM was opened through an SKT-network MVNO, the first call
should be to the MVNO that issued the line. SKT's customer center can explain
network requirements, but it may be unable to change an MVNO subscription or
register its device profile directly.
