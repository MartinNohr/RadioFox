#pragma once

const char* FOX_Version = "0.01";

const char* StartFileName = "START.FOX";
// some config things
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

#include <Update.h>

#include <time.h>
#if USE_STANDARD_SD
#include "SD.h"
#else
#include <SdFatConfig.h>
#include <sdfat.h>
#endif
#include "SPI.h"
#include "RFconfig.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include "WebServerX.h"
String wifiMacs = "FOX-" + WiFi.macAddress();
const char* ssid = wifiMacs.c_str();
const char* password = "12345678"; // not critical stuff, therefore simple password is enough

// we need to change the file handling for exFat, the name function is different than standard SD lib
#if USE_STANDARD_SD
WebServer server(80);
#else
WebServerX server(80);
#endif

String webpage = "";
char localIpAddress[16];

String file_size(int bytes) {
    String fsize = "";
    if (bytes < 1024)                 fsize = String(bytes) + " B";
    else if (bytes < (1024 * 1024))      fsize = String(bytes / 1024.0, 3) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
    else                              fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
    return fsize;
}

#include <nvs_flash.h>
#include <Preferences.h>
#include "RotaryDialButton.h"
#include <TFT_eSPI.h>
#include "fonts.h"
#include <stack>

// definitions for preferences
const char* prefsName = "FOX";
const char* prefsVars = "vars";
const char* prefsVersion = "version";
const char* prefsSystemInfo = "systeminfo";
// rotary dial values
const char* prefsLongPressTimer = "longpress";
const char* prefsDialSensitivity = "dialsense";
const char* prefsDialSpeed = "dialspeed";
const char* prefsDialReverse = "dialreverse";

#if USE_STANDARD_SD
SPIClass spiSDCard;
#else
//SPIClass spi1(VSPI);
SdFs SD; // fat16/32 and exFAT
#endif

// the display
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define BTN_SELECT      CRotaryDialButton::BTN_CLICK
#define BTN_NONE        CRotaryDialButton::BTN_NONE
#define BTN_LEFT        CRotaryDialButton::BTN_LEFT
#define BTN_LEFT_LONG   CRotaryDialButton::BTN_LEFT_LONG
#define BTN_RIGHT       CRotaryDialButton::BTN_RIGHT
#define BTN_RIGHT_LONG  CRotaryDialButton::BTN_RIGHT_LONG
#define BTN_LONG        CRotaryDialButton::BTN_LONGPRESS
#define BTN_B0_CLICK    CRotaryDialButton::BTN0_CLICK
#define BTN_B0_LONG     CRotaryDialButton::BTN0_LONGPRESS
#define BTN_B1_CLICK    CRotaryDialButton::BTN1_CLICK
#define BTN_B1_LONG     CRotaryDialButton::BTN1_LONGPRESS
#define BTN_B2_LONG     CRotaryDialButton::BTN2_LONGPRESS
#define BTN_LEFT_RIGHT_LONG CRotaryDialButton::BTN_LEFT_RIGHT_LONG

// functions
void SetDisplayBrightness(int val);
void DisplayLine(int lineNum, String text, int16_t color = TFT_WHITE, int16_t backColor = TFT_BLACK);
void DisplayMenuLine(int lineNum, int displine, String text);
void WriteMessage(String txt, bool error = false, int wait = 2000, bool process = false);
void append_page_header();
void append_page_footer();
void HomePage();
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
bool SaveLoadSettings(bool save = false, bool nodisplay = false);
CRotaryDialButton::Button ReadButton();
bool CheckCancel(bool bLeaveButton = false);
void GetFileNamesFromSD(std::vector<String>& FileNames, String ext = "", String dir = "/");

bool bWebRunning = false;                 // set while running from web

// display dim modes, make sure sensor mode is last
enum DISPLAY_DIM_MODES { DISPLAY_DIM_MODE_NONE, DISPLAY_DIM_MODE_TIME};
const char* DisplayDimModeText[] = { "None","Timer"};

const char* DisplayRotationText[] = { "90","180","270","0" };

