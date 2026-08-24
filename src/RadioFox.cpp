/*
 Name:		RadioFox.ino
 Created:	3/25/2023 8:37:33 AM
 Author:	Sven Schumacher & Martin Nohr
 Call Sign: VE6IDK & KK7JTE
*/
#include <Arduino.h>
#include "RadioFox.h"
#include <PhoneDTMF.h>
#include "pitches.h"
#include <EEPROM.h>
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2SNoDAC.h"

AudioGeneratorMP3 *audioMP3 = nullptr;
AudioGeneratorWAV *audioWav = nullptr;
AudioFileSourceSPIFFS *file = nullptr;
AudioFileSourceSD *audioSource = nullptr;
AudioOutputI2SNoDAC *audioOut = nullptr;
AudioFileSourceID3 *id3 = nullptr;

PhoneDTMF dtmf = PhoneDTMF();

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
	(void)cbData;
	Serial.printf("ID3 callback for: %s = '", type);

	if (isUnicode)
	{
		string += 2;
	}

	while (*string)
	{
		char a = *(string++);
		if (isUnicode)
		{
			string++;
		}
		Serial.printf("%c", a);
	}
	Serial.printf("'\n");
	Serial.flush();
}

// timer called every second
void periodic_Second_timer_callback(void *arg)
{
	if (SystemInfo.eDisplayDimMode == DISPLAY_DIM_MODE_TIME && displayDimTimer)
	{
		--displayDimTimer;
		if (displayDimTimer == 0)
		{
			displayDimNow = true;
		}
	}
}

