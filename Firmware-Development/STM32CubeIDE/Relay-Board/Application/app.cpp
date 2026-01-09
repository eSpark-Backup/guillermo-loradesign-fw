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

#if (RELAYBOARD == MCU_1) || (RELAYBOARD == MCU_2)
// OK
#else
 #error "RELAYBOARD must be MCU_1 or MCU_2"
#endif

#if (RELAYBOARD == MCU_1)
#include "flash_data.h"
static bool GNSSUpdated = false;
#endif

#if (RELAYBOARD == MCU_2)
static DAY newCalendar[15];
static bool calendarUpdated = false;
#endif
/* ===================== STARTUP UPDATE MODE ===================== */

#define UPDATE_WINDOW_MS   (60UL * 1000UL)

static bool updateMode = true;
static uint32_t bootTick = 0;

volatile uint32_t led1OffTick = 0;
volatile uint32_t led2OffTick = 0;
/* =====================  TIMING CONFIG ===================== */
#define HELLO_TX_INTERVAL_MS      3000UL
#define RX_LISTEN_DURATION_MS     2000UL
#define SLEEP_DURATION_MS         1000UL

/* ===================== GLOBALS ===================== */

BoardState state = BOOTUP_STATE;
uint32_t stateTimestamp = 0;

uint8_t HelloCommand[] = MY_IDENTIFIER "HELLO";


DAY days[15];
CommPacket lastMessage;

uint8_t data[64];



/* ===================== UART RX CALLBACK ===================== */

void processData(UART_HandleTypeDef *huart) {
	LOG_D("processData - UART RX complete\n");
#if (RELAYBOARD == MCU_1)
  LOG_D("MCU_1: Calendar received from MCU_2, storing in flash...\n");
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
	    led1OffTick = HAL_GetTick() + 300;
  Flash_Multiple_Write_Calendar(days);
  LOG_D("Calendar written to flash\n");
  if (Radio.Send((uint8_t*)days, sizeof(days)) == RADIO_STATUS_OK)
  	LOG_D("MCU_1: Calendar sent to Base successfully\n");
  else LOG_D("MCU_1: Calendar send failed\n");
  HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));
#elif (RELAYBOARD == MCU_2)
  LOG_D("MCU_2: GNSS data received from MCU_1, forwarding to Base Station...\n");
   if (Radio.Send((uint8_t*)data, sizeof(data)) == RADIO_STATUS_OK)
   {
  	// Led1 is led3 of MCU2 on solder mask

  	LOG_D("MCU_2: GNSS coordinates sent successfully\n");
  	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  	    led1OffTick = HAL_GetTick() + 300;
   } else LOG_D("MCU_2: GNSS send failed\n");
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
        led2OffTick = HAL_GetTick() + 300;   // 300 ms pulse
    }
#endif

#if (RELAYBOARD == MCU_1)
    if (strncmp(buffer, TX_BOARD_IDENTIFIER,
                strlen(TX_BOARD_IDENTIFIER)) == 0)
    {
        memcpy(&lastMessage,
               buffer + strlen(TX_BOARD_IDENTIFIER),
               sizeof(lastMessage));
        GNSSUpdated = true;
        state = SEND_SHUTOFF_STATE;
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        led2OffTick = HAL_GetTick() + 300;
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

    /* -------- START UPDATE MODE -------- */
    bootTick = HAL_GetTick();
    updateMode = true;

#if (RELAYBOARD == MCU_2)
    Radio.Rx(0);   // listen for Base Station calendar immediately
#endif

    /* DO NOT start HELLO yet */
    state = BOOTUP_STATE;
    stateTimestamp = HAL_GetTick();
}

/* ===================== MAIN LOOP ===================== */

void App::loop(void)
{
#if (RELAYBOARD == MCU_2)
    /* ============ UPDATE MODE (first 60 seconds) ============ */
    if (updateMode)
    {
        /* Exit update mode after 1 minute */
        if ((HAL_GetTick() - bootTick) >= UPDATE_WINDOW_MS)
        {
            updateMode = false;
            state = SEQUENCE_END_STATE;   // start HELLO now
            stateTimestamp = HAL_GetTick();
        }

        /* During update mode:RX calendar only*/
        return;
    }
#endif
    switch (state)
    {
    /* ================= TX HELLO ================= */
    case SEQUENCE_END_STATE:
        SwitchToTxMode(HelloCommand, sizeof(HelloCommand));
        while (!isTXDone());
        SwitchToRxMode();

        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        led1OffTick = HAL_GetTick() + 300;   // 300 ms pulse

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
        uint8_t mergedData[sizeof(lastMessage.ids) + 7];
        memcpy(mergedData, lastMessage.ids, sizeof(lastMessage.ids));
        memcpy(mergedData + sizeof(lastMessage.ids), "SHUTOFF", 7);
        SwitchToTxMode(mergedData, sizeof(mergedData));
        while(!isTXDone());
        MX_SubGHz_Phy_Process();
        SwitchToRxMode();
        static int count;
        if(2 == count++)
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
        led1OffTick = HAL_GetTick() + 300;
        calendarUpdated = false;
    }
#endif

#if (RELAYBOARD == MCU_1)
    if (GNSSUpdated)
    {
        HAL_UART_Transmit(&huart1,
                          (uint8_t*)&lastMessage,
                          sizeof(lastMessage),
                          1000);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        led1OffTick = HAL_GetTick() + 300;
        GNSSUpdated = false;
    }
#endif

    if (led2OffTick && HAL_GetTick() > led2OffTick)
    {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        led2OffTick = 0;
    }

    if (led1OffTick && HAL_GetTick() > led1OffTick)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        led1OffTick = 0;
    }


}
