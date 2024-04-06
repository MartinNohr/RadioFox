#pragma once

const char* FOX_Version = "0.16";

const char* StartFileName = "START.FOX";
// some config things
// ***** Various switches for options are set here *****

// also remember to change User_Setup_Select.h correctly
// comment out this line
//#include <User_Setup.h>           // Default setup is root library folder
// 
// uncomment one of these in that file
//#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
//#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>     // For the LilyGo T-Display S3 based ESP32S3 with ST7789 170 x 320 TFT

#include <Update.h>

#include "RFconfig.h"
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

// serial port for the 818 radio module
HardwareSerial RadioSerial(1);

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
void DisplayLine(int lineNum, String text, uint16_t color = TFT_WHITE, uint16_t backColor = TFT_BLACK);
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

// NOTE: update CompareRadioSettings if anything important is changed that needs the radio to initialized
typedef struct SYSTEM_INFO {
    uint16_t menuTextColor = TFT_WHITE;
    bool bMenuStar = false;
    int nPreviewScrollCols = 20;                // now many columns to scroll with dial during preview
    int nDisplayBrightness = 100;               // this is in %
    bool bAllowMenuWrap = false;                // allows menus to wrap around from end and start instead of pinning
    int nSidewayScrollSpeed = 25;               // mSec for pixel scroll
    int nSidewaysScrollPause = 20;              // how long to wait at each end
    int nSidewaysScrollReverse = 3;             // reverse speed multiplier
    int nBatteryFullLevel = 3330;               // 100% battery
    int nBatteryEmptyLevel = 2180;              // 0% battery, should cause a shutdown to save the batteries
    int bShowBatteryLevel = HAS_BATTERY_LEVEL;  // display the battery level on the bottom line
    int bCriticalBatteryLevel = false;          // set true if battery too low
    CRotaryDialButton::ROTARY_DIAL_SETTINGS DialSettings;
    int eDisplayDimMode = DISPLAY_DIM_MODE_NONE;// 0 is none, 1 is dimtime, 2 is light sensor
    int nDisplayDimTime = 0;                    // seconds before lcd is dimmed
    int nDisplayDimValue = 10;                  // the value to dim to
    int nPreviewAutoScroll = 0;                 // mSec for preview autoscroll, 0 means no scroll
    bool bRunWebServer = false;                 // run the web server
    // radio settings
    char cRadioCallSign[21] = "CALLSIGN";       // ID to transmit
    char cBeaconString[31] = "BEACON";          // beacon string to send
    char cSerialNumber[13] = "G2023XX";         // fox serial number
	int nTxTime = 2 * 60;                       // tx time in seconds
	int nTxPause = 3 * 60;                      // tx pause time in seconds
    bool bTxPowerLow = false;                   // tx power control
    int nBandWidth = 0;                         // 0 for 12.5k and 1 for 25k
#if RADIO_UHF
    int nFrequency = 400000;                    // UHF radio frequency in kHz
#else
    int nFrequency = 140000;                    // VHF radio frequency in kHz
#endif
    int nRfOffset = 1;                          // RX frequeny offset 0=-600 1=0 2=+600 kHz, 1200 for UHF
    int nRxVolume = 6;                          // volume from 1 to 8
    int nSquelch = 2;                           // squelch setting, 0 to 8, 0 is monitor mode
    bool bCTCSS = true;                         // false for DCS
    int nRxCTCSS = 12;                          // RX CSS 0 to 38, 12 is 100Hz, 0 is none, see SubToneText[] below
    int nTxCTCSS = 12;                          // TX CSS 0 to 38
    bool bTxDcsNI = false;                      // true for DCS 'N', false for 'I'
    bool bRxDcsNI = false;                      // true for DCS 'N', false for 'I'
    int nTxDcs = 0;                             // DCS code, 0 is none
    int nRxDcs = 0;                             // DCS code, 0 is none
    bool bPlayAudioFile = false;
    char cAudioFile[31] = "";                   // choose the music file
    int nMorseInterval = 200;                   // mSec morse timer
    bool bXmitEnable = false;                   // if xmit = false, don't transmit
    bool bStopImmediately = true;               // set to true to cancel transmitting without waiting to finish
    int nDtmfEnableTimer = 10;                  // the number of seconds after '*' that DTMF commands will work
    int nStartDelayTimer = 0;                   // seconds before the first transmission
    //
};
RTC_DATA_ATTR SYSTEM_INFO SystemInfo;

