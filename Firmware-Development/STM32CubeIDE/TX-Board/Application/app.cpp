#include "app.h"
#include "main.h"
#include "usart.h"
#include "gps.h"
#include "rtc.h"
#include "subghz_phy_app.h"
#include <string.h>
#include "stm32_lpm_if.h"
#include "radio.h"
#include "timer.h"
#include <cstdio>

enum BoardState
{
	BOOTUP_STATE,
	SHUTOFF_STATE
};

BoardState state = BOOTUP_STATE;

static GPS gps;
TimerEvent_t SleepTimeoutTimer;
#define SLEEP_TIMEOUT 25000
static int shutoffCount =0;

void generatePayload(uint8_t* buffer, uint32_t size)
{
	snprintf((char*)buffer, size, "Lat: %0.2f, Long:%0.2f, ID:%d",gps.gps.location.lat(), gps.gps.location.lng(), BOARD_ID);
}

void UpTimerEventCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
}

void sleep()
{
	LOG_D("Going to sleep for %d seconds\r\n", SLEEP_TIMEOUT);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	Radio.Sleep();
	TimerSetValue( &SleepTimeoutTimer, SLEEP_TIMEOUT );
	TimerStart( &SleepTimeoutTimer );
	PWR_EnterStopMode();
	PWR_ExitStopMode();
	LOG_D("Starts Receiving Data");
	Radio.Rx(5000);
}

extern "C" void receiveTimeout(void)
{
	uint8_t data[64];
	switch(state)
	{
	case BOOTUP_STATE:
		LOG_D("No Signal Received\r\n");
		sleep();
		break;

	case SHUTOFF_STATE:
		shutoffCount++;
		generatePayload(data, sizeof(data));
		SwitchToTxMode(data, sizeof(data));
		SwitchToRxMode();
		if(shutoffCount == 6)
		{
			LOG_D("Sleeping for a day\r\n");
			TimerSetValue(&SleepTimeoutTimer, 60 * 1000);
			TimerStart(&SleepTimeoutTimer);
		}
		break;
	}

}

extern "C" void dataReceived(char* buffer, uint32_t size)
{
	if(strcmp(buffer, "SHUTOFF") == 0)
	{
		LOG_D("Sleeping for a day\r\n");
		TimerSetValue(&SleepTimeoutTimer, 60 * 1000);
		TimerStart(&SleepTimeoutTimer);
	}
	else if(strcmp(buffer, "HELLO") == 0)
	{
		state = SHUTOFF_STATE;
		memset(buffer, 0, size);
		uint8_t data[64];
		generatePayload(data, sizeof(data));
		SwitchToTxMode(data, sizeof(data));
		SwitchToRxMode();
	}
}

void App::setup(void)
{

	TimerInit( &SleepTimeoutTimer, UpTimerEventCallback );
	TimerStop( &SleepTimeoutTimer );
	gps.setup();
//	sleep();
}

void App::loop(void)
{
	gps.loop();
//	HAL_Delay(5000);
//	SwitchToRxMode();
	static uint32_t previousTime;
	uint32_t currentTime = HAL_GetTick();
	uint32_t duration = currentTime - previousTime;
	if (duration < 5000)
	{


	}
	else
	{
		previousTime = currentTime;
	}
}
