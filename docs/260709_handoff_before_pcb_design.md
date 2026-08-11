# 회로 설계 인수인계서

Date: 2026-07-09

Project: GCDP Heat Risk Alert Wristband

Purpose: 회로 설계 채팅으로 넘어가기 전, 지금까지 직접 실험한 사실, 검증 결과, 판단 근거, 확정 결정, 미해결 리스크를 끊기지 않게 전달한다.

## 0. 최종 요약

1차 PCB는 완제품형 칩 통합 PCB가 아니라, 검증된 모듈들을 얹는 carrier board 성격으로 진행한다.

최종 결정:

```text
MCU: ESP32-C3 SuperMini 모듈을 1차 PCB에 그대로 얹는다.
MCU 칩 단위 통합: 1차 PCB에서는 하지 않는다.
GPS/GNSS: 반드시 넣는다.
통신 모듈: 1차 PCB에 통합한다. 기존 대형 devkit은 최종 형상에 부적합하므로 기능 검증용으로만 본다.
배터리: Li-Po 사용.
충전회로: 1차 PCB에는 넣지 않는다.
```

현재까지 보드 기반 기능 검증은 1차 완료로 본다. 남은 핵심 작업은 기능 검증이 아니라 회로 설계 결정이다.

가장 중요한 설계 쟁점:

```text
1. GNSS 포함 통신 모듈 선정
2. SIM7080G 전원 레일 설계
3. Li-Po에서 MCU와 통신 모듈에 어떤 전압을 공급할지 결정
4. SIM7080G PWRKEY/PWRK 회로를 devkit 기준이 아니라 최종 모듈 reference circuit 기준으로 설계
5. MAX30102/TMP117의 실제 착용 구조 반영
```

## 1. 작업 공간 정책

작업 공간 정책은 다음과 같이 확정했다.

```text
N: 드라이브 / NAVER MYBOX cloud
-> 문서, 로그, 설계 메모 보관

C:\Users\sukhe\Desktop\GCDP_codex_temp
-> 임시 개발, PlatformIO build, firmware, capture, 기타 실험 작업
```

활성 펌웨어 작업 경로:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware
```

문서/로그 경로:

```text
N:\Own\01_YU\프로젝트 및 활동\2025-2_GCDP(Thailand-Korea)\GCDP_하계(Korea)\project
```

기존에 N 드라이브 쪽에 생겼던 `project\firmware`는 cloud sync와 build/cache 문제를 피하기 위해 삭제했다. 프로젝트 완료 전까지 N 드라이브에는 문서/기록만 둔다.

## 2. 기존 문서와 로그

기존 로그:

```text
N:\Own\01_YU\프로젝트 및 활동\2025-2_GCDP(Thailand-Korea)\GCDP_하계(Korea)\project\logs\2026-07-08_hardware_bringup_log.md
N:\Own\01_YU\프로젝트 및 활동\2025-2_GCDP(Thailand-Korea)\GCDP_하계(Korea)\project\logs\2026-07-09_lte_sms_integrated_firmware_log.md
```

회로 설계 전 메모:

```text
N:\Own\01_YU\프로젝트 및 활동\2025-2_GCDP(Thailand-Korea)\GCDP_하계(Korea)\project\design\pre_pcb_circuit_notes.md
```

본 인수인계서:

```text
N:\Own\01_YU\프로젝트 및 활동\2025-2_GCDP(Thailand-Korea)\GCDP_하계(Korea)\project\design\handoff_before_pcb_design_2026-07-09.md
```

## 3. 개발 환경

PlatformIO 기반으로 진행했다.

설치/사용 확인:

```text
PlatformIO Core
VS Code PlatformIO IDE extension
VS Code C/C++ extension
Espressif32 PlatformIO platform
ESP32 / ESP32-C3 toolchains
```

PlatformIO 환경:

```text
esp32dev
xiao_esp32c3
esp32c3_supermini
```

현재 `platformio.ini`에는 다음 env가 존재한다.

```ini
[env:xiao_esp32c3]
board = seeed_xiao_esp32c3
build_flags =
  -D I2C_SDA_PIN=6
  -D I2C_SCL_PIN=7
  -D LTE_RX_PIN=20
  -D LTE_TX_PIN=21
  -D BUZZER_PIN=10
  -D LTE_PWRK_PIN=5