// handle the sideways scrolling of long lines
void TaskScrollSideways(void *params)
{
	// use this to make task run every second
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(SystemInfo.nSidewayScrollSpeed);
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	for (;;)
	{
		for (int ix = 0; ix < nMenuLineCount; ++ix)
		{
			// see if we need to restart because the screen was cleared
			if (ulTaskNotifyTake(pdTRUE, 0) != 0)
			{
				break;
			}
			int offset = TextLines[ix].nRollOffset;
			if (TextLines[ix].nRollLength)
			{
				if (TextLines[ix].nRollOffset == 0 && TextLines[ix].nRollDirection == 0)
				{
					TextLines[ix].nRollDirection = SystemInfo.nSidewaysScrollPause;
					continue;
				}
				if (TextLines[ix].nRollDirection > 1)
				{
					--TextLines[ix].nRollDirection;
				}
				if (TextLines[ix].nRollDirection == 1)
				{
					++TextLines[ix].nRollOffset;
				}
				if (TextLines[ix].nRollOffset >= (TextLines[ix].nRollLength - tft.width()) && TextLines[ix].nRollDirection > 0)
				{
					TextLines[ix].nRollDirection = -SystemInfo.nSidewaysScrollPause;
				}
				if (TextLines[ix].nRollDirection < -1)
				{
					++TextLines[ix].nRollDirection;
				}
				if (TextLines[ix].nRollDirection == -1)
				{
					TextLines[ix].nRollOffset -= SystemInfo.nSidewaysScrollReverse;
					if (TextLines[ix].nRollOffset < 0)
					{
						TextLines[ix].nRollOffset = 0;
					}
					if (TextLines[ix].nRollOffset == 0)
					{
						TextLines[ix].nRollDirection = 0;
					}
				}
				if (offset != TextLines[ix].nRollOffset)
				{
					DisplayLine(ix, TextLines[ix].Line, TextLines[ix].foreColor, TextLines[ix].backColor);
				}
			}
		}
		// Wait for the next cycle.
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

// send the latitude and longitude
void TaskSendGPS(void *parameter)
{
	ledcAttach(AUDIO_OUT_PORT, SystemInfo.nBuzzerFrequency, 8);
	// format the latitude and longitude
	String sendThese[2];
	// format the values of Lat and Long
	char tmp[40];
	sprintf(tmp, "Lat%.6f", SystemInfo.fLatitude);
	sendThese[0] = tmp;
	sprintf(tmp, "Long%.6f", SystemInfo.fLongitude);
	sendThese[1] = tmp;
	for (String str : sendThese)
	{
		for (char ch : str)
		{
			sendLetter(ch);
		}
		if (!IsTransmitting)
		{
			break;
		}
	}
	ledcDetach(AUDIO_OUT_PORT);
	// terminate this task
	TaskSendGpsHandle = NULL;
	vTaskDelete(NULL);
}

// send the radio id followed by the callsign
void TaskSendRadio(void *parameter)
{
	// take copies in case they get changed while we are here
	String sendThese[2];
	sendThese[0] = SystemInfo.cRadioString;
	sendThese[1] = SystemInfo.cRadioCallSign;
	ledcAttach(AUDIO_OUT_PORT, SystemInfo.nBuzzerFrequency, 8);
	for (String str : sendThese)
	{
		for (char ch : str)
		{
			sendLetter(ch);
		}
		if (!IsTransmitting)
		{
			break;
		}
	}
	ledcDetach(AUDIO_OUT_PORT);
	// terminate this task
	TaskSendRadioHandle = NULL;
	vTaskDelete(NULL);
}

// task to send the music or audio
void TaskSendMusic(void *parameter)
{
	String sFullName = SystemInfo.cAudioFile;
	// SD needs a full qualified path
	if (sFullName[0] != '/')
		sFullName = "/" + sFullName;
	if (SD.exists(sFullName))
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		// open the audio file and start reading lines from it
		File audioFile;
		audioFile = SD.open(sFullName);
		String ext = sFullName.substring(sFullName.length() - 3);
		ext.toUpperCase();
		if (ext == "TXT")
		{
			if (audioFile)
			{
				ledcAttach(AUDIO_OUT_PORT, SystemInfo.nBuzzerFrequency, 8);
				// put some defaults in just in case they are missing in the music file
				int noteLength = 90;
				int tempo = 145;
				// this holds the duration of a whole note in ms (60s/tempo)*4 beats
				int wholenote = (60000 * 4) / tempo;
				// first read until the ~ which marks the start of the data stream
				audioFile.readStringUntil('~');
				// read tokens until done
				String key, value;
				while ((key = audioFile.readStringUntil(',')))
				{
					// clean the key
					key.trim();
					key.toUpperCase();
					// next get the value
					value = audioFile.readStringUntil(',');
					value.trim();
					// if empty must be missing value, give up
					if (value.isEmpty())
						break;
					// check for tempo setting
					if (key.equals("TEMPO"))
					{
						// set the tempo
						tempo = value.toInt();
						wholenote = (60000 * 4) / tempo;
					}
					else if (key.equals("LENGTH"))
					{
						noteLength = value.toInt();
					}
					else
					{
						// process the notes and durations
						int note = mapNotes[key.c_str()];
						int divider = value.toInt();
						int duration = wholenote / divider;
						// negative duration means times 1.5 (dotted note)
						if (duration < 0)
						{
							// dotted notes are represented with negative durations!!
							duration *= -1.5;
						}
						// we only play the note for noteLength % of the duration, leaving the rest as a pause
						ledcWriteTone(AUDIO_OUT_PORT, note);
						vTaskDelay(pdMS_TO_TICKS((float)duration * noteLength / 100.0));
						ledcWriteTone(AUDIO_OUT_PORT, 0);
						vTaskDelay(pdMS_TO_TICKS((float)duration * ((100 - noteLength) / 100.0)));
					}
				}
				ledcDetach(AUDIO_OUT_PORT);
				audioFile.close();
			}
		}
		else if (ext == "WAV")
		{
			audioSource = new AudioFileSourceSD(sFullName.c_str());
			audioOut = new AudioOutputI2SNoDAC(32);
			audioWav = new AudioGeneratorWAV();
			audioWav->begin(audioSource, audioOut);
			// play until done
			bool bDone = false;
			while (!bDone)
			{
				if (audioWav->isRunning())
				{
					if (!audioWav->loop())
					{
						// Serial.printf("WAV stopped\n");
						audioWav->stop();
						bDone = true;
					}
				}
				vTaskDelay(pdMS_TO_TICKS(5));
			}
			if (audioSource)
				audioSource->close();
			// clean up just in case
			delete audioWav;
			audioWav = nullptr;
			delete audioSource;
			audioSource = nullptr;
			delete audioOut;
			audioOut = nullptr;
		}
		else if (ext == "MP3")
		{
			audioSource = new AudioFileSourceSD(sFullName.c_str());
			// audioSource->RegisterMetadataCB(MDCallback, (void *)"ID3TAG");
			audioOut = new AudioOutputI2SNoDAC(32);
			audioMP3 = new AudioGeneratorMP3();
			audioMP3->begin(audioSource, audioOut);
			// play until done
			bool bDone = false;
			while (!bDone)
			{
				if (audioMP3->isRunning())
				{
					if (!audioMP3->loop())
					{
						// Serial.printf("MP3 stopped\n");
						audioMP3->stop();
						bDone = true;
					}
				}
				vTaskDelay(pdMS_TO_TICKS(5));
			}
			if (audioSource)
				audioSource->close();
			// clean up just in case
			delete audioMP3;
			audioMP3 = nullptr;
			delete audioSource;
			audioSource = nullptr;
			delete audioOut;
			audioOut = nullptr;
		}
	}
	// terminate this task
	TaskSendMusicHandle = NULL;
	vTaskDelete(NULL);
}

// this controls the radio sending operations
void TaskRunTransmit(void *parameter)
{
	if (IsTransmitEnabled && IsRadioReady)
	{
		gpio_set_level((gpio_num_t)PTT_PORT, PTT_TALK);
		xEventGroupSetBits(gRadioEventsHandle, RadioEventIsTransmitting);
	}
	xTaskNotify(TaskRunRadioHandle, (uint32_t)"TX Start", eSetValueWithOverwrite);
	// wait for PTT to take effect
	vTaskDelay(pdMS_TO_TICKS(500));
	// a list of our tasks to run
	static const struct RFTaskEntry
	{
		const char *name;
		void (*task)(void *pArgs);
		TaskHandle_t *pTaskHandle;
		bool *bRunThis1; // NULL to always run, else a boolean address
		bool *bRunThis2; // if this is here, they must both be true
	} RFTaskList[] = {
		{"ID+Call", TaskSendRadio, &TaskSendRadioHandle, NULL, NULL},
		{"GPS", TaskSendGPS, &TaskSendGpsHandle, &SystemInfo.bBeaconMode, &SystemInfo.bSendGPS},
		{"Audio", TaskSendMusic, &TaskSendMusicHandle, &SystemInfo.bPlayAudioFile, NULL},
	};
	bool bDone = false;
	while (IsRadioReady && IsTransmitEnabled && !bDone && ulTaskNotifyTake(pdTRUE, 0) == 0)
	{
		int nLoopCount = 0;
		for (const struct RFTaskEntry &pte : RFTaskList)
		{
			bool b1 = pte.bRunThis1 ? *pte.bRunThis1 : true;
			bool b2 = pte.bRunThis2 ? *pte.bRunThis2 : true;
			// see if this should be run, NULL or *true means do it
			if (b1 && b2)
			{
				// send the name for display
				xTaskNotify(TaskRunRadioHandle, (uint32_t)pte.name, eSetValueWithOverwrite);
				// start the task
				xTaskCreate(pte.task, pte.name, 6000, NULL, 6, pte.pTaskHandle);
				// wait for it to complete or be cancelled
				while (*pte.pTaskHandle)
				{
					// // check for timeout or cancel task
					// if (SystemInfo.bStopImmediately && ulTaskNotifyTake(pdTRUE, 0))
					// {
					// 	if (*pte.pTaskHandle)
					// 		vTaskDelete(*pte.pTaskHandle);
					// 	*pte.pTaskHandle = NULL;
					// 	bDone = true;
					// 	break;
					// }
					vTaskDelay(pdMS_TO_TICKS(100));
				}
				if (bDone || !IsTransmitEnabled)
					break;
			}
			++nLoopCount;
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
	// Turn off the output tone also
	ledcWriteTone(AUDIO_OUT_PORT, 0);
	// turn PTT off here
	gpio_set_level((gpio_num_t)PTT_PORT, PTT_LISTEN);
	xEventGroupClearBits(gRadioEventsHandle, RadioEventIsTransmitting);
	// stop this task after clearing the handle
	TaskRunTransmitHandle = NULL;
	vTaskDelete(NULL);
	// int freestack = uxTaskGetStackHighWaterMark(NULL);
	// Serial.println(String("xmit high water: ") + freestack);
}

// return random between two numbers, including both of them
int GetRandomBetween(int one, int two)
{
	int delta = abs(one - two);
	++delta; // include the larger one
	return (rand() % delta) + min(one, two);
}

// return the next tx time
int NextTxTime()
{
	return SystemInfo.bUseFixedTimers ? SystemInfo.nTxTimeFixed : GetRandomBetween(SystemInfo.nTxTimeMin, SystemInfo.nTxTimeMax);
}

// return the next pause time
int NextPauseTime()
{
	return SystemInfo.bUseFixedTimers ? SystemInfo.nTxPauseFixed : GetRandomBetween(SystemInfo.nTxPauseMin, SystemInfo.nTxPauseMax);
}

// all the timing and screen display is done here
void TaskRunRadio(void *parameter)
{
	// init the random generator used for timers
	srand(analogRead(BATTERY_SENSOR_GPIO));
	const char *cStatusText = NULL;
	uint32_t status = 0;
	bool bTransmitting = false;
	int secondsLeft = 0;
	int txCount = 0;
	bool bWaitingForStop = false;
	bool bWasXmit = false;
	// use this to make task run every second
	TickType_t xLastWakeTime;
	// run every second
	const TickType_t xFrequency = pdMS_TO_TICKS(1000);
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	int delayedSeconds = 0;
	// keep the time we transmit and pause in here
	unsigned long TxTime = 0;
	while (true)
	{
		if (IsTransmitEnabled)
		{
			++TxTime;
		}
		if (!IsRadioReady && bWasXmit)
		{
			// stop the radio
			delayedSeconds = secondsLeft = 0;
		}
		if (IsTransmitEnabled || TaskRunTransmitHandle)
		{
			bool bTx = IsTransmitEnabled;
			if (bWasXmit != IsTransmitEnabled)
			{
				// tell the xmitter to stop
				if (TaskRunTransmitHandle && bWasXmit)
					xTaskNotify(TaskRunTransmitHandle, 1, eSetValueWithOverwrite);
				// set if we were xmitting
				bWaitingForStop = bWasXmit;
				// don't do this code again until the bXmit flag actually changes
				bWasXmit = IsTransmitEnabled;
				// make sure we start right away or after the delay specified
				secondsLeft = 0;
				if (SystemInfo.nStartDelayTimer)
				{
					delayedSeconds = SystemInfo.nStartDelayTimer * 60;
				}
			}
			// see if the timer has run out and we need to change state
			if (secondsLeft == 0 && delayedSeconds == 0)
			{
				bTransmitting = !bTransmitting;
				if (bTransmitting)
				{
					bWaitingForStop = false;
					// start the xmitter task, only one copy to be safe
					if (TaskRunTransmitHandle == NULL)
					{
						if (SystemInfo.bSleepWhilePausing)
						{
							RadioEnable(true);
							vTaskDelay(pdMS_TO_TICKS(250));
						}
						xTaskCreate(TaskRunTransmit, "XMITFOX", 2000, NULL, 2, &TaskRunTransmitHandle);
					}
					// wait for it to start and let us know
					int nWaitCounter = 50;
					while ((status = ulTaskNotifyTake(pdTRUE, 0)) == 0)
					{
						vTaskDelay(pdMS_TO_TICKS(10));
						if (--nWaitCounter <= 0)
						{
							break;
						}
					}
					// it must be a string they sent us
					cStatusText = (const char *)status;
					txCount++;
					secondsLeft = NextTxTime();
				}
				else
				{
					// tell the xmitter to stop
					if (TaskRunTransmitHandle)
						xTaskNotify(TaskRunTransmitHandle, 1, eSetValueWithOverwrite);
					vTaskDelay(pdMS_TO_TICKS(2));
					bWaitingForStop = true;
					// -1 keeps us waiting
					secondsLeft = -1;
					delayedSeconds = 0;
				}
			}
			// see if the task sent us anything
			status = ulTaskNotifyTake(pdTRUE, 0);
			// check if this is a string pointer
			if (status)
			{
				cStatusText = (const char *)status;
			}
		}
		if (bWaitingForStop)
		{
			// check if the xmitter is finished
			if (!TaskRunTransmitHandle)
			{
				bWaitingForStop = false;
				secondsLeft = NextPauseTime();
				// sleep radio if required
				if (SystemInfo.bSleepWhilePausing)
				{
					RadioEnable(false);
				}
			}
		}
		// some logic to get the correct status message
		if (!IsTransmitEnabled)
			delayedSeconds = 0;
		if (delayedSeconds)
			cStatusText = "Delay";
		else if (IsTransmitEnabled && !bTransmitting && !bWaitingForStop)
			cStatusText = "Pause";
		// set string if not transmitting
		else if (!IsTransmitEnabled && !bWaitingForStop)
		{
			cStatusText = "Transmit Off";
			secondsLeft = 0;
			delayedSeconds = 0;
		}
		if (!g_bSettingsMode)
		{
			int lineNo = 0;
			char fmt[20];
			String str = cStatusText;
			if (bWaitingForStop)
			{
				str = cStatusText + String(": Stopping");
			}
			else
			{
				if (delayedSeconds)
				{
					str += String(": ") + (delayedSeconds / 60) + " Min " + (delayedSeconds % 60) + " Sec";
				}
				else if (secondsLeft)
				{
					str += String(": ") + (secondsLeft / 60) + " Min " + (secondsLeft % 60) + " Sec";
				}
			}
			if (IsRadioReady)
			{
				DisplayLine(lineNo++, str, SystemInfo.menuTextColor);
			}
			else
			{
				DisplayLine(lineNo++, "Waiting for Radio", SystemInfo.menuTextColor);
			}
			// show cycle count and time active (HH:MM)
			char tm[12];
			sprintf(tm, "%2d:%02d", (TxTime / (60 * 60)), (TxTime / 60) % 60);
			DisplayLine(lineNo++, String(String("Count: ") + txCount + "  Time: " + tm), SystemInfo.menuTextColor);
			DisplayLine(lineNo++, String(SystemInfo.cRadioString) + " " + SystemInfo.cRadioCallSign + " " + ((SystemInfo.bBeaconMode && SystemInfo.bSendGPS) ? "GPS" : ""), SystemInfo.menuTextColor);
			if (SystemInfo.bPlayAudioFile)
				DisplayLine(lineNo++, "Audio: " + String(SystemInfo.cAudioFile), SystemInfo.menuTextColor);
			sprintf(fmt, "%03d MHz ", SystemInfo.nFrequency % 1000);
			DisplayLine(lineNo++, String(SystemInfo.nFrequency / 1000) + "." + fmt + (SystemInfo.bTxPowerLow ? "Lo" : "Hi") + " Power", SystemInfo.menuTextColor);
			DisplayLine(lineNo++, String("RX Offset: ") + RxOffsetModeText[SystemInfo.nRfOffset] + " kHz", SystemInfo.menuTextColor);
		}
		if (secondsLeft > 0)
			--secondsLeft;
		if (delayedSeconds > 0)
			--delayedSeconds;
		// see if we need to cancel the pauses
		if (IsCancelWaits)
		{
			delayedSeconds = secondsLeft = 0;
			xEventGroupClearBits(gRadioEventsHandle, RadioEventCancelWaits);
		}
		// Wait for the next cycle.
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

// show the transmit animation while sending
#define TX_CIRCLE_SIZE 10
void TaskXmitDisplay(void *parameters)
{
	// use this to make task run periodically
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(500);
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	int cycle = 0;
	while (true)
	{
		if (!g_bSettingsMode)
		{
			if (xSemaphoreTake(MutexDisplayHandle, portMAX_DELAY) == pdTRUE)
			{
				cycle = cycle % TX_CIRCLE_SIZE;
				if (!IsTransmitting)
					cycle = 0;
				if (cycle)
				{
					tft.drawCircle(10, tft.height() - TX_CIRCLE_SIZE, cycle, TFT_RED);
				}
				else
				{
					tft.fillCircle(10, tft.height() - TX_CIRCLE_SIZE, TX_CIRCLE_SIZE, TFT_BLACK);
				}
				++cycle;
				xSemaphoreGive(MutexDisplayHandle);
			}
		}
		else
		{
			cycle = 0;
		}
		// Wait for the next cycle if nothing happened this time
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

// show the battery every 60 seconds
void TaskShowBattery(void *parameters)
{
	while (true)
	{
		// show battery level if on
		if (SystemInfo.bShowBatteryLevel && !g_bSettingsMode)
		{
			int raw;
			ReadBattery(&raw);
			ShowBattery(NULL);
		}
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10 * 1000));
	}
}

// handle DTMF commands, runs periodically
// TODO: should probably turn off sending while working in here
void TaskDTMF(void *parameter)
{
	// use this to make task run periodically
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(400);
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	bool bEnabled = false;
	unsigned long enableTimer = 0;

	while (true)
	{
		bool bReply = false;
		uint8_t tones;
		char button;
		// detect tone
		tones = dtmf.detect();
		// if (tones)
		//	Serial.println(String("tones:") + tones);
		//  if valid tone was found, proof for validity
		button = dtmf.tone2char(tones);
		if (button > 0)
		{
			// Serial.println(String("button:")+button);
			//  measure 4 times, result of each measurement should be always the same
			//  time needed for this process: 80ms, so the tone must be present at least 100ms to be valid
			tones |= dtmf.detect() | dtmf.detect() | dtmf.detect();
			char ch = dtmf.tone2char(tones);
			Serial.println(String("ch:") + ch);
			// unless the timer is on, only accept '*'
			if ((millis() > enableTimer) && ch != '*')
			{
				continue;
			}
			// display it
			DisplayBottomChar(ch);
			// enable for the timer active seconds
			enableTimer = millis() + SystemInfo.nDtmfEnableTimer * 1000;
			bool bValidCommand = true;
			switch (ch)
			{
			case '*':
				bEnabled = true;
				digitalWrite(PTT_PORT, PTT_LISTEN);
				bReply = false;
				break;
			case '1':					// Start Loop
				SetRadioTransmit(true); // set the flag to ENABLE transmissions
				bReply = true;
				break;
			case '2':					 // turn off transmissions - send a short letter to confirm receive
				SetRadioTransmit(false); // set the flag to DISABLE transmissions
				bReply = true;
				break;
			case '4': // LOW Power Mode - No Loop
				digitalWrite(TXPOWER_PORT, SystemInfo.bTxPowerLow = true);
				RadioSetup(false);
				bReply = true;
				break;
			case '5': // High Power Mode
				digitalWrite(TXPOWER_PORT, SystemInfo.bTxPowerLow = false);
				RadioSetup(false);
				bReply = true;
				break;
			case '6': // start tx by cancelling the delay and pause
				CancelWaitTimers(NULL);
				break;
			default:
				bValidCommand = false;
				break;
			}
		}
		// TODO: this reply seems to kill the command, probably something to do with half duplex radio
		// if (bReply) {
		//	digitalWrite(PTT_PORT, PTT_TALK);
		//	delay(500);
		//	sendLetter('R');
		//	digitalWrite(PTT_PORT, PTT_LISTEN);
		//	bReply = false;
		//}
		// check timer
		if (bEnabled)
		{
			if (millis() > enableTimer)
			{
				bEnabled = false;
				//// restore PTT setting
				// if (IsTransmitting) {
				//	digitalWrite(PTT_PORT, PTT_TALK);
				// }
			}
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		else
		{
			// Wait for the next cycle if nothing happened this time
			vTaskDelayUntil(&xLastWakeTime, xFrequency);
		}
	}
}

// set the radio transmit event flag
void SetRadioTransmit(bool bTx)
{
	// make sure it is up to date
	SystemInfo.bXmitEnable = bTx;
	if (SystemInfo.bXmitEnable)
		xEventGroupSetBits(gRadioEventsHandle, RadioEventEnableTransmit);
	else
		xEventGroupClearBits(gRadioEventsHandle, RadioEventEnableTransmit);
}

// send a string to the radio
// wait for a response, empty string return when it works
String SendToRadio(char *msg)
{
	if (*msg == '\0')
		return "";
	bool retval = false;
	String retstr = "Radio timeout";
	String rxString;
	bool done = false;
	// try a few times
	for (int tries = 0; !done && tries < 3; ++tries)
	{
		// purge the input from the radio first
		while (RadioSerial.available())
		{
			byte b;
			RadioSerial.readBytes(&b, sizeof(b));
			// Serial.println("clearing radio input");
		}
		RadioSerial.println(msg);
		// Serial.println(String("TX:") + msg);
		//  wait for a response
		//  see if the radio answers
		for (int i = 100; i > 0; --i)
		{
			if (RadioSerial.available())
			{
				rxString = RadioSerial.readString();
				// Serial.println(String("RX:") + rxString);
				rxString.trim();
				// check the return value
				retval = rxString.indexOf(":0") > 0;
				retstr = retval ? "" : rxString;
				done = true;
				break;
			}
			vTaskDelay(pdMS_TO_TICKS(5));
		}
	}
	return (retval ? String("") : (String(msg) + " : ")) + retstr;
}

// load the radio settings
// bInit is set to send the data to the radio, if false just update the transmit flag for the radio task
bool RadioSetup(bool bIniit)
{
	bool retval = true;
	if (bIniit)
	{
		// tell people the radio is not ready
		xEventGroupClearBits(gRadioEventsHandle, RadioEventReady);
		char line[200];
		// loop through all the commands we need to send to the radio
		bool done = false;
		for (int which = 0; !done; ++which)
		{
			switch (which)
			{
			case 0: // connect
				strcpy(line, "AT+DMOCONNECT");
				break;
			case 1: // radio group settings
			{
				// set the radio data, e.g. AT+DMOSETGROUP=0,415.1250,415.1250,0012,4, 0013
				float fRX = SystemInfo.nFrequency / 1000.0;
				float fTX = (SystemInfo.nFrequency + atof(RxOffsetModeText[SystemInfo.nRfOffset])) / 1000.0;
				if (SystemInfo.bCTCSS)
				{
					sprintf(line, "AT+DMOSETGROUP=%d,%.4f,%.4f,%04d,%d,%04d",
							SystemInfo.nBandWidth, fTX, fRX,
							SystemInfo.nTxCTCSS,
							SystemInfo.nSquelch,
							SystemInfo.nRxCTCSS);
				}
				else
				{
					sprintf(line, "AT+DMOSETGROUP=%d,%.4f,%.4f,%s%c,%d,%s%c",
							SystemInfo.nBandWidth, fTX, fRX,
							SystemInfo.nTxDcs ? DcsText[SystemInfo.nTxDcs] : "000",
							SystemInfo.nTxDcs ? (SystemInfo.bTxDcsNI ? 'N' : 'I') : '0',
							SystemInfo.nSquelch,
							SystemInfo.nRxDcs ? DcsText[SystemInfo.nRxDcs] : "000",
							SystemInfo.nRxDcs ? (SystemInfo.bRxDcsNI ? 'N' : 'I') : '0');
				}
			}
			break;
			case 2: // set receive volume
				sprintf(line, "AT+DMOSETVOLUME=%d", SystemInfo.nRxVolume);
				break;
			default:
				line[0] = '\0';
				done = true;
				break;
			}
			// send it to the radio and see if it worked or timed out
			String str;
			if (line[0])
			{
				str = SendToRadio(line);
			}
			if (!str.isEmpty())
			{
				retval = false;
				done = true;
				WriteMessage(str, true, 5000);
			}
		}
		if (retval)
		{
			WriteMessage("Radio Initialized");
			// tell everybody
			xEventGroupSetBits(gRadioEventsHandle, RadioEventReady);
		}
	}
	// set the radio power control output
	digitalWrite(TXPOWER_PORT, SystemInfo.bTxPowerLow);
	// finally, set the transmit correctly
	SetRadioTransmit(SystemInfo.bXmitEnable);
	return retval;
}

void setup()
{
	Serial.begin(115200);
	while (!Serial.availableForWrite())
	{
		delay(10);
	}
	InitMenuSystem();
	// start the DTMF detector
	pinMode(AUDIO_IN_PORT, INPUT);
	dtmf.begin(AUDIO_IN_PORT, 2000);

	// Serial.println("flash:" + String(ESP.getFlashChipSize()));
	// Serial.print("setup() is running on core ");
	// Serial.println(xPortGetCoreID());

	// radio sleep control
	pinMode(RADIO_SLEEP_PORT, OUTPUT);
	// keep it awake
	RadioEnable(true);
	// set the power control to output
	pinMode(TXPOWER_PORT, OUTPUT);

	periodic_Second_timer_args = {
		periodic_Second_timer_callback,
		/* argument specified here will be passed to timer callback function */
		(void *)0,
		ESP_TIMER_TASK,
		"seconds timer"};
	esp_timer_create(&periodic_Second_timer_args, &periodic_Second_timer);
	esp_timer_start_periodic(periodic_Second_timer, (int64_t)1000 * 1000);

	// WiFi
	if (SystemInfo.bRunWebServer)
	{
		InitServer();
	}
	gRadioEventsHandle = xEventGroupCreate();
	// set the PTT port
	gpio_set_direction((gpio_num_t)PTT_PORT, GPIO_MODE_INPUT_OUTPUT);
	gpio_set_level((gpio_num_t)PTT_PORT, PTT_LISTEN);
	// start the transmit and management tasks
	xTaskCreate(TaskRunRadio, "FOXRADIO", 2000, NULL, 3, &TaskRunRadioHandle);
	xTaskCreate(TaskShowBattery, "BATTERYLEVEL", 2000, NULL, 0, &TaskShowBatteryHandle);
	xTaskCreate(TaskDTMF, "DTMFHANDLER", 2000, NULL, 2, &TaskDTMFHandle);
	xTaskCreate(TaskScrollSideways, "SCROLLSIDEWAYS", 2000, NULL, 1, &TaskScrollSidewaysHandle);
	xTaskCreate(TaskMenu, "MENU", 3000, NULL, 4, &TaskMenuHandle);
	xTaskCreate(TaskXmitDisplay, "XMITDISPLAY", 2000, NULL, 0, NULL);
	ResetDimTimer();
	//  start the radio serial port, wait until here to make sure the radio is powered up completely
	RadioSerial.begin(9600, SERIAL_8N1, RADIO_SERIAL_RX, RADIO_SERIAL_TX, false);
	// init the radio
	RadioSetup(true);
}

// the main loop
void loop()
{
	if (SystemInfo.bRunWebServer)
	{
		server.handleClient();
	}
	delay(100);
}

// Functions
void sendLetter(char c)
{
	const char *morseCodeLetter[] = {
		".-",	// A
		"-...", // B
		"-.-.", // C
		"-..",	// D
		".",	// E
		"..-.", // F
		"--.",	// G
		"....", // H
		"..",	// I
		".---", // J
		"-.-",	// K
		".-..", // L
		"--",	// M
		"-.",	// N
		"---",	// O
		".--.", // P
		"--.-", // Q
		".-.",	// R
		"...",	// S
		"-",	// T
		"..-",	// U
		"...-", // V
		".--",	// W
		"-..-", // X
		"-.--", // Y
		"--..", // Z
	};
	const char *morseCodeDigit[] = {
		"-----", // 0
		".----", // 1
		"..---", // 2
		"...--", // 3
		"....-", // 4
		".....", // 5
		"-....", // 6
		"--...", // 7
		"---..", // 8
		"----.", // 9
	};

	c = toupper(c);
	char const *pm = NULL;
	if (isalpha(c))
	{
		pm = morseCodeLetter[c - 'A'];
	}
	else if (isdigit(c))
	{
		pm = morseCodeDigit[c - '0'];
	}
	else if (c == '/')
	{
		pm = "-..-.";
	}
	else if (c == ' ')
	{
		pm = " ";
	}
	else if (c == '.')
	{
		pm = "._._._";
	}
	if (pm)
		sendMorseCode(pm);
}

void sendMorseCode(const char *tokens)
{
	int i;
	// Serial.println("Morse: " + String(tokens));
	for (i = 0; tokens[i]; ++i)
	{
		switch (tokens[i])
		{
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

void sendEndOfWord()
{
	vTaskDelay(pdMS_TO_TICKS(4 * SystemInfo.nMorseInterval));
	//   Serial.print("  ");
}

// basic functions - Morse code concepts
void sendDot()
{
	ledcWriteTone(AUDIO_OUT_PORT, SystemInfo.nBuzzerFrequency);
	vTaskDelay(pdMS_TO_TICKS(1 * SystemInfo.nMorseInterval));
	ledcWriteTone(AUDIO_OUT_PORT, 0);
	vTaskDelay(pdMS_TO_TICKS(1 * SystemInfo.nMorseInterval));
	//   Serial.print(".");
}

void sendDash()
{
	ledcWriteTone(AUDIO_OUT_PORT, SystemInfo.nBuzzerFrequency);
	vTaskDelay(pdMS_TO_TICKS(3 * SystemInfo.nMorseInterval));
	ledcWriteTone(AUDIO_OUT_PORT, 0);
	vTaskDelay(pdMS_TO_TICKS(1 * SystemInfo.nMorseInterval));
	//   Serial.print("-");
}

// enable or disable radio
void RadioEnable(bool bEnable)
{
	digitalWrite(RADIO_SLEEP_PORT, bEnable ? LOW : HIGH);
}
