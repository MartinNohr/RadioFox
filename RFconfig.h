#pragma once
// ***** Various switches for options are set here *****
// Choose UHF or VHF SA818, 0 is VHF, 1 is UHF
#define RADIO_UHF 0

// also remember to change User_Setup_Select.h correctly
// use one of these in that file
//#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT
// make sure this one is commented out
//#include <User_Setup.h>           // Default setup is root library folder

// pick only one of these and set to 1
#define TTGO_T_DISPLAY 1
#define TTGO_T_DISPLAY_S3 0

// 1 for standard SD library, 0 for the new exFat library which allows > 32GB SD cards
#define USE_STANDARD_SD 0
// battery level
#define HAS_BATTERY_LEVEL 1
// cw pitch
#define BUZZER_FREQUENCY 700

#define PTT_TALK 0
#define PTT_LISTEN 1

// all the gpio pins
#if TTGO_T_DISPLAY
// tone generator
const int toneChannel = 2;
constexpr int TFT_ENABLE = 4;
// use these to control the LCD brightness
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
// SD details
#define SDSckPin   25  // GIPO25
#define SDMosiPin  26  // GPIO26
#define SDMisoPin  27  // GPIO27
#define SDcsPin    33  // GPIO33
// battery sensor GPIO
#define BATTERY_SENSOR_GPIO 13
// set the push button GPIO port, tried 39, but it seems to pulse every second, somebody is using it!
#define DIAL_BTN 2
// default dial direction GPIO ports
#define DIAL_A 38
#define DIAL_B 37
#define PTT_PORT 17
#define AUDIO_IN_PORT 36
#define AUDIO_OUT_PORT 32        // send music and cw out this pin
#define TXPOWER_PORT 12	// Pin to control TX Power, not connected -> high power, connect to GND -> Low power
// the radio serial ports
#define RADIO_SLEEP_PORT 15	// LOW for awake, HIGH for sleep
#define RADIO_SERIAL_RX 21
#define RADIO_SERIAL_TX 22	// NOTE: do not use the LED_BUILTIN which is also 22, the TTGO T-Display does not have an LED
#endif
#if TTGO_T_DISPLAY_S3
// tone generator
const int toneChannel = 2;

constexpr int TFT_ENABLE = 38;
// use these to control the LCD brightness
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
// SD details
#define SDSckPin   12
#define SDMosiPin  13
#define SDMisoPin  11
#define SDcsPin    10
// battery sensor GPIO
#define BATTERY_SENSOR_GPIO 16
#define DIAL_BTN 3
// default dial direction GPIO ports
#define DIAL_A 1
#define DIAL_B 2
#define PTT_PORT 21
#define AUDIO_IN_PORT 44
#define AUDIO_OUT_PORT 43        // send music and cw out this pin
#define TXPOWER_PORT 21	// Pin to control TX Power, not connected -> high power, connect to GND -> Low power
// the radio serial ports
#define RADIO_SLEEP_PORT 15	// LOW for awake, HIGH for sleep *************** not available *************
#define RADIO_SERIAL_RX 18
#define RADIO_SERIAL_TX 17	// NOTE: do not use the LED_BUILTIN which is also 22, the TTGO T-Display does not have an LED
#endif