[env:esp32c3_supermini]
board = esp32-c3-devkitm-1
build_flags =
  -D I2C_SDA_PIN=8
  -D I2C_SCL_PIN=9
  -D LTE_RX_PIN=20
  -D LTE_TX_PIN=21
  -D BUZZER_PIN=10
  -D LTE_PWRK_PIN=5
```

주의:

- 대화 중 XIAO ESP32-C3와 ESP32-C3 SuperMini 비교가 있었다.
- 최종 1차 PCB 결정은 `ESP32-C3 SuperMini 모듈 탑재`이다.
- 펌웨어에는 XIAO와 SuperMini env가 둘 다 들어 있다.
- 회로 설계 채팅에서는 SuperMini 실제 핀맵 기준으로 다시 고정해야 한다.

## 4. MCU 관련 판단

### 4.1 MCU 칩 단위 통합을 하지 않는 이유

1차 PCB에서 MCU를 ESP32-C3 칩 단위로 직접 넣지 않기로 결정했다.

판단 근거:

- ESP32-C3 칩 단위 설계는 USB 회로, EN/BOOT 회로, 3.3 V regulator, crystal/flash/antenna/RF layout, 업로드/디버깅 회로까지 고려해야 한다.
- RF/안테나 layout과 인증/간섭 이슈가 커진다.
- 현재 프로젝트 단계에서는 기능 검증과 1차 착용형 prototype이 우선이다.
- 일정과 디버깅 난이도를 고려하면 MCU 모듈을 carrier PCB에 얹는 것이 현실적이다.

결정:

```text
1차 PCB: ESP32-C3 SuperMini 모듈을 그대로 얹는다.
2차 이후: 필요하면 ESP32-C3 module/chip integration 검토.
```

### 4.2 SuperMini vs XIAO 판단

XIAO ESP32-C3로 바꾸는 데 펌웨어 시간은 크지 않다. 같은 ESP32-C3 계열이고 PlatformIO env도 이미 있다.

하지만 최종 판단:

```text
지금은 SuperMini 유지.
```

판단 근거:

- 현재 주요 병목은 MCU가 아니라 통신 모듈, GPS/GNSS, 전원부다.
- SuperMini는 작고 1차 PCB 크기 감각에 더 가깝다.
- 이미 ESP32-C3 계열에서 센서/부저/LTE 통신 검증이 진행되었다.
- XIAO로 바꾸면 큰 기능 이득보다 핀맵/배선 재확인 비용이 생긴다.

## 5. 센서 검증 결과

### 5.1 TMP117 온도 센서

사용 핀:

```text
VIN
GND
SCL
SDA
INT
ADDR
```

실제 사용:

```text
VIN -> 3V3
GND -> GND
SDA -> I2C SDA
SCL -> I2C SCL
INT -> 미사용
ADDR -> 미사용
```

I2C address:

```text
0x48
```

검증 결과:

- I2C scan에서 `0x48` 감지.
- 온도 읽기 성공.
- 예시 측정값:

```text
32.84 C
33.01 C
33.12 C
33.21 C
```

통합 firmware에서는 TMP117 값이 비정상 범위를 벗어나면 reject하도록 했다.

현재 적용한 sanity range:

```text
-40 C to 125 C
```

설계 주의:

- 피부온도를 의미 있게 보려면 회로보다 기구적 열접촉 구조가 중요하다.
- PCB 위에만 올리면 주변 온도/보드 열 영향을 받을 수 있다.
- 최종 착용 구조에서 TMP117 위치를 피부 쪽에 가깝게 배치해야 한다.

### 5.2 MAX30102 심박/착용 감지 센서

사용 핀 라벨:

```text
GND
D
IRO
INT
VIN
SDA
SCL
GND
```

실제 사용:

```text
VIN -> 3V3
GND -> GND
SDA -> I2C SDA
SCL -> I2C SCL
INT/IRO/D -> 미사용
```

I2C address:

```text
0x57
```

Part ID:

```text
0x15
```

검증 결과:

- I2C scan에서 `0x57` 감지.
- RED/IR raw data 읽기 성공.
- 손가락 올린 상태에서 RED/IR 값 증가 확인.
- 예시 값:

```text
red = 104552
ir  = 130099
```

연속 sampling 예시:

```text
Sample rate: about 49.98 Hz
RED range: 107248 to 107786
IR range: 133719 to 134472
```

초기 BPM 추정:

```text
Mean BPM: 115.2
Median BPM: 115.4
```

해석:

- BPM 추정은 waveform 검증용 crude peak detector였다.
- 최종 심박 알고리즘으로 확정한 것이 아니다.
- 현재 중요한 검증은 optical sensor가 살아 있고 RED/IR waveform이 나온다는 점이다.

착용 감지:

```text
wear on threshold  = 80000
wear off threshold = 60000
confirmation count = 3
```

사용자 확인:

- 손가락/접촉 감지 시 부저 반응.
- delay는 약 1~2초 내외로 느껴졌고 현재 prototype 단계에서는 만족.

설계 주의:

- MAX30102는 회로보다 물리 배치가 중요하다.
- 피부 밀착, 차광, 압력, 센서 창 높이, 주변광 차단을 고려해야 한다.
- PCB에 고정할지, 별도 작은 sensor board로 빼서 피부 접촉부에 둘지 설계 단계에서 결정해야 한다.

### 5.3 TMP117 + MAX30102 동시 I2C 검증

두 센서는 같은 I2C bus에서 동시에 동작했다.

동시 scan 결과:

```text
0x48 TMP117
0x57 MAX30102
```

결론:

```text
TMP117과 MAX30102는 하나의 I2C bus에 같이 연결 가능.
```

회로 설계 주의:

- pull-up 저항 값을 최종 PCB에서 정해야 한다.
- breakout module에 이미 pull-up이 있는 경우 prototype에서는 중복 pull-up이 있을 수 있다.
- 1차 PCB에서 센서를 직접 실장한다면 SDA/SCL에 3.3 V pull-up을 명확히 넣어야 한다.

## 6. 부저 검증 결과

부저 연결:

```text
ESP32-C3 GPIO10 -> buzzer signal
GND -> GND
```

검증 결과:

- GPIO10으로 부저 ON/OFF 확인.
- 착용 감지와 연동해 접촉 시 부저 ON, 제거 시 OFF 테스트 완료.
- 사용자 체감 delay는 prototype 단계에서 만족.

회로 설계 주의:

- active buzzer가 저전류이면 GPIO 직접 구동 가능할 수 있다.
- 부저 전류가 GPIO safe limit을 넘으면 transistor driver를 넣어야 한다.
- wearable 용도이므로 소리 크기/소비전류/사용자 불편감을 같이 고려해야 한다.

## 7. 통신 모듈 실험 결과

### 7.1 작은 SIM7080G 6핀 모듈

핀 라벨:

```text
5V
3V3
GND
Rf
Tx
Rx
```

실험:

- 5V, GND, TX/RX를 ESP32-C3 UART에 연결.
- TX/RX direction 바꿔봄.
- 여러 baud scan.

결과:

```text
LED는 켜졌으나 AT 응답 없음.
```

가능 원인:

- 별도 power key 필요.
- UART가 비활성.
- 전원 조건 문제.
- 보드 내부 동작 조건 불명.

결론:

```text
해당 작은 6핀 모듈은 bring-up 실패. 최종 설계 근거로 사용하지 않는다.
```

### 7.2 SIM7080G developer kit

핀 라벨:

```text
DTR
GND
VDD
PWRK
UTX
URX
GND
```

추가 구성:

- micro USB connector
- 4-pin connector
- 3개 antenna connector

PCB marking:

```text
CAT-1-A76-80X
NB-SIM-7080
SIM-800/868
```

초기 USB test:

- micro USB 연결 시 LED는 켜졌지만 Windows COM port는 새로 잡히지 않았다.
- cable이 power-only일 가능성 또는 USB-UART 미노출 가능성이 있었다.

ESP32-C3 UART 경유 test:

동작한 UART:

```text
SIM7080G DevKit UTX -> ESP32-C3 GPIO20 or GPIO21 실험 중 최종 firmware는 LTE_RX_PIN=20 사용
SIM7080G DevKit URX -> ESP32-C3 GPIO21 or GPIO20 실험 중 최종 firmware는 LTE_TX_PIN=21 사용
GND common
baud = 115200
```

주의:

- 초기 로그에는 배선 방향을 바꿔가며 찾은 흔적이 있다.
- 최종 통합 firmware 기준은 `LTE_RX_PIN=20`, `LTE_TX_PIN=21`.
- 회로 설계 시에는 최종 채택 통신 모듈 datasheet의 TX/RX naming 기준으로 다시 교차 연결해야 한다.

AT 성공:

```text
AT -> OK
ATI -> R1951.07 / OK
AT+CPIN? -> +CPIN: READY
```

## 8. SIM / SKT / SMS 검증

SIM:

- 물리 SIM을 SKT망으로 개통 후 사용.
- SIM 전화번호 및 수신 테스트 전화번호는 대화에 존재하지만 문서에는 개인정보 보호를 위해 그대로 적지 않는다.

SKT network:

성공 PLMN:

```text
45005
```

실패한 PLMN:

```text
45012 -> no network service
```

안테나 전:

```text
AT+CSQ -> 99,99
AT+CEREG? -> 0,2
```

안테나 후:

```text
AT+CSQ -> 23 to 26,99
AT+CEREG? -> registered
AT+COPS? -> 45005 or SKTelecom
AT+CGATT? -> 1
```

Cat-M mode:

```text
AT+CMNB=1
AT+CNMP? -> +CNMP: 38
AT+CBAND? -> ALL_MODE
```

SMS:

실제 SMS 전송 성공.

테스트 메시지:

```text
GCDP SIM7080G SMS TEST
```

사용자 확인:

```text
문자 수신 성공.
```

중요 안전 이슈:

- 초기 test firmware에서 one-shot flag가 RAM에만 있어서 reset/re-upload 후 같은 SMS가 다시 발송되었다.
- 사용자가 SMS 500건 한도를 우려했다.
- 이후 automatic SMS는 모두 막았다.

현재 정책:

```text
SMS는 수동 serial command로만 전송.
부팅, reset, LTE registration, state change만으로는 SMS 전송 금지.
```

현재 SMS 명령:

```text
SMS <number> <message>
```

회로 설계와 직접 관련된 결론:

- SIM7080G 계열로 SKT망 registration과 SMS 송신이 실제로 가능함을 확인했다.
- LTE antenna는 필수다.
- 통신 모듈 전원 안정성이 매우 중요하다.

## 9. GNSS/GPS 검증

GPS/GNSS는 프로젝트 최종 요구사항으로 확정했다.

SIM7080G developer kit에서 GNSS command path는 검증되었다.

추가한 firmware command:

```text
GNSS
GNSSOFF
```

관측 sequence:

```text
AT+CGNSPWR=1
OK
AT+CGNSINF
+CGNSINF: 1,,,36.050000,127.330003,-19.819,,,1,,0.1,0.1,0.1,,,,191841.8,6000.0
OK
[gnss] coordinate acquired
```

GNSS off:

```text
AT+CGNSPWR=0
OK
```

해석:

- `+CGNSINF: 1,1` 형태가 아니어도 latitude/longitude field가 유효하면 coordinate acquired로 처리하도록 firmware 수정했다.
- 실내/창가/안테나 조건에 따라 GNSS fix reliability는 추가 확인 필요.
- 최종 PCB에는 LTE antenna와 GNSS antenna를 별도로 고려해야 한다.

중요 부품 판단:

### M5Stack U137 CatM+GNSS Unit

특징:

- SIM7080G 기반.
- Cat-M/NB-IoT + GNSS 지원.
- LTE/GNSS dual SMA antenna.
- MicroSIM.
- 외부 DC 9-24 V 또는 HY2.0 5 V.
- UART 3.3 V TTL.
- GNSS 지원.
- 크기 약 `62 x 40 x 18.4 mm`.

판단:

- 기능적으로는 통신 + GNSS가 모두 들어 있어 prototype 검증에 유리.
- 하지만 wristband 최종/1차 PCB 관점에서는 크다.

### M5Stamp CAT-M Module S003

특징:

- SIM7080G 기반.
- Cat-M/NB-IoT 지원.
- UART 115200 8N1.
- MicroSIM.
- IPEX LTE antenna.
- 5 V 입력.
- 제품 크기 약 `30.1 x 20.1 x 5.5 mm`.
- GNSS 없음.

판단:

- 작고 1차 PCB에 올리기 좋다.
- 그러나 GPS/GNSS가 없다.
- 최종 요구사항이 GPS 필수이므로 S003 단독은 부적합.

최종 GPS 관련 결정:

```text
GPS/GNSS는 반드시 넣는다.
S003 단독 사용은 안 된다.
통신 모듈은 PCB에 통합하되, GNSS 지원 모듈이거나 별도 GNSS 모듈을 추가해야 한다.
```

회로 설계 채팅에서 가장 먼저 정해야 할 점:

```text
1. SIM7080G 기반 GNSS 지원 모듈을 PCB에 올릴 것인가?
2. 아니면 S003 같은 작은 Cat-M module + 별도 GNSS module 조합으로 갈 것인가?
```

현재 판단 우선순위:

- 크기만 보면 S003이 매력적.
- 하지만 GPS 필수이므로 S003 단독은 탈락.
- 통신+GNSS 통합 모듈을 찾는 것이 회로 단순성 측면에서 유리하다.
- 크기를 더 줄이려면 Cat-M module과 GNSS module을 분리해야 한다.

## 10. PWRK/PWRKEY 전원 제어 실험

SIM7080G developer kit에서 PWRK 관련 실험을 했다.

초기 관찰:

- PWRK를 GND에 1~2초 연결했다가 떼면 green LED가 깜빡이며 살아난 적이 있다.
- 이후 firmware에서 `MODEMPWR` command를 추가해 ESP32-C3 GPIO5로 PWRK를 LOW pulse하도록 했다.

추가 firmware command:

```text
MODEMPWR [ms]
MODEMOFF
PWRKHIGH [ms]
LTEPROBE
```

`MODEMOFF`:

```text
AT+CPOWD=1
```

결과:

- `AT+CPOWD=1`로 modem power-down은 확인했다.
- power-down 후 AT no response 상태 확인.
- `MODEMPWR` LOW pulse로는 관찰 시간 내 modem이 확실히 다시 살아나는 것을 확인하지 못했다.
- `PWRKHIGH 3000`은 modem이 켜져 있을 때 `NORMAL POWER DOWN`을 유발했다.
- 이후 green LED가 깜빡이는 상태에서는 실제로 modem이 살아 있었고, re-upload/reset 및 UART probe 후 AT 통신이 복구되었다.

중요 해석:

- developer kit의 PWRK 핀은 단순한 active-low button이라고 단정하면 안 된다.
- HIGH pulse가 power-down을 유발하는 상황도 관찰되었다.
- external micro-USB power 또는 보조배터리 전원 공급 후 modem 안정성이 좋아졌다.
- no response 상태가 항상 modem death를 의미하지는 않았다. reset/timing/UART synchronization issue일 수 있다.
- 최종 PCB에서 devkit의 PWRK behavior를 그대로 복사하면 안 된다.

최종 설계 지침:

```text
PWRKEY/PWRK는 최종 선택한 SIM7080G module datasheet/reference circuit 기준으로 설계한다.
developer kit behavior는 참고 기록으로만 본다.
```

예상 회로 원칙:

- MCU GPIO가 PWRKEY를 직접 push-pull로 강하게 drive하지 않도록 한다.
- 일반적으로 open-drain/open-collector style low-side control 또는 reference circuit 권장 방식을 따른다.
- power-on/off timing은 module datasheet timing을 따른다.

## 11. 전원 관련 관찰과 최종 결정

관찰:

- SIM7080G developer kit는 전원 조건에 민감했다.
- antenna 연결 전에는 신호가 사실상 없었다.
- 외부 micro-USB/보조배터리 전원 공급 후 modem 안정성이 좋아졌다.
- 통신 모듈은 MCU 3.3 V에서 직접 공급하면 안 된다.
- LTE attach/SMS/GNSS 시 전류 spike를 고려해야 한다.

최종 결정:

```text
1차 PCB 전원: Li-Po 사용.
1차 PCB 충전회로: 넣지 않는다.
```

이 결정의 의미:

- PCB에 charging IC, USB charging path, power-path management를 넣지 않는다.
- Li-Po는 외부 충전 또는 분리 충전을 전제로 해야 한다.
- 단, 안전을 위해 battery protection이 내장된 Li-Po pack을 쓰는지 확인해야 한다.
- PCB에는 최소한 reverse/overcurrent/전원 switch/fuse/protection 필요 여부를 회로 설계에서 판단해야 한다.

필수 설계 과제:

```text
Li-Po nominal 3.7 V, full 4.2 V, cutoff 약 3.0 V대
ESP32-C3 SuperMini 입력 요구 전압
통신 모듈 입력 요구 전압
SIM7080G peak current
5 V boost converter 필요 여부
3.3 V regulator 필요 여부
bulk capacitor 용량과 배치
```

전원 관련 강한 권고:

- SIM7080G 전원 rail 근처에 bulk capacitor를 둔다.
- LTE module power trace는 짧고 넓게 잡는다.
- MCU와 sensor 3.3 V rail이 modem current spike에 흔들리지 않게 분리/필터링을 고려한다.
- 1차 PCB라도 power test point를 반드시 둔다.

## 12. 현재 통합 펌웨어 상태

파일:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware\src\main.cpp
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware\platformio.ini
C:\Users\sukhe\Desktop\GCDP_codex_temp\firmware\include\pins.h
```

