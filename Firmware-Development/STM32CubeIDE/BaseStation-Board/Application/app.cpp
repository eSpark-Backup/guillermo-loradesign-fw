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
	uint8_t identifier_length = strlen(MY_PACKET_IDENTIFIER);

	if (strncmp(buffer, MY_PACKET_IDENTIFIER, identifier_length) == 0)
	{
		uint8_t *packet = (uint8_t*)buffer + identifier_length;
		HAL_UART_Transmit(&huart1, packet, sizeof(CommPacket), 1000);
	}
}

void App::setup(void)
{
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
}

void App::loop(void)
{

}
