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

BoardState state = BOOTUP_STATE;

static GPS gps;
TimerEvent_t SleepTimeoutTimer;
static uint32_t shutoffCount = 0;
uint32_t shutOffStartTime = 0;

struct Timestamp
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

struct GeoCoordinates
{
	double lattitude;
	double longitude;
};

struct CommPacket
{
	uint32_t ids[3];
	Timestamp timestamp;
	GeoCoordinates geoCoordinates;
};

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
	packet.timestamp.hour = gps.gps.time.hour();
	packet.timestamp.minute = gps.gps.time.minute();
	packet.timestamp.second = gps.gps.time.second();
	packet.geoCoordinates.lattitude = gps.gps.location.lat();
	packet.geoCoordinates.longitude = gps.gps.location.lng();
	memcpy(payload + strlen(MY_IDENTIFIER), &packet, sizeof(packet));
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