구현된 state:

```text
BOOT
NOT_WORN
NORMAL
SUSPECT
ALARM
EMERGENCY_READY
ERROR
```

상태 의미:

```text
BOOT -> 초기화
NOT_WORN -> 착용/접촉 없음
NORMAL -> 착용 감지, 위험 조건 없음
SUSPECT -> 위험 의심 조건 시작
ALARM -> 위험 조건 지속
EMERGENCY_READY -> 알림 전송 준비 상태
ERROR -> sensor failure 지속
```

현재 주요 parameter:

```text
SENSOR_SAMPLE_MS = 100
TEMP_SAMPLE_MS = 1000
STATUS_PRINT_MS = 1000
LTE_CHECK_MS = 60000
SUSPECT_CONFIRM_MS = 10000
ALARM_COUNTDOWN_MS = 30000
WEAR_ON_IR_THRESHOLD = 80000
WEAR_OFF_IR_THRESHOLD = 60000
WEAR_CONFIRM_COUNT = 3
TEMP_SUSPECT_C = 38.0
```

현재 serial commands:

```text
HELP
STATUS
LTE
LTEPROBE
GNSS
GNSSOFF
MODEMPWR [ms]
PWRKHIGH [ms]
MODEMOFF
ALARM
CANCEL
SMS <number> <message>
```

