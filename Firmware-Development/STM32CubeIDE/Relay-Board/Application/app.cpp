#include "app.h"
#include "main.h"
#include "usart.h"
#include "subghz_phy_app.h"
#include "app_subghz_phy.h"
#include "radio.h"
#include "timer.h"
#include "stm32_lpm_if.h"
#include <string.h>

/* =====================  TIMING CONFIG ===================== */
#define HELLO_TX_INTERVAL_MS      3000UL
#define RX_LISTEN_DURATION_MS     5000UL
#define SLEEP_DURATION_MS         1000UL

/* ===================== GLOBALS ===================== */

BoardState state = BOOTUP_STATE;
uint32_t stateTimestamp = 0;

uint8_t HelloCommand[] = MY_IDENTIFIER "HELLO";

#if (RELAYBOARD == MCU_1)
DAY days[15];
CommPacket lastMessage;
#else
uint8_t data[64];
static DAY newCalendar[15];
static bool calendarUpdated = false;
#endif

/* ===================== UART RX CALLBACK ===================== */

void processData(UART_HandleTypeDef *huart)
{
#if (RELAYBOARD == MCU_1)
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));
#else
    HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
#endif
}

/* ===================== RADIO RX CALLBACK ===================== */

extern "C" void RX_Done_Process(char* buffer, uint32_t size)
{
#if (RELAYBOARD == MCU_2)
    if (strncmp(buffer, BASE_BOARD_IDENTIFIER,
                strlen(BASE_BOARD_IDENTIFIER)) == 0)
    {
        memcpy(newCalendar,
               buffer + strlen(BASE_BOARD_IDENTIFIER),
               sizeof(newCalendar));
        calendarUpdated = true;
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    }
#endif

#if (RELAYBOARD == MCU_1)
    if (strncmp(buffer, TX_BOARD_IDENTIFIER,
                strlen(TX_BOARD_IDENTIFIER)) == 0)
    {
        memcpy(&lastMessage,
               buffer + strlen(TX_BOARD_IDENTIFIER),
               sizeof(lastMessage));
        state = SEND_SHUTOFF_STATE;
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    }
#endif
}

/* ===================== RADIO RX TIMEOUT ===================== */

extern "C" void RX_Timeout_Process(void)
{
    Radio.Sleep();
    stateTimestamp = HAL_GetTick();
}

/* ===================== SETUP ===================== */

void App::setup(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

    HAL_UART_RegisterCallback(&huart1,
                              HAL_UART_RX_COMPLETE_CB_ID,
                              processData);

#if (RELAYBOARD == MCU_1)
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));
#else
    HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
#endif

    state = SEQUENCE_END_STATE;   // First HELLO
    stateTimestamp = HAL_GetTick();
}

/* ===================== MAIN LOOP ===================== */

void App::loop(void)
{
    switch (state)
    {
    /* ================= TX HELLO ================= */
    case SEQUENCE_END_STATE:
        SwitchToTxMode(HelloCommand, sizeof(HelloCommand));
        while (!isTXDone());

        MX_SubGHz_Phy_Process();
        SwitchToRxMode();
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        state = BOOTUP_STATE;
        stateTimestamp = HAL_GetTick();
        break;

    /* ================= RX LISTEN ================= */
    case BOOTUP_STATE:
        if (HAL_GetTick() - stateTimestamp >= RX_LISTEN_DURATION_MS)
        {
            Radio.Sleep();
            PWR_EnterStopMode();
            PWR_ExitStopMode();
            stateTimestamp = HAL_GetTick();
        }

        if (HAL_GetTick() - stateTimestamp >= HELLO_TX_INTERVAL_MS)
        {
            state = SEQUENCE_END_STATE;
        }
        break;

    /* ================= EVENT TX ================= */
    case SEND_SHUTOFF_STATE:
        // keep your existing SHUTOFF TX logic here
        state = BOOTUP_STATE;
        stateTimestamp = HAL_GetTick();
        break;

    default:
        state = BOOTUP_STATE;
        break;
    }

#if (RELAYBOARD == MCU_2)
    if (calendarUpdated)
    {
        HAL_UART_Transmit(&huart1,
                          (uint8_t*)newCalendar,
                          sizeof(newCalendar),
                          1000);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        calendarUpdated = false;
    }
#endif
}
