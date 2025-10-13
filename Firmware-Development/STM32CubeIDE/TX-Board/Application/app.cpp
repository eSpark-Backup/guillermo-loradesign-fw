#include "app.h"
#include "main.h"
#include "usart.h"
#include "gps.h"
#include "rtc.h"
#include "subghz_phy_app.h"
#include "app_subghz_phy.h"
#include <string.h>
#include "stm32_lpm_if.h"
#include "radio.h"
#include "timer.h"
#include "MCP7940.h"
#include <cstdio>
#include "flash_data.h"

BoardState state = BOOTUP_STATE;

static GPS gps;
TimerEvent_t SleepTimeoutTimer;
static uint32_t shutoffCount = 0;
uint32_t shutOffStartTime = 0;
MCP7940_Class  MCP7940;

// Returns the time difference in seconds between ref_time and rtc_time, wrapping if needed
uint32_t get_wrapped_seconds_diff(uint8_t ref_hour, uint8_t ref_minute, uint8_t ref_second)
{
	DateTime now = MCP7940.now();
    uint32_t rtc_seconds = (now.hour() * 3600) + (now.minute() * 60) + now.second();
    uint32_t ref_seconds = (ref_hour * 3600) + (ref_minute * 60) + ref_second;
    // uint32_t rtc_seconds = rtc->hour * 3600 + rtc->minute * 60 + rtc->second;
    // uint32_t ref_seconds = ref_hour * 3600 + ref_minute * 60 + 0;

    if (ref_seconds >= rtc_seconds) {
        return ref_seconds - rtc_seconds;
    } else {
        return (24 * 3600 - rtc_seconds) + ref_seconds;
    }
}

void generatePayload(uint8_t* buffer, uint32_t size)
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
//	 RtcData_t rtcStoredData;
//	 GPS::getGnssData(&rtcStoredData);


	 FlashData_t flashStoredData;
	 GPS::getGnssData(&flashStoredData);

	packet.timestamp.hour = gps.gnssData.hour;
	packet.timestamp.minute = gps.gnssData.minute;
	packet.timestamp.second = gps.gnssData.second;
	packet.geoCoordinates.lattitude = gps.gnssData.latitude;
	packet.geoCoordinates.longitude = gps.gnssData.longitude;
	packet.geoCoordinates.altitude = gps.gnssData.altitude;
	memcpy(payload + strlen(MY_IDENTIFIER), &packet, sizeof(packet));
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
	TimerStop( &SleepTimeoutTimer );

	extern SUBGHZ_HandleTypeDef hsubghz;
	HAL_SUBGHZ_DeInit(&hsubghz);

	DateTime now = MCP7940.now();
	DateTime alarmTime = now + TimeSpan(seconds);

	MCP7940.setAlarm(0, matchAll, alarmTime, true);  // Set one-time alarm

	LOG_D("Alarm 0 set for: %04d-%02d-%02d %02d:%02d:%02d\r\n",
		   alarmTime.year(), alarmTime.month(), alarmTime.day(),
		   alarmTime.hour(), alarmTime.minute(), alarmTime.second());

//	PWR_EnterOffMode();
//	PWR_ExitOffMode();

	LOG_D("Starts Receiving Data\r\n");
	Radio.Rx(RCV_TIMEOUT_VALUE);
}

