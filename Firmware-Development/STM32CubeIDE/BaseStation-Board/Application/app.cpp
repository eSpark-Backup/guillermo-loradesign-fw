#include "app.h"
#include "main.h"
#include "usart.h"
#include "subghz_phy_app.h"
#include "app_subghz_phy.h"
#include "radio.h"
#include "timer.h"
#include "stm32_lpm_if.h"
#include <string.h>
#include <cstdio>

// -------------------- Buffers & Flags --------------------
uint8_t uart_rx_buffer[sizeof(DAY) * 15];
DAY calendar[15];
bool newCalendarFromNRF = false;

// -------------------- Helper to print GNSS packets --------------------
void Print_CommPacket_From_Buffer(char* buffer)
{
    const uint8_t identifier_length = strlen(MY_PACKET_IDENTIFIER);
    CommPacket* pkt = (CommPacket*)(buffer + identifier_length);

    LOG_D("CommPacket: IDs=%08X %08X %08X\n", pkt->ids[0], pkt->ids[1], pkt->ids[2]);
    LOG_D("  Time: %02u:%02u:%02u\n", pkt->timestamp.hour, pkt->timestamp.minute, pkt->timestamp.second);
    LOG_D("  Lat: %.6f, Lon: %.6f, Alt: %.2f\n",
          pkt->geoCoordinates.lattitude,
          pkt->geoCoordinates.longitude,
          pkt->geoCoordinates.altitude);
}

// -------------------- LoRa RX (from Relay Board MCU2) --------------------
extern "C" void RX_Done_Process(char* buffer, uint32_t size)
{
    if (strncmp(buffer, MY_PACKET_IDENTIFIER, strlen(MY_PACKET_IDENTIFIER)) == 0)
    {
        LOG_D("GNSS data received from MCU2\n");
        Print_CommPacket_From_Buffer(buffer);

        // Forward to NRF MCU over UART
        uint8_t packet[UART_PACKET_SIZE];
        packet[0] = UART_PACKET_HEADER;
        memcpy(&packet[1], buffer + strlen(MY_PACKET_IDENTIFIER), COMM_PAYLOAD_SIZE);
        packet[UART_PACKET_SIZE - 1] = UART_PACKET_FOOTER;

        if (HAL_UART_Transmit(&huart1, packet, UART_PACKET_SIZE, 1000) == HAL_OK)
            LOG_D("GNSS data forwarded to NRF MCU\n");
        else
            LOG_D("UART TX to NRF failed\n");
    }
    else
    {
        LOG_D("Unknown LoRa message received: %s\n", buffer);
    }

    // Re-enable RX mode
    Radio.Rx(0);
}

// -------------------- UART RX Callback (from NRF) --------------------
void Base_UART_RxCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        memcpy(calendar, uart_rx_buffer, sizeof(calendar));
        newCalendarFromNRF = true;
        LOG_D("BaseStation: New calendar received from NRF MCU");

        // Re-arm UART RX
        HAL_UART_Receive_IT(&huart1, uart_rx_buffer, sizeof(calendar));
    }
}

// -------------------- Setup --------------------
void App::setup(void)
{
    LOG_D("Base Station STM started\n");

    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

    // Register our UART receive callback
    HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, Base_UART_RxCallback);

    // Start UART receive for NRF calendar data
    HAL_UART_Receive_IT(&huart1, uart_rx_buffer, sizeof(calendar));

    // Start continuous LoRa listening
    Radio.Rx(0);
}

// -------------------- Loop --------------------
void App::loop(void)
{
    // If new calendar was received from NRF MCU
    if (newCalendarFromNRF)
    {
        newCalendarFromNRF = false;
        LOG_D("Sending updated calendar to Relay Board (MCU2)\n");

        if (Radio.Send((uint8_t*)calendar, sizeof(calendar)) == RADIO_STATUS_OK)
            LOG_D("Calendar sent successfully via LoRa\n");
        else
            LOG_D("Calendar send failed\n");

        // After sending, go back to RX mode
        Radio.Rx(0);
    }

    // Continuously stay in RX to receive GNSS packets
}
