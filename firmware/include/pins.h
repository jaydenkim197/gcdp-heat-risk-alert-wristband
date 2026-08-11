#pragma once

#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif

#ifndef SAMPLE_INTERVAL_MS
#define SAMPLE_INTERVAL_MS 5000
#endif

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 21
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 22
#endif

#ifndef LTE_RX_PIN
#define LTE_RX_PIN 16
#endif

#ifndef LTE_TX_PIN
#define LTE_TX_PIN 17
#endif

#ifndef GPS_RX_PIN
#define GPS_RX_PIN -1
#endif

#ifndef GPS_TX_PIN
#define GPS_TX_PIN -1
#endif

#ifndef GPS_BAUD
#define GPS_BAUD 38400
#endif

#ifndef BUZZER_PIN
#define BUZZER_PIN 25
#endif

#ifndef LTE_PWRK_PIN
#define LTE_PWRK_PIN -1
#endif

#ifndef TEST_TRIGGER_PIN
#define TEST_TRIGGER_PIN -1
#endif

#ifndef BATTERY_TEST_PIN
#define BATTERY_TEST_PIN -1
#endif

#ifndef ENABLE_STATE_BUZZER
#define ENABLE_STATE_BUZZER 1
#endif

#ifndef AUTO_LTE_INIT
#define AUTO_LTE_INIT 0
#endif

#ifndef STANDALONE_ALERT_NUMBER
#define STANDALONE_ALERT_NUMBER "01000000000"
#endif

#ifndef TMP117_ADDR
#define TMP117_ADDR 0x48
#endif

#ifndef MAX30102_ADDR
#define MAX30102_ADDR 0x57
#endif
