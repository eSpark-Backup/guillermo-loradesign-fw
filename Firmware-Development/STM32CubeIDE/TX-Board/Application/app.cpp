#include "app.h"
#include "main.h"
#include "usart.h"
#include "gps.h"
#include "subghz_phy_app.h"
#include "app_subghz_phy.h"
#include <string.h>
#include "stm32_lpm_if.h"
#include "radio.h"
#include "timer.h"
#include "MCP7940.h"
#include <cstdio>
#include "data_storage.h"


BoardState state = BOOTUP_STATE;

static GPS gps;
//TimerEvent_t SleepTimeoutTimer;
static uint32_t shutoffCount = 0;
uint32_t shutOffStartTime = 0;
MCP7940_Class  MCP7940;

static void initiateHardware(void)
{
#if GNSS_VBACKUP_VIA_MCU
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPS_VBCKUP_EN_GPIO_Port, GPS_VBCKUP_EN_Pin, GPIO_PIN_SET);

	/*Configure GPIO pins : PBPin PBPin */
	GPIO_InitStruct.Pin = GPS_VBCKUP_EN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPS_VBCKUP_EN_GPIO_Port, &GPIO_InitStruct);
#endif
}

// Returns the time difference in seconds between ref_time and rtc_time, wrapping if needed
static uint32_t get_wrapped_seconds_diff(uint8_t ref_hour, uint8_t ref_minute, uint8_t ref_second)
{
	DateTime now = MCP7940.now();
    uint32_t rtc_seconds = (now.hour() * 3600) + (now.minute() * 60) + now.second();
    uint32_t ref_seconds = (ref_hour * 3600) + (ref_minute * 60) + ref_second;

    if (ref_seconds >= rtc_seconds) {
        return ref_seconds - rtc_seconds;
    } else {
        return (24 * 3600 - rtc_seconds) + ref_seconds;
    }
}

static void generatePayload(uint8_t* buffer, uint32_t size)
{
	char* payload = (char*) buffer;

	CommPacket packet;
	memcpy(payload, MY_IDENTIFIER, strlen(MY_IDENTIFIER));
#if CUSTOM_BOARD_ID
	packet.ids[0] = BOARD_ID_1;
	packet.ids[1] = BOARD_ID_2;
	packet.ids[2] = BOARD_ID_2;
#else
	uint32_t boardIds[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
	memcpy(packet.ids, boardIds, sizeof(packet.ids));
#endif
	// RtcData_t rtcStoredData;
	// GPS::getGnssData(&rtcStoredData);
	packet.timestamp.hour = gps.gnssData.hour;
	packet.timestamp.minute = gps.gnssData.minute;
	packet.timestamp.second = gps.gnssData.second;
	packet.geoCoordinates.lattitude = gps.gnssData.latitude;
	packet.geoCoordinates.longitude = gps.gnssData.longitude;
	packet.geoCoordinates.altitude = gps.gnssData.altitude;
	memcpy(payload + strlen(MY_IDENTIFIER), &packet, sizeof(packet));
}

void apply_timezone_offset(Storage_data_t* data, int tz_hour, int tz_minute) {
    int y = data->year;
    int mo = data->month;
    int d = data->day;
    int h = data->hour;
    int m = data->minute + tz_minute;
    int s = data->second;

    h += tz_hour + m / 60;
    m %= 60;
    if (m < 0) { m += 60; h -= 1; }

    d += h / 24;
    h %= 24;
    if (h < 0) { h += 24; d -= 1; }

    static const uint8_t daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

    while (1) {
        if (d < 1) {
            mo -= 1;
            if (mo < 1) { mo = 12; y -= 1; }
            bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
            int max_day = daysInMonth[mo - 1];
            if (leap && mo == 2) max_day = 29;
            d += max_day;
        } else {
            bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
            int max_day = daysInMonth[mo - 1];
            if (leap && mo == 2) max_day = 29;
            if (d > max_day) {
                d -= max_day;
                mo += 1;
                if (mo > 12) { mo = 1; y += 1; }
            } else break;
        }
    }

    data->year = y;
    data->month = mo;
    data->day = d;
    data->hour = h;
    data->minute = m;
    // data->second and centisecond are unchanged
}

void UpTimerEventCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
}

void sleep(uint32_t timeout)
{
	uint32_t seconds = timeout/1000;
	LOG_D("Going to sleep for %d seconds\r\n", seconds);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	Radio.Sleep();

	extern TimerEvent_t RxTimeoutTimer;
	extern TimerEvent_t TxTimeoutTimer;
	TimerStop( &RxTimeoutTimer );
	TimerStop( &TxTimeoutTimer );
//	TimerStop( &SleepTimeoutTimer );

	extern SUBGHZ_HandleTypeDef hsubghz;
	HAL_SUBGHZ_DeInit(&hsubghz);

	(storage_commit()) ? LOG_D("Storage commit successful\r\n") : LOG_D("Failed to commit storage data\r\n");

	DateTime now = MCP7940.now();
	DateTime alarmTime = now + TimeSpan(seconds);

	MCP7940.setAlarm(0, matchAll, alarmTime, true);  // Set one-time alarm

	LOG_D("Alarm 0 set for: %04d-%02d-%02d %02d:%02d:%02d\r\n",
		   alarmTime.year(), alarmTime.month(), alarmTime.day(),
		   alarmTime.hour(), alarmTime.minute(), alarmTime.second());

	PWR_EnterOffMode();
	PWR_ExitOffMode();

	LOG_D("Starts Receiving Data\r\n");
	// Radio.Rx(0xFFFFFFFF);
	Radio.Rx(RCV_TIMEOUT_VALUE);
}

