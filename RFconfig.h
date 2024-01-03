#pragma once
// ***** Various switches for options are set here *****

// also remember to change User_Setup_Select.h correctly
// use one of these in that file
//#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT

// 1 for standard SD library, 0 for the new exFat library which allows > 32GB SD cards
#define USE_STANDARD_SD 0

// SD details
#define SDSckPin   25  // GIPO25
#define SDMosiPin  26  // GPIO26
#define SDMisoPin  27  // GPIO27
#define SDcsPin    33  // GPIO33
// battery level
#define HAS_BATTERY_LEVEL 1
// battery sensor GPIO
#define BATTERY_SENSOR_GPIO 13
// set the push button GPIO port, tried 39, but it seems to pulse every second, somebody is using it!
#define DIAL_BTN 2
// default dial direction GPIO ports
#define DIAL_A 38
#define DIAL_B 37
#define PTT_PORT 17
#define PTT_TALK 0
#define PTT_LISTEN 1
#define AUDIO_IN_PORT 36
#define AUDIO_OUT_PORT 32        // send music and cw out this pin
#define BUZZER_FREQUENCY 700  // cw pitch
#define TXPOWER_PORT 12	// Pin to control TX Power, not connected -> high power, connect to GND -> Low power
// the radio serial ports
#define RADIO_SLEEP_PORT 15
#define RADIO_SERIAL_RX 21
#define RADIO_SERIAL_TX 22	// NOTE: do not use the LED_BUILTIN which is also 22, the TTGO T-Display does not have an LED
// Choose UHF or VHF SA818, 0 is VHF, 1 is UHF
#define RADIO_UHF 0
