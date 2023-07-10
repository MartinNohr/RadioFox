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
const char* prefsAutoload = "autoload";
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
void DisplayLine(int line, String text, int16_t color = TFT_WHITE, int16_t backColor = TFT_BLACK);
void DisplayMenuLine(int line, int displine, String text);
void WriteMessage(String txt, bool error = false, int wait = 2000, bool process = false);
void ReportCouldNotCreateFile(String target);
void handleFileUpload();
void append_page_header();
void append_page_footer();
void HomePage();
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void SelectInput(String heading1, String command, String arg_calling_name);
void ReportFileNotPresent(String target);
void ReportSDNotPresent();
void IncreaseRepeatButton();
void DecreaseRepeatButton();
bool SaveLoadSettings(bool save = false, bool autoloadonly = false, bool ledonly = false, bool nodisplay = false);
CRotaryDialButton::Button ReadButton();
bool CheckCancel(bool bLeaveButton = false);

bool bAutoLoadSettings = false;
bool bWebRunning = false;                 // set while running from web

// a stack to hold the file indexes as we navigate folders, put it in RTC memory for waking from sleep
typedef struct FILEINDEXINFO {
    int nFileIndex;     // current file index
    int nFileCursor;    // used when scrolling cursor, IE current file not always on top
};
#define FILEINDEXSTACKSIZE 10
RTC_DATA_ATTR FILEINDEXINFO FileIndexStack[FILEINDEXSTACKSIZE];
RTC_DATA_ATTR int FileIndexStackSize = 0;
// hold the current information
RTC_DATA_ATTR FILEINDEXINFO currentFileIndex = { 0,0 };

// display dim modes, make sure sensor mode is last
enum DISPLAY_DIM_MODES { DISPLAY_DIM_MODE_NONE, DISPLAY_DIM_MODE_TIME, DISPLAY_DIM_MODE_SENSOR };
const char* DisplayDimModeText[] = { "None","Timer","Sensor" };
const char* DisplayRotationText[] = { "90","180","270","0" };

typedef struct SYSTEM_INFO {
    uint16_t menuTextColor = TFT_BLUE;
    bool bMenuStar = false;
    bool bHiLiteCurrentFile = true;
    int nPreviewScrollCols = 20;                // now many columns to scroll with dial during preview
    bool bShowProgress = true;                  // show the progress bar
    bool bShowFolder = false;                   // show the path in front of the file
#if TTGO_T == 1
    int nDisplayBrightness = 50;                // this is in %
#elif TTGO_T == 4
    int nDisplayBrightness = 75;                // this is in %
#endif
    bool bAllowMenuWrap = false;                // allows menus to wrap around from end and start instead of pinning
    int nSidewayScrollSpeed = 25;               // mSec for pixel scroll
    int nSidewaysScrollPause = 20;              // how long to wait at each end
    int nSidewaysScrollReverse = 3;             // reverse speed multiplier
    int nBatteryFullLevel = 1760;               // 100% battery
    int nBatteryEmptyLevel = 1230;              // 0% battery, should cause a shutdown to save the batteries
    int bShowBatteryLevel = HAS_BATTERY_LEVEL;  // display the battery level on the bottom line
    int bSleepOnLowBattery = false;             // sleep on low battery
    int bCriticalBatteryLevel = false;          // set true if battery too low
    //int bShowBatteryLevel = 0;  // display the battery level on the bottom line
    int nBatteries = 2;                         // how many batteries
    CRotaryDialButton::ROTARY_DIAL_SETTINGS DialSettings;
    int nSleepTime = 0;                         // value in minutes before going to sleep, 0 means never
    int nDisplayBrightness = 50;                // display brightness setting
    int eDisplayDimMode = DISPLAY_DIM_MODE_NONE;// 0 is none, 1 is dimtime, 2 is light sensor
    int nDisplayDimTime = 0;                    // seconds before lcd is dimmed
    int nDisplayDimValue = 10;                  // the value to dim to
    int nDisplayRotation = 1;                   // rotates display 0, 180, 90, 270
    bool bSimpleMenu = false;                   // full or simple menu
    int nLightSensorDim = 4000;                 // value for the dimmest setting
    int nLightSensorBright = 100;               // value for the brightest setting
    int nPreviewAutoScroll = 0;                 // mSec for preview autoscroll, 0 means no scroll
    int nPreviewAutoScrollAmount = 1;           // now many pixels to auto scroll
    bool bPreviewScrollFiles = false;           // set for preview to scroll files instead of sideways
    int nPreviewStartOffset = 5;                // how many pixels to offset the start, the display is only 135, not 144
    bool bKeepFileOnTopLine = false;            // keep the active file on the top line
    bool bInitTest = true;                      // test the LED's on boot
    bool bRunWebServer = false;                 // run the web server
    //
};
RTC_DATA_ATTR SYSTEM_INFO SystemInfo;

