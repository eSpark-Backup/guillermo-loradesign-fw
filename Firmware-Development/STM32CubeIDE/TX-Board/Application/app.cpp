#include "app.h"
#include "main.h"
#include "usart.h"
#include "gps.h"
#include "rtc.h"
#include "subghz_phy_app.h"
#include <string.h>
#include "stm32_lpm_if.h"
#include "radio.h"

static GPS gps;

extern "C" void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\n");
}

void sleep()
{
	HAL_StatusTypeDef status;

	uint32_t wakeup_time_seconds = 10;
	uint32_t rtc_frequency = 32768; // Default RTC clock frequency (32.768 kHz)
	uint32_t wakeup_clock_divider = 16;
	uint32_t wakeup_counter = wakeup_time_seconds * (rtc_frequency / wakeup_clock_divider);
	status = HAL_RTCEx_SetWakeUpTimer_IT(
	    &hrtc,                    // RTC handle
	    wakeup_counter,           // Counter value
	    RTC_WAKEUPCLOCK_RTCCLK_DIV16, // Wakeup clock source
		1     // Do not auto-clear the wakeup flag
	);
	if (status != HAL_OK)
	{
	    // Handle error
	    Error_Handler();
	}
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	Radio.Sleep();
	PWR_EnterStopMode();
	Radio.Sleep();
}

extern "C" void receiveTimeout()
{
	sleep();
}

void App::setup(void)
{
	gps.setup();
	sleep();
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

		if(strcmp((char*) data_buffer, "HELLO") == 0)
		{
			memset(data_buffer, 0, sizeof(data_buffer));
			currentTime = HAL_GetTick();

		}
	}
	else
	{
		previousTime = currentTime;
	}
}