typedef struct SYSTEM_INFO {
    uint16_t menuTextColor = TFT_WHITE;
    bool bMenuStar = false;
    int nPreviewScrollCols = 20;                // now many columns to scroll with dial during preview
    int nDisplayBrightness = 100;               // this is in %
    bool bAllowMenuWrap = false;                // allows menus to wrap around from end and start instead of pinning
    int nSidewayScrollSpeed = 25;               // mSec for pixel scroll
    int nSidewaysScrollPause = 20;              // how long to wait at each end
    int nSidewaysScrollReverse = 3;             // reverse speed multiplier
    int nBatteryFullLevel = 1760;               // 100% battery
    int nBatteryEmptyLevel = 1230;              // 0% battery, should cause a shutdown to save the batteries
    int bShowBatteryLevel = HAS_BATTERY_LEVEL;  // display the battery level on the bottom line
    int bCriticalBatteryLevel = false;          // set true if battery too low
    //int bShowBatteryLevel = 0;  // display the battery level on the bottom line
    int nBatteries = 2;                         // how many batteries
    CRotaryDialButton::ROTARY_DIAL_SETTINGS DialSettings;
    int eDisplayDimMode = DISPLAY_DIM_MODE_NONE;// 0 is none, 1 is dimtime, 2 is light sensor
    int nDisplayDimTime = 0;                    // seconds before lcd is dimmed
    int nDisplayDimValue = 10;                  // the value to dim to
    int nDisplayRotation = 1;                   // rotates display 0, 180, 90, 270
    int nLightSensorDim = 4000;                 // value for the dimmest setting
    int nLightSensorBright = 100;               // value for the brightest setting
    int nPreviewAutoScroll = 0;                 // mSec for preview autoscroll, 0 means no scroll
    bool bRunWebServer = false;                 // run the web server
    // radio settings
    char cRadioID[21] = "KK7JTE";               // ID to transmit
    int nTxTime = 10;                           // tx pause
    bool bRfPowerHi = false;                    // rf power control
    int nFrequency = 140;                       // radio frequency
    char cAudioFile[31] = "";                   // choose the audio file
    //
};
RTC_DATA_ATTR SYSTEM_INFO SystemInfo;

// settings
bool bSdCardValid = false;              // set to true when card is found
bool bControllerReboot = false;         // set this when controllers or led count changed
// settings TODO: this should be changed to a semaphore
volatile bool bSettingsMode = false;    // set true when settings are displayed

// esp timers
// seconds before dimming the display
volatile int displayDimTimer = 30;
volatile bool displayDimNow = false;
esp_timer_handle_t periodic_Second_timer;
esp_timer_create_args_t periodic_Second_timer_args;

#if USE_STANDARD_SD
SDFile dataFile;
#else
FsFile dataFile;
#endif

enum eDisplayOperation {
    eTerminate = 0,     // must be last in a menu, (or use {})
    eText,              // handle text with optional %s value, display only
    eTextInt,           // handle text with optional %d value
    eEditText,          // edit a text string
    eChooseFile,        // choose a file from the SD card
    eBool,              // handle bool using %s and on/off values
    eMenu,              // load another menu
    eExit,              // closes this menu, handles optional %d or %s in string
    eIfEqual,           // start skipping menu entries if match with boolean data value
    eIfIntEqual,        // start skipping menu entries if match with int data value
    eElse,              // toggles the skipping
    eEndif,             // ends an if block
    eReboot,            // reboot the system
    eList,              // used to rotate selection from a list of choices
};

// we need to have a pointer reference to this in the MenuItem, the full declaration follows later
struct BuiltInItem;
std::vector<bool> bMenuValid;   // set to indicate menu item  is valid
typedef struct MenuItem {
    enum eDisplayOperation op;
    const char* text;                   // text to display
    union {
        void(*function)(MenuItem*);     // called on click
        MenuItem* menu;                 // jump to another menu
        BuiltInItem* builtin;           // builtin items
    };
    const void* value;                  // associated variable
    long min;                           // the minimum value, also used for ifequal, min length for string
    long max;                           // the maximum value, also size to compare for if, max length for string
    int decimals;                       // 0 for int, 1 for 0.1
    const char* on;                     // text for boolean true
    const char* off;                    // text for boolean false
    // flag is 1 for first time, 0 for changes, and -1 for last call, bools only call this with -1
    void(*change)(MenuItem*, int flag); // call for each change, example: brightness change show effect, can be NULL
    const char** nameList;              // used for multichoice of items, example wiring mode, .max should be count-1 and .min=0
    const char* cHelpText;              // a place to put some menu help
};