// settings
bool bSdCardValid = false;              // set to true when card is found
bool bControllerReboot = false;         // set this when controllers or led count changed
// settings TODO: this should be changed to a semaphore
volatile bool g_bSettingsMode = false;    // set true when settings are displayed

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
    int decimals;                       // 0 for int, 1 for 0.1, 2 for 0.01 etc
    const char* on;                     // text for boolean true
    const char* off;                    // text for boolean false
    // flag is 1 for first time, 0 for changes, and -1 for last call, bools only call this with -1
    void(*change)(MenuItem*, int flag); // call for each change, example: brightness change show effect, can be NULL
    const char** nameList;              // used for multichoice of items, example wiring mode, .max should be count-1 and .min=0
    const char* cHelpText;              // a place to put some menu help
};

// some menu functions using menus
void FactorySettings(MenuItem* menu);
void EraseFlash(MenuItem* menu);
void EraseStartFile(MenuItem* menu);
void CheckUpdateBin(MenuItem* menu);
void SaveEepromSettings(MenuItem* menu);
void LoadEepromSettings(MenuItem* menu);
void GetIntegerValue(MenuItem* menu);
void GetSelectChoice(MenuItem* menu);
void GetSelectChoiceList(MenuItem* menu);
void ToggleBool(MenuItem* menu);
void ToggleWebServer(MenuItem* menu);
void UpdateDisplayBrightness(MenuItem* menu, int flag);
void UpdateDisplayDimMode(MenuItem* menu, int flag);
void SetMenuColor(MenuItem* menu);
void ShowBattery(MenuItem* menu);
void GetNetworkName(MenuItem* menu);
//void ChangeNetCredentials(MenuItem* menu);
void GetText(MenuItem* menu);
void GetAudioFile(MenuItem* menu);
void ShowUsbVoltage(MenuItem* menu);
void CancelWaitTimers(MenuItem*);

