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

void Print_CommPacket_From_Buffer(char* buffer)
{
    const uint8_t identifier_length = strlen(MY_PACKET_IDENTIFIER);
    CommPacket* pkt = (CommPacket*)(buffer + identifier_length);

    LOG_D("CommPacket contents:\n");
    LOG_D("  ids[0]: 0x%08X\n", pkt->ids[0]);
    LOG_D("  ids[1]: 0x%08X\n", pkt->ids[1]);
    LOG_D("  ids[2]: 0x%08X\n", pkt->ids[2]);

    LOG_D("  Timestamp:\n");
    LOG_D("    Hour  : %u\n", pkt->timestamp.hour);
    LOG_D("    Minute: %u\n", pkt->timestamp.minute);
    LOG_D("    Second: %u\n", pkt->timestamp.second);

    // Print doubles manually
    int32_t lat_int = (int32_t)(pkt->geoCoordinates.lattitude);
    int32_t lat_frac = (int32_t)((pkt->geoCoordinates.lattitude - lat_int) * 1000000);

    int32_t lon_int = (int32_t)(pkt->geoCoordinates.longitude);
    int32_t lon_frac = (int32_t)((pkt->geoCoordinates.longitude - lon_int) * 1000000);

    LOG_D("  GeoCoordinates:\n");
    LOG_D("    Latitude : %ld.%06ld\n", lat_int, abs(lat_frac));
    LOG_D("    Longitude: %ld.%06ld\n", lon_int, abs(lon_frac));
}

extern "C" void RX_Done_Process(char* buffer, uint32_t size)
{
	LOG_D("Received %s \r\n", buffer);
	Print_CommPacket_From_Buffer(buffer);
	uint8_t identifier_length = strlen(MY_PACKET_IDENTIFIER);

	if (strncmp(buffer, MY_PACKET_IDENTIFIER, identifier_length) == 0)
	{
		uint8_t packet[UART_PACKET_SIZE];
		packet[0] = UART_PACKET_HEADER;
		memcpy(&packet[1], (uint8_t*)buffer + identifier_length, COMM_PAYLOAD_SIZE);
		packet[UART_PACKET_SIZE - 1] = UART_PACKET_FOOTER;
		HAL_UART_Transmit(&huart1, packet, UART_PACKET_SIZE, 1000);
	}
}

void App::setup(void)
{
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
}

void App::loop(void)
{
#if IS_TEST_MODE
	static uint32_t cnt = 0;

	CommPacket pkt = {
		.ids = {++cnt, 0x00, 0x00},
		.timestamp = {12, 30, 45},
		.geoCoordinates = {
			// Latitude oscillates between -90 and +90
			-90.0 + fmod(cnt * 1.3, 180.0),
			// Longitude wraps around within -180 to +180
			fmod((cnt * 2.7) + 180.0, 360.0) - 180.0
		}
	};

	// Serialize: identifier + binary data of CommPacket
	const char* identifier = MY_PACKET_IDENTIFIER;
	const uint8_t id_len = strlen(identifier);
	const uint32_t total_size = id_len + sizeof(CommPacket);

	// Create full buffer
	uint8_t buffer[128];  // large enough
	memcpy(buffer, identifier, id_len);
	memcpy(buffer + id_len, &pkt, sizeof(CommPacket));

	// Call the function (simulating a received packet)
	RX_Done_Process((char*)buffer, total_size);

	HAL_Delay(1000);
#endif
}