extern "C" void receiveTimeout(void)
{
	switch(state)
	{
	case BOOTUP_STATE:
	{
		LOG_D("No Signal Received\r\n");

		DateTime now = MCP7940.now();

		if(now.hour() >= HOUR_THRESHOLD)
		{
			LOG_D("Hour threshold exceeded. ");
			LOG_D("Sleeping for a day\r\n");
			// storage_set_sleep_state(DAY_SLEEP_STATE);
			// sleep(DAY_SLEEP_TIMEOUT);

			// TEST ----------- REMOVE BEFORE FINALIZING
			storage_set_sleep_state(SHORT_SLEEP_STATE);
			sleep(SHORT_SLEEP_TIMEOUT);
		}
		else
		{
			storage_set_sleep_state(SHORT_SLEEP_STATE);
			sleep(SHORT_SLEEP_TIMEOUT);
		}
		break;
	}

	default:
		break;
	}

}

extern "C" void dataReceived(char* buffer, uint32_t size)
{
	LOG_D("Received ");
	for (uint32_t i = 0; i < size; i++) {
	    LOG_D("%02X ", (unsigned char)buffer[i]);
	}
	LOG_D("\r\n");


	uint32_t boardIds[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
	char mergedData[sizeof(boardIds) + 7];  // 3 * uint32_t (12 bytes) + "SHUTOFF" (7 bytes) = 19 bytes
	memcpy(mergedData, boardIds, sizeof(boardIds));
	memcpy(mergedData + sizeof(boardIds), "SHUTOFF", 7);  // Append "SHUTOFF"

	if(memcmp(buffer, mergedData, sizeof(mergedData)) == 0)
	{
		LOG_D("Sleeping for a day\r\n");
		state = BOOTUP_STATE;
		uint32_t val = DAY_SLEEP_STATE;
//		Flash_WriteSleepState(val);
		sleep(DAY_SLEEP_TIMEOUT);
	}
	else if(strcmp(buffer, PING_MESSAGE) == 0)
	{
		if (state != SHUTOFF_STATE)
		{
			state = TRANSMIT_STATE;
			LOG_D("Entering transmit state\r\n");
		}
	}
}

void setRtcTime()
{
    DateTime currentTime(gps.gnssData.year, gps.gnssData.month, gps.gnssData.day, gps.gnssData.hour, gps.gnssData.minute, gps.gnssData.second);
    MCP7940.adjust(currentTime);
    LOG_D("RTC time set to: %04d-%02d-%02d %02d:%02d:%02d\r\n",
               currentTime.year(), currentTime.month(), currentTime.day(),
               currentTime.hour(), currentTime.minute(), currentTime.second());
}

static void printCurrentRtcTime(void)
{
	static uint8_t secs = 0;
    DateTime now = MCP7940.now();  // Get the current time

    if (secs != now.second()) {
        secs = now.second();
        LOG_D("Current RTC time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
                       now.year(), now.month(), now.day(),
                       now.hour(), now.minute(), now.second());
    }
}

void rtcModuleInit() {
    // Initialize I2C communication with RTC module
    while (!MCP7940.begin()) {
        HAL_Delay(3000);
    }

    // Ensure the oscillator is running
    while (!MCP7940.deviceStatus()) {
        bool deviceStatus = MCP7940.deviceStart();
        if (!deviceStatus) {
            HAL_Delay(1000);
        }
    }

	uint32_t state = storage_get_sleep_state();
	if(state == STORAGE_DEFAULT_SLEEP_STATE)
	{
		// If first boot, set the RTC module to the compile time
		DateTime compileTime(__DATE__, __TIME__);

		// adjust the compile time to the local timezone before setting it to RTC, since MCP7940 does not handle timezone internally and we want RTC to reflect local time
		Storage_data_t rtc_time;
		rtc_time.year   = compileTime.year();
		rtc_time.month  = compileTime.month();
		rtc_time.day    = compileTime.day();
		rtc_time.hour   = compileTime.hour();
		rtc_time.minute = compileTime.minute();
		rtc_time.second = compileTime.second();
		apply_timezone_offset(&rtc_time, TIMEZONE_HOUR_OFFSET, TIMEZONE_MINUTE_OFFSET);
		DateTime adjustedTime(rtc_time.year, rtc_time.month, rtc_time.day, rtc_time.hour, rtc_time.minute, rtc_time.second);

		MCP7940.adjust(adjustedTime);
		LOG_D("RTC time set to: %04d-%02d-%02d %02d:%02d:%02d\r\n",
				   adjustedTime.year(), adjustedTime.month(), adjustedTime.day(),
				   adjustedTime.hour(), adjustedTime.minute(), adjustedTime.second());
	}


    MCP7940.setAlarmPolarity(false);

	printCurrentRtcTime();
}

void loop_test() {
    printCurrentRtcTime();

    if (MCP7940.isAlarm(0)) {
		LOG_D("*Alarm 0 Triggered!*\r\n");
		MCP7940.clearAlarm(0);
	}
}

static void gnssTestValues()
{
	DateTime compileTime(__DATE__, __TIME__);
	gps.gnssData.latitude = 37.7749;
	gps.gnssData.longitude = -122.4194;
	gps.gnssData.altitude = 30.0;
	gps.gnssData.year = compileTime.year();
	gps.gnssData.month = compileTime.month();
	gps.gnssData.day = compileTime.day();
	gps.gnssData.hour = compileTime.hour();
	gps.gnssData.minute = compileTime.minute();
	gps.gnssData.second = compileTime.second();
	gps.gnssData.centisecond = 0;

	apply_timezone_offset(&gps.gnssData, TIMEZONE_HOUR_OFFSET, TIMEZONE_MINUTE_OFFSET);

	storage_set_sleep_state(SHORT_SLEEP_STATE);  // Set to short sleep state to avoid waiting for a long sleep during testing
	storage_set_metadata(&gps.gnssData);

	storage_commit();
	storage_print_metadata();
}

void App::setup(void)
{
	if(!storage_init())
	{
		storage_format();
	}
	storage_print_metadata();
	initiateHardware();
	rtcModuleInit();
//	TimerInit( &SleepTimeoutTimer, UpTimerEventCallback );
//	TimerStop( &SleepTimeoutTimer );
	// while(1)
	// 	loop_test();

	// gnssTestValues();


	// if its first boot than go to sleep as soon as GNSS is locked
	// if its boot from rtc wakeup signal than proceed to listen for HELLO signal
	uint32_t flag = storage_get_sleep_state();

	bool firstBoot = (STORAGE_DEFAULT_SLEEP_STATE == flag) ? true : false;
	bool wakeFromDaySleep = (DAY_SLEEP_STATE == flag) ? true : false;
	
	if(firstBoot || wakeFromDaySleep)
	{
		gps.setup(&huart1);
		do
		{
			gps.loop();
		} while (!gps.gnssSignalReceived);
		gps.displayInfo();

		// Persist last valid fix for future boots that don't re-enable GNSS
		GPS::setGnssData(&gps.gnssData);
		setRtcTime();
		HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);	// turn of GNSS at this point
	}
	else{
		// Woke up from a short sleep and we are not turning GNSS back on.
		// Load the last stored fix from flash so TX payload has valid coords/time.
		if(!storage_get_metadata(&gps.gnssData))
		{
			DateTime now = MCP7940.now();
			// gps.gnssData.latitude = 0.0;
			// gps.gnssData.longitude = 0.0;
			gps.gnssData.year = now.year();
			gps.gnssData.month = now.month();
			gps.gnssData.day = now.day();
			gps.gnssData.hour = now.hour();
			gps.gnssData.minute = now.minute();
			gps.gnssData.second = now.second();
			gps.gnssData.centisecond = 0;
		}
	}

	if(firstBoot)
	{
		// uint32_t val = DAY_SLEEP_STATE;
		storage_set_sleep_state(DAY_SLEEP_STATE);
		sleep(DAY_SLEEP_TIMEOUT);
	}
	else
	{
		Radio.Rx(RCV_TIMEOUT_VALUE);
	}
}

