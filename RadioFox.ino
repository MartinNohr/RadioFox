/*
 Name:		RadioFox.ino
 Created:	3/25/2023 8:37:33 AM
 Author:	Sven Schumacher & Martin Nohr
 Call Sign: VE6IDK & KK7JTE
*/
#include <phoneDTMF.h>
#include "pitches.h"
#include <EEPROM.h>
#include "RadioFox.h"

PhoneDTMF dtmf = PhoneDTMF();

const int tempo = 145; // Tempo of Melody

// notes in the melody:
int melody[] = {

  NOTE_E5, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_C5, 8,  NOTE_B4, 8,
  NOTE_A4, 4,  NOTE_A4, 8,  NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8,
  NOTE_B4, -4,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4,
  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 8,  NOTE_A4, 4,  NOTE_B4, 8,  NOTE_C5, 8,

  NOTE_D5, -4,  NOTE_F5, 8,  NOTE_A5, 4,  NOTE_G5, 8,  NOTE_F5, 8,
  NOTE_E5, -4,  NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8,
  NOTE_B4, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4,
  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 4, REST, 4,
};

// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
// there are two values per note (pitch and duration), so for each note there are four bytes
int notes = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms (60s/tempo)*4 beats
int wholenote = (60000 * 4) / tempo;
int divider = 0, noteDuration = 0;

// timer called every second
void periodic_Second_timer_callback(void* arg)
{
	if (SystemInfo.eDisplayDimMode == DISPLAY_DIM_MODE_TIME && displayDimTimer) {
		--displayDimTimer;
		if (displayDimTimer == 0) {
			displayDimNow = true;
		}
	}
}

// tone generator
const int toneChannel = 2;

constexpr int TFT_ENABLE = 4;
// use these to control the LCD brightness
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
TFT_eSprite LineSprite = TFT_eSprite(&tft);  // Create Sprite object "LineSprite" with pointer to "tft" object
#define BATTERY_BAR_HEIGHT 5
TFT_eSprite BatterySprite = TFT_eSprite(&tft);  // Create Sprite object "BatterySprite" with pointer to "tft" object

