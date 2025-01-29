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
#include <cstdio>

#define MY_IDENTIFIER "TX:"
#define RELAY_BOARD_IDENTIFIER "RELAY:"
#define HELLO_MESSAGE "HELLO"
#define PING_MESSAGE RELAY_BOARD_IDENTIFIER HELLO_MESSAGE

enum BoardState
{
	BOOTUP_STATE,
	SHUTOFF_STATE,
	TRANSMIT_STATE
};

BoardState state = BOOTUP_STATE;

static GPS gps;
TimerEvent_t SleepTimeoutTimer;
#define SLEEP_TIMEOUT 25000
static uint32_t shutoffCount = 0;
uint32_t shutOffStartTime = 0;

void generatePayload(uint8_t* buffer, uint32_t size)
{
	char* payload = (char*) buffer;

	int len = 0;
#if CUSTOM_BOARD_ID
	len += snprintf(payload, size, "ID: %08X, ", BOARD_ID);
#else
	uint32_t boardIds[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
	len += snprintf(payload, size, "ID: %08X%08X%08X, ", (unsigned int)boardIds[0], (unsigned int)boardIds[1], (unsigned int)boardIds[2]);
#endif
	size -= len;
	len += snprintf(payload, size, "TimeStamp: %d:%d:%d ", gps.gps.time.hour(), gps.gps.time.minute(), gps.gps.time.second());
	size -= len;
	len += snprintf(payload, size, "Lat: %0.2f, ", gps.gps.location.lat());
	size -= len;
	len += snprintf(payload, size, "Long: %0.2f ", gps.gps.location.lng());
}

void UpTimerEventCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
}

void sleep(uint32_t timeout)
{
	LOG_D("Going to sleep for %d seconds\r\n", timeout);
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
	TimerSetValue( &SleepTimeoutTimer, timeout );
	TimerStart( &SleepTimeoutTimer );
	PWR_EnterStopMode();
	PWR_ExitStopMode();
	LOG_D("Starts Receiving Data\r\n");
	Radio.Rx(5000);
}

extern "C" void receiveTimeout(void)
{
	switch(state)
	{
	case BOOTUP_STATE:
		LOG_D("No Signal Received\r\n");
		sleep(SLEEP_TIMEOUT);
		break;

	default:
		break;
	}

}

extern "C" void dataReceived(char* buffer, uint32_t size)
{
	LOG_D("Received %s \r\n", buffer);

	if(strcmp(buffer, "SHUTOFF") == 0)
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

void App::setup(void)
{

	TimerInit( &SleepTimeoutTimer, UpTimerEventCallback );
	TimerStop( &SleepTimeoutTimer );
	gps.setup();
}

void App::loop(void)
{
	gps.loop();

	uint32_t currentTime = HAL_GetTick();
	uint8_t data[64];
	switch(state)
	{
	case SHUTOFF_STATE:
		if((currentTime - shutOffStartTime) > 30000)
		{
			LOG_D("Sleeping for a day\r\n");
			state = BOOTUP_STATE;
			sleep(DAY_TIMEOUT);
		}
		else if( ( (currentTime - shutOffStartTime) / 5000U ) > shutoffCount)
		{
			LOG_D("SHUTOFF_STATE Count : %d\r\n", shutoffCount);
			shutoffCount++;
			generatePayload(data, sizeof(data));
			SwitchToTxMode(data, sizeof(data));
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
