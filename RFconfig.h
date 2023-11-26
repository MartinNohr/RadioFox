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
#define BATTERY_SENSOR_GPIO 36
// set the push button GPIO port
#define DIAL_BTN 39
// default dial direction GPIO ports
#define DIAL_A 38
#define DIAL_B 37
#define PTT_PORT 17
#define AUDIO_IN_PORT 36
#define AUDIO_OUT_PORT 32        // send music and cw out this pin
#define BUZZER_FREQUENCY 700  // cw pitch
#define HL 2                 // Pin to control TX Power, not conncted -> high power, connect to GND -> Low power
#define RELAY 15
