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

BoardState state = BOOTUP_STATE;

static GPS gps;
TimerEvent_t SleepTimeoutTimer;
static uint32_t shutoffCount = 0;
uint32_t shutOffStartTime = 0;
MCP7940_Class  MCP7940;

void generatePayload(uint8_t* buffer, uint32_t size)
{
	char* payload = (char*) buffer;

	CommPacket packet;
	memcpy(payload, MY_IDENTIFIER, strlen(MY_IDENTIFIER));
#if CUSTOM_BOARD_ID
	packet.ids[0] = BOARD_ID;
#else
	uint32_t boardIds[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
	memcpy(packet.ids, boardIds, sizeof(packet.ids));
#endif
	RtcData_t rtcStoredData;
	GPS::getGnssData(&rtcStoredData);
	packet.timestamp.hour = rtcStoredData.hour;
	packet.timestamp.minute = rtcStoredData.minute;
	packet.timestamp.second = rtcStoredData.second;
	packet.geoCoordinates.lattitude = rtcStoredData.latitude;
	packet.geoCoordinates.longitude = rtcStoredData.longitude;
	memcpy(payload + strlen(MY_IDENTIFIER), &packet, sizeof(packet));
}

void UpTimerEventCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
}

void sleep(uint32_t timeout)
{
	// uint32_t seconds = timeout/1000;
	// LOG_D("Going to sleep for %d seconds\r\n", seconds);
	// HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	// HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);
	// HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	// HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	// Radio.Sleep();

	// extern TimerEvent_t RxTimeoutTimer;
	// extern TimerEvent_t TxTimeoutTimer;
	// TimerStop( &RxTimeoutTimer );
	// TimerStop( &TxTimeoutTimer );
	// TimerStop( &SleepTimeoutTimer );

	// extern SUBGHZ_HandleTypeDef hsubghz;
	// HAL_SUBGHZ_DeInit(&hsubghz);

	// DateTime now = MCP7940.now();
	// DateTime alarmTime = now + TimeSpan(seconds);

	// MCP7940.setAlarm(0, matchAll, alarmTime, true);  // Set one-time alarm

	// LOG_D("Alarm 0 set for: %04d-%02d-%02d %02d:%02d:%02d\r\n",
	// 	   alarmTime.year(), alarmTime.month(), alarmTime.day(),
	// 	   alarmTime.hour(), alarmTime.minute(), alarmTime.second());

	// PWR_EnterOffMode();
	// PWR_ExitOffMode();

	// LOG_D("Starts Receiving Data\r\n");
	// Radio.Rx(5000);
}

extern "C" void receiveTimeout(void)
{
	switch(state)
	{
	case BOOTUP_STATE:
		LOG_D("No Signal Received\r\n");
		RtcData_t rtcStoredData;
		GPS::getGnssData(&rtcStoredData);

		if(rtcStoredData.hour >= HOUR_THRESHOLD)
		{
			LOG_D("Hour threshold exceeded. ");
			LOG_D("Sleeping for a day\r\n");
			sleep(DAY_TIMEOUT);
		}
		else
		{
			sleep(SLEEP_TIMEOUT);
		}
		break;

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

void rtcInit() {
    /*!
        @brief  STM32 HAL setup function
    */

    // Initialize I2C communication with RTC
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

    // Set the RTC to the compile time
    DateTime compileTime(__DATE__, __TIME__);
    MCP7940.adjust(compileTime);
    LOG_D("RTC time set to: %04d-%02d-%02d %02d:%02d:%02d\r\n",
               compileTime.year(), compileTime.month(), compileTime.day(),
               compileTime.hour(), compileTime.minute(), compileTime.second());

    MCP7940.setAlarmPolarity(false);
}

void loop_test() {
    /*!
        @brief  STM32 HAL main loop function
    */
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

//	TimerInit( &SleepTimeoutTimer, UpTimerEventCallback );
//	TimerStop( &SleepTimeoutTimer );
	gps.setup();
//	rtcInit();
//	while(1)
//	  loop_test();

//	// dont execute app state machine unless we get gnss lock
//	uint32_t flag;
//	do
//	{
//		gps.rtcOperation(RETRIEVE, GNSS_LOCK_FLAG_ADDR, &flag);
//		gps.loop();
//	}while(flag != GNSS_LOCK_FLAG);
}

void App::loop(void)
{
	gps.loop();

//	uint32_t currentTime = HAL_GetTick();
//	uint8_t data[64];
//	switch(state)
//	{
//	case SHUTOFF_STATE:
//		if((currentTime - shutOffStartTime) > 30000)
//		{
//			LOG_D("Sleeping for a day\r\n");
//			state = BOOTUP_STATE;
//			sleep(DAY_TIMEOUT);
//		}
//		else if( ( (currentTime - shutOffStartTime) / 5000U ) > shutoffCount)
//		{
//			LOG_D("SHUTOFF_STATE Count : %d\r\n", shutoffCount);
//			shutoffCount++;
//			generatePayload(data, sizeof(data));
//			SwitchToTxMode(data, sizeof(data));
//			while(!isTXDone());
//			MX_SubGHz_Phy_Process();
//			SwitchToRxMode();
//		}
//		break;
//
//	case TRANSMIT_STATE:
//	{
//		LOG_D("Entering shutoff state\r\n");
//		shutOffStartTime = HAL_GetTick();
//		shutoffCount = 0;
//		uint8_t data[64];
//		generatePayload(data, sizeof(data));
//		SwitchToTxMode(data, sizeof(data));
//		while(!isTXDone());
//		MX_SubGHz_Phy_Process();
//		SwitchToRxMode();
//		state = SHUTOFF_STATE;
//	}
//		break;
//
//	default:
//		break;
//	}
}
