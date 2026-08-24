// #pragma once

inline const char *FOX_Version = "1.16.0";

// Use these define settings in the platformio.ini file
// build_flags =
//   -D USER_SETUP_LOADED=1
//   -include $PROJECT_LIBDEPS_DIR/$PIOENV/TFT_eSPI/User_Setups/Setup25_TTGO_T_Display.h
// Use the following if not using the above ini file entry
// change User_Setup_Select.h correctly
// comment out this line
// #include <User_Setup.h>           // Default setup is root library folder
//
// uncomment one of these in that file
// #include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//
#include <Update.h>
#include "RFconfig.h"
#include <time.h>
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
inline String wifiMacs = "FOX-" + WiFi.macAddress();
inline const char *ssid = wifiMacs.c_str();
inline const char *password = "12345678"; // not critical stuff, therefore simple password is enough

inline WebServer server(80);

inline String webpage = "";
inline char localIpAddress[16];

#include <nvs_flash.h>
#include <Preferences.h>
#include "RotaryDialButton.h"
#include <TFT_eSPI.h>
#include "fonts.h"
#include <stack>

// serial port for the 818 radio module
inline HardwareSerial RadioSerial(1);

inline SPIClass spiSDCard;

// the display
inline TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

#define BTN_SELECT CRotaryDialButton::BTN_CLICK
#define BTN_NONE CRotaryDialButton::BTN_NONE
#define BTN_LEFT CRotaryDialButton::BTN_LEFT
#define BTN_LEFT_LONG CRotaryDialButton::BTN_LEFT_LONG
#define BTN_RIGHT CRotaryDialButton::BTN_RIGHT
#define BTN_RIGHT_LONG CRotaryDialButton::BTN_RIGHT_LONG
#define BTN_LONG CRotaryDialButton::BTN_LONGPRESS
#define BTN_B0_CLICK CRotaryDialButton::BTN0_CLICK
#define BTN_B0_LONG CRotaryDialButton::BTN0_LONGPRESS
#define BTN_B1_CLICK CRotaryDialButton::BTN1_CLICK
#define BTN_B1_LONG CRotaryDialButton::BTN1_LONGPRESS
#define BTN_B2_LONG CRotaryDialButton::BTN2_LONGPRESS
#define BTN_LEFT_RIGHT_LONG CRotaryDialButton::BTN_LEFT_RIGHT_LONG

inline bool bWebRunning = false; // set while running from web

// display dim modes, make sure sensor mode is last (NOTE: no sensor on Radio Fox PCB)
enum DISPLAY_DIM_MODES
{
    DISPLAY_DIM_MODE_NONE,
    DISPLAY_DIM_MODE_TIME
};
inline const char *DisplayDimModeText[] = {"None", "Timer"};

// NOTE: update CompareRadioSettings if anything important is changed that needs the radio to initialized
// Version 2: added morse buzzer frequency
struct SYSTEM_INFO
{
    int nSystemInfoVersion = 3; // change as necessary
    uint16_t menuTextColor = TFT_WHITE;
    uint16_t menuHiLiteColor = TFT_WHITE;
    bool bMenuStar = false;
    int nPreviewScrollCols = 20;               // now many columns to scroll with dial during preview
    int nDisplayBrightness = 100;              // this is in %
    bool bAllowMenuWrap = false;               // allows menus to wrap around from end and start instead of pinning
    int nSidewayScrollSpeed = 25;              // mSec for pixel scroll
    int nSidewaysScrollPause = 20;             // how long to wait at each end
    int nSidewaysScrollReverse = 3;            // reverse speed multiplier
    int bShowBatteryLevel = HAS_BATTERY_LEVEL; // display the battery level on the bottom line
    bool bCriticalBatteryLevel = false;        // set when battery is too low
    CRotaryDialButton::ROTARY_DIAL_SETTINGS DialSettings;
    int eDisplayDimMode = DISPLAY_DIM_MODE_NONE; // 0 is none, 1 is dimtime
    int nDisplayDimTime = 5;                     // seconds before lcd is dimmed
    int nDisplayDimValue = 10;                   // the value to dim to
    int nPreviewAutoScroll = 0;                  // mSec for preview autoscroll, 0 means no scroll
    bool bRunWebServer = false;                  // run the web server
    // radio settings
    char cRadioCallSign[21] = "CALLSIGN"; // ID to transmit
    char cRadioString[31] = "RADIO";      // Radio ID string to send
    // transmit timers
    bool bUseFixedTimers = true; // fixed or random timers
    int nTxTimeFixed = 1 * 60;   // tx time in seconds
    int nTxPauseFixed = 5 * 60;  // tx pause time in seconds

    // for random timers
    int nTxTimeMin = 30;
    int nTxTimeMax = 1 * 60;
    int nTxPauseMin = 1 * 60;
    int nTxPauseMax = 10 * 60;