SMS safety:

```text
Automatic SMS disabled.
Manual command only.
```

센서 안정성 patch:

- TMP117 transient fail이 몇 번 발생해도 즉시 ERROR로 가지 않도록 했다.
- MAX30102 transient fail도 즉시 ERROR로 가지 않도록 했다.
- 짧은 I2C failure에는 last valid reading을 유지한다.
- 지속 failure일 때만 ERROR.

## 13. 1차 PCB에 반영할 기능 블록

1차 PCB block:

```text
Li-Po battery input
Power switch / protection / regulation
ESP32-C3 SuperMini module seat
GNSS-capable Cat-M/NB-IoT communication module or Cat-M + separate GNSS
MicroSIM holder if module does not include one
LTE antenna connector
GNSS antenna connector
TMP117 temperature sensor
MAX30102 optical sensor
Buzzer driver/output
Debug/programming access
Test points
```

1차 PCB에서 하지 않는 것:

```text
ESP32-C3 chip-level integration
charging circuit
fully optimized final wearable miniaturization
RF tuning beyond reasonable module reference layout
```

## 14. 회로 설계에 넘길 확정 결정

확정:

```text
GPS/GNSS 기능은 반드시 포함한다.
1차 PCB에서 MCU는 ESP32-C3 SuperMini 모듈을 그대로 얹는다.
1차 PCB에서 MCU 칩 단위 통합은 하지 않는다.
통신 모듈은 PCB에 통합한다.
기존 SIM7080G developer kit는 기능 검증용이며 최종 1차 PCB 탑재 대상이 아니다.
Li-Po battery를 사용한다.
1차 PCB에는 충전회로를 넣지 않는다.
SMS는 자동 발송하지 않는다. 자동 emergency workflow는 별도 구현 전까지 막아둔다.
TMP117과 MAX30102는 같은 I2C bus에 연결 가능하다.
부저는 GPIO로 제어 가능함을 확인했다.
LTE/SMS/GNSS는 SIM7080G 계열에서 실제 동작 확인했다.
```