// handle the sideways scrolling of long lines
void TaskScrollSideways(void* params)
{
	// use this to make task run every second
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(SystemInfo.nSidewayScrollSpeed);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	for (;;) {
		for (int ix = 0; ix < nMenuLineCount; ++ix) {
			// see if we need to restart because the screen was cleared
			if (ulTaskNotifyTake(pdTRUE, 0) != 0) {
				break;
			}
			int offset = TextLines[ix].nRollOffset;
			if (TextLines[ix].nRollLength) {
				if (TextLines[ix].nRollOffset == 0 && TextLines[ix].nRollDirection == 0) {
					TextLines[ix].nRollDirection = SystemInfo.nSidewaysScrollPause;
					continue;
				}
				if (TextLines[ix].nRollDirection > 1) {
					--TextLines[ix].nRollDirection;
				}
				if (TextLines[ix].nRollDirection == 1) {
					++TextLines[ix].nRollOffset;
				}
				if (TextLines[ix].nRollOffset >= (TextLines[ix].nRollLength - tft.width()) && TextLines[ix].nRollDirection > 0) {
					TextLines[ix].nRollDirection = -SystemInfo.nSidewaysScrollPause;
				}
				if (TextLines[ix].nRollDirection < -1) {
					++TextLines[ix].nRollDirection;
				}
				if (TextLines[ix].nRollDirection == -1) {
					TextLines[ix].nRollOffset -= SystemInfo.nSidewaysScrollReverse;
					if (TextLines[ix].nRollOffset < 0) {
						TextLines[ix].nRollOffset = 0;
					}
					if (TextLines[ix].nRollOffset == 0) {
						TextLines[ix].nRollDirection = 0;
					}
				}
				if (offset != TextLines[ix].nRollOffset) {
					DisplayLine(ix, TextLines[ix].Line, TextLines[ix].foreColor, TextLines[ix].backColor);
				}
			}
		}
		// Wait for the next cycle.
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void TaskSendBeacon(void* parameter)
{
	for (int i = 0; i < sizeof(SystemInfo.cBeaconString); i++) {
		if (SystemInfo.cBeaconString[i] == '\0')
			break;
		sendLetter(SystemInfo.cBeaconString[i]);
	}
	for (int i = 0; i < sizeof(SystemInfo.cRadioID); i++) {
		if (SystemInfo.cRadioID[i] == '\0')
			break;
		sendLetter(SystemInfo.cRadioID[i]);
	}
	// terminate this task
	TaskSendBeaconHandle = NULL;
	vTaskDelete(NULL);
}

//task to send the music
void TaskSendMusic(void* parameter)
{
	// iterate over the notes of the melody.
	// Remember, the array is twice the number of notes (notes + durations)
	for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
		// calculates the duration of each note
		divider = melody[thisNote + 1];
		if (divider > 0) {
			// regular note, just proceed
			noteDuration = (wholenote) / divider;
		}
		else if (divider < 0) {
			// dotted notes are represented with negative durations!!
			noteDuration = (wholenote) / abs(divider);
			noteDuration *= 1.5; // increases the duration in half for dotted notes
		}
		// we only play the note for 90% of the duration, leaving 10% as a pause
		ledcWriteTone(toneChannel, melody[thisNote]);
		vTaskDelay(pdMS_TO_TICKS(noteDuration * 0.9));
		ledcWriteTone(toneChannel, 0);
		vTaskDelay(pdMS_TO_TICKS(noteDuration * 0.1));
		// Wait for the special duration before playing the next note.
		//delay(noteDuration);
		// stop the waveform generation before the next note.
		//noTone(BUZZER_PIN);
	}
	// terminate this task
	TaskSendMusicHandle = NULL;
	vTaskDelete(NULL);
}

// this controls the radio sending operations
void TaskRunTransmit(void* parameter)
{
	gpio_set_level((gpio_num_t)PTT_PORT, 0);
	xTaskNotify(TaskRunRadioHandle, (uint32_t)"TX Start", eSetValueWithOverwrite);
	// wait for PTT to take effect
	vTaskDelay(pdMS_TO_TICKS(500));
	// a list of our tasks to run
	static const struct RFTaskEntry {
		char* name;
		void (*task)(void* pArgs);
		TaskHandle_t* pTaskHandle;
	} RFTaskList[] = {
		{"Music",TaskSendMusic,&TaskSendMusicHandle},
		{"Beacon",TaskSendBeacon,&TaskSendBeaconHandle},
	};
	bool bDone = false;
	while (!bDone && ulTaskNotifyTake(pdTRUE, 0) == 0) {
		for (const struct RFTaskEntry& pte : RFTaskList) {
			// send the name for display
			xTaskNotify(TaskRunRadioHandle, (uint32_t)pte.name, eSetValueWithOverwrite);
			// start the task
			xTaskCreate(pte.task, pte.name, 2000, NULL, 4, pte.pTaskHandle);
			// wait for it to complete or be cancelled
			while (*pte.pTaskHandle) {
				// check for timeout or cancel task
				if (SystemInfo.bStopImmediately && ulTaskNotifyTake(pdTRUE, 0)) {
					if (*pte.pTaskHandle)
						vTaskDelete(*pte.pTaskHandle);
					*pte.pTaskHandle = NULL;
					bDone = true;
					break;
				}
				vTaskDelay(pdMS_TO_TICKS(1000));
			}
			if (bDone)
				break;
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
	// Turn off the output tone also
	ledcWriteTone(toneChannel, 0);
	// turn PTT off here
	gpio_set_level((gpio_num_t)PTT_PORT, 1);
	TaskRunTransmitHandle = NULL;
	vTaskDelete(NULL);
	//int freestack = uxTaskGetStackHighWaterMark(NULL);
	//Serial.println(String("xmit high water: ") + freestack);
}

// all the timing and screen display is done here
void TaskRunRadio(void* parameter)
{
	const char* cStatusText = NULL;
	uint32_t status = 0;
	bool bTransmitting = false;
	int secondsLeft = 0;
	int txCount;
	bool bWaitingForStop = false;
	bool bWasXmit = SystemInfo.bXmit;
	// use this to make task run every second
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(1000);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	while (true) {
		if (SystemInfo.bXmit || TaskRunTransmitHandle) {
			if (bWasXmit != SystemInfo.bXmit) {
				// tell the xmitter to stop
				if (TaskRunTransmitHandle && bWasXmit)
					xTaskNotify(TaskRunTransmitHandle, 1, eSetValueWithOverwrite);
				// set if we were xmitting
				bWaitingForStop = bWasXmit;
				// don't do this code again until the bXmit flag actually changes
				bWasXmit = SystemInfo.bXmit;
			}
			if (bWaitingForStop) {
				// check if the xmitter is finished
				if (!TaskRunTransmitHandle) {
					bWaitingForStop = false;
					secondsLeft = SystemInfo.nTxPause;
				}
				// check for strings
				status = ulTaskNotifyTake(pdTRUE, 0);
				if (status == 0) {
					// update the running display to show we're waiting
					if (!g_bSettingsMode) {
						DisplayLine(0, String(cStatusText) + ": Stopping");
					}
				}
				else {
					// must be a string pointer
					cStatusText = (const char*)status;
				}
			}
			// see if the timer has run out and we need to change state
			else if (secondsLeft <= 0) {
				bTransmitting = !bTransmitting;
				if (bTransmitting) {
					// start the xmitter task
					xTaskCreate(TaskRunTransmit, "XMITFOX", 2000, NULL, 2, &TaskRunTransmitHandle);
					vTaskDelay(pdMS_TO_TICKS(5));
					txCount++;
					secondsLeft = SystemInfo.nTxTime;
				}
				else {
					// tell the xmitter to stop
					if (TaskRunTransmitHandle)
						xTaskNotify(TaskRunTransmitHandle, 1, eSetValueWithOverwrite);
					bWaitingForStop = true;
				}
			}
			// see if the task sent us anything
			status = ulTaskNotifyTake(pdTRUE, 0);
			// check if this is a string pointer
			if (status > 1) {
				cStatusText = (const char*)status;
			}
			if (!bTransmitting && !bWaitingForStop) {
				cStatusText = "Pause";
			}
			if (!g_bSettingsMode && !bWaitingForStop) {
				int lineNo = 0;
				char fmt[20];
				DisplayLine(lineNo++, String(cStatusText) + ": " + (secondsLeft / 60) + " Min " + (secondsLeft % 60) + " Sec");
				DisplayLine(lineNo++, String("TX Count: ") + txCount);
				DisplayLine(lineNo++, String(SystemInfo.cBeaconString) + " " + SystemInfo.cRadioID, SystemInfo.menuTextColor);
				sprintf(fmt, "%03d MHz ", SystemInfo.nFrequency % 1000);
				DisplayLine(lineNo++, String(SystemInfo.nFrequency / 1000) + "." + fmt + (SystemInfo.bTxPowerHi ? "High" : "Low"), SystemInfo.menuTextColor);
				DisplayLine(lineNo++, String("RX Offset: ") + RxOffsetModeText[SystemInfo.nRfOffset] + " kHz", SystemInfo.menuTextColor);
				DisplayLine(lineNo++, String("Audio: ") + SystemInfo.cAudioFile, SystemInfo.menuTextColor);
			}
			if (secondsLeft)
				--secondsLeft;
			//int freestack = uxTaskGetStackHighWaterMark(NULL);
			//Serial.println(String("radio high water: ") + freestack);
		}
		if (!SystemInfo.bXmit && !TaskRunTransmitHandle) {
			if (!g_bSettingsMode) {
				DisplayLine(0, "Not Transmitting");
			}
			bTransmitting = false;
			bWaitingForStop = false;
			secondsLeft = 0;
		}
		// Wait for the next cycle.
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

// show the battery every 60 seconds
void TaskShowBattery(void* parameters)
{
	while (true) {
		// show battery level if on
		if (SystemInfo.bShowBatteryLevel && !g_bSettingsMode) {
			int raw;
			ReadBattery(&raw);
			ShowBattery(NULL);
		}
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(60000));
	}
}

// handle DTMF commands, runs every second
void TaskDTMF(void* parameter)
{
	// use this to make task run every second
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(1000);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	while (true) {
		uint8_t   tones;
		char      button;
		// detect tone
		tones = dtmf.detect();

		// if valid tone was found, proof for validity
		button = dtmf.tone2char(tones);
		//Serial.println(String("tone:") + (int)(dtmf.tone2char(tones)));
		if (button > 0)
		{
			// TODO: suspend transmitting here?

			//Serial.print("Detected tone...");
			// measure 4 times, result of each measurement should be always the same 
			// time need for this process: 80ms, so the tone must be present at least 100ms to be valid
			tones |= dtmf.detect() | dtmf.detect() | dtmf.detect();
			switch (dtmf.tone2char(tones)) {
			case '1':// Number 1 - Start Loop
				digitalWrite(TXHIPOWER_PORT, LOW);
				digitalWrite(PTT_PORT, LOW);
				delay(1500);
				sendLetter('R');
				digitalWrite(PTT_PORT, HIGH);
				SystemInfo.bXmit = true;        // set the flag to ENABLE transmissions
				break;

			case '2':// Number 2 - LOW Power Mode - No Loop           
				digitalWrite(PTT_PORT, LOW);
				delay(1500);
				sendLetter('R');
				digitalWrite(PTT_PORT, HIGH);
				digitalWrite(TXHIPOWER_PORT, LOW);
				SystemInfo.bXmit = false;
				break;

			case '3':// Number 3 - High Power Mode
				digitalWrite(TXHIPOWER_PORT, HIGH);
				digitalWrite(PTT_PORT, LOW);
				delay(1500);
				sendLetter('R');
				digitalWrite(PTT_PORT, HIGH);
				SystemInfo.bXmit = true;
				break;

			default:     // any other number, turn off transmissions - send a short letter to confirm receive
				digitalWrite(PTT_PORT, LOW);
				delay(1500);
				sendLetter('R');
				digitalWrite(PTT_PORT, HIGH);
				SystemInfo.bXmit = false;    // set the flag to DISABLE transmissions
				break;
			}
		}
		// Wait for the next cycle.
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

// load the radio settings
// TODO: do we need to wait for radio not transmitting?
bool RadioSetup()
{
	bool retval = false;
	Serial.println("radio setup");
	RadioSerial.println("AT+DMOCONNECT");
	delay(1000);
	// see if the radio answers
	if (RadioSerial.available()) {
		String str = RadioSerial.readString();
		Serial.println("Radio:" + str);
		// check the return value
		if (str.indexOf(":0") > 0) {
			// set the radio data, e.g. AT+DMOSETGROUP=0,415.1250,415.1250,0012,4, 0013
			char line[200];
			float fRX = SystemInfo.nFrequency / 1000.0;
			float fTX = (SystemInfo.nFrequency + atof(RxOffsetModeText[SystemInfo.nRfOffset])) / 1000.0;
			sprintf(line, "AT+DMOSETGROUP=0,%.4f,%.4f,0012,4,0013", fTX, fRX);
			Serial.println(line);
			RadioSerial.println(line);
			delay(100);
			if (RadioSerial.available()) {
				str = RadioSerial.readString();
				Serial.println("Radio Group Reply:" + str);
				if (str.indexOf(":0") > 0) {
					Serial.println("Radio Ready");
					// it worked
					retval = true;
				}
			}
		}
	}
	else {
		Serial.println("Radio did not respond");
		WriteMessage("Radio Not Found", true);
	}
	if (retval) {
		Serial.println("Radio init successful");
		WriteMessage("Radio Initialized");
	}
	// set the radio power control output
	digitalWrite(TXHIPOWER_PORT, SystemInfo.bTxPowerHi);
	return retval;
}

// handle the dial and menu system
// also check for some system settings changes and deal with them, e.g. radio settings
void TaskMenu(void* params)
{
	static SYSTEM_INFO SystemInfoSaved;
	static bool bLastSettingsMode = false;
	for (;;) {
		if (g_bSettingsMode) {
			HandleMenus();
		}
		else {
			HandleRunMode();
		}
		// did settings mode just turn on?
		if (g_bSettingsMode && !bLastSettingsMode) {
			memcpy(&SystemInfoSaved, &SystemInfo, sizeof(SystemInfo));
			ClearScreen();
		}
		// did settings mode just turn off?
		if (!g_bSettingsMode && bLastSettingsMode) {
			// show the battery display by telling the task to run
			xTaskNotifyGive(TaskShowBatteryHandle);
			if (memcmp(&SystemInfoSaved, &SystemInfo, sizeof(SystemInfo))) {
				// make sure that the lcd dim is less than the bright
				if (SystemInfo.nDisplayDimValue > SystemInfo.nDisplayBrightness)
					SystemInfo.nDisplayDimValue = SystemInfo.nDisplayBrightness;
				// see if any radio settings changed
				if (SystemInfo.nFrequency != SystemInfoSaved.nFrequency
					|| SystemInfo.nRfOffset != SystemInfoSaved.nRfOffset
					|| SystemInfo.bTxPowerHi != SystemInfoSaved.bTxPowerHi) {
					// tell the radio
					if (RadioSetup()) {
						// worked
					}
					else {
						// failed, turn off transmit
						//SystemInfo.bXmit = false;
					}
				}
				ClearScreen();
				SaveLoadSettings(true, false);
				// copy so we know we updated things
				memcpy(&SystemInfoSaved, &SystemInfo, sizeof(SystemInfo));
			}
		}
		bLastSettingsMode = g_bSettingsMode;
		vTaskDelay(2);
		//int freestack = uxTaskGetStackHighWaterMark(NULL);
		//Serial.println(String("menu high water: ") + freestack);
	}
}

void setup()
{
	Serial.begin(115200);
	while (!Serial.availableForWrite()) {
		delay(10);
	}
	// create the display mutex
	MutexDisplayHandle = xSemaphoreCreateMutex();
	// and the settingsmode one
	MutexSettingsMode = xSemaphoreCreateBinary();
	// init the display
	tft.init();
	tft.fillScreen(TFT_BLACK);
	tft.setRotation(3);
	tft.setFreeFont(&Dialog_bold_16);
	tft.setTextSize(1);
	tft.setTextPadding(tft.width());
	nMenuLineCount = tft.height() / tft.fontHeight();
	TextLines.resize(nMenuLineCount);
	// start the radio serial port
	RadioSerial.begin(9600, SERIAL_8N1, RADIO_SERIAL_RX, RADIO_SERIAL_TX, false);
	// start the tone generator
	ledcSetup(toneChannel, 0, 8);
	ledcAttachPin(AUDIO_OUT_PORT, toneChannel);
	ledcWrite(toneChannel, 127);
	ledcWriteTone(toneChannel, 0);
	// start the DTMF detector
	dtmf.begin(AUDIO_IN_PORT);

	//Serial.println("flash:" + String(ESP.getFlashChipSize()));
	//Serial.print("setup() is running on core ");
	//Serial.println(xPortGetCoreID());

	// configure LCD PWM functionalitites
	pinMode(TFT_ENABLE, OUTPUT);
	digitalWrite(TFT_ENABLE, 1);
	ledcSetup(ledChannel, freq, resolution);
	// attach the channel to the GPIO to be controlled
	ledcAttachPin(TFT_ENABLE, ledChannel);
	CRotaryDialButton::begin((gpio_num_t)DIAL_A, (gpio_num_t)DIAL_B, (gpio_num_t)DIAL_BTN, (gpio_num_t)0, (gpio_num_t)35, (gpio_num_t)-1, (gpio_num_t)-1, &SystemInfo.DialSettings);
	setupSDcard();
	//gpio_set_direction((gpio_num_t)LED, GPIO_MODE_OUTPUT);
	//digitalWrite(LED, HIGH);
	// init the onboard buttons
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);

	// set the power control to output
	pinMode(TXHIPOWER_PORT, OUTPUT);

	periodic_Second_timer_args = {
				periodic_Second_timer_callback,
				/* argument specified here will be passed to timer callback function */
				(void*)0,
				ESP_TIMER_TASK,
				"seconds timer"
	};
	esp_timer_create(&periodic_Second_timer_args, &periodic_Second_timer);
	esp_timer_start_periodic(periodic_Second_timer, (int64_t)1000 * 1000);

	SystemInfo.bCriticalBatteryLevel = false;
	SetDisplayBrightness(SystemInfo.nDisplayBrightness);
	// see if the button is down, if so clear all settings
	if (gpio_get_level((gpio_num_t)DIAL_BTN) == 0) {
		Preferences prefs;
		prefs.begin(prefsName);
		prefs.clear();
		prefs.end();
		WriteMessage("Factory Reset");
	}
	String msg;
	// see if we can read the settings
	if (SaveLoadSettings(false, true)) {
		msg = "Settings Loaded";
	}
	else {
		// set the dial type
		CheckRotaryDialType();
		// must not be anything there, so save it
		SaveLoadSettings(true);
	}
	//ClearScreen();
	SetDisplayBrightness(SystemInfo.nDisplayBrightness);
	//WiFi
	if (SystemInfo.bRunWebServer) {
		WiFi.softAP(ssid, password);
		IPAddress myIP = WiFi.softAPIP();
		// save for the menu system
		strncpy(localIpAddress, myIP.toString().c_str(), sizeof(localIpAddress));
		Serial.print("AP IP address: ");
		Serial.println(myIP);
		server.begin();
		Serial.println("Server started");
		for (OnServerItem item : OnServerList) {
			server.on(item.path, item.function);
		}
		//server.on("/fupload", HTTP_POST, []() { server.send(200); }, handleFileUpload);
		//server.on("/settings/increpeat", HTTP_GET, []() { server.send(200); }, IncRepeat);
		//server.on("/settings/increpeat", HTTP_GET, IncRepeat);
		/////////////////////////// End of Request commands
		server.begin();
	}
#if !HAS_BATTERY_LEVEL
	SystemInfo.bShowBatteryLevel = false;
#endif
	tft.setFreeFont(&Dialog_bold_16);
	tft.setTextColor(SystemInfo.menuTextColor);
	// get our text line sprite ready
	LineSprite.setColorDepth(16);
	LineSprite.createSprite(tft.width(), tft.fontHeight());
	LineSprite.fillSprite(TFT_BLACK);
	LineSprite.setFreeFont(&Dialog_bold_16);
	LineSprite.setTextPadding(tft.width());
	// get our Battery sprite ready
	BatterySprite.setColorDepth(16);
	// TODO fix this so it will work with a width other than 100, needs code change also
	BatterySprite.createSprite(100, tft.fontHeight() + BATTERY_BAR_HEIGHT);
	BatterySprite.fillSprite(TFT_BLACK);
	BatterySprite.setFreeFont(&Dialog_bold_16);
	BatterySprite.setTextPadding(tft.width());
	// get the menu system ready
	menuPtr = new MenuInfo;
	MenuStack.push(menuPtr);
	MenuStack.top()->menu = MainMenu;
	MenuStack.top()->index = 0;
	MenuStack.top()->offset = 0;
	tft.setTextColor(SystemInfo.menuTextColor);
	tft.setTextColor(TFT_BLUE);
	tft.setFreeFont(&Irish_Grover_Regular_24);
	tft.drawRect(0, 0, tft.width() - 1, tft.height() - 1, SystemInfo.menuTextColor);
	tft.drawRect(1, 1, tft.width() - 2, tft.height() - 2, SystemInfo.menuTextColor);
	tft.drawString("Radio Fox", 60, 10);
	tft.setFreeFont(&Dialog_bold_16);
	tft.drawString(String("Version ") + FOX_Version, 20, 70);
	tft.setTextSize(1);
	tft.drawString(__DATE__, 20, 90);
	if (msg.length()) {
		tft.drawString(msg, 20, 110);
	}
	// clear the button buffer
	CRotaryDialButton::clear();
	delay(3000);
	//// leave the intro screen up for 4 seconds or until a button is pressed or dial rotated
	//for (int cnt = 0; cnt < 400; ++cnt) {
	//	if (ReadButton() != BTN_NONE) {
	//		break;
	//	}
	//	vTaskDelay(pdMS_TO_TICKS(10));
	//}
	// set the PTT port
	gpio_set_direction((gpio_num_t)PTT_PORT, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)PTT_PORT, 1);
	ClearScreen();
	// start the transmit and management tasks
	xTaskCreate(TaskRunRadio, "FOXRADIO", 2000, NULL, 4, &TaskRunRadioHandle);
	xTaskCreate(TaskShowBattery, "BATTERYLEVEL", 2000, NULL, 0, &TaskShowBatteryHandle);
	xTaskCreate(TaskDTMF, "DTMFHANDLER", 2000, NULL, 2, &TaskDTMFHandle);
	xTaskCreate(TaskScrollSideways, "SCROLLSIDEWAYS", 2000, NULL, 0, &TaskScrollSidewaysHandle);
	xTaskCreate(TaskMenu, "MENU", 2000, NULL, 1, &TaskMenuHandle);
	ResetDimTimer();
	// init the radio
	RadioSetup();
}

// check and handle the rotary dial type
// if either A or B is 0, then this is a toggle dial
// else
// tell user to rotate one click
// delay
// if A or B is 0, then this is a toggle
// else it is a pulse dial
void CheckRotaryDialType()
{
	bool bA, bB;
	WriteMessage("checking dial type...", false, 1000);
	bA = gpio_get_level((gpio_num_t)DIAL_A);
	bB = gpio_get_level((gpio_num_t)DIAL_B);
	//Serial.println("ab " + String(bA) + String(bB));
	if (!bA && !bB) {
		// if both low must be a toggle
		SystemInfo.DialSettings.m_bToggleDial = true;
	}
	else {
		CRotaryDialButton::clear();
		WriteMessage("Rotate dial 1 click", false, -1);
		// wait for rotate, they were both high before if we got this far, so just look at A
		while (gpio_get_level((gpio_num_t)DIAL_A))
			delay(10);
		// wait for button bounce
		delay(250);
		// read them again
		bA = gpio_get_level((gpio_num_t)DIAL_A);
		bB = gpio_get_level((gpio_num_t)DIAL_B);
		//Serial.println("ab " + String(bA) + String(bB));
		// if both low must be a toggle
		SystemInfo.DialSettings.m_bToggleDial = !bA && !bB;
		// we shouldn't need this again
	}
	if (!SystemInfo.DialSettings.m_bToggleDial)
		SystemInfo.DialSettings.m_nDialPulseCount = 2;
	WriteMessage(String("Dial Type: ") + (SystemInfo.DialSettings.m_bToggleDial ? "Toggle" : "Pulse"), false, 2000);
}

void ResetDimTimer() {
	displayDimTimer = SystemInfo.nDisplayDimTime;
	if (SystemInfo.eDisplayDimMode == DISPLAY_DIM_MODE_TIME && SystemInfo.nDisplayDimTime) {
		SetDisplayBrightness(SystemInfo.nDisplayBrightness);
	}
}

// the main loop
void loop()
{
	if (SystemInfo.bRunWebServer) {
		server.handleClient();
	}
	delay(100);
}

// do something from the menu depending on the button argument
// only two buttons are actually handled, SELECT and HELP
void RunMenus(int button)
{
	// see if we got a menu match
	bool gotmatch = false;
	int menuix = 0;
	MenuInfo* oldMenu;
	bool bExit = false;
	for (int ix = 0; !gotmatch && MenuStack.top()->menu[ix].op != eTerminate; ++ix) {
		// see if this is one is valid
		if (!bMenuValid[ix]) {
			continue;	// and don't increment menix
		}
		if (menuix == MenuStack.top()->index) {
			gotmatch = true;
			switch (button) {
			case BTN_B0_LONG:	// handle help if there is any
				if (MenuStack.top()->menu[ix].cHelpText) {
					WriteMessage(MenuStack.top()->menu[ix].cHelpText, false, -1, true);
				}
				bMenuChanged = true;
				break;
			case BTN_SELECT:	// handle selection
				// got one, service it
				switch (MenuStack.top()->menu[ix].op) {
				case eTerminate:	// not used, tell compiler
				case eIfEqual:
				case eIfIntEqual:
				case eElse:
				case eEndif:
					break;
				case eText:
				case eTextInt:
				case eEditText:
				case eChooseFile:
				case eBool:
				case eList:
					bMenuChanged = true;
					if (MenuStack.top()->menu[ix].change != NULL) {
						(*MenuStack.top()->menu[ix].change)(&MenuStack.top()->menu[ix], 1);
					}
					if (MenuStack.top()->menu[ix].function) {
						(*MenuStack.top()->menu[ix].function)(&MenuStack.top()->menu[ix]);
					}
					if (MenuStack.top()->menu[ix].change != NULL) {
						(*MenuStack.top()->menu[ix].change)(&MenuStack.top()->menu[ix], -1);
					}
					break;
				case eMenu:
					if (MenuStack.top()->menu) {
						oldMenu = MenuStack.top();
						MenuStack.push(new MenuInfo);
						MenuStack.top()->menu = oldMenu->menu[ix].menu;
						bMenuChanged = true;
						MenuStack.top()->index = 0;
						MenuStack.top()->offset = 0;
						//Serial.println("change menu");
					}
					break;
				case eExit: // go back a level
					bExit = true;
					break;
				case eReboot:
					WriteMessage("Rebooting in 2 seconds\nHold button for factory reset", false, 2000);
					ESP.restart();
					break;
				}
			}
		}
		++menuix;
	}
	// if no match, and we are in a submenu, go back one level, or if bExit is set
	if (bExit || (!bMenuChanged && MenuStack.size() > 1)) {
		UpMenuLevel(false);
	}
}

// display the menu
// if MenuStack.top()->index is > MENU_LINES, then shift the lines up by enough to display them
// remember that we only have room for MENU_LINES lines
void ShowMenu(struct MenuItem* menu)
{
	// let the sideways scroller know that we changed the screen
	if (TaskScrollSidewaysHandle)
		xTaskNotifyGive(TaskScrollSidewaysHandle);
	MenuStack.top()->menucount = 0;
	int y = 0;
	int x = 0;
	// load with a false to start with
	std::stack<bool> skipStack;
	skipStack.push(false);
	// this is the active stack level, I.E. which level should be processed
	int skipLevel = 1;
	bool bSkipping = false;
	// loop through the menu
	for (int menix = 0; menu->op != eTerminate; ++menu, ++menix) {
		// make sure menu valid vector is big enough
		if (bMenuValid.size() < menix + 1) {
			bMenuValid.resize(menix + 1);
		}
		bMenuValid[menix] = false;
		switch ((menu->op)) {
		case eIfEqual:
			// skip the next one if match, this is boolean only
			skipStack.push(*(bool*)menu->value != (menu->min ? true : false));
			//Serial.println("ifequal test: skip: " + String(skip));
			if (!bSkipping) {
				++skipLevel;
				bSkipping = skipStack.top();
			}
			break;
		case eIfIntEqual:
			// skip the next one if match, this is int values
			skipStack.push(*(int*)menu->value != menu->min);
			//Serial.println("ifIntequal test: skip: " + String(skip));
			if (!bSkipping) {
				++skipLevel;
				bSkipping = skipStack.top();
			}
			break;
		case eElse:
			skipStack.top() = !skipStack.top();
			break;
		case eEndif:
			skipStack.pop();
			if (!bSkipping || skipLevel > skipStack.size()) {
				--skipLevel;
			}
			break;
		default:
			break;
		}
		bSkipping = skipLevel < skipStack.size() ? true : skipStack.top();
		if (bSkipping) {
			bMenuValid[menix] = false;
			continue;
		}
		char line[120]{}, xtraline[100]{};
		// only displayable menu items should be in this switch
		line[0] = '\0';
		int val;
		bool exists = false;
		switch (menu->op) {
		case eTextInt:
		case eText:
		case eEditText:
		case eChooseFile:
			bMenuValid[menix] = true;
			if (menu->value) {
				val = *(int*)menu->value;
				if (menu->op == eText || menu->op == eEditText || menu->op == eChooseFile) {
					sprintf(line, menu->text, (char*)(menu->value));
				}
				else if (menu->op == eTextInt) {
					sprintf(line, menu->text, (int)(val / pow10(menu->decimals)), val % (int)(pow10(menu->decimals)));
				}
			}
			else {
				strcpy(line, menu->text);
			}
			// next line
			++y;
			break;
		case eList:
			bMenuValid[menix] = true;
			val = *(int*)menu->value;
			sprintf(line, menu->text, menu->nameList[val]);
			// next line
			++y;
			break;
		case eBool:
			bMenuValid[menix] = true;
			if (menu->value) {
				bool* pb = (bool*)menu->value;
				sprintf(line, menu->text, *pb ? menu->on : menu->off);
			}
			else {
				strcpy(line, menu->text);
			}
			// increment displayable lines
			++y;
			break;
		case eMenu:
		case eExit:
		case eReboot:
			bMenuValid[menix] = true;
			if (menu->value) {
				// check for %d or %s in string, be lazy and assume %s if %d not there
				if (String(menu->text).indexOf("%d") != -1)
					sprintf(xtraline, menu->text, *(int*)menu->value);
				else
					sprintf(xtraline, menu->text, (char*)menu->value);
			}
			else {
				strcpy(xtraline, menu->text);
			}
			if (menu->op == eExit)
				sprintf(line, "%s%s", "-", xtraline);
			else
				sprintf(line, "%s%s", (menu->op == eReboot) ? "" : "+", xtraline);
			++y;
			//Serial.println("menu text4: " + String(line));
			break;
		default:
			break;
		}
		if (strlen(line) && y >= MenuStack.top()->offset) {
			DisplayMenuLine(y - 1, y - 1 - MenuStack.top()->offset, line);
		}
	}
	MenuStack.top()->menucount = y;
	// blank the rest of the lines
	for (int ix = y; ix < nMenuLineCount; ++ix) {
		DisplayLine(ix, "");
	}
	// show line if menu has been scrolled
	if (MenuStack.top()->offset > 0)
		tft.fillTriangle(0, 0, 2, 0, 0, tft.fontHeight() / 3, TFT_DARKGREY);
	//tft.drawLine(0, 0, 5, 0, menuLineActiveColor);TFT_DARKGREY
// show bottom line if last line is showing
	if (MenuStack.top()->offset + (nMenuLineCount - 1) < MenuStack.top()->menucount - 1) {
		int ypos = tft.height() - 2 - tft.fontHeight() / 3;
		tft.fillTriangle(0, ypos, 2, ypos, 0, ypos - tft.fontHeight() / 3, TFT_DARKGREY);
	}
	//if (MenuStack.top()->offset + (MENU_LINES - 1) < MenuStack.top()->menucount - 1)
	//	tft.drawLine(0, tft.height() - 1, 5, tft.height() - 1, menuLineActiveColor);
	//else
	//	tft.drawLine(0, tft.height() - 1, 5, tft.height() - 1, TFT_BLACK);
	// see if we need to clean up the end, like when the menu shrank due to a choice
	int extra = MenuStack.top()->menucount - MenuStack.top()->offset - nMenuLineCount;
	while (extra < 0) {
		DisplayLine(nMenuLineCount + extra, "");
		++extra;
	}
}

// toggle a boolean value
void ToggleBool(MenuItem * menu)
{
	bool* pb = (bool*)menu->value;
	*pb = !*pb;
	if (menu->change != NULL) {
		(*menu->change)(menu, 0);
	}
	ResetTextLines();
}

// choose from one of the values, update the index and wrap around if past max
void GetSelectChoice(MenuItem * menu)
{
	int* pVal = (int*)menu->value;
	++* pVal;
	*pVal %= menu->max + 1;
	if (menu->change != NULL) {
		(*menu->change)(menu, 0);
	}
	ResetTextLines();
}

// get integer values
void GetIntegerValue(MenuItem * menu)
{
	ClearScreen();
	// -1 means to reset to original
	int stepSize = 1;
	int originalValue = *(int*)menu->value;
	//Serial.println("int: " + String(menu->text) + String(*(int*)menu->value));
	char line[50];
	CRotaryDialButton::Button button = BTN_NONE;
	bool done = false;
	const char* fmt;
	if (menu->decimals) {
		static char fmtInfo[30];
		//String st = String("%ld.%0") + menu->decimals + "ld";
		sprintf(fmtInfo,"%%ld.%%0%dld",menu->decimals);
		fmt = fmtInfo;
	}
	else {
		fmt = "%ld";
	}
	char minstr[20], maxstr[20], valstr[20];
	sprintf(line, menu->text, *(int*)menu->value / (int)pow10(menu->decimals), *(int*)menu->value % (int)pow10(menu->decimals));
	DisplayLine(0, line, SystemInfo.menuTextColor);
	sprintf(minstr, fmt, menu->min / (int)pow10(menu->decimals), menu->min % (int)pow10(menu->decimals));
	sprintf(maxstr, fmt, menu->max / (int)pow10(menu->decimals), menu->max % (int)pow10(menu->decimals));
	DisplayLine(1, String(minstr) + " to " + String(maxstr), SystemInfo.menuTextColor);
	DisplayLine(5, "Long Press B0 to reset", SystemInfo.menuTextColor);
	DisplayLine(6, "Long Press to Accept", SystemInfo.menuTextColor);
	int oldVal = *(int*)menu->value;
	bool bChange = true;
	do {
		if (bChange) {
			// make sure within limits
			*(int*)menu->value = constrain(*(int*)menu->value, menu->min, menu->max);
			// show slider bar
			tft.fillRect(0, 2 * tft.fontHeight(), tft.width() - 1, 6, TFT_BLACK);
			DrawProgressBar(0, 2 * tft.fontHeight() + 4, tft.width() - 1, 12, map(*(int*)menu->value, menu->min, menu->max, 0, 100), true);
			sprintf(valstr, fmt, *(int*)menu->value / (int)pow10(menu->decimals), *(int*)menu->value % (int)pow10(menu->decimals));
			DisplayLine(3, String("New Value: ") + valstr, SystemInfo.menuTextColor);
			sprintf(valstr, fmt, stepSize / (int)pow10(menu->decimals), stepSize % (int)pow10(menu->decimals));
			DisplayLine(4, stepSize == -1 ? "Reset: long press (Click +)" : "Step: " + String(valstr) + " (Click +)", SystemInfo.menuTextColor);
			if (menu->change != NULL && oldVal != *(int*)menu->value) {
				(*menu->change)(menu, 0);
				oldVal = *(int*)menu->value;
			}
			bChange = false;
		}
		// let other people run for a moment so the watchdog doesn't time out and reboot
		vTaskDelay(2);
		button = ReadButton();
		switch (button) {
		case BTN_LEFT:
			if (stepSize != -1)
				*(int*)menu->value -= stepSize;
			break;
		case BTN_RIGHT:
			if (stepSize != -1)
				*(int*)menu->value += stepSize;
			break;
		case BTN_SELECT:
		case BTN_B0_CLICK:
			if (stepSize == -1) {
				stepSize = 1;
			}
			else {
				stepSize *= 10;
			}
			if (stepSize > (menu->max / 2)) {
				stepSize = -1;
			}
			break;
		case BTN_B0_LONG:	// reset
			*(int*)menu->value = originalValue;
			stepSize = 1;
			break;
		case BTN_LONG:
			if (stepSize == -1) {
				*(int*)menu->value = originalValue;
				stepSize = 1;
			}
			else {
				done = true;
			}
			break;
		}
		bChange = (button != BTN_NONE);
	} while (!done);
}

// update the batterie default settings, 1-4 batteries
void UpdateBatteries(MenuItem * menu, int flag)
{
	int batLo[4] = { 553, 1276, 1999, 2710 };
	int batHi[4] = { 809, 1790, 2763, 4094 };
	switch (flag) {
	case 1:		// first time
		break;
	case 0:		// every change
		break;
	case -1:	// last time, load the defaults
		SystemInfo.nBatteryEmptyLevel = batLo[SystemInfo.nBatteries - menu->min];
		SystemInfo.nBatteryFullLevel = batHi[SystemInfo.nBatteries - menu->min];
		break;
	}
}

void UpdateDisplayBrightness(MenuItem * menu, int flag)
{
	// control LCD brightness
	SetDisplayBrightness(*(int*)menu->value);
}

void UpdateDisplayDimMode(MenuItem * menu, int flag)
{
	switch (flag) {
	case 0:		// first time
		break;
	case 1:		// every change
		break;
	case -1:	// last time
		SetDisplayBrightness(SystemInfo.nDisplayBrightness);
	}
}

void SetDisplayBrightness(int val)
{
	ledcWrite(ledChannel, map(val, 0, 100, 0, 255));
}

uint16_t ColorList[] = {
	//TFT_NAVY,
	//TFT_MAROON,
	//TFT_OLIVE,
	TFT_WHITE,
	TFT_LIGHTGREY,
	TFT_BLUE,
	TFT_SKYBLUE,
	TFT_CYAN,
	TFT_RED,
	TFT_BROWN,
	TFT_GREEN,
	TFT_MAGENTA,
	TFT_YELLOW,
	TFT_ORANGE,
	TFT_GREENYELLOW,
	TFT_GOLD,
	TFT_SILVER,
	TFT_VIOLET,
	TFT_PURPLE,
};

// find the color in the list
int FindMenuColor(uint16_t col)
{
	int ix;
	int colors = sizeof(ColorList) / sizeof(*ColorList);
	for (ix = 0; ix < colors; ++ix) {
		if (col == ColorList[ix])
			break;
	}
	return constrain(ix, 0, colors - 1);
}

void SetMenuColor(MenuItem * menu)
{
	int maxIndex = sizeof(ColorList) / sizeof(*ColorList) - 1;
	int colorIndex = FindMenuColor(SystemInfo.menuTextColor);
	ClearScreen();
	DisplayLine(4, "Rotate change value", SystemInfo.menuTextColor);
	DisplayLine(5, "Long Press Exit", SystemInfo.menuTextColor);
	bool done = false;
	bool change = true;
	while (!done) {
		if (change) {
			DisplayLine(0, "Text Color", SystemInfo.menuTextColor);
			change = false;
		}
		switch (ReadButton()) {
		case CRotaryDialButton::BTN_LONGPRESS:
			done = true;
			break;
		case CRotaryDialButton::BTN_RIGHT:
			change = true;
			colorIndex = ++colorIndex;
			break;
		case CRotaryDialButton::BTN_LEFT:
			change = true;
			colorIndex = --colorIndex;
			break;
		}
		colorIndex = constrain(colorIndex, 0, maxIndex);
		SystemInfo.menuTextColor = ColorList[colorIndex];
	}
}

// go up one menu level, return true if anything done
// set gotoMain to go all the way back to the top
bool UpMenuLevel(bool gotoMain)
{
	if (gotoMain) {
		while (UpMenuLevel(false))
			;
	}
	else if (MenuStack.size() > 1) {
		bMenuChanged = true;
		menuPtr = MenuStack.top();
		MenuStack.pop();
		delete menuPtr;
		return true;
	}
	return false;
}

// handle the menus
bool HandleMenus()
{
	if (bMenuChanged) {
		ShowMenu(MenuStack.top()->menu);
		bMenuChanged = false;
	}
	bool didsomething = true;
	CRotaryDialButton::Button button = ReadButton();
	int lastOffset = MenuStack.top()->offset;
	int lastMenu = MenuStack.top()->index;
	int lastMenuCount = MenuStack.top()->menucount;
	//MenuItem* pCurrentMenu = &MenuStack.top()->menu[MenuStack.top()->index];
	int btnRepeatCount = 1;
	switch (button) {
	case BTN_LEFT_LONG:
	case BTN_RIGHT_LONG:
		btnRepeatCount = 5;
		break;
	}
	switch (button) {
	case BTN_B0_CLICK:	// go back a menu level if we can
		UpMenuLevel(false);
		break;
	case BTN_B0_LONG:	// look for help
		//UpMenuLevel(true);	// go back to the top menu
		RunMenus(button);
		bMenuChanged = true;
		break;
	case BTN_B1_LONG:
		button = BTN_SELECT;
		// yes, no break here
	case BTN_SELECT:
		RunMenus(button);
		bMenuChanged = true;
		break;
	case BTN_RIGHT_LONG:
	case BTN_RIGHT:
		while (btnRepeatCount--) {
			if (SystemInfo.bAllowMenuWrap || MenuStack.top()->index < MenuStack.top()->menucount - 1) {
				++MenuStack.top()->index;
			}
			if (MenuStack.top()->index >= MenuStack.top()->menucount) {
				MenuStack.top()->index = 0;
				bMenuChanged = true;
				MenuStack.top()->offset = 0;
			}
			// see if we need to scroll the menu
			if (MenuStack.top()->index - MenuStack.top()->offset > (nMenuLineCount - 1)) {
				if (MenuStack.top()->offset < MenuStack.top()->menucount - nMenuLineCount) {
					++MenuStack.top()->offset;
				}
			}
		}
		break;
	case BTN_LEFT_LONG:
	case BTN_LEFT:
		while (btnRepeatCount--) {
			if (SystemInfo.bAllowMenuWrap || MenuStack.top()->index > 0) {
				--MenuStack.top()->index;
			}
			if (MenuStack.top()->index < 0) {
				MenuStack.top()->index = MenuStack.top()->menucount - 1;
				bMenuChanged = true;
				MenuStack.top()->offset = MenuStack.top()->menucount - nMenuLineCount;
			}
			// see if we need to adjust the offset
			if (MenuStack.top()->offset && MenuStack.top()->index < MenuStack.top()->offset) {
				--MenuStack.top()->offset;
			}
		}
		break;
	case BTN_LONG:
		ClearScreen();
		g_bSettingsMode = false;
		bMenuChanged = true;
		break;
	default:
		didsomething = false;
		break;
	}
	// check some conditions that should redraw the menu
	if (lastMenu != MenuStack.top()->index || lastOffset != MenuStack.top()->offset) {
		bMenuChanged = true;
		//Serial.println("menu changed");
	}
	return didsomething;
}

// handle keys in run mode
bool HandleRunMode()
{
	bool didsomething = true;
	int maxMenuLine = nMenuLineCount - (SystemInfo.bShowBatteryLevel ? 2 : 1);
	CRotaryDialButton::Button button = ReadButton();
	switch (button) {
	case BTN_SELECT:
		break;
	case BTN_RIGHT_LONG:
	case BTN_RIGHT:
		break;
	case BTN_LEFT_LONG:
	case BTN_LEFT:
		break;
	case BTN_LONG:
		g_bSettingsMode = true;
		break;
	case BTN_B0_CLICK:
		// handle on board button 0
		break;
	case BTN_B0_LONG:
		break;
	case BTN_B1_CLICK:
		break;
	case BTN_B1_LONG:
	case BTN_LEFT_RIGHT_LONG:
		break;
	default:
		didsomething = false;
		taskYIELD();
		break;
	}
	return didsomething;
}

// check buttons and return if one pressed
// check the rotation buttons during running
enum CRotaryDialButton::Button ReadButton()
{
	if (SystemInfo.bRunWebServer) {
		server.handleClient();
	}
	enum CRotaryDialButton::Button retValue = BTN_NONE;
	// read the next button, or NONE if none there
	retValue = CRotaryDialButton::dequeue();
	// reboot?
	if (retValue == BTN_B2_LONG)
		ESP.restart();
	// turn the b1 button into a dial long click
	if (retValue == BTN_B1_CLICK)
		retValue = BTN_LONG;
	if (retValue != BTN_NONE) {
		ResetDimTimer();
	}
	else if (displayDimNow) {
		for (int val = SystemInfo.nDisplayBrightness - 1; val >= SystemInfo.nDisplayDimValue; --val) {
			SetDisplayBrightness(val);
			if (CRotaryDialButton::getCount()) {
				// if button pressed finish and get out of here
				SetDisplayBrightness(SystemInfo.nDisplayBrightness);
				break;
			}
			delay(10);
		}
		displayDimNow = false;
	}
	return retValue;
}

// just check for longpress and cancel if it was there
bool CheckCancel(bool bLeaveButton)
{
	ResetDimTimer();
	// if it has been set, just return true
	CRotaryDialButton::Button button = ReadButton();
	if (button) {
		if (button == BTN_LONG) {
			return true;
		}
		else if (bLeaveButton) {
			CRotaryDialButton::pushButton(button);
		}
	}
	return false;
}

void setupSDcard()
{
	bSdCardValid = false;
#if USE_STANDARD_SD
	gpio_set_direction((gpio_num_t)SDcsPin, GPIO_MODE_OUTPUT);
	delay(50);
	//SPIClass(1);
	spiSDCard.begin(SDSckPin, SDMisoPin, SDMosiPin, SDcsPin);	// SCK,MISO,MOSI,CS
	delay(20);

	if (!SD.begin(SDcsPin, spiSDCard)) {
		//Serial.println("Card Mount Failed");
		return;
	}
	uint8_t cardType = SD.cardType();

	if (cardType == CARD_NONE) {
		//Serial.println("No SD card attached");
		return;
	}
#else
#define SD_CONFIG SdSpiConfig(SDcsPin, /*DEDICATED_SPI*/SHARED_SPI, SD_SCK_MHZ(10))
	SPI.begin(SDSckPin, SDMisoPin, SDMosiPin, SDcsPin);	// SCK,MISO,MOSI,CS
	if (!SD.begin(SD_CONFIG)) {
		Serial.println("SD initialization failed.");
		uint8_t err = SD.card()->errorCode();
		Serial.println("err: " + String(err));
		return;
	}
	//Serial.println("Mounted SD card");
	//SD.printFatType(&Serial);

	//uint64_t cardSize = (uint64_t)SD.clusterCount() * SD.bytesPerCluster() / (1024 * 1024 * 1024);
	//Serial.printf("SD Card Size: %llu GB\n", cardSize);
#endif
}

// display a line in selected colors and clear to the end of the line
void DisplayLine(int lineNum, String text, uint16_t color, uint16_t backColor)
{
	if (xSemaphoreTake(MutexDisplayHandle, portMAX_DELAY) == pdTRUE) {
		if (lineNum >= 0 && lineNum < nMenuLineCount) {
			if (TextLines[lineNum].Line != text || TextLines[lineNum].backColor != backColor || TextLines[lineNum].foreColor != color) {
				int pixels = tft.textWidth(text);
				if (pixels > tft.width()) {
					TextLines[lineNum].nRollLength = pixels;
					TextLines[lineNum].nRollOffset = 0;
				}
				else {
					TextLines[lineNum].nRollOffset = TextLines[lineNum].nRollLength = 0;
				}
				// save the line for scrolling purposes
				TextLines[lineNum].Line = text;
				TextLines[lineNum].foreColor = color;
				TextLines[lineNum].backColor = backColor;
			}
			// push the sprite text to the display
			int y = lineNum * tft.fontHeight();
			LineSprite.setTextColor(color, backColor);
			LineSprite.drawString(text, -TextLines[lineNum].nRollOffset, 0);
			LineSprite.pushSprite(0, y);
		}
		xSemaphoreGive(MutexDisplayHandle);
	}
}

// clear the screen and reset the scrolling lines
void ClearScreen()
{
	if (xSemaphoreTake(MutexDisplayHandle, portMAX_DELAY) == pdTRUE) {
		// let the sideways scroller know that we cleared the screen
		if (TaskScrollSidewaysHandle)
			xTaskNotifyGive(TaskScrollSidewaysHandle);
		tft.fillScreen(TFT_BLACK);
		ResetTextLines();
		xSemaphoreGive(MutexDisplayHandle);
	}
}

void ResetTextLines()
{
	for (int ix = 0; ix < nMenuLineCount; ++ix) {
		TextLines[ix].Line.clear();
		TextLines[ix].nRollOffset = 0;
		TextLines[ix].nRollLength = 0;
		TextLines[ix].foreColor = 0;
		TextLines[ix].backColor = 0;
		TextLines[ix].nRollDirection = 0;
	}
}

// active menu line is in reverse video or * at front depending on bMenuStar
void DisplayMenuLine(int lineNum, int displine, String text)
{
	bool hilite = MenuStack.top()->index == lineNum;
	String mline = (hilite && SystemInfo.bMenuStar ? "*" : " ") + text;
	if (displine >= 0 && displine < nMenuLineCount) {
		if (SystemInfo.bMenuStar) {
			DisplayLine(displine, mline, SystemInfo.menuTextColor, TFT_BLACK);
		}
		else {
			DisplayLine(displine, mline, hilite ? TFT_BLACK : SystemInfo.menuTextColor, hilite ? SystemInfo.menuTextColor : TFT_BLACK);
		}
	}
}

// insert newlines into a string so it doesn't wrap in the middle of words when displayed
// existing newlines are honored
String FormatMultiLine(String & input)
{
	String output;
	int lineWidth = 0;
	int lastInputSpace = 0;
	int lastOutputSpace = 0;
	int lastOutputStart = 0;
	// look for spaces and add words to the output, when each line is too long start a new line
	for (int inIx = 0; inIx < input.length(); ++inIx) {
		switch (input[inIx]) {
		case '\n':
			// flush the line
			output += '\n';
			lastOutputStart = output.length();
			break;
		case ' ':
			output += input[inIx];
			// mark the space location so we can go back
			lastInputSpace = inIx;
			lastOutputSpace = output.length();
			break;
		default:
			// check the width after adding this character
			output += input[inIx];
			lineWidth = tft.textWidth(output.substring(lastOutputStart));
			if (lineWidth > tft.width()) {
				// too wide, backup
				output = output.substring(0, lastOutputSpace);
				// add a newline to the output
				output += '\n';
				inIx = lastInputSpace;
				lastOutputStart = output.length();
			}
			break;
		}
	}
	return output;
}

// display message on first line, if wait is -1, wait for a key press
void WriteMessage(String txt, bool error, int wait, bool process)
{
	ClearScreen();
	if (process) {
		txt = FormatMultiLine(txt);
	}
	if (error) {
		txt = "**" + txt + "**";
		tft.setTextColor(TFT_RED);
	}
	else {
		tft.setTextColor(SystemInfo.menuTextColor);
	}
	if (xSemaphoreTake(MutexDisplayHandle, portMAX_DELAY) == pdTRUE) {
		tft.setCursor(0, tft.fontHeight());
		tft.setTextWrap(true);
		tft.print(txt);
		if (wait == -1) {
			// wait for a key
			while (ReadButton() == BTN_NONE)
				delay(10);
		}
		else {
			delay(wait);
		}
		tft.setTextColor(TFT_WHITE);
		xSemaphoreGive(MutexDisplayHandle);
	}
	ClearScreen();
}

// compare strings for sort ignoring case
bool CompareNames(const String & a, const String & b)
{
	String a1 = a, b1 = b;
	a1.toUpperCase();
	b1.toUpperCase();
	return a1.compareTo(b1) < 0;
}

// save the eeprom settings
void SaveEepromSettings(MenuItem * menu)
{
	SaveLoadSettings(true);
}

// load eeprom settings
void LoadEepromSettings(MenuItem * menu)
{
	SaveLoadSettings(false);
}

// get a yes/no response
bool GetYesNo(String msg)
{
	int pad = tft.getTextPadding();
	tft.setTextPadding(0);
	bool retval = false;
	bool change = true;
	ClearScreen();
	DisplayLine(1, msg, SystemInfo.menuTextColor);
	DisplayLine(4, "Rotate to Change", SystemInfo.menuTextColor);
	DisplayLine(5, "Click to Select", SystemInfo.menuTextColor);
	CRotaryDialButton::Button button = BTN_NONE;
	bool done = false;
	while (!done) {
		button = ReadButton();
		switch (button) {
		case BTN_LEFT:
		case BTN_RIGHT:
			retval = !retval;
			change = true;
			break;
		case BTN_SELECT:
			//case BTN_LONG:
			done = true;
			break;
		default:
			break;
		}
		if (change) {
			// draw the buttons
			tft.fillRoundRect(60, 42, 60, 22, 8, SystemInfo.menuTextColor);
			tft.setTextColor(TFT_BLACK, SystemInfo.menuTextColor);
			tft.drawString(retval ? "Yes" : "No", 72, 46);
			change = false;
		}
	}
	tft.setTextPadding(pad);
	return retval;
}

// draw a progress bar
void DrawProgressBar(int x, int y, int dx, int dy, int percent, bool rect)
{
	if (rect)
		tft.drawRoundRect(x, y, dx, dy, 2, SystemInfo.menuTextColor);
	int fill = (dx - 2) * percent / 100;
	// fill the filled part
	tft.fillRect(x + 1, y + 1, fill, dy - 2, TFT_DARKGREEN);
	// blank the empty part
	tft.fillRect(x + 1 + fill, y + 1, dx - 2 - fill, dy - 2, TFT_BLACK);
}

// save/load settings
// return false if not found or wrong version
bool SaveLoadSettings(bool save, bool nodisplay)
{
	bool retvalue = true;
	Preferences prefs;
	prefs.begin(prefsName, !save);
	if (save) {
		//Serial.println("saving");
		prefs.putString(prefsVersion, FOX_Version);
		prefs.putBytes(prefsSystemInfo, &SystemInfo, sizeof(SystemInfo));
		// save things
		if (!nodisplay)
			WriteMessage("Settings Saved", false, 500);
	}
	else {
		// load things
		String vsn = prefs.getString(prefsVersion, "");
		if (vsn == FOX_Version) {
			setupSDcard();
			// set the brightness values since they might have changed
			SetDisplayBrightness(SystemInfo.nDisplayBrightness);
			if (!nodisplay)
				WriteMessage("Settings Loaded", false, 500);
			// these are always done
			prefs.getBytes(prefsSystemInfo, &SystemInfo, sizeof(SystemInfo));
		}
		else {
			retvalue = false;
			if (!nodisplay)
				WriteMessage("Settings not saved yet", true, 2000);
		}
	}
	prefs.end();
	return retvalue;
}

// delete saved settings
void FactorySettings(MenuItem * menu)
{
	if (GetYesNo("Reset All Settings? (reboot)")) {
		Preferences prefs;
		prefs.begin(prefsName);
		prefs.clear();
		prefs.end();
		ESP.restart();
	}
}

void EraseFlash(MenuItem * menu)
{
	if (GetYesNo("Format EEPROM? (factory reset)")) {
		nvs_flash_erase(); // erase the NVS partition and...
		nvs_flash_init(); // initialize the NVS partition.
	}
}

void load_page_header(bool bRefresh) {
	webpage = "<!DOCTYPE html><html>";
	webpage += "<head>";
	webpage += "<title>RadioFox</title>";
	webpage += "<META name='viewport' content='width=device-width, initial-scale=1.0'>";
	if (bRefresh)
		webpage += "<META http-equiv='refresh' content='2'>";
	webpage += "<style>";
	webpage += "body{max-width:98%;margin:0 auto;font-family:arial;font-size:100%;text-align:center;color:black;background-color:#888888;}";
	webpage += "ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.17em;overflow:hidden;background-color:#EEEEEE;font-size:1em;}";
	webpage += "li{float:left;border-radius:0.17em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}";
	// fontsize was 65%, changed to 100 to make tabs easier to hit on a phone
	webpage += "li a{display: block;border-radius:0.17em;padding:0.44em 0.44em;text-decoration:none;font-size:100%}";
	webpage += "li a:hover{background-color:#DDDDDD;border-radius:0.17em;font-size:85%}";
	webpage += "section {font-size:0.88em;}";
	webpage += "h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#444444;}";
	webpage += "h2{color:orange;font-size:1.0em;}";
	webpage += "h3{font-size:0.8em;}";
	webpage += "table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:100%;}";
	webpage += "th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}";
	webpage += "tr:nth-child(odd) {background-color:#eeeeee;}";
	webpage += ".rcorners_n {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
	webpage += ".rcorners_m {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
	webpage += ".rcorners_w {border-radius:0.2em;background:#CCCCCC;padding:0.3em 0.3em;width:100%;color:white;font-size:75%;}";
	webpage += ".column{float:left;width:100%;height:100%;}";
	webpage += ".row:after{content:'';display:table;clear:both;}";
	webpage += "*{box-sizing:border-box;}";
	webpage += "footer{background-color:#AAAAAA; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}";
	webpage += "button{border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:100%;}";
	webpage += ".buttons {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:80%;}";
	webpage += ".buttonsm{border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%; color:white;font-size:70%;}";
	webpage += ".buttonm {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:70%;}";
	webpage += ".buttonw {border-radius:0.5em;background:#666666;padding:0.3em 0.3em;width:45%;color:white;font-size:70%;}";
	webpage += "a{font-size:75%;}";
	webpage += "p{font-size:75%;}";
	webpage += "</style></head><body>";
	webpage += "<body><h1>MIW Server<br>";
	webpage + "</h1>";
}

void append_page_footer() {
	webpage += "<ul>";
	webpage += "<li><a href='/'>Home</a></li>";
	webpage += "<li><a href='/download'>Download</a></li>";
	webpage += "<li><a href='/upload'>Upload</a></li>";
	webpage += "<li><a href='/settings'>Settings</a></li>";
	webpage += "<li><a href='/utilities'>Utilities</a></li>";
	webpage += "</ul>";
	webpage += "<footer>Radio Fox ";
	webpage += FOX_Version;
	webpage += "</footer>";
	webpage += "</body></html>";
}

void SendHTML_Header() {
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
	load_page_header(false);
	server.sendContent(webpage);
	webpage = "";
}

void SendHTML_Content() {
	server.sendContent(webpage);
	webpage = "";
}

void SendHTML_Stop() {
	server.sendContent("");
	server.client().stop(); // Stop is needed because no content length was sent
}

// display the homepage on the web browser
void HomePage() {
	bWebRunning = false;
	SendHTML_Header();
	//webpage += "<a href='/download'><button style=\"width:auto\">Download</button></a>";
	//webpage += "<a href='/upload'><button style=\"width:auto\">Upload</button></a>";
	//webpage += "<a href='/settings'><button style='width:auto'>Settings</button></a>";
	//webpage += "<a href='/utilities'><button style='width:auto'>Utilities</button></a>";
	webpage += "<br><h2>" + String("Folder: ") + "/" + "</h2>";
	webpage += "<a href='/runimage'><button style='width:90%;font-size:200%;color:#00ff00'>";
	webpage += String("Run File:<br>") + "/" + "</button></a>";
	webpage += "<br><br>";
	webpage += "<br>";
	webpage += "<br><br>";
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
}

// these are used for the list of things that can be set on the settings web page
enum WEB_SETTINGS_TYPE {
	WST_NUMBER,		// a number value, decimals 0 for integer
	WST_BOOL,		// boolean values
	WST_STRING,		// a string of characters
	WST_TEXT_ONLY,	// text that will display as H2, use to separate sections
	WST_SLIDER,		// a slider control
};
typedef WEB_SETTINGS_TYPE WEB_SETTINGS_TYPE;
struct WEB_SETTINGS {
	WEB_SETTINGS_TYPE type;		// what type of data
	bool* display;				// if not NULL, compare with displayTest to display this line
	bool displayTest;			// compare with display to see if this line should display or not
	char* text;					// show on page
	char* name;					// the data name
	void* data;					// a pointer to the data
	int width;					// how wide to make the field
	int decimals;				// decimals for floats, although stored as ints, also used for max string length
	int min, max;				// not used yet, TODO, but will limit range of numbers
};
typedef WEB_SETTINGS WebSettings;
WebSettings WebSettingsPage[] = {
	{(WEB_SETTINGS_TYPE)WST_TEXT_ONLY,NULL,true,"Image Settings",NULL},
	//{WST_BOOL,NULL,true,"Use Fixed Image Time","use_fixed_time",&ImgInfo.bFixedTime},
	//{WST_NUMBER,&ImgInfo.bFixedTime,true,"Fixed Time Value (S)","fixed_time",&ImgInfo.nFixedImageTime,4,0},
	//{WST_NUMBER,&ImgInfo.bFixedTime,false,"Column Time(mS)","column_time",&ImgInfo.nFrameHold,4,0},
	//{WST_SLIDER,&ImgInfo.bFixedTime,false,"Column Time(mS)","column_time_slider",&ImgInfo.nFrameHold,4,0,0,500},
	//{WST_NUMBER,NULL,true,"Start Delay (S)","start_delay",&ImgInfo.startDelay,4,1},
	//{WST_BOOL,NULL,true,"Upside Down","upside_down",&ImgInfo.bUpsideDown},
	//{WST_BOOL,NULL,true,"Reverse Walk (left-right)","reverse_walk",&ImgInfo.bReverseImage},
	//{WST_BOOL,NULL,true,"Play Mirror Image","mirror_image",&ImgInfo.bMirrorPlayImage},
	//{WST_NUMBER,&ImgInfo.bMirrorPlayImage,true,"Middle Mirror Delay (S)","mirror_delay",&ImgInfo.nMirrorDelay,4,1},
	//{WST_BOOL,NULL,true,"Scale Height to Fit Pixels","scale_height",&ImgInfo.bScaleHeight},
	//{WST_BOOL,NULL,true,"Double Pixels (144 to 288)","double_pixels",&ImgInfo.bDoublePixels},
	//{WST_BOOL,NULL,true,"Chain Images","chain_images",&ImgInfo.bChainFiles},
	//{WST_NUMBER,&ImgInfo.bChainFiles,true,"Chain Delay (S)","chain_delay",&ImgInfo.nChainDelay,4,1},
	//{WST_NUMBER,&ImgInfo.bChainFiles,true,"Chain Repeats","chain_repeats",&ImgInfo.nChainRepeats,4,0},
	//{WST_TEXT_ONLY,NULL,true,"File Repeat Settings",NULL},
	//{WST_NUMBER,NULL,true,"Repeat Count","repeat_count",&ImgInfo.repeatCount,4,0},
	//{WST_NUMBER,NULL,true,"Repeat Delay (S)","repeat_delay",&ImgInfo.repeatDelay,4,1},
	//{WST_TEXT_ONLY,NULL,true,"Macro Repeat Settings",NULL},
	//{WST_NUMBER,NULL,true,"Repeat Count","macro_repeat_count",&ImgInfo.nRepeatCountMacro,4,0},
	//{WST_NUMBER,NULL,true,"Repeat Delay (S)","macro_repeat_delay",&ImgInfo.nRepeatWaitMacro,4,1},
	//{WST_TEXT_ONLY,NULL,true,"LED Settings",NULL},
	//{WST_NUMBER,NULL,true,"LED Brightness (1-255)","LED_brightness",&LedInfo.nLEDBrightness,4,0},
	//{WST_BOOL,NULL,true,"Gamma Correction","gamma_correction",&LedInfo.bGammaCorrection},
	//{WST_TEXT_ONLY,NULL,true,"DMX512 Settings",NULL},
	//{WST_BOOL,NULL,true,"DMX Enabled","dmx_enabled",&SystemInfo.bRunArtNetDMX},
	//{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Art-Net Name","artnet_name",&SystemInfo.cArtNetName,14,sizeof(SystemInfo.cArtNetName)},
	//{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Network to Connect To","network_name",&SystemInfo.cNetworkName,20,sizeof(SystemInfo.cNetworkName)},
	//{WST_STRING,&SystemInfo.bRunArtNetDMX,true,"Password","password",&SystemInfo.cNetworkPassword,30,sizeof(SystemInfo.cNetworkPassword)},
	//{WST_BOOL,&SystemInfo.bRunArtNetDMX,true,"Universe Start 1 (off for 0)","universe_start",&SystemInfo.bStartUniverseOne},
};

// change the settings from the web page
void WebChangeSettings()
{
	if (server.args()) {
		void* lastData = NULL;
		void* thisData = NULL;
		//Serial.println("argcnt: " + String(server.args()));
		for (WebSettings val : WebSettingsPage) {
			thisData = val.data;
			//if (thisData == lastData)
			//	continue;
			lastData = thisData;
			if (val.type != WST_BOOL && !server.hasArg(val.name))
				continue;
			//Serial.println(String(val.name) + ": ~" + server.arg(val.name) + "~");
			switch (val.type) {
			case WST_NUMBER:
				*(int*)(val.data) = (int)(server.arg(val.name).toDouble() * pow10(val.decimals));
				break;
			case WST_SLIDER:
				//Serial.println("slider value:" + server.arg(val.name));
				*(int*)(val.data) = (int)(server.arg(val.name).toDouble() * pow10(val.decimals));
				break;
			case WST_BOOL:
				*(bool*)(val.data) = server.arg(val.name).length() ? true : false;
				break;
			case WST_STRING:
				memset(val.data, 0, val.decimals);
				strncpy((char*)(val.data), server.arg(val.name).c_str(), val.decimals - 1);
				break;
			case WST_TEXT_ONLY:
				break;
			}
		}
	}
	//Serial.println("fixed: " + String(server.arg("fixed_time")));
	WebShowSettings();
}

void WebShowSettings() {
	String stmp;
	double sfloat;
	bool bDoneFirst = false;
	load_page_header(false);
	//webpage += ".slidecontainer{  width: 100 %;	}";
	webpage += "<form id='allsettings' onchange='document.forms[\"allsettings\"].submit()' action='/changesettings' method='post'>";
	//webpage += "<form onchange='document.getElementById(\"settingssubmitbutton\").disabled=false' action='/changesettings' method='post'>";
	for (WebSettings val : WebSettingsPage) {
		if (val.display && (*(val.display) != val.displayTest))
			continue;
		if (bDoneFirst)
			webpage += "<br>";
		else
			bDoneFirst = true;
		switch (val.type) {
		case WST_NUMBER:
			webpage += "<label>" + String(val.text) + ": ";
			stmp = String(*(int*)(val.data));
			sfloat = stmp.toDouble() / pow10(val.decimals);
			stmp = String(sfloat, val.decimals);
			webpage += "<input type='text' name='" + String(val.name) + "' size='" + String(val.width) + "' value='" + stmp + "'>";
			webpage += "</label>";
			break;
		case WST_SLIDER:
			//webpage += "<label>" + String(val.text) + ": ";
			stmp = String(*(int*)(val.data));
			sfloat = stmp.toDouble() / pow10(val.decimals);
			stmp = String(sfloat, val.decimals);
			webpage += "<input type='range' name='" + String(val.name) + "' min='" + String(val.min) + "' max='" + String(val.max) + "' value='" + stmp + "' class='slider' id='" + val.name + "'>";
			//webpage += "</label>";
			break;
		case WST_BOOL:
			webpage += "<label>" + String(val.text) + ": ";
			webpage += "<input type='checkbox' name='" + String(val.name) + "' value='" + val.name + "'";
			if (*(bool*)(val.data))
				webpage += " checked='checked'";
			webpage += ">";
			webpage += "</label>";
			break;
		case WST_STRING:
			webpage += "<label>" + String(val.text) + ": ";
			webpage += "<input type='text' name='" + String(val.name) + "' size='" + String(val.width) + "' value='" + (char*)(val.data) + "'>";
			webpage += "</label>";
			break;
		case WST_TEXT_ONLY:
			webpage += "<h2>" + String(val.text) + "</h2>";
			bDoneFirst = false;
			break;
		}
	}
	//webpage += "<br><input type='range' name='test' min='1' max='255' value='50' class='slider' id='myRange'>";
	//webpage += "<br><br><input id='settingssubmitbutton' disabled type='submit' value='Update MIW'>";
	webpage += "</form><br>";
	//if (ImgInfo.bFixedTime) {
	//	webpage += String("<p>Fixed Image Time: ") + String(ImgInfo.nFixedImageTime) + " S";
	//}
	//else {
	//	webpage += String("<p>Column Time: ") + String(ImgInfo.nFrameHold) + " mS";
	//}
	////IncreaseRepeatButton();
	////DecreaseRepeatButton();
	append_page_footer();
	server.send(200, "text/html", webpage);
}

// interlock so repeat of webpage won't reboot, verifyreboot has to be called first
bool b_RebootArmed = false;
void UtilitiesPage()
{
	load_page_header(false);
	webpage += "<h2>Utilities</h2>";
	webpage += "<br><br><a href='/verifyrebootsystem'><button style='width:50%;font-size:150%;color:#ffffff'>Reboot System</button></a>";
	webpage += "<br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
	b_RebootArmed = false;
}

// verify reboot
void VerifyRebootSystem()
{
	load_page_header(false);
	webpage += "<h2>Confirm System Reboot</h2>";
	webpage += "<a href='/utilities'><button style='width:30%;font-size:150%;color:#ffffff'>Cancel</button></a>";
	webpage += "<a href='/rebootsystem'><button style='width:30%;font-size:150%;color:#ffffff'> Reboot</button></a>";
	webpage += "<br><br>";
	append_page_footer();
	server.send(200, "text/html", webpage);
	b_RebootArmed = true;
}

// reboot system from web page
void RebootSystem()
{
	if (b_RebootArmed) {
		ESP.restart();
	}
	else {
		UtilitiesPage();
	}
}

// map a menuitem list to html
// recurses for each eIf
String MenuToHtml(MenuItem * pMenu, bool bActive, int nLevel)
{
	static String str;
	static int ix;
	static MenuItem* menu, * StartMenu;
	static String line, name, stmp;
	double sfloat;
	if (nLevel == 0) {
		StartMenu = pMenu;
		str = "";
		ix = 0;
	}
	else {
		++ix;
	}
	for (menu = &StartMenu[ix]; menu->op != eTerminate; ++ix, ++menu) {
		if (!bActive) {
			// skip if not one of the if else parts when not active
			switch (menu->op) {
			case eIfEqual:
			case eIfIntEqual:
			case eElse:
			case eEndif:
				break;
			default:
				continue;
				break;
			}
		}
		name = "bi_" + String(ix);
		//Serial.println("name: " + name);
		// keep the line without %x's
		line = menu->text;
		if (line.indexOf("%d.%d") >= 0) {
			line.remove(line.indexOf("%d.%d"), 5);
		}
		while (line.indexOf('%') >= 0) {
			line.remove(line.indexOf('%'), 2);
		}
		switch (menu->op) {
		case eText:
		case eEditText:
		case eChooseFile:
			str += String("<p>") + line + "</p>";
			break;
		case eTextInt:
			str += "<label>" + line;
			stmp = String(*(int*)(menu->value));
			sfloat = stmp.toDouble() / pow10(menu->decimals);
			stmp = String(sfloat, menu->decimals);
			str += "<input type='text' name='" + name + "' size='" + String(5) + "' value='" + stmp + "'>";
			str += "</label>";
			break;
		case eBool:	// show the two text choices in (...)
			str += "<label>" + line + " (" + menu->on + "&#x2611; | " + menu->off + "&#9633;)";
			str += "<input type='checkbox' name='" + name + "' value='" + name + "'";
			if (*(bool*)(menu->value))
				str += " checked='checked'";
			str += ">";
			str += "</label>";
			break;
		case eList:	// a dropdown list
			str += "<label>" + line;
			str += "<select name='" + name + "'>";
			for (int ix = 0; ix <= menu->max; ++ix) {
				str += "<option ";
				if (ix == *(int*)(menu->value))
					str += "selected='" + String(ix) + "' ";
				str += "<value='" + String(ix) + "'>" + menu->nameList[ix] + "</option>";
			}
			str += "</select>";
			str += "</label>";
			break;
		case eExit:
			break;
		case eIfEqual:
			MenuToHtml(NULL, *(bool*)(menu->value) == (menu->min ? true : false), nLevel + 1);
			continue;
			break;
		case eIfIntEqual:
			MenuToHtml(NULL, *(int*)(menu->value) == (menu->min), nLevel + 1);
			continue;
			break;
		case eElse:
			bActive = !bActive;
			break;
		case eEndif:
			return str;
			break;
		case eMenu:
		case eReboot:
		case eTerminate:
			//Serial.println("unsupported menutype to html: " + String(menu->op));
			break;
		default:
			break;
		}
		if (bActive) {
			str += "<br>";
		}
	}
	return str;
}

// read the battery level, LiIon cells are 4.2 fully charged and should not be discharged below 2.75
// smoothing of the reading is done using an exponential moving average
int ReadBattery(int* raw)
{
	const float alpha = 0.9;
	static float eSmooth = 0.0;
	int percent;
	float nextLevel;
	for (int tries = 0; tries < 5; ++tries) {
		nextLevel = (float)analogRead(BATTERY_SENSOR_GPIO);
		// calculate the next value
		eSmooth = (alpha * eSmooth) + ((1 - alpha) * nextLevel);
		// calculate the %
		if (eSmooth >= SystemInfo.nBatteryFullLevel)
			percent = 100;
		else if (eSmooth <= SystemInfo.nBatteryEmptyLevel)
			percent = 0;
		else {
			percent = (eSmooth - SystemInfo.nBatteryEmptyLevel) * 100 / (SystemInfo.nBatteryFullLevel - SystemInfo.nBatteryEmptyLevel);
		}
		delay(2);
	}
	if (raw)
		*raw = (int)eSmooth;
	if (percent == 0) {
		static int count0 = 0;
		// we need at least 5 0's before claiming no power, it takes a few cycles to stabilize
		if (count0++ > 5) {
			SystemInfo.bCriticalBatteryLevel = true;
		}
	}
	return percent;
}

// this code shows the battery on the main display when menu is NULL
// otherwise it shows the current raw integer readings of the battery sensor
// if this call comes from the menu system the display is already locked, otherwise we lock it here
void ShowBattery(MenuItem * menu)
{
	static int percent = 0, raw = 0;
	if (menu)
		ClearScreen();
	static unsigned long showtime = 0;
	while (!menu || ReadButton() != BTN_LONG) {
		percent = ReadBattery(&raw);
		if (millis() > showtime + 1000) {
			if (menu) {
				DisplayLine(0, "Battery: " + String(percent) + "%", SystemInfo.menuTextColor);
				DisplayLine(1, "Raw Battery: " + String(raw), SystemInfo.menuTextColor);
				DisplayLine(3, "Long Press to Cancel", SystemInfo.menuTextColor);
			}
			else {
				if (xSemaphoreTake(MutexDisplayHandle, portMAX_DELAY) == pdTRUE) {
					// TODO: fix this so it works for a sprite that is not 100 pixels wide
					int sh = BatterySprite.height();
					BatterySprite.fillSprite(TFT_BLACK);
					BatterySprite.setTextColor(SystemInfo.menuTextColor);
					// show % full
					BatterySprite.fillRect(0, sh - BATTERY_BAR_HEIGHT - 2, percent, BATTERY_BAR_HEIGHT, SystemInfo.menuTextColor);
					// thin line rest of line
					BatterySprite.fillRect(percent, sh - 4, 100 - percent, 2, SystemInfo.menuTextColor);
					// show the text above the bar
					String pc = "Bat: " + String(percent) + "%";
					BatterySprite.drawString(pc, 100 - tft.textWidth(pc) - 2, 0);
					// push the sprite to the display
					BatterySprite.pushSprite(tft.width() - 101, tft.height() - sh + 2);
					xSemaphoreGive(MutexDisplayHandle);
				}
			}
			showtime = millis();
		}
		if (menu)
			delay(100);
		else
			break;
	}
}

void ShowProgressBar(int percent)
{
	static int lastpercent = 0;
	if (lastpercent && (lastpercent == percent))
		return;
	int x = tft.width() - 1;
	int y = (tft.fontHeight() + 4);
	int h = 8;
	if (percent == 0) {
		tft.fillRect(0, y, x, h, TFT_BLACK);
	}
	DrawProgressBar(0, y, x, h, percent, true);
	lastpercent = percent;
}

// show the update bin file progress
void ShowUpdateProgress(size_t x, size_t total)
{
	ShowProgressBar(x * 100 / total);
}

// see if there is an update bin file in the SD slot
void CheckUpdateBin(MenuItem * menu)
{
	const char* binFileName = "/RadioFox.bin";
	if (SD.exists(binFileName)) {
		if (GetYesNo("Load New Firmware?")) {
			ClearScreen();
			DisplayLine(2, "loading...");
#if USE_STANDARD_SD
			SDFile binFile;
			binFile = SD.open(binFileName);
			if (binFileName) {
#else
			FsFile binFile;
			binFile = SD.open(binFileName);
			if (binFile.getError() == 0) {
#endif
				size_t binSize = binFile.size();
				//Serial.println("size: " + String(binSize));
				Update.begin(binSize);
				Update.onProgress(ShowUpdateProgress);
				size_t bytesWritten = Update.writeStream(binFile);
				//Serial.println("written: " + String(bytesWritten));
				Update.end();
				binFile.close();
				ClearScreen();
				if (GetYesNo("Delete BIN file?")) {
					SD.remove(binFileName);
				}
				ClearScreen();
				ESP.restart();
			}
			}
		}
	else {
		WriteMessage("No RadioFox.BIN", true);
	}
}

// scan for networks
int ScanForNetworks()
{
	//Serial.println("scan start");
	WiFi.disconnect();
	// WiFi.scanNetworks will return the number of networks found
	int retval = WiFi.scanNetworks();
	//Serial.println("scan done");
	if (retval == 0) {
		Serial.println("no networks found");
	}
	else {
		Serial.print(retval);
		Serial.println(" networks found");
		for (int i = 0; i < retval; ++i) {
			// Print SSID and RSSI for each network found
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print(WiFi.SSID(i));
			Serial.print(" (");
			Serial.print(WiFi.RSSI(i));
			Serial.print(")");
			Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
		}
	}
	return retval;
}

// choose a network name from the first 5 found
void GetNetworkName(MenuItem * menu)
{
	ClearScreen();
	DisplayLine(0, "Scanning Networks...", SystemInfo.menuTextColor, TFT_BLACK);
	int nets = ScanForNetworks();
	// maximum 5 nets
	constexpr int maxNetworks = 5;
	nets = min(nets, maxNetworks);
	for (int ix = 0; ix < nets; ++ix) {
		DisplayLine(ix, WiFi.SSID(ix), SystemInfo.menuTextColor, TFT_BLACK);
	}
	// loop handling key presses
	int which = 0;
	bool done = false;
	DisplayLine(6, "Longpress=accept LongB0=cancel", SystemInfo.menuTextColor, TFT_BLACK);
	DisplayLine(which, WiFi.SSID(which), TFT_BLACK, SystemInfo.menuTextColor);
	while (!done) {
		CRotaryDialButton::Button btn = ReadButton();
		switch (btn) {
		case CRotaryDialButton::BTN_LONGPRESS:
			strncpy((char*)menu->value, WiFi.SSID(which).c_str(), menu->max - 1);
			bControllerReboot = true;
			done = true;
			break;
		case CRotaryDialButton::BTN0_LONGPRESS:
			done = true;
			break;
		case CRotaryDialButton::BTN_RIGHT:
			if (which < maxNetworks - 1) {
				DisplayLine(which, WiFi.SSID(which), SystemInfo.menuTextColor, TFT_BLACK);
				++which;
				DisplayLine(which, WiFi.SSID(which), TFT_BLACK, SystemInfo.menuTextColor);
			}
			break;
		case CRotaryDialButton::BTN_LEFT:
			if (which > 0) {
				DisplayLine(which, WiFi.SSID(which), SystemInfo.menuTextColor, TFT_BLACK);
				--which;
				DisplayLine(which, WiFi.SSID(which), TFT_BLACK, SystemInfo.menuTextColor);
			}
			break;
		default:
			break;
		}
	}
	delay(10);
}

// get the text from a menu item
void GetText(MenuItem* menu)
{
	char* str = (char*)menu->value;
	if (str) {
		String text = str;
		ClearScreen();
		String upperLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_@#$%^&|";
		String lowerLetters = "abcdefghijklmnopqrstuvwxyz0123456789 -_@#$%^&|";
		String letters = upperLetters;
		bool bUpper = true;
		CRotaryDialButton::Button button = BTN_NONE;
		bool done = false;
		if (menu->op == eBool) {
			DisplayLine(5, "Rotate dial to select, Click appends char, '|' separates OR fields", SystemInfo.menuTextColor);
			DisplayLine(6, "Long press dial exits, BTN0 deletes last char, BTN0 Long clears text", SystemInfo.menuTextColor);
		}
		else {
			DisplayLine(5, "Rotate dial to select, Click appends char, BTN1 Long toggles case", SystemInfo.menuTextColor);
			DisplayLine(6, "Long press dial exits, BTN0 deletes last char, BTN0 Long clears text", SystemInfo.menuTextColor);
		}
		int nLetterIndex = 0;
		// redraw screen only when necessary
		bool bRedraw = true;
		const int partA = 13;	// half the alphabet
		do {
			if (bRedraw) {
				DisplayLine(0, text, SystemInfo.menuTextColor);
				// draw the text
				DisplayLine(1, letters.substring(0, partA), SystemInfo.menuTextColor);
				DisplayLine(2, letters.substring(partA, partA * 2), SystemInfo.menuTextColor);
				DisplayLine(3, letters.substring(partA * 2), SystemInfo.menuTextColor);
				// figure out which letter to hilite
				int y = nLetterIndex / partA;
				y = constrain(y, 0, 2);
				int x = tft.textWidth(letters.substring(y * partA, nLetterIndex));
				char ch[2] = { 0 };
				ch[0] = letters[nLetterIndex];
				// the width calculation for ' ' is 0 (an error!), so we use something close
				if (ch[0] == ' ')
					ch[0] = '|';
				if (SystemInfo.bMenuStar) {
					tft.drawChar(x + 1, tft.fontHeight() * (y + 2) - 6, letters[nLetterIndex], TFT_WHITE, TFT_BLACK, 1);
				}
				else {
					tft.fillRect(x + 1, tft.fontHeight() * (y + 1) - 4, tft.textWidth(ch), (y + 2) + tft.fontHeight(), SystemInfo.menuTextColor);
					tft.drawChar(x + 1, tft.fontHeight() * (y + 2) - 6, letters[nLetterIndex], TFT_BLACK, TFT_BLACK, 1);
				}
				bRedraw = false;
			}
			button = ReadButton();
			switch (button) {
			case BTN_NONE:
			case BTN_B1_CLICK:
			case BTN_B2_LONG:
				break;
			case BTN_B1_LONG:
				if (menu->op != eBool) {
					bUpper = !bUpper;
					letters = bUpper ? upperLetters : lowerLetters;
					bRedraw = true;
				}
				break;
			case BTN_LEFT:
				if (nLetterIndex)
					--nLetterIndex;
				else
					nLetterIndex = letters.length() - 1;
				bRedraw = true;
				break;
			case BTN_RIGHT:
				if (nLetterIndex < letters.length() - 1)
					++nLetterIndex;
				else
					nLetterIndex = 0;
				bRedraw = true;
				break;
			case BTN_SELECT:	// add a letter if room
				if (text.length() < menu->max) {
					text += letters[nLetterIndex];
					bRedraw = true;
				}
				break;
			case BTN_B0_CLICK:	// delete last character
				if (text.length())
					text = text.substring(0, text.length() - 1);
				bRedraw = true;
				break;
			case BTN_B0_LONG:	// clear the text
				text.clear();
				bRedraw = true;
				break;
			case BTN_LONG:
				done = true;
				break;
			}
		} while (!done);
		// copy the string back, we already checked to make sure not too long
		strcpy(str, text.c_str());
	}
}

// get an audio file from the SD card
void GetAudioFile(MenuItem* menu)
{
	char* str = (char*)menu->value;
	// keep the files here
	std::vector<String> FileNames;
	// read all the filenames
	GetFileNamesFromSD(FileNames, "WAV");
	if (str) {
		// holds the current selection
		int nNameIndex = 0;
		// holds the list starting index
		int nStartIndex = 0;
		String text = str;
		// find the current audio file and set the index
		int foundIx = 0;
		for (String fn : FileNames) {
			String to = str;
			to.toUpperCase();
			fn.toUpperCase();
			if (fn.compareTo(to) == 0) {
				//Serial.print(fn + " " + foundIx + "\n");
				// set the current index and start if necessary
				nNameIndex = foundIx;
				if (foundIx >= nMenuLineCount - 1) {
					nStartIndex = nNameIndex - nMenuLineCount + 2;
				}
				break;
			}
			++foundIx;
		}
		ClearScreen();
		DisplayLine(nMenuLineCount - 1, "Long=OK Click=Cancel", SystemInfo.menuTextColor);
		CRotaryDialButton::Button button = BTN_NONE;
		bool done = false;
		// redraw screen only when necessary
		bool bRedraw = true;
		do {
			if (bRedraw) {
				String line;
				bool hilite;
				for (int ix = 0; ix < FileNames.size() && ix < nMenuLineCount - 1; ++ix) {
					// high light the current selection
					hilite = (nNameIndex - nStartIndex) == ix;
					line = FileNames[ix + nStartIndex];
					if (SystemInfo.bMenuStar) {
						line = (hilite ? "*" : " ") + line;
						DisplayLine(ix, line, SystemInfo.menuTextColor);
					}
					else {
						DisplayLine(ix, line, hilite ? TFT_BLACK : SystemInfo.menuTextColor, hilite ? SystemInfo.menuTextColor : TFT_BLACK);
					}
				}
				bRedraw = false;
			}
			button = ReadButton();
			switch (button) {
			case BTN_NONE:
			case BTN_B1_CLICK:
			case BTN_B2_LONG:
			case BTN_B1_LONG:
			case BTN_B0_CLICK:
			case BTN_B0_LONG:
				break;
			case BTN_LEFT:	// previous line
				if (nNameIndex > 0) {
					// select the previous one
					--nNameIndex;
					// check if we need to scroll
					if (nNameIndex - nStartIndex < 0) {
						--nStartIndex;
					}
					bRedraw = true;
				}
				break;
			case BTN_RIGHT:	// next line
				// check if more available
				if (nNameIndex < FileNames.size() - 1) {
					++nNameIndex;
					// check if we need to scroll
					if (nNameIndex - nStartIndex >= nMenuLineCount - 1) {
						++nStartIndex;
					}
					bRedraw = true;
				}
				break;
			case BTN_LONG:	// set the new name
				text = FileNames[nNameIndex];
				done = true;
				break;
			case BTN_SELECT:	// use this to cancel
				done = true;
				break;
			}
		} while (!done);
		// copy the string back
		if (text.length())
			strcpy(str, text.c_str());
	}
}

// toggle web server running, reboot if needed
void ToggleWebServer(MenuItem* menu)
{
	// save existing value
	bool bWas = *(bool*)menu->value;
	ToggleBool(menu);
	bControllerReboot = (bWas != *(bool*)menu->value);
}

void EraseStartFile(MenuItem* menu)
{
	//if (GetYesNo("Erase START.MIW?"))
	//	WriteOrDeleteConfigFile("", true, true, false);
}

// read the files from the card
void GetFileNamesFromSD(std::vector<String>& FileNames, String ext, String dir)
{
	ext.toUpperCase();
	ext = "." + ext;
	bool worked = true;
	//Serial.print("reading files: " + dir + "*" + ext);
	// start over
	// first empty the current file names
	FileNames.clear();
	String startfile;
	if (dir.length() > 1)
		dir = dir.substring(0, dir.length() - 1);
#if USE_STANDARD_SD
	File root = SD.open(dir);
	File file;
#else
	FsFile root = SD.open(dir, O_RDONLY);
	FsFile file;
#endif
	String CurrentFilename = "";
	if (!root) {
		//Serial.println("Failed to open directory: " + dir + "\n");
		//Serial.println("error: " + String(root.getError()));
		//SD.errorPrint("fail");
		worked = false;
	}
	if (!root.isDirectory()) {
		//Serial.println("Not a directory: " + dir);
		worked = false;
	}
	if (worked) {
		file = root.openNextFile();
		if (dir != "/") {
			// add an arrow to go back
			//String sdir = currentFolder.substring(0, currentFolder.length() - 1);
			//sdir = sdir.substring(0, sdir.lastIndexOf("/"));
			//if (sdir.length() == 0)
			//	sdir = "/";
			//FileNames.push_back(String(PREVIOUS_FOLDER_CHAR));
		}
		while (file) {
#if USE_STANDARD_SD
			CurrentFilename = file.name();
#else
			char fname[100];
			file.getName(fname, sizeof(fname));
			CurrentFilename = fname;
#endif
			// strip path
			CurrentFilename = CurrentFilename.substring(CurrentFilename.lastIndexOf('/') + 1);
			//Serial.println("name: " + CurrentFilename);
			if (CurrentFilename != "System Volume Information") {
				if (file.isDirectory()) {
					//FileNames.push_back(String(NEXT_FOLDER_CHAR) + CurrentFilename);
				}
				else {
					String uppername = CurrentFilename;
					uppername.toUpperCase();
					//find files with our extension only
					if (uppername.endsWith(ext)) {
						//Serial.println("name: " + CurrentFilename);
						FileNames.push_back(CurrentFilename);
					}
					else if (uppername == StartFileName) {
						startfile = CurrentFilename;
					}
				}
			}
			file.close();
			file = root.openNextFile();
		}
		root.close();
		std::sort(FileNames.begin(), FileNames.end(), CompareNames);
		bSdCardValid = true;
	}
	else {
		bSdCardValid = false;
	}
	return;
}

// Functions
void sendLetter(char c) {
	const char* morseCodeLetter[] = {
	  ".-",       // A
	  "-...",     // B
	  "-.-.",     // C
	  "-..",      // D
	  ".",        // E
	  "..-.",     // F
	  "--.",      // G
	  "....",     // H
	  "..",       // I
	  ".---",     // J
	  "-.-",      // K
	  ".-..",     // L
	  "--",       // M
	  "-.",       // N
	  "---",      // O
	  ".--.",     // P
	  "--.-",     // Q
	  ".-.",      // R
	  "...",      // S
	  "-",        // T
	  "..-",      // U
	  "...-",     // V
	  ".--",      // W
	  "-..-",     // X
	  "-.--",     // Y
	  "--..",     // Z
	};
	const char* morseCodeDigit[] = {
	  "-----",    // 0
	  ".----",    // 1
	  "..---",    // 2
	  "...--",    // 3
	  "....-",    // 4
	  ".....",    // 5
	  "-....",    // 6
	  "--...",    // 7
	  "---..",    // 8
	  "----.",    // 9
	};

	c = toupper(c);
	char const* pm = NULL;
	if (isalpha(c)) {
		pm = morseCodeLetter[c - 'A'];
	}
	else if (isdigit(c)) {
		pm = morseCodeDigit[c - '0'];
	}
	else if (c == '/') {
		pm = "-..-.";
	}
	else if (c == ' ') {
		pm = " ";
	}
	if (pm)
		sendMorseCode(pm);
}

void sendMorseCode(const char* tokens) {
	int i;
	// Serial.println("Morse: " + String(tokens));
	for (i = 0; tokens[i]; ++i) {
		switch (tokens[i]) {
		case '-':
			sendDash();
			break;
		case '.':
			sendDot();
			break;
		case ' ':
			sendEndOfWord();
			break;
		}
	}
	vTaskDelay(pdMS_TO_TICKS(2 * SystemInfo.nMorseInterval));
	//   Serial.print(" ");
}

void sendEndOfWord() {
	vTaskDelay(pdMS_TO_TICKS(4 * SystemInfo.nMorseInterval));
	//   Serial.print("  ");
}

// basic functions - Morse code concepts
void sendDot() {
	ledcWriteTone(toneChannel, BUZZER_FREQUENCY);
	vTaskDelay(pdMS_TO_TICKS(1 * SystemInfo.nMorseInterval));
	ledcWriteTone(toneChannel, 0);
	vTaskDelay(pdMS_TO_TICKS(1 * SystemInfo.nMorseInterval));
	//   Serial.print(".");
}

void sendDash() {
	ledcWriteTone(toneChannel, BUZZER_FREQUENCY);
	vTaskDelay(pdMS_TO_TICKS(3 * SystemInfo.nMorseInterval));
	ledcWriteTone(toneChannel, 0);
	vTaskDelay(pdMS_TO_TICKS(1 * SystemInfo.nMorseInterval));
	//   Serial.print("-");
}