    bool bSleepWhilePausing = false; // turn the radio off while pausing
    bool bTxPowerLow = true;         // tx power control
    int nBandWidth = 0;              // 0 for 12.5k and 1 for 25k
#if RADIO_UHF
    int nFrequency = 400000; // UHF radio frequency in kHz
#else
    int nFrequency = 140000; // VHF radio frequency in kHz
#endif
    int nRfOffset = 1;     // RX frequency offset 0=-600 1=0 2=+600 kHz, 1200 for UHF
    int nRxVolume = 6;     // volume from 1 to 8
    int nSquelch = 2;      // squelch setting, 0 to 8, 0 is monitor mode
    bool bCTCSS = true;    // false for DCS
    int nRxCTCSS = 12;     // RX CSS 0 to 38, 12 is 100Hz, 0 is none, see SubToneText[] below
    int nTxCTCSS = 12;     // TX CSS 0 to 38
    bool bTxDcsNI = false; // true for DCS 'N', false for 'I'
    bool bRxDcsNI = false; // true for DCS 'N', false for 'I'
    int nTxDcs = 0;        // DCS code, 0 is none
    int nRxDcs = 0;        // DCS code, 0 is none
    bool bPlayAudioFile = false;
    char cAudioFile[31] = "";     // choose the music file
    int nMorseInterval = 200;     // mSec morse timer
    bool bXmitEnable = false;     // if xmit = false, don't transmit
    // bool bStopImmediately = true; // set to true to cancel transmitting without waiting to finish
    int nDtmfEnableTimer = 10;    // the number of seconds after '*' that DTMF commands will work
    int nStartDelayTimer = 0;     // seconds before the first transmission
    int nBuzzerFrequency = 700;   // the morse pitch
    // beacon mode settings
    bool bBeaconMode = false; // limit the frequency for beacon use and turn GPS info on
    double fLatitude = 0.0;   // 6 decimals +-180 degrees
    double fLongitude = 0.0;  // 6 decimals +-180 degrees
    bool bSendGPS = false;    // enable GPS transmission, bBeaconMode must be on
    //
};
inline RTC_DATA_ATTR SYSTEM_INFO SystemInfo;

struct BATTERY_INFO
{
    int nBatteryFullLevel = 3490; // 100% battery
    // int nBatteryLowLevel = 2700;                // the low battery
};
inline RTC_DATA_ATTR BATTERY_INFO BatteryInfo;
#define LOW_BATTERY_VALUE ((int)(BatteryInfo.nBatteryFullLevel * 3.25 / 4.2))

// settings
inline bool bSdCardValid = false;      // set to true when card is found
inline bool bControllerReboot = false; // set this when controllers or led count changed
// settings TODO: this should be changed to a semaphore
inline bool g_bSettingsMode = false; // set true when settings are displayed

// esp timers
// seconds before dimming the display
inline int displayDimTimer = 30;
inline bool displayDimNow = false;
inline esp_timer_handle_t periodic_Second_timer;
inline esp_timer_create_args_t periodic_Second_timer_args;

#include "Foxmenus.h"
#include "FoxServer.h"

// radio event flags, 8 possible, 1,2,4,8,16,32,64,128
inline EventGroupHandle_t gRadioEventsHandle;
#define RadioEventReady 0x01          // the radio is ready
#define RadioEventDelayStart 0x02     // delay start active
#define RadioEventEnableTransmit 0x04 // transmit enabled
#define RadioEventIsTransmitting 0x08 // keep track of transmitting or not
#define RadioEventCancelWaits 0x10    // clear the delay and pause timers
#define RadioEventFailed 0x20         // flag if radio fails
// some useful macros
#define IsRadioReady ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventReady) != 0)
#define IsTransmitEnabled ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventEnableTransmit) != 0)
#define IsTransmitting ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventIsTransmitting) != 0)
#define IsCancelWaits ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventCancelWaits) != 0)

// task handles for running the radio parts
inline TaskHandle_t TaskRunRadioHandle;
inline TaskHandle_t TaskRunTransmitHandle;
inline TaskHandle_t TaskSendRadioHandle;
inline TaskHandle_t TaskSendMusicHandle;
inline TaskHandle_t TaskSendGpsHandle;
inline TaskHandle_t TaskShowBatteryHandle;
inline TaskHandle_t TaskDTMFHandle;
inline TaskHandle_t TaskScrollSidewaysHandle;
inline TaskHandle_t TaskMenuHandle;
// a mutex to control access to writing on the display, the TFT driver is not re-entrant
inline SemaphoreHandle_t MutexDisplayHandle;

// enums for what to fill the web page dropdowns with
enum WEB_PAGE_DROP_DOWNS
{
    WPDD_FILES, // image file types
    WPDD_MACROS,
};
typedef WEB_PAGE_DROP_DOWNS WebPageDropDowns;

void RebootSystem();
void VerifyRebootSystem();
void UtilitiesPage();
void WebCancel();
void WebRunMacro();
void WebRunImage();
void WebChangeMacro();
void WebChangeBuiltinSettings();
void WebBuiltinSettings();
void WebChangeFile();
void WebChangeSettings();
void WebShowSettings();

struct ON_SERVER_ITEM
{
    const char *path;
    void (*function)();
};
typedef ON_SERVER_ITEM OnServerItem;
inline OnServerItem OnServerList[] = {
    {"/", HomePage},
    {"/settings", WebShowSettings},
    {"/changesettings", WebChangeSettings},
    {"/verifyrebootsystem", VerifyRebootSystem},
    {"/rebootsystem", RebootSystem},
};

inline TFT_eSprite LineSprite = TFT_eSprite(&tft); // Create Sprite object "LineSprite" with pointer to "tft" object
#define BATTERY_BAR_HEIGHT 5
inline TFT_eSprite BatterySprite = TFT_eSprite(&tft); // Create Sprite object "BatterySprite" with pointer to "tft" object

void WavPlayer(char *wavfile);

// functions
void append_page_header();
void append_page_footer();
void HomePage();
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void RadioEnable(bool bEnable);
void SetRadioTransmit(bool bTx);
bool RadioSetup(bool bIniit);
void sendEndOfWord();
void sendDot();
void sendDash();
void sendMorseCode(const char *tokens);
bool UpMenuLevel(bool gotoMain);
void sendLetter(char c);