미확정:

```text
최종 GNSS 포함 통신 모듈 part number
Cat-M+GNSS 통합 모듈 vs Cat-M 모듈 + 별도 GNSS 모듈
SIM holder를 module 내장으로 쓸지 PCB에 둘지
LTE antenna connector type
GNSS antenna connector type
Li-Po 전압을 어떤 rail로 변환할지
5 V boost 필요 여부
3.3 V regulator 구성
PWRKEY/PWRK 회로
MAX30102/TMP117 물리 배치
부저 driver 필요 여부
배터리 protection/fuse/switch 방식
```

## 15. 회로 설계 시작 순서 권장

다음 채팅에서 회로 설계를 시작할 때는 이 순서가 좋다.

```text
1. 최종 통신/GNSS 모듈 선정
2. 해당 module datasheet/reference circuit 확보
3. module 전원 요구사항 확인
4. Li-Po -> required rails power tree 작성
5. ESP32-C3 SuperMini carrier pin map 확정
6. I2C sensor bus 확정
7. LTE/GNSS antenna connector and placement 결정
8. SIM holder/module 내장 여부 결정
9. buzzer driver 여부 결정
10. test point/debug connector 추가
11. schematic 작성
12. ERC/DFM 검토
```

가장 먼저 볼 것:

```text
GPS 필수이므로 S003 단독은 탈락.
통신+GNSS를 통합한 작은 module을 찾거나, S003급 Cat-M module + small GNSS module 조합으로 간다.
```