const char* PreviousMenu = "Back";
MenuItem BatteryMenu[] = {
    {eExit,"Battery"},
    {eBool,"Show Battery: %s",ToggleBool,&SystemInfo.bShowBatteryLevel,0,0,0,"Yes","No"},
    {eText,"Read Battery",ShowBattery},
    {eTextInt,"100%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryFullLevel,900,4200},
    {eTextInt,"0%% Battery: %d",GetIntegerValue,&SystemInfo.nBatteryEmptyLevel,500,3000},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem SidewaysScrollMenu[] = {
    {eExit,"Sideways Scrolling"},
    {eTextInt,"Speed: %d mS",GetIntegerValue,&SystemInfo.nSidewayScrollSpeed,1,1000},
    {eTextInt,"Pause: %d",GetIntegerValue,&SystemInfo.nSidewaysScrollPause,1,100},
    {eTextInt,"Reverse Speed: %dx",GetIntegerValue,&SystemInfo.nSidewaysScrollReverse,1,20},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem DialMenu[] = {
    {eExit,"Rotary Dial Settings"},
    {eBool,"Direction: %s",ToggleBool,&SystemInfo.DialSettings.m_bReverseDial,0,0,0,"Reverse","Normal"},
    //{eTextInt,"Pulse Count: %d",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseCount,1,5},
    //{eTextInt,"Pulse Timer: %d mS",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseTimer,100,1000},
    {eTextInt,"Long Press timer: %d",GetIntegerValue,&SystemInfo.DialSettings.m_nLongPressTimerValue,2,200},
    //{eBool,"Rotate Dial Type: %s",ToggleBool,&SystemInfo.DialSettings.m_bToggleDial,0,0,0,"Toggle","Pulse"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
#define MAX_DIM_MODE (sizeof(DisplayDimModeText) / sizeof(*DisplayDimModeText) - 1)
MenuItem DisplayMenu[] = {
    {eExit,"Display Settings"},
    {eList, "Dimming Mode: %s", GetSelectChoice, &SystemInfo.eDisplayDimMode, 0, MAX_DIM_MODE, 0, NULL, NULL, UpdateDisplayDimMode, DisplayDimModeText},
    {eTextInt,"Bright Value: %d%%",GetIntegerValue,&SystemInfo.nDisplayBrightness,1,100,0,NULL,NULL,UpdateDisplayBrightness},
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_NONE},
    {eElse},
        {eTextInt,"Dim Value: %d%%",GetIntegerValue,&SystemInfo.nDisplayDimValue,0,100},
    {eEndif},
    {eIfIntEqual,"",NULL,&SystemInfo.eDisplayDimMode,DISPLAY_DIM_MODE_TIME},
        {eTextInt,"Display Dim Time: %d S",GetIntegerValue,&SystemInfo.nDisplayDimTime,0,120},
    {eEndif},
    {eMenu,"Sideways Scroll Settings",{.menu = SidewaysScrollMenu}},
    {eBool,"Menu Choice: %s",ToggleBool,&SystemInfo.bMenuStar,0,0,0,"*","Color"},
    {eText,"Text Color",SetMenuColor},
    //{eBool,"Menu Wrap: %s",ToggleBool,&SystemInfo.bAllowMenuWrap,0,0,0,"Yes","No"},
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
    //{eText,"5V Measurement",ShowUsbVoltage},
    //{eMenu,"WiFi Settings",{.menu = WiFiMenu}},
    {eText,"New Version BIN file",CheckUpdateBin},
    {eText,"Reset All Settings",FactorySettings},
    {eEditText,"Serial #: %s",NULL,SystemInfo.cSerialNumber ,1,sizeof(SystemInfo.cSerialNumber) - 1},
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
#if RADIO_UHF
    const char* RxOffsetModeText[] = { "-1200","0","+1200" };
#else
    const char* RxOffsetModeText[] = { "-600","0","+600" };
#endif
// the CTSS subtone list
const char* SubToneText[] = {
    "None","67","71.9","74.4","77","79.7","82.5","85.4","88.5","91.5","94.8",
    "97.4","100","103.5","107.2","110.9","114.8","118.8","123","127.3","131.8",
    "136.5","141.3","146.2","151.4","156.7","162.2","167.9","173.8","179.9","186.2",
    "192.8","203.5","210.7","218.1","225.7","233.6","241.8","250.3"
};
const char* DcsText[] = {
    "None",
	"023", "025", "026", "031", "032", "036", "043", "047", "051", "053", "054",
	"065", "071", "072", "073", "074", "114", "115", "116", "125", "131", "132",
	"134", "143", "152", "155", "156", "162", "165", "172", "174", "205", "223",
	"226", "243", "244", "245", "251", "261", "263", "265", "271", "306", "311",
	"315", "331", "343", "346", "351", "364", "365", "371", "411", "412", "413",
	"423", "431", "432", "445", "464", "465", "466", "503", "506", "516", "532",
	"546", "565", "606", "612", "624", "627", "631", "632", "654", "662", "664",
	"703", "712", "723", "731", "732", "734", "743", "754"
};

// the bandwidth
const char* BandWidthText[] = { "12.5","25" };

MenuItem RadioMenuMore[] = {
    {eExit,"More Radio Settings"},
    {eList,"RX Offset: %s kHz",GetSelectChoice,&SystemInfo.nRfOffset,0,sizeof(RxOffsetModeText) / sizeof(*RxOffsetModeText) - 1,0,NULL,NULL,NULL,RxOffsetModeText},
    {eList,"BandWidth: %s kHz",GetSelectChoice,&SystemInfo.nBandWidth,0,sizeof(BandWidthText) / sizeof(*BandWidthText) - 1,0,NULL,NULL,NULL,BandWidthText},
    {eBool,"CTCSS/DCS: %s",ToggleBool,&SystemInfo.bCTCSS,0,0,0,"CTCSS","DCS"},
    {eIfEqual,"",NULL,&SystemInfo.bCTCSS,true},
        {eList,"TX CTCSS: %s Hz",GetSelectChoiceList,&SystemInfo.nTxCTCSS,0,sizeof(SubToneText) / sizeof(*SubToneText) - 1,0,NULL,NULL,NULL,SubToneText},
        {eList,"RX CTCSS: %s Hz",GetSelectChoiceList,&SystemInfo.nRxCTCSS,0,sizeof(SubToneText) / sizeof(*SubToneText) - 1,0,NULL,NULL,NULL,SubToneText},
    {eElse},
        {eList,"TX DCS: %s",GetSelectChoiceList,&SystemInfo.nTxDcs,0,sizeof(DcsText) / sizeof(*DcsText) - 1,0,NULL,NULL,NULL,DcsText},
        {eBool,"TX I/N: %s",ToggleBool,&SystemInfo.bTxDcsNI,0,0,0,"N","I"},
        {eList,"RX DCS: %s",GetSelectChoiceList,&SystemInfo.nRxDcs,0,sizeof(DcsText) / sizeof(*DcsText) - 1,0,NULL,NULL,NULL,DcsText},
        {eBool,"RX I/N: %s",ToggleBool,&SystemInfo.bRxDcsNI,0,0,0,"N","I"},
    {eEndif},
    {eTextInt,"RX Volume: %d",GetIntegerValue,&SystemInfo.nRxVolume,1,8},
    {eTextInt,"RX Squelch: %d",GetIntegerValue,&SystemInfo.nSquelch,0,8},
    {eTextInt,"DTMF Active: %d Sec",GetIntegerValue,&SystemInfo.nDtmfEnableTimer,1,20},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem RadioTimersMenu[] = {
    {eExit,"Radio Timers"},
    {eTextInt,"Start Delay: %d Min",GetIntegerValue,&SystemInfo.nStartDelayTimer,0,120},
    {eTextInt,"TX Send: %d Sec",GetIntegerValue,&SystemInfo.nTxTime,1,300},
    {eTextInt,"TX Pause: %d Sec",GetIntegerValue,&SystemInfo.nTxPause,1,600},
    {eBool,"TX Stop: %s",ToggleBool,&SystemInfo.bStopImmediately,0,0,0,"Immediate","Finish Cycle"},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem RadioMenu[] = {
#if RADIO_UHF
    {eExit,"UHF Radio Settings"},
#else
    {eExit,"VHF Radio Settings"},
#endif
    {eBool,"XMIT: %s",ToggleBool,&SystemInfo.bXmitEnable,0,0,0,"On","Off"},
    {eBool,"TX Power: %s",ToggleBool,&SystemInfo.bTxPowerLow,0,0,0,"Low","High"},
#if RADIO_UHF
    {eTextInt,"TX: %d.%03d MHz",GetIntegerValue,&SystemInfo.nFrequency,400000,480000,3},
#else
    {eTextInt,"TX: %d.%03d MHz",GetIntegerValue,&SystemInfo.nFrequency,134000,174000,3},
#endif
    {eEditText,"Call Sign: %s",GetText,SystemInfo.cRadioCallSign,1,sizeof(SystemInfo.cRadioCallSign) - 1},
    {eEditText,"Beacon: %s",GetText,SystemInfo.cBeaconString,1,sizeof(SystemInfo.cBeaconString) - 1},
    {eBool,"Play Audio File: %s",ToggleBool,&SystemInfo.bPlayAudioFile,0,0,0,"Yes","No"},
    {eIfEqual,"",NULL,&SystemInfo.bPlayAudioFile,true},
        {eEditText,"Audio File: %s",GetAudioFile,SystemInfo.cAudioFile,1,sizeof(SystemInfo.cAudioFile) - 1},
    {eEndif},
    {eTextInt,"Morse Interval: %d mS",GetIntegerValue,&SystemInfo.nMorseInterval,50,500},
    {eExit,PreviousMenu},
    // make sure this one is last
    {eTerminate}
};
MenuItem MainMenu[] = {
    {eMenu,"Radio Settings",{.menu = RadioMenu}},
    {eMenu,"Radio Timers",{.menu = RadioTimersMenu}},
    {eMenu,"More Radio Settings",{.menu = RadioMenuMore}},
    {eText,"Cancel Waits/TX Now",CancelWaitTimers},
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

bool g_bMenuChanged = true;

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
    // whether we are going right or left
    int nRollDirection;
};
std::vector<struct TEXTLINES> TextLines;

// radio event flags, 8 possible, 1,2,4,8,16,32,64,128
EventGroupHandle_t gRadioEventsHandle;
#define RadioEventReady 0x01            // the radio is ready
#define RadioEventDelayStart 0x02       // delay start active
#define RadioEventEnableTransmit 0x04   // transmit enabled
#define RadioEventIsTransmitting 0x08   // keep track of transmitting or not
#define RadioEventCancelWaits 0x10      // clear the delay and pause timers
// some useful macros
#define IsRadioReady ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventReady) != 0)
#define IsTransmitEnabled ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventEnableTransmit) != 0)
#define IsTransmitting ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventIsTransmitting) != 0)
#define IsCancelWaits ((xEventGroupGetBits(gRadioEventsHandle) & RadioEventCancelWaits) != 0)

// task handles for running the radio parts
TaskHandle_t TaskRunRadioHandle;
TaskHandle_t TaskRunTransmitHandle;
TaskHandle_t TaskSendBeaconHandle;
TaskHandle_t TaskSendMusicHandle;
TaskHandle_t TaskShowBatteryHandle;
TaskHandle_t TaskDTMFHandle;
TaskHandle_t TaskScrollSidewaysHandle;
TaskHandle_t TaskMenuHandle;
// a mutex to control access to writing on the display, the TFT driver is not re-entrant
SemaphoreHandle_t MutexDisplayHandle;

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
void WavPlayer(char* wavfile);
