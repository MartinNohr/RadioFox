#pragma once
// menu entry types
enum eDisplayOperation
{
    eTerminate = 0, // must be last in a menu, (or use {})
    eText,          // handle text with optional %s value, display only
    eTextInt,       // handle text with optional %d value
    eTextFloat,     // handle double float values
    eEditText,      // edit a text string
    // eChooseFile,        // choose a file from the SD card
    eBool,       // handle bool using %s and on/off values
    eMenu,       // load another menu
    eExit,       // closes this menu, handles optional %d or %s in string
    eIfEqual,    // start skipping menu entries if match with boolean data value
    eIfIntEqual, // start skipping menu entries if match with int data value
    eElse,       // toggles the skipping
    eEndif,      // ends an if block
    eReboot,     // reboot the system
    eList,       // used to rotate selection from a list of choices
};

// we need to have a pointer reference to this in the MenuItem, the full declaration follows later
struct BuiltInItem;
// define the menu item structure, this is used to define the menu items and their properties
struct MenuItem
{
    enum eDisplayOperation op;
    const char *text; // text to display
    union
    {
        void (*function)(MenuItem *); // called on click
        MenuItem *menu;               // jump to another menu
        BuiltInItem *builtin;         // builtin items
    };
    const void *value; // associated variable
    union
    {
        long min; // the minimum value, also used for ifequal, min length for string
        double fmin;
    };
    union
    {
        long max; // the maximum value, also size to compare for if, max length for string or eList
        double fmax;
    };
    int decimals;    // 0 for int, 1 for 0.1, 2 for 0.01 etc
    const char *on;  // text for boolean true
    const char *off; // text for boolean false
    // flag is 1 for first time, 0 for changes, and -1 for last call, bools only call this with -1
    void (*change)(MenuItem *, int flag); // call for each change, example: brightness change show effect, can be NULL
    const char **nameList;                // used for multichoice of items, example wiring mode, .max should be count-1 and .min=0
    const char *cHelpText;                // a place to put some menu help
};

void RunMenus(int button);
void ShowMenu(MenuItem *menu);
bool HandleRunMode();
bool HandleMenus();
bool UpMenuLevel(bool gotoMain);
void SetMenuColor(MenuItem *menu);
int FindMenuColor(uint16_t col);
void GetFloatValue(MenuItem *menu);
void GetIntegerValue(MenuItem *menu);
bool GetSelectChoiceListHelper(MenuItem *menu);
void GetSelectChoiceList(MenuItem *menu);
void GetSelectChoice(MenuItem *menu);
void ToggleBool(MenuItem *menu);
void TaskMenu(void *params);
bool GetYesNo(String msg);
void ResetDimTimer();
void UpdateDisplayBrightness(MenuItem *menu, int flag);
void UpdateDisplayDimMode(MenuItem *menu, int flag);
void SetDisplayBrightness(int val);
void DrawProgressBar(int x, int y, int dx, int dy, int percent, bool rect);
void SaveEepromSettings(MenuItem *menu);
void DeleteSettingsFile(MenuItem *);
void LoadSettingsFromFile(MenuItem *);
void SaveSettingsInFile(MenuItem *);
void CreateSettingsFile(MenuItem *);
String GetSettingsFilename();
String GetFilename();
bool CompareNames(const String &a, const String &b);
void WriteMessage(String txt, bool error = false, int wait = 2000, bool process = false);
String FormatMultiLine(String &input);
void DisplayMenuLine(int lineNum, int displine, String text);
void ResetTextLines();
void ClearScreen();
void DisplayLine(int lineNum, String text, uint16_t color = TFT_WHITE, uint16_t backColor = TFT_BLACK);
void CheckUpdateBin(MenuItem *menu);
void ShowUpdateProgress(size_t x, size_t total);
void ShowProgressBar(int percent);
void SetHighBattery(MenuItem *);
void ShowBattery(MenuItem *menu);
int ReadBattery(int *raw);
void DrawProgressBar(int x, int y, int dx, int dy, int percent, bool rect);
void SaveEepromSettings(MenuItem *menu);
void DeleteSettingsFile(MenuItem *);
void LoadSettingsFromFile(MenuItem *);
void SaveSettingsInFile(MenuItem *);
void CreateSettingsFile(MenuItem *);
String GetSettingsFilename();
String GetFilename();
void CheckUpdateBin(MenuItem *menu);
void GetNetworkName(MenuItem *menu);
int ScanForNetworks();
void GetText(MenuItem *menu);
void GetAudioFile(MenuItem *menu);
void ToggleWebServer(MenuItem *menu);
void CancelWaitTimers(MenuItem *);
void DisplayBottomChar(char ch);
String MenuToHtml(MenuItem *menu, bool bActive = true, int nLevel = 0);
void GetFileNamesFromSD(std::vector<String> &FileNames, bool bAppend = false, String ext = "", String dir = "/");
void FactorySettings(MenuItem *menu);
void EraseFlash(MenuItem *menu);
bool SaveLoadBatterySettings(bool save);
void setupSDcard();
bool SaveLoadSettings(bool save, bool nodisplay);
bool CheckCancel(bool bLeaveButton);
enum CRotaryDialButton::Button ReadButton();
bool CompareRadioSettings(SYSTEM_INFO *pSystemInfo, SYSTEM_INFO *pSystemInfoSaved);
void InitMenuSystem();
// the SD card
inline SDFile dataFile;

