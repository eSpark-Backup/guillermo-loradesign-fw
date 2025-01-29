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

#define TX_BOARD_IDENTIFIER "TX:"
#define MY_IDENTIFIER "RELAY:"

#define MCU_1 1
#define MCU_2 2

#define RELAYBOARD MCU_1

uint8_t HelloCommand[] = MY_IDENTIFIER"HELLO";
uint8_t data[64];
TimerEvent_t WaitTimer;
bool dur_flag = 0;

void DurationElapsedCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
    dur_flag = 1;

}

void processData(UART_HandleTypeDef *huart)
{
	Radio.Send(data, sizeof(data));
	MX_USART1_UART_Init();
	HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
}

void sleep(uint32_t timeout)
{
	LOG_D("Going to sleep for %d seconds\r\n", timeout);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	Radio.Sleep();
	PWR_EnterStopMode();
	PWR_ExitStopMode();
	LOG_D("Starts Receiving Data\r\n");
	Radio.Rx(5000);
}

extern "C" void RX_Done_Process(char* buffer, uint32_t size)
{
	LOG_D("Received %s \r\n", buffer);
	uint8_t identifier_length = strlen(TX_BOARD_IDENTIFIER);
	if (strncmp(buffer, TX_BOARD_IDENTIFIER, identifier_length) == 0)
	{
		HAL_UART_Transmit(&huart1, (uint8_t*)MY_IDENTIFIER, strlen(MY_IDENTIFIER), 1000);
		HAL_UART_Transmit(&huart1, (uint8_t*)(buffer + identifier_length), size, 1000);
	}
}

extern "C" void RX_Timeout_Process(void)
{
	Radio.Sleep();
	TimerSetValue( &WaitTimer, 2000 );
	TimerStart( &WaitTimer );
}

void App::setup(void)
{
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
#if (RELAYBOARD == MCU_1)
	TimerInit( &WaitTimer, DurationElapsedCallback );
	TimerStop( &WaitTimer );
	Radio.Send(HelloCommand, sizeof(HelloCommand));
	while(!isTXDone());
	MX_SubGHz_Phy_Process();
	SwitchToRxMode();
#elif (RELAYBOARD == MCU_2)
	HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, processData);
	HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
#endif
}

void App::loop(void)
{
	if(dur_flag)
	{
		SwitchToTxMode(HelloCommand, sizeof(HelloCommand));
		while(!isTXDone());
		MX_SubGHz_Phy_Process();
		SwitchToRxMode();
		dur_flag =0;
	}
}