// settings
bool bSdCardValid = false;              // set to true when card is found
bool bControllerReboot = false;         // set this when controllers or led count changed
// settings
constexpr auto NEXT_FOLDER_CHAR = '>';
constexpr auto PREVIOUS_FOLDER_CHAR = '<';
String currentFolder = "/";
RTC_DATA_ATTR char sleepFolder[50];       // a place to save the folder during sleeping
FILEINDEXINFO lastFileIndex = { 0,0 };    // save between switching of internal and SD
String lastFolder = "/";
std::vector<String> FileNames;
bool bSettingsMode = false;               // set true when settings are displayed
volatile int nTimerSeconds;

// esp timers
esp_timer_handle_t oneshot_LED_timer;
esp_timer_create_args_t oneshot_LED_timer_args;
// use this timer for seconds countdown
volatile int sleepTimer = 0;
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
// system state, idle or running
bool bIsRunning = false;

enum eDisplayOperation {
    eTerminate = 0,     // must be last in a menu, (or use {})
    eText,              // handle text with optional %s value
    eTextInt,           // handle text with optional %d value
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
void GetIntegerValueHue(MenuItem* menu);
void GetSelectChoice(MenuItem* menu);
void ToggleBool(MenuItem* menu);
void ToggleWebServer(MenuItem* menu);
void UpdateDisplayBrightness(MenuItem* menu, int flag);
void UpdateBatteries(MenuItem* menu, int flag);
void UpdateDisplayRotation(MenuItem* menu, int flag);
void UpdateDisplayDimMode(MenuItem* menu, int flag);
void SetMenuColor(MenuItem* menu);
void UpdateKeepOnTop(MenuItem* menu, int flag);
void Sleep(MenuItem* menu);
void ShowBattery(MenuItem* menu);
void GetStringName(MenuItem* menu);
void GetNetworkName(MenuItem* menu);
void ChangeNetCredentials(MenuItem* menu);

const char* PreviousMenu = "Back";
MenuItem BatteryMenu[] = {
    {eExit,"Battery"},
    {eBool,"Low Battery Sleep: %s",ToggleBool,&SystemInfo.bSleepOnLowBattery,0,0,0,"Yes","No"},
    {eBool,"Show Battery: %s",ToggleBool,&SystemInfo.bShowBatteryLevel,0,0,0,"Yes","No"},
    {eText,"Read Battery",ShowBattery},
    {eTextInt,"100%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryFullLevel,900,4200},
    {eTextInt,"0%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryEmptyLevel,500,3000},
    {eTextInt,"Battery Count: %d",GetIntegerValue,&SystemInfo.nBatteries,1,4,0,NULL,NULL,UpdateBatteries},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem LightSensorMenu[] = {
    {eExit,"Light Sensor"},
    {eTextInt,"Dim Value: %d",GetIntegerValue,&SystemInfo.nLightSensorDim,1000,5000},
    {eTextInt,"Bright Value: %d",GetIntegerValue,&SystemInfo.nLightSensorBright,0,1000},
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
MenuItem HomeScreenMenu[] = {
    {eExit,"Run Screen Settings"},
    {eBool,"Current File: %s",ToggleBool,&SystemInfo.bHiLiteCurrentFile,0,0,0,"Color","Normal"},
    {eBool,"File on Top Line: %s",ToggleBool,&SystemInfo.bKeepFileOnTopLine,0,0,0,"Yes","No",UpdateKeepOnTop},
    {eBool,"Show Folder: %s",ToggleBool,&SystemInfo.bShowFolder,0,0,0,"Yes","No"},
    {eBool,"Progress Bar: %s",ToggleBool,&SystemInfo.bShowProgress,0,0,0,"On","Off"},
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
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_SENSOR},
        {eMenu,"Light Sensor",{.menu = LightSensorMenu}},
    {eEndif},
    {eMenu,"Sideways Scroll Settings",{.menu = SidewaysScrollMenu}},
    {eBool,"Menu Choice: %s",ToggleBool,&SystemInfo.bMenuStar,0,0,0,"*","Color"},
    {eText,"Text Color",SetMenuColor},
    {eBool,"Menu Wrap: %s",ToggleBool,&SystemInfo.bAllowMenuWrap,0,0,0,"Yes","No"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem PreviewMenu[] = {
    {eExit,"Preview Settings"},
    {eBool,"Scroll Mode: %s",ToggleBool,&SystemInfo.bPreviewScrollFiles,0,0,0,"Files","Sideways"},
    {eTextInt,"Top Start Offset: %d px",GetIntegerValue,&SystemInfo.nPreviewStartOffset,0,10},
    {eTextInt,"Dial Scroll Pixels: %d px",GetIntegerValue,&SystemInfo.nPreviewScrollCols,1,240},
    {eTextInt,"Auto Scroll Time: %d mS",GetIntegerValue,&SystemInfo.nPreviewAutoScroll,0,1000},
    {eTextInt,"Auto Scroll Pixels: %d px",GetIntegerValue,&SystemInfo.nPreviewAutoScrollAmount,1,240},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem WiFiMenu[] = {
    {eExit,"WiFi Settings"},
    {eBool,"MIW Web Server: %s",ToggleWebServer,&SystemInfo.bRunWebServer,0,0,0,"On","Off"},
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
    {eMenu,"Run Screen Settings",{.menu = HomeScreenMenu}},
    {eMenu,"Dial & Button Settings",{.menu = DialMenu}},
    {eMenu,"Preview Settings",{.menu = PreviewMenu}},
#if HAS_BATTERY_LEVEL
    {eMenu,"Battery Settings",{.menu = BatteryMenu}},
#endif
    {eTextInt,"Sleep Time: %d Min",GetIntegerValue,&SystemInfo.nSleepTime,0,120},
    {eBool,"Startup LED Test: %s",ToggleBool,&SystemInfo.bInitTest,0,0,0,"On","Off"},
    {eMenu,"WiFi Settings",{.menu = WiFiMenu}},
    {eText,"New Version BIN file",CheckUpdateBin},
    {eText,"Reset All Settings",FactorySettings},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem StartFileMenu[] = {
    {eExit,"Start File"},
    {eText,"Save  START.MIW",SaveStartFile},
    {eText,"Load  START.MIW",LoadStartFile},
    {eText,"Erase START.MIW",EraseStartFile},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem EepromMenu[] = {
    {eExit,"Saved Settings"},
    {eBool,"Autoload Settings: %s",ToggleBool,&bAutoLoadSettings,0,0,0,"On","Off"},
    {eText,"Save Current Settings",SaveEepromSettings},
    {eText,"Load Saved Settings",LoadEepromSettings},
    {eText,"Reset All Settings",FactorySettings},
    {eText,"Format EEPROM",EraseFlash},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem MainMenu[] = {
    {eIfEqual,"",NULL,&SystemInfo.bSimpleMenu,true},
        {eText,"Sleep",Sleep},
    {eElse},
        {eMenu,"Saved Settings",{.menu = EepromMenu}},
        {eMenu,"System Settings",{.menu = SystemMenu}},
        {eReboot,"Reboot"},
        {eText,"Sleep",Sleep},
    {eEndif},
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

// task for LED test on startup and then for sideways scrolling
TaskHandle_t TaskLEDTest;
TaskHandle_t TaskArtNet;
// enums for what to fill the web page dropdowns with
enum WEB_PAGE_DROP_DOWNS {
    WPDD_FILES,     // image file types
    WPDD_MACROS,
};
typedef WEB_PAGE_DROP_DOWNS WebPageDropDowns;

void RebootSystem();
void VerifyRebootSystem();
void DoFileDelete();
void VerifyFileDelete();
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

void DisplayMainScreen();