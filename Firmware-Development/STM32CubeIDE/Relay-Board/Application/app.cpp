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

// If MCU_1 needs to persist calendar:
#if (RELAYBOARD == MCU_1)
#include "flash_data.h"   // Flash_Multiple_Write / Flash_Read_Multiple
#endif

BoardState state = BOOTUP_STATE;
uint8_t HelloCommand[] = MY_IDENTIFIER "HELLO";
TimerEvent_t WaitTimer;

uint8_t data[64];
CommPacket lastMessage;
DAY days[15];                // calendar buffer
unsigned long wait_time = 0;

void DurationElapsedCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
    state = SEQUENCE_END_STATE;
}

/**
 * processData - called on UART RX complete
 *
 * Behaviour:
 *  - MCU_1: receives calendar array from MCU_2 via UART -> store in flash and send calendar over LoRa to TX board
 *  - MCU_2: this callback would be used if MCU_2 receives something from MCU_1; keep minimal behavior
 */
void processData(UART_HandleTypeDef *huart)
{
    LOG_D("processData - UART RX complete\n");

#if (RELAYBOARD == MCU_1)
    // We assume 'days' buffer has been filled by HAL UART DMA receive into (uint8_t*)days.
    // Save to flash and transmit calendar to TX board.
    LOG_D("MCU_1: Calendar received from MCU_2 via UART. Storing & forwarding...\n");

    // Debug print received calendar
    for (int i = 0; i < 15; i++) {
        LOG_D("  rcvd day[%d] = %02d-%02d %c\n", i, days[i].Month, days[i].Day, days[i].Schedule);
    }

    // Persist calendar to Flash (minimal & synchronous)
    FlashData_t dummy; // we don't need to modify GNSS snapshot here; flash function expects both args
    memset(&dummy, 0, sizeof(dummy));
    Flash_Multiple_Write(&dummy, days);
    LOG_D("Calendar written to flash\n");

    // Try to send calendar to TX board over radio
    if (Radio.Send((uint8_t*)days, sizeof(days)) == RADIO_STATUS_OK) {
        LOG_D("Radio Calendar Data Sent Successfully\n");
    } else {
        LOG_D("Radio Calendar Data Not Sent Successfully!!\n");
    }

    // Re-arm UART DMA for next calendar reception
    MX_USART1_UART_Init();
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));

#elif (RELAYBOARD == MCU_2)
    // MCU_2 minimal behavior: we treat incoming UART on MCU_2 as something else (keep existing)
    LOG_D("MCU_2: UART RX complete - forwarding or processing as required\n");
    // In your earlier code MCU_2 used 'data' buffer; preserve that behavior if needed.
    MX_USART1_UART_Init();
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
    if (strncmp(buffer, TX_BOARD_IDENTIFIER, identifier_length) == 0)
    {
        memcpy(&lastMessage, buffer + identifier_length, sizeof(lastMessage));
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
    // MCU_1: send first HELLO & prepare to receive calendar from MCU_2 via UART
    TimerInit( &WaitTimer, DurationElapsedCallback );
    TimerStop( &WaitTimer );

    // Start by sending an initial HELLO (will also be sent periodically in loop)
    Radio.Send(HelloCommand, sizeof(HelloCommand));
    while(!isTXDone());
    LOG_D("Initial HELLO sent (MCU_1)\n");
    MX_SubGHz_Phy_Process();
    SwitchToRxMode();

    // Register UART callback to accept calendar from MCU_2
    HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, processData);
    MX_USART1_UART_Init();
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)days, sizeof(days));

    // If you have a default calendar, ensure flash contains something
    // (optional: check if flash empty and write default)
    // Flash_Read or your flash-check can be added here if needed.

#elif (RELAYBOARD == MCU_2)
    // MCU_2: register UART callback for GNSS/commands from MCU_1, prepare DMA
    HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, processData);
    MX_USART1_UART_Init();
    HAL_UART_Receive_DMA(&huart1, data, sizeof(data));
#endif
}

void App::loop(void)
{
    static unsigned long lastHello = 0;

    // Minimal state machine handling (unchanged)
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
    }
    default:
        break;
    }

#if (RELAYBOARD == MCU_1)
    // MCU_1: send HELLO every 3 seconds
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
    // MCU_2: when it needs to send calendar to MCU_1, wait a short time (3s) to avoid collisions
    // This block triggers a periodic send of local calendar to MCU_1 every 5 seconds (example)
    if (HAL_GetTick() - wait_time >= 5000) {
        wait_time = HAL_GetTick();

        // small protection: delay slightly so MCU_1 initial HELLOs are not collided
        HAL_Delay(3000);

        // Send calendar via UART to MCU_1
        if ((HAL_UART_Transmit(&huart1, (uint8_t*)days, sizeof(days), 1000)) == HAL_OK) {
            LOG_D("MCU_2: Calendar Data Transmitted to MCU_1 Successfully\n");
        } else {
            LOG_D("MCU_2: Calendar Data NOT Transmitted Successfully\n");
        }
    }
#endif
}
