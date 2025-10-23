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

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message ("Building firmware for RELAYBOARD = " STR(RELAYBOARD))


#if (RELAYBOARD == MCU_1) || (RELAYBOARD == MCU_2)
 // OK
#else
 #error "RELAYBOARD must be MCU_1 or MCU_2"
#endif

#if (RELAYBOARD == MCU_1)
#include "flash_data.h"
#endif

#if (RELAYBOARD == MCU_2)
static DAY newCalendar[15];
static bool calendarUpdated = false;
#endif

BoardState state = BOOTUP_STATE;
uint8_t HelloCommand[] = MY_IDENTIFIER "HELLO";
TimerEvent_t WaitTimer;

uint8_t data[64];
CommPacket lastMessage;
DAY days[15];
unsigned long wait_time = 0;

const DAY defaultDays[15] = {
    {1,1,'A'},{1,2,'B'},{1,3,'A'},{1,4,'A'},{1,5,'A'},
    {1,6,'A'},{1,7,'A'},{1,8,'A'},{1,9,'A'},{1,10,'A'},
    {1,11,'A'},{1,12,'A'},{1,13,'A'},{1,14,'A'},{1,15,'A'}
};

void DurationElapsedCallback(void *context)
{
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
    state = SEQUENCE_END_STATE;
}

void processData(UART_HandleTypeDef *huart)
{
    LOG_D("processData - UART RX complete\n");

#if (RELAYBOARD == MCU_1)
    LOG_D("MCU_1: Calendar received from MCU_2, storing in flash...\n");
    Flash_Multiple_Write_Calendar(days);
    LOG_D("Calendar written to flash\n");

    if (Radio.Send((uint8_t*)days, sizeof(days)) == RADIO_STATUS_OK)
        LOG_D("MCU_1: Calendar sent to Base successfully\n");
    else
        LOG_D("MCU_1: Calendar send failed\n");

    HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));

#elif (RELAYBOARD == MCU_2)
    LOG_D("MCU_2: GNSS data received from MCU_1, forwarding to Base Station...\n");
    if (Radio.Send((uint8_t*)data, sizeof(data)) == RADIO_STATUS_OK)
        LOG_D("MCU_2: GNSS coordinates sent successfully\n");
    else
        LOG_D("MCU_2: GNSS send failed\n");

    HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
#endif
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

#if (RELAYBOARD == MCU_2)
    if (strncmp(buffer, BASE_BOARD_IDENTIFIER, strlen(BASE_BOARD_IDENTIFIER)) == 0)
    {
        // Calendar packet from Base Station
        if (size >= sizeof(DAY) * 15)
        {
            memcpy(newCalendar, buffer + strlen(BASE_BOARD_IDENTIFIER), sizeof(newCalendar));
            calendarUpdated = true;
            LOG_D("MCU_2: New calendar received from Base Station\n");
        }
    }
    else if (strncmp(buffer, TX_BOARD_IDENTIFIER, strlen(TX_BOARD_IDENTIFIER)) == 0)
    {
        LOG_D("MCU_2: GNSS data received (unexpected on MCU_2)\n");
    }
#endif

#if (RELAYBOARD == MCU_1)
    if (strncmp(buffer, TX_BOARD_IDENTIFIER, identifier_length) == 0)
    {
        memcpy(&lastMessage, buffer + identifier_length, sizeof(lastMessage));
        HAL_UART_Transmit(&huart1, (uint8_t*)MY_PACKET_IDENTIFIER, strlen(MY_PACKET_IDENTIFIER), 1000);
        HAL_UART_Transmit(&huart1, (uint8_t*)(buffer + identifier_length), size, 1000);
        state = SEND_SHUTOFF_STATE;
    }
#endif
}


extern "C" void RX_Timeout_Process(void)
{
    Radio.Sleep();
    TimerSetValue(&WaitTimer, 2000);
    TimerStart(&WaitTimer);
}

void App::setup(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

#if (RELAYBOARD == MCU_1)
    TimerInit(&WaitTimer, DurationElapsedCallback);
    TimerStop(&WaitTimer);

    Radio.Send(HelloCommand, sizeof(HelloCommand));
    while(!isTXDone());
    LOG_D("Initial HELLO sent (MCU_1)\n");

    MX_SubGHz_Phy_Process();
    SwitchToRxMode();

    HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, processData);
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));

    // --- Flash Initialization ---
    DAY flashDays[15];
    Flash_Read_Multiple_Calendar(flashDays);

    bool empty = true;
    for (int i = 0; i < 15; i++) {
        if (flashDays[i].Month != 0 && flashDays[i].Day != 0) {
            empty = false;
            break;
        }
    }

    if (empty) {
        LOG_D("No calendar in flash, writing default...\n");
        Flash_Multiple_Write_Calendar((DAY*)defaultDays);
    } else {
        LOG_D("Calendar already present in flash.\n");
    }

#elif (RELAYBOARD == MCU_2)
    HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, processData);
    HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
    LOG_D("MCU_2: Ready to receive GNSS from MCU_1\n");
#endif
}

void App::loop(void)
{
    static unsigned long lastHello = 0;

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
    {
        uint8_t mergedData[sizeof(lastMessage.ids) + 7];
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
    }
    default:
        break;
    }

#if (RELAYBOARD == MCU_1)
    if (HAL_GetTick() - lastHello >= 3000) {
        lastHello = HAL_GetTick();
        LOG_D("MCU_1: periodic HELLO\n");
        SwitchToTxMode(HelloCommand, sizeof(HelloCommand));
        while(!isTXDone());
        MX_SubGHz_Phy_Process();
        SwitchToRxMode();
    }
#endif

#if (RELAYBOARD == MCU_2)
    if (calendarUpdated) {
        LOG_D("MCU_2: New calendar detected — sending to MCU_1...\n");
        if (HAL_UART_Transmit(&huart1, (uint8_t*)days, sizeof(days), 1000) == HAL_OK) {
            LOG_D("MCU_2: Calendar transmitted successfully to MCU_1\n");
            calendarUpdated = false; // clear flag after successful send
        } else {
            LOG_D("MCU_2: Calendar transmit failed, will retry\n");
        }
    }

#endif
}
