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
uint8_t HelloCommand[] = MY_IDENTIFIER"HELLO";
TimerEvent_t WaitTimer;

uint8_t data[64];
CommPacket lastMessage;
DAY days[15];
unsigned long wait_time;

void DurationElapsedCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
    state = SEQUENCE_END_STATE;
}

void processData(UART_HandleTypeDef *huart)
{
	LOG_D("processData\n");
	if((Radio.Send((uint8_t*)days, sizeof(days)) == RADIO_STATUS_OK)){
		LOG_D("Radio Calendar Data Sent Successfully\n");
	}
	else{
		LOG_D("Radio Calendar Data Not Sent Successfully!!\n");
	}

	 for(int i = 0; i < 15; i++){
	    LOG_D("Data Received: %d %d %c\n", days[i].Month, days[i].Day, days[i].Schedule);
	 }

	MX_USART1_UART_Init();
	HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));
	//LOG_D("Data Received: %d", data);
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
		memcpy(&lastMessage, buffer+ identifier_length, sizeof(lastMessage));
		HAL_UART_Transmit(&huart1, (uint8_t*)MY_PACKET_IDENTIFIER, strlen(MY_PACKET_IDENTIFIER), 1000);
		HAL_UART_Transmit(&huart1, (uint8_t*)(buffer + identifier_length), size, 1000);
		state = SEND_SHUTOFF_STATE;
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
	LOG_D("Radio Send Hello Successfully\n");
	MX_SubGHz_Phy_Process();
	SwitchToRxMode();
	HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, processData);
	HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));
#elif (RELAYBOARD == MCU_2)
	HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, processData);
	HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
#endif
}


void App::loop(void)
{
	switch(state)
	{
	case SEQUENCE_END_STATE:
		SwitchToTxMode(HelloCommand, sizeof(HelloCommand));
		while(!isTXDone());
		MX_SubGHz_Phy_Process();
		SwitchToRxMode();
		state = BOOTUP_STATE;
		break;
	case SEND_SHUTOFF_STATE:
		uint8_t mergedData[sizeof(lastMessage.ids) + 7];  // 3 * uint32_t (12 bytes) + "SHUTOFF" (7 bytes) = 19 bytes
		memcpy(mergedData, lastMessage.ids, sizeof(lastMessage.ids));
		memcpy(mergedData + sizeof(lastMessage.ids), "SHUTOFF", 7);
		SwitchToTxMode(mergedData, sizeof(mergedData));
		while(!isTXDone());
		MX_SubGHz_Phy_Process();
		SwitchToRxMode();
		static int count;
		if(2 == count++)
		{
			count = 0;
			state = BOOTUP_STATE;
		}
		break;
	default:
		break;
	}
#if (RELAYBOARD == MCU2)
	if(HAL_GetTick() - wait_time >= 5000){
		char data[] = "Hello\r\n";
		if((HAL_UART_Transmit(&huart1, (uint8_t*)days, sizeof(days),1000)) == HAL_OK){
			LOG_D("Calendar Data Transmitted Successfully\n");
		}
		else{
			LOG_D("Calendar Data Not Transmitted Successfully\n");
		}
	}
#endif
}
