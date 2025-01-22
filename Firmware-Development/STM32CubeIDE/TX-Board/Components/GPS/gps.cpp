#include "gps.h"
#include "usart.h"
#include "TinyGPS++.h"

static TinyGPSPlus gps;
uint8_t temp;

extern "C" void GPSInterruptHandler(UART_HandleTypeDef *huart)
{
	gps.encode(temp);
	HAL_UART_Receive_IT(&huart1, &temp, 1);
}

void GPS::setup(void)
{
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(400);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_SET);
	HAL_UART_Receive_IT(&huart1, &temp, 1);
}

void GPS::displayInfo(void)
{
    LOG_D("Location: ");
    if (gps.location.isValid())
    {
        LOG_D("%.6f,%.6f", gps.location.lat(), gps.location.lng());
    }
    else
    {
        LOG_D("INVALID");
    }

    LOG_D("  Date/Time: ");
    if (gps.date.isValid())
    {
        uint8_t month, day;
        uint16_t year;
        month = gps.date.month();
        day = gps.date.day();
        year = gps.date.year();
        year = gps.date.year();
        LOG_D("%02d/%02d/%04d ", gps.date.month(), gps.date.day(), gps.date.year());

    }
    else
    {
        LOG_D("INVALID ");
    }

    if (gps.time.isValid())
    {
        LOG_D("%02d:%02d:%02d.%02d\r\n",
               gps.time.hour(),
               gps.time.minute(),
               gps.time.second(),
               gps.time.centisecond());
    }
    else
    {
        LOG_D("INVALID\r\n");
    }
}

void GPS::loop(void)
{
	static uint32_t lastGnssDisplay = 0;
	if ((HAL_GetTick() - lastGnssDisplay) > 500)
	{
	 lastGnssDisplay = HAL_GetTick();
	 displayInfo();
	}

	if (HAL_GetTick() > 5000 && gps.charsProcessed() < 10)
	{
	  LOG_D("No GPS detected: check wiring.\r\n");
	  while (1)
	  {
		  HAL_Delay(10);
	  }
	}
}