// a stack for menus so we can find our way back
struct MenuInfo
{
    int index;      // active entry
    int offset;     // scrolled amount
    int menucount;  // how many entries in this menu
    MenuItem *menu; // pointer to the menu
};
inline MenuInfo *menuPtr;
inline std::stack<MenuInfo *> MenuStack;
inline bool g_bMenuChanged = true;
inline RTC_DATA_ATTR int nMenuLineCount = 7;
// keep the display lines in here so we can scroll sideways if necessary
struct TEXTLINES
{
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
inline std::vector<struct TEXTLINES> TextLines;

// definitions for preferences
inline const char *prefsName = "FOX";
inline const char *prefsVars = "vars";
inline const char *prefsVersion = "version";
inline const char *prefsSystemInfo = "systeminfo";
inline const char *prefsBatteryInfo = "batteryinfo";
// used in menus
inline std::vector<bool> bMenuValid; // set to indicate menu item  is valid
inline const char *PreviousMenu = "Back";
// menu definition

// all the menus
inline MenuItem BatteryMenu[] = {
    {eExit, "Battery"},
    {eBool, "Show Battery: %s", ToggleBool, &SystemInfo.bShowBatteryLevel, 0, 0, 0, "Yes", "No"},
    //{eTextInt,"Reset Calibration",ResetBattery},
    {eTextInt, "Full Battery: %d", GetIntegerValue, &BatteryInfo.nBatteryFullLevel, 2000, 4000},
    {eTextInt, "Set Full Battery", SetHighBattery},
    //{eTextInt,"Low Battery: %d",GetIntegerValue,&BatteryInfo.nBatteryLowLevel,2000,4000},
    //{eTextInt,"Set Low Battery",SetLowBattery},
    {eText, "Read Battery Raw Data", ShowBattery},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
inline MenuItem SidewaysScrollMenu[] = {
    {eExit, "Sideways Scrolling"},
    {eTextInt, "Speed: %d mS", GetIntegerValue, &SystemInfo.nSidewayScrollSpeed, 1, 1000},
    {eTextInt, "Pause: %d mS", GetIntegerValue, &SystemInfo.nSidewaysScrollPause, 1, 100},
    {eTextInt, "Reverse Speed: %dx", GetIntegerValue, &SystemInfo.nSidewaysScrollReverse, 1, 20},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
inline MenuItem DialMenu[] = {
    {eExit, "Rotary Dial Settings"},
    {eBool, "Direction: %s", ToggleBool, &SystemInfo.DialSettings.m_bReverseDial, 0, 0, 0, "Reverse", "Normal"},
    //{eTextInt,"Pulse Count: %d",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseCount,1,5},
    //{eTextInt,"Pulse Timer: %d mS",GetIntegerValue,&SystemInfo.DialSettings.m_nDialPulseTimer,100,1000},
    {eTextInt, "Long Press timer: %d", GetIntegerValue, &SystemInfo.DialSettings.m_nLongPressTimerValue, 2, 200},
    //{eBool,"Rotate Dial Type: %s",ToggleBool,&SystemInfo.DialSettings.m_bToggleDial,0,0,0,"Toggle","Pulse"},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
#define MAX_DIM_MODE (sizeof(DisplayDimModeText) / sizeof(*DisplayDimModeText) - 1)
inline MenuItem DisplayMenu[] = {
    {eExit, "Display Settings"},
    {eList, "Dimming Mode: %s", GetSelectChoice, &SystemInfo.eDisplayDimMode, 0, MAX_DIM_MODE, 0, NULL, NULL, UpdateDisplayDimMode, DisplayDimModeText},
    {eTextInt, "Bright Value: %d%%", GetIntegerValue, &SystemInfo.nDisplayBrightness, 1, 100, 0, NULL, NULL, UpdateDisplayBrightness},
    {eIfIntEqual, "", NULL, &SystemInfo.eDisplayDimMode, DISPLAY_DIM_MODE_NONE},
    {eElse},
    {eTextInt, "Dim Value: %d%%", GetIntegerValue, &SystemInfo.nDisplayDimValue, 0, 100},
    {eEndif},
    {eIfIntEqual, "", NULL, &SystemInfo.eDisplayDimMode, DISPLAY_DIM_MODE_TIME},
    {eTextInt, "Display Dim Time: %d S", GetIntegerValue, &SystemInfo.nDisplayDimTime, 0, 120},
    {eEndif},
    {eMenu, "Sideways Scroll Settings", {.menu = SidewaysScrollMenu}},
    {eBool, "Menu Selection: %s", ToggleBool, &SystemInfo.bMenuStar, 0, 0, 0, "*", "Color"},
    {eText, "Text/HiLite Colors", SetMenuColor},
    //{eBool,"Menu Wrap: %s",ToggleBool,&SystemInfo.bAllowMenuWrap,0,0,0,"Yes","No"},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
inline MenuItem WiFiMenu[] = {
    {eExit, "WiFi Settings"},
    {eBool, "Web Server: %s", ToggleWebServer, &SystemInfo.bRunWebServer, 0, 0, 0, "On", "Off"},
    {eIfEqual, "", NULL, &SystemInfo.bRunWebServer, true},
    {eText, "Homepage: %s", NULL, localIpAddress},
    {eEndif},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
inline MenuItem SystemMenu[] = {
    {eExit, "System Settings"},
    {eMenu, "Display Settings", {.menu = DisplayMenu}},
    {eMenu, "Dial & Button Settings", {.menu = DialMenu}},
#if HAS_BATTERY_LEVEL
    {eMenu, "Battery Settings", {.menu = BatteryMenu}},
#endif
    //{eText,"5V Measurement",ShowUsbVoltage},
    // {eMenu, "WiFi Settings", {.menu = WiFiMenu}},
    {eText, "New Version BIN file", CheckUpdateBin},
    {eText, "Factory Settings", FactorySettings},
    {eText, "Format EEPROM", EraseFlash},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
inline MenuItem SaveSettingsMenu[] = {
    {eExit, "Saved Settings Files"},
    {eText, "Create File", CreateSettingsFile},
    {eText, "Save/Update File", SaveSettingsInFile},
    {eText, "Load File", LoadSettingsFromFile},
    {eText, "Delete File", DeleteSettingsFile},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
#if RADIO_UHF
inline const char *RxOffsetModeText[] = {"-1200", "0", "+1200"};
#else
inline const char *RxOffsetModeText[] = {"-600", "0", "+600"};
#endif
// the CTSS subtone list
inline const char *SubToneText[] = {
    "None", "67", "71.9", "74.4", "77", "79.7", "82.5", "85.4", "88.5", "91.5", "94.8",
    "97.4", "100", "103.5", "107.2", "110.9", "114.8", "118.8", "123", "127.3", "131.8",
    "136.5", "141.3", "146.2", "151.4", "156.7", "162.2", "167.9", "173.8", "179.9", "186.2",
    "192.8", "203.5", "210.7", "218.1", "225.7", "233.6", "241.8", "250.3"};
inline const char *DcsText[] = {
    "None",
    "023", "025", "026", "031", "032", "036", "043", "047", "051", "053", "054",
    "065", "071", "072", "073", "074", "114", "115", "116", "125", "131", "132",
    "134", "143", "152", "155", "156", "162", "165", "172", "174", "205", "223",
    "226", "243", "244", "245", "251", "261", "263", "265", "271", "306", "311",
    "315", "331", "343", "346", "351", "364", "365", "371", "411", "412", "413",
    "423", "431", "432", "445", "464", "465", "466", "503", "506", "516", "532",
    "546", "565", "606", "612", "624", "627", "631", "632", "654", "662", "664",
    "703", "712", "723", "731", "732", "734", "743", "754"};

// the bandwidth
inline const char *BandWidthText[] = {"12.5", "25"};

inline MenuItem RadioMenuMore[] = {
    {eExit, "More Radio Settings"},
    {eList, "RX Offset: %s kHz", GetSelectChoice, &SystemInfo.nRfOffset, 0, sizeof(RxOffsetModeText) / sizeof(*RxOffsetModeText) - 1, 0, NULL, NULL, NULL, RxOffsetModeText},
    {eList, "BandWidth: %s kHz", GetSelectChoice, &SystemInfo.nBandWidth, 0, sizeof(BandWidthText) / sizeof(*BandWidthText) - 1, 0, NULL, NULL, NULL, BandWidthText},
    {eBool, "CTCSS/DCS: %s", ToggleBool, &SystemInfo.bCTCSS, 0, 0, 0, "CTCSS", "DCS"},
    {eIfEqual, "", NULL, &SystemInfo.bCTCSS, true},
    {eList, "TX CTCSS: %s Hz", GetSelectChoiceList, &SystemInfo.nTxCTCSS, 0, sizeof(SubToneText) / sizeof(*SubToneText) - 1, 0, NULL, NULL, NULL, SubToneText},
    {eList, "RX CTCSS: %s Hz", GetSelectChoiceList, &SystemInfo.nRxCTCSS, 0, sizeof(SubToneText) / sizeof(*SubToneText) - 1, 0, NULL, NULL, NULL, SubToneText},
    {eElse},
    {eList, "TX DCS: %s", GetSelectChoiceList, &SystemInfo.nTxDcs, 0, sizeof(DcsText) / sizeof(*DcsText) - 1, 0, NULL, NULL, NULL, DcsText},
    {eBool, "TX I/N: %s", ToggleBool, &SystemInfo.bTxDcsNI, 0, 0, 0, "N", "I"},
    {eList, "RX DCS: %s", GetSelectChoiceList, &SystemInfo.nRxDcs, 0, sizeof(DcsText) / sizeof(*DcsText) - 1, 0, NULL, NULL, NULL, DcsText},
    {eBool, "RX I/N: %s", ToggleBool, &SystemInfo.bRxDcsNI, 0, 0, 0, "N", "I"},
    {eEndif},
    {eTextInt, "RX Volume: %d", GetIntegerValue, &SystemInfo.nRxVolume, 1, 8},
    {eTextInt, "RX Squelch: %d", GetIntegerValue, &SystemInfo.nSquelch, 0, 8},
    {eTextInt, "DTMF Active: %d Sec", GetIntegerValue, &SystemInfo.nDtmfEnableTimer, 1, 20},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
inline MenuItem RadioTimersMenu[] = {
    {eExit, "Radio Timers"},
    {eTextInt, "Start Delay: %d Min", GetIntegerValue, &SystemInfo.nStartDelayTimer, 0, 120},
    {eBool, "Timer Mode: %s", ToggleBool, &SystemInfo.bUseFixedTimers, 0, 0, 0, "Fixed", "Random"},
    {eIfEqual, "", NULL, &SystemInfo.bUseFixedTimers, true},
    {eTextInt, "TX Send: %d Sec", GetIntegerValue, &SystemInfo.nTxTimeFixed, 10, 90}, // too much xmit time causes hot radio
    {eTextInt, "TX Pause: %d Sec", GetIntegerValue, &SystemInfo.nTxPauseFixed, 10, 1200},
    {eElse},
    {eTextInt, "TX Send Min: %d Sec", GetIntegerValue, &SystemInfo.nTxTimeMin, 10, 90}, // too much xmit time causes hot radio
    {eTextInt, "TX Send Max: %d Sec", GetIntegerValue, &SystemInfo.nTxTimeMax, 10, 90}, // too much xmit time causes hot radio
    {eTextInt, "TX Pause Min: %d Sec", GetIntegerValue, &SystemInfo.nTxPauseMin, 10, 1200},
    {eTextInt, "TX Pause Max: %d Sec", GetIntegerValue, &SystemInfo.nTxPauseMax, 10, 1200},
    {eEndif},
    // {eBool, "TX Stop: %s", ToggleBool, &SystemInfo.bStopImmediately, 0, 0, 0, "Immediate", "Finish Cycle"},
    {eBool, "Radio Pause: %s", ToggleBool, &SystemInfo.bSleepWhilePausing, 0, 0, 0, "Sleep", "Awake"},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
#define BEACON_LOW_FREQUENCY 144275
#define BEACON_HIGH_FREQUENCY 144300
inline MenuItem RadioMenu[] = {
#if RADIO_UHF
    {eExit, "UHF Radio Settings"},
#else
    {eExit, "VHF Radio Settings"},
#endif
    {eBool, "XMIT: %s", ToggleBool, &SystemInfo.bXmitEnable, 0, 0, 0, "On", "Off"},
    {eBool, "TX Power: %s", ToggleBool, &SystemInfo.bTxPowerLow, 0, 0, 0, "Low", "High"},
#if RADIO_UHF
    {eTextInt, "TX: %d.%03d MHz", GetIntegerValue, &SystemInfo.nFrequency, 400000, 480000, 3},
#else // VHF here
    {eIfEqual, "", NULL, &SystemInfo.bBeaconMode, true},
    {eTextInt, "TX: %d.%03d MHz", GetIntegerValue, &SystemInfo.nFrequency, BEACON_LOW_FREQUENCY, BEACON_HIGH_FREQUENCY, 3},
    {eElse},
    {eTextInt, "TX: %d.%03d MHz", GetIntegerValue, &SystemInfo.nFrequency, 134000, 174000, 3},
    {eEndif},
    {eBool, "Beacon Mode: %s", ToggleBool, &SystemInfo.bBeaconMode, 0, 0, 0, "Yes", "No"},
    {eIfEqual, "", NULL, &SystemInfo.bBeaconMode, true},
    {eBool, "Send GPS: %s", ToggleBool, &SystemInfo.bSendGPS, 0, 0, 0, "Yes", "No"},
    {eIfEqual, "", NULL, &SystemInfo.bSendGPS, true},
    {eTextFloat, "Latitude: %.6f", GetFloatValue, &SystemInfo.fLatitude, {.fmin = -180.0}, {.fmax = 180.0}, 6},
    {eTextFloat, "Longitude: %.6f", GetFloatValue, &SystemInfo.fLongitude, {.fmin = -180.0}, {.fmax = 180.0}, 6},
    {eEndif},
    {eEndif},
#endif
    {eEditText, "Radio ID: %s", GetText, SystemInfo.cRadioString, 1, sizeof(SystemInfo.cRadioString) - 1},
    {eEditText, "Call Sign: %s", GetText, SystemInfo.cRadioCallSign, 1, sizeof(SystemInfo.cRadioCallSign) - 1},
    {eBool, "Play Audio File: %s", ToggleBool, &SystemInfo.bPlayAudioFile, 0, 0, 0, "Yes", "No"},
    {eIfEqual, "", NULL, &SystemInfo.bPlayAudioFile, true},
    {eEditText, "Audio File: %s", GetAudioFile, SystemInfo.cAudioFile, 1, sizeof(SystemInfo.cAudioFile) - 1},
    {eEndif},
    {eTextInt, "Morse Interval: %d mS", GetIntegerValue, &SystemInfo.nMorseInterval, 50, 500},
    {eTextInt, "Morse Pitch: %d Hz", GetIntegerValue, &SystemInfo.nBuzzerFrequency, 300, 3000},
    {eExit, PreviousMenu},
    // make sure this one is last
    {eTerminate}};
inline MenuItem MainMenu[] = {
    {eExit, "Main Screen"},
    {eMenu, "Radio Settings", {.menu = RadioMenu}},
    {eMenu, "Radio Timers", {.menu = RadioTimersMenu}},
    {eMenu, "More Radio Settings", {.menu = RadioMenuMore}},
    {eText, "Cancel Waits/TX Now", CancelWaitTimers},
    {eMenu, "Saved Settings Files", {.menu = SaveSettingsMenu}},
    {eMenu, "System Settings", {.menu = SystemMenu}},
    {eText, "Save Current Settings", SaveEepromSettings},
    {eReboot, "Reboot"},
    {eExit, "Main Screen"},
    // make sure this one is last
    {eTerminate}};