void App::loop(void)
{
	// gps.loop();		// no need to process GNSS data, valid coordinates have been acquired

	uint32_t currentTime = HAL_GetTick();
	uint8_t data[64];
	switch(state)
	{
	case SHUTOFF_STATE:
		if((currentTime - shutOffStartTime) > 30000)
		{
			LOG_D("Sleeping for a day\r\n");
			state = BOOTUP_STATE;
			uint32_t val = DAY_SLEEP_STATE;
//			Flash_WriteSleepState(val);
			sleep(DAY_SLEEP_TIMEOUT);
		}
		else if( ( (currentTime - shutOffStartTime) / 5000U ) > shutoffCount)
		{
			LOG_D("SHUTOFF_STATE Count : %d\r\n", shutoffCount);
			shutoffCount++;
			generatePayload(data, sizeof(data));
			SwitchToTxMode(data, sizeof(data));
			while(!isTXDone());
			MX_SubGHz_Phy_Process();
			SwitchToRxMode();
		}
		break;

	case TRANSMIT_STATE:
	{
		LOG_D("Entering shutoff state\r\n");
		shutOffStartTime = HAL_GetTick();
		shutoffCount = 0;
		uint8_t data[64];
		generatePayload(data, sizeof(data));
		SwitchToTxMode(data, sizeof(data));
		while(!isTXDone());
		MX_SubGHz_Phy_Process();
		SwitchToRxMode();
		state = SHUTOFF_STATE;
	}
		break;

	default:
		break;
	}
}