// some menu functions using menus
void CheckUpdateBin(MenuItem* menu);
void FactorySettings(MenuItem* menu);
void EraseFlash(MenuItem* menu);
void EraseStartFile(MenuItem* menu);
void SaveStartFile(MenuItem* menu);
void LoadStartFile(MenuItem* menu);
void SaveEepromSettings(MenuItem* menu);
void LoadEepromSettings(MenuItem* menu);
void GetIntegerValue(MenuItem* menu);
void GetSelectChoice(MenuItem* menu);
void ToggleBool(MenuItem* menu);
void ToggleWebServer(MenuItem* menu);
void UpdateDisplayBrightness(MenuItem* menu, int flag);
void UpdateBatteries(MenuItem* menu, int flag);
void UpdateDisplayRotation(MenuItem* menu, int flag);
void UpdateDisplayDimMode(MenuItem* menu, int flag);
void SetMenuColor(MenuItem* menu);
void ShowBattery(MenuItem* menu);
void GetNetworkName(MenuItem* menu);
void ChangeNetCredentials(MenuItem* menu);
void GetText(MenuItem* menu);
void GetAudioFile(MenuItem* menu);

const char* PreviousMenu = "Back";
MenuItem BatteryMenu[] = {
    {eExit,"Battery"},
    {eBool,"Show Battery: %s",ToggleBool,&SystemInfo.bShowBatteryLevel,0,0,0,"Yes","No"},
    {eText,"Read Battery",ShowBattery},
    {eTextInt,"100%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryFullLevel,900,4200},
    {eTextInt,"0%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryEmptyLevel,500,3000},
    {eTextInt,"Battery Count: %d",GetIntegerValue,&SystemInfo.nBatteries,1,4,0,NULL,NULL,UpdateBatteries},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem SidewaysScrollMenu[] = {
    {eExit,"Sideways Scrolling"},
    {eTextInt,"Sideways Scroll Speed: %d mS",GetIntegerValue,&SystemInfo.nSidewayScrollSpeed,1,1000},
    {eTextInt,"Sideways Scroll Pause: %d",GetIntegerValue,&SystemInfo.nSidewaysScrollPause,1,100},
    {eTextInt,"Sideways Scroll Reverse: %dx",GetIntegerValue,&SystemInfo.nSidewaysScrollReverse,1,20},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem DialMenu[] = {
    {eExit,"Rotary Dial Settings"},
    {eBool,"Direction: %s",ToggleBool,&SystemInfo.DialSettings.m_bReverseDial,0,0,0,"Reverse","Normal"},
    {eTextInt,"Pulse Count: %d",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseCount,1,5},
    {eTextInt,"Pulse Timer: %d mS",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseTimer,100,1000},
    {eTextInt,"Long Press count: %d",GetIntegerValue,&SystemInfo.DialSettings.m_nLongPressTimerValue,2,200},
    {eBool,"Rotate Dial Type: %s",ToggleBool,&SystemInfo.DialSettings.m_bToggleDial,0,0,0,"Toggle","Pulse"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
#define MAX_DIM_MODE (sizeof(DisplayDimModeText) / sizeof(*DisplayDimModeText) - 1)
MenuItem DisplayMenu[] = {
    {eExit,"Display Settings"},
    {eList, "Display Rotation: %s", GetSelectChoice, &SystemInfo.nDisplayRotation, 0, sizeof(DisplayRotationText) / sizeof(*DisplayRotationText) - 1, 0, NULL, NULL, UpdateDisplayRotation, DisplayRotationText},
    {eList, "Dimming Mode: %s", GetSelectChoice, &SystemInfo.eDisplayDimMode, 0, MAX_DIM_MODE, 0, NULL, NULL, UpdateDisplayDimMode, DisplayDimModeText},
    {eTextInt,"Bright Value: %d%%",GetIntegerValue,&SystemInfo.nDisplayBrightness,1,100,0,NULL,NULL,UpdateDisplayBrightness},
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_NONE},
    {eElse},
        {eTextInt,"Dim Value: %d%%",GetIntegerValue,&SystemInfo.nDisplayDimValue,1,100},
    {eEndif},
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_TIME},
        {eTextInt,"Display Dim Time: %d S",GetIntegerValue,&SystemInfo.nDisplayDimTime,0,120},
    {eEndif},
    {eMenu,"Sideways Scroll Settings",{.menu = SidewaysScrollMenu}},
    {eBool,"Menu Choice: %s",ToggleBool,&SystemInfo.bMenuStar,0,0,0,"*","Color"},
    {eText,"Text Color",SetMenuColor},
    {eBool,"Menu Wrap: %s",ToggleBool,&SystemInfo.bAllowMenuWrap,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem WiFiMenu[] = {
    {eExit,"WiFi Settings"},
    {eBool,"Web Server: %s",ToggleWebServer,&SystemInfo.bRunWebServer,0,0,0,"On","Off"},
    {eIfEqual,"",NULL,&SystemInfo.bRunWebServer,true},
        {eText,"Homepage: %s",NULL,localIpAddress},
    {eEndif},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem SystemMenu[] = {
    {eExit,"System Settings"},
    {eMenu,"Display Settings",{.menu = DisplayMenu}},
    {eMenu,"Dial & Button Settings",{.menu = DialMenu}},
#if HAS_BATTERY_LEVEL
    {eMenu,"Battery Settings",{.menu = BatteryMenu}},
#endif
    {eMenu,"WiFi Settings",{.menu = WiFiMenu}},
    {eText,"New Version BIN file",CheckUpdateBin},
    {eText,"Reset All Settings",FactorySettings},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem EepromMenu[] = {
    {eExit,"Saved Settings"},
    {eText,"Save Current Settings",SaveEepromSettings},
    {eText,"Reset All Settings",FactorySettings},
    {eText,"Format EEPROM",EraseFlash},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem RadioMenu[] = {
    {eExit,"Radio Settings"},
    {eTextInt,"TX Time: %d Min",GetIntegerValue,&SystemInfo.nTxTime,1,60},
    {eBool,"RF Power: %s",ToggleBool,&SystemInfo.bRfPowerHi,0,0,0,"High","Low"},
    {eTextInt,"Frequency: %d MHz",GetIntegerValue,&SystemInfo.nFrequency,137,174},
	{eEditText,"Call Sign: %s",GetText,SystemInfo.cRadioID,1,sizeof(SystemInfo.cRadioID) - 1},
    {eEditText,"Audio: %s",GetAudioFile,SystemInfo.cAudioFile,1,sizeof(SystemInfo.cAudioFile) - 1},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem MainMenu[] = {
    {eMenu,"Radio Settings",{.menu = RadioMenu}},
    {eMenu,"Saved Settings",{.menu = EepromMenu}},
    {eMenu,"System Settings",{.menu = SystemMenu}},
    {eReboot,"Reboot"},
    // make sure this one is last
{eTerminate}
};

// a stack for menus so we can find our way back
typedef struct MenuInfo {
    int index;      // active entry
    int offset;     // scrolled amount
    int menucount;  // how many entries in this menu
    MenuItem* menu; // pointer to the menu
};
MenuInfo* menuPtr;
std::stack<MenuInfo*> MenuStack;

bool bMenuChanged = true;

RTC_DATA_ATTR int nMenuLineCount = 7;

// keep the display lines in here so we can scroll sideways if necessary
struct TEXTLINES {
    String Line;
    // the pixels length of this line
    int nRollLength;
    // current scroll pixel offsets
    int nRollOffset;
    // colors
    uint16_t foreColor, backColor;
    // whether we are going up or down
    int nRollDirection;
};
std::vector<struct TEXTLINES> TextLines;

// task for running the radio
TaskHandle_t TaskRunRadio;
// enums for what to fill the web page dropdowns with
enum WEB_PAGE_DROP_DOWNS {
    WPDD_FILES,     // image file types
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

struct ON_SERVER_ITEM {
    char* path;
    void(*function)();
};
typedef ON_SERVER_ITEM OnServerItem;
OnServerItem OnServerList[] = {
    {"/", HomePage},
    {"/settings", WebShowSettings},
    {"/changesettings", WebChangeSettings},
    {"/verifyrebootsystem", VerifyRebootSystem},
    {"/rebootsystem", RebootSystem},
};

String MenuToHtml(MenuItem* menu, bool bActive = true, int nLevel = 0);