## 16. 다음 채팅에 반드시 전달해야 하는 경고

1. `PWRK`는 devkit에서 이상하게 동작했다. 최종 PCB는 devkit wiring을 베끼지 말고 최종 module datasheet를 따라야 한다.

2. `S003 M5Stamp CAT-M`은 작지만 GNSS가 없다. GPS 필수 조건과 충돌한다.

3. `U137 CatM+GNSS Unit`은 GNSS가 있지만 크다. 1차 PCB에 그대로 쓰기보다는 기능 reference로 보는 편이 낫다.

4. SIM7080G 계열은 전원 안정성이 중요하다. 보조배터리/외부전원 연결 후 안정성이 개선된 경험이 있다.

5. SMS 자동 발송은 조심해야 한다. reset/re-upload로 중복 문자 발송된 경험이 있으므로 rate limit과 persistent sent flag 없이는 자동 SMS를 켜면 안 된다.

6. MAX30102는 회로보다 기구/착용 구조가 성능에 큰 영향을 준다.

7. TMP117도 피부온도 측정 목적이면 기구적 열접촉 설계가 필요하다.

8. N drive에는 firmware/build를 두지 않는다. 문서와 기록만 둔다.

## 17. 현재 판단의 한 문장 버전

```text
센서, 부저, LTE, SMS, GNSS의 기능 검증은 끝났고, 1차 PCB는 ESP32-C3 SuperMini를 얹는 carrier board로 가며, GPS는 필수이므로 GNSS 포함 통신 모듈 또는 별도 GNSS 모듈을 선정한 뒤 Li-Po 기반 전원부와 SIM7080G reference 회로를 중심으로 schematic을 시작해야 한다.
```