extern "C" void receiveTimeout(void)
{
	switch(state)
	{
	case BOOTUP_STATE:
	{
		LOG_D("No Signal Received\r\n");
//		 RtcData_t rtcStoredData;
//		 GPS::getGnssData(&rtcStoredData);
//		 gps.gnssData.hour;


		FlashData_t flashStoredData;
		GPS::getGnssData(&flashStoredData);
		gps.gnssData.hour;

		DateTime now = MCP7940.now();

		if(now.hour() >= HOUR_THRESHOLD)
		{
			LOG_D("Hour threshold exceeded. ");
			LOG_D("Sleeping for a day\r\n");
			uint32_t val = SLEEP_STATE_DAY;
//			gps.rtcOperation(STORE, SLEEP_ADDR, &val);
			Flash_Write(SLEEP_ADDR, val);
			LOG_D("Value stored in Flash: %lu \n", val);
			sleep(DAY_TIMEOUT);
		}
		else
		{
			uint32_t val = SLEEP_STATE_25S;
//			gps.rtcOperation(STORE, SLEEP_ADDR, &val);
			Flash_Write(SLEEP_ADDR, val);
			LOG_D("Value stored in Flash: %lu \n", val);
			sleep(SLEEP_TIMEOUT);
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
		uint32_t val = SLEEP_STATE_DAY;
//		gps.rtcOperation(STORE, SLEEP_ADDR, &val);
		Flash_Write(SLEEP_ADDR, val);
		LOG_D("Value stored in Flash: %lu \n", val);
		sleep(DAY_TIMEOUT);
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

    // Set the RTC module to the compile time					// This should be changed according to the time received from GNSS module
    // DateTime compileTime(__DATE__, __TIME__);
    // MCP7940.adjust(compileTime);
    // LOG_D("RTC time set to: %04d-%02d-%02d %02d:%02d:%02d\r\n",
    //            compileTime.year(), compileTime.month(), compileTime.day(),
    //            compileTime.hour(), compileTime.minute(), compileTime.second());

    MCP7940.setAlarmPolarity(false);
}

void loop_test() {
    static uint8_t secs = 0;
    DateTime now = MCP7940.now();  // Get the current time

    if (secs != now.second()) {
        secs = now.second();
        LOG_D("Current RTC time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
                       now.year(), now.month(), now.day(),
                       now.hour(), now.minute(), now.second());
    }

    if (MCP7940.isAlarm(0)) {
            LOG_D("*Alarm 0 Triggered!*\r\n");
            MCP7940.clearAlarm(0);
        }
}



void App::setup(void)
{
	TimerInit( &SleepTimeoutTimer, UpTimerEventCallback );
	TimerStop( &SleepTimeoutTimer );
//	rtcModuleInit();
	//	while(1)
	//	  loop_test();
	
	
	// if its first boot than go to sleep as soon as GNSS is locked
	// if its boot from rtc wakeup signal than proceed to listen for HELLO signal
	uint32_t flag;
//	gps.rtcOperation(RETRIEVE, SLEEP_ADDR, &flag);
	flag = Flash_Read(SLEEP_ADDR);

	LOG_D("Value Read from Flash: %lu \n", flag);

//	Flash Read: 4294967295 RTC time set to: 2000-00-00 00:00:00



//	  uint32_t readData;



	bool firstBoot = ((SLEEP_STATE_25S != flag) && (SLEEP_STATE_DAY != flag)) ? true : false;
	bool wakeFromDaySleep = (SLEEP_STATE_DAY == flag) ? true : false;
	
	if(firstBoot || wakeFromDaySleep)
	{
//		gps.setup(&hrtc, &huart1);
//		do
//		{
//			gps.loop();
//			//gps.rtcOperation(RETRIEVE, GNSS_LOCK_FLAG_ADDR, &flag);
//		} while ((!gps.gnssSignalReceived) || (!gps.has3DFix()));
//		gps.displayInfo();
		setRtcTime();
		HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);	// turn of GNSS at this point
	}

	if(firstBoot)
	{
		uint32_t val = SLEEP_STATE_DAY;
//		gps.rtcOperation(STORE, SLEEP_ADDR, &val);
		Flash_Write(SLEEP_ADDR, val);
		LOG_D("Value stored in Flash: %lu \n", val);
		sleep(DAY_TIMEOUT);
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
			uint32_t val = SLEEP_STATE_DAY;
//			gps.rtcOperation(STORE, SLEEP_ADDR, &val);
			Flash_Write(SLEEP_ADDR, val);
			LOG_D("Value stored in Flash: %lu \n", val);
			sleep(DAY_TIMEOUT);
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
