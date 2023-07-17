#pragma once
// ***** Various switches for options are set here *****

// also remember to change User_Setup_Select.h correctly
// use one of these in that file
//#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT

// 1 for standard SD library, 0 for the new exFat library which allows > 32GB SD cards
#define USE_STANDARD_SD 0
// reverse A and B for some PCB or wired versions, this is set for rev 2 PCB, 0 for older PCB, and 0 for rev 3 pcb
#define ROTARY_DIAL_REVERSE 0
// The push button setting, set to 1 for onboard PS version 1.4
#define PUSH_BUTTON_PORT 0

// SD details
#define SDcsPin    33  // GPIO33
#define SDSckPin   25  // GIPO25
#define SDMisoPin  27  // GPIO27
#define SDMosiPin  26  // GPIO26
// LED controller pins
#define DATA_PIN1 2
#define DATA_PIN2 17
// battery level
#define HAS_BATTERY_LEVEL 1
// battery sensor GPIO
#define BATTERY_SENSOR_GPIO 36
// set the push button GPIO port
#if PUSH_BUTTON_PORT
	#define DIAL_BTN 37
#else
	#define DIAL_BTN 15
#endif
// default dial direction GPIO ports
#if ROTARY_DIAL_REVERSE
	#define DIAL_A 12
	#define DIAL_B 13
#else
	#define DIAL_A 13
	#define DIAL_B 12
#endif
#define PTT_PORT 17
