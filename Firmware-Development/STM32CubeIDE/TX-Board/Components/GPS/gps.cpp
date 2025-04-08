#include "gps.h"
#include "rtc.h"

TinyGPSPlus GPS::gps;
uint8_t GPS::data;

void GPS::rtcOperation(RtcOperation opr, RtcData_t* data)
{
    uint32_t* rtc_words = reinterpret_cast<uint32_t*>(data);

    for (int i = 0; i < RTC_BKP_WORD_COUNT; i++)
    {
        uint32_t addr = RTC_BKP_BASE_ADDR + i;

        if (opr == STORE)
        {
            HAL_RTCEx_BKUPWrite(&hrtc, addr, rtc_words[i]);
        }
        else if (opr == RETRIEVE)
        {
            rtc_words[i] = HAL_RTCEx_BKUPRead(&hrtc, addr);
        }
        else
        {
            LOG_D("Invalid RTC operation.\r\n");
            break;
        }
    }
}

void GPS::processData(UART_HandleTypeDef *huart)
{
	gps.encode(data);
	HAL_UART_Receive_IT(&huart1, &data, 1);
}

void GPS::setup(void)
{
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(400);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_SET);
	HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, GPS::processData);
	HAL_UART_Receive_IT(&huart1, &data, 1);
}

void GPS::displayInfo(void)
{
    bool updatedFlag = false;
    RtcData_t gnssData;

    if(gps.location.isUpdated() &&
       gps.date.isUpdated() &&
       gps.time.isUpdated())
    {
        updatedFlag = true;
    }

    LOG_D("Location: ");
    if (gps.location.isValid())
    {
        gnssData.latitude = gps.location.lat();
        gnssData.longitude = gps.location.lng();
        LOG_D("%.6f,%.6f", gnssData.latitude, gnssData.longitude);
    }
    else
    {
        LOG_D("INVALID");
    }

    LOG_D("  Date/Time: ");
    if (gps.date.isValid())
    {
        // uint8_t month, day;
        // uint16_t year;
        gnssData.month = gps.date.month();
        gnssData.day = gps.date.day();
        gnssData.year = gps.date.year();
        LOG_D("%02d/%02d/%04d ", gnssData.month, gnssData.day, gnssData.year);

    }
    else
    {
        LOG_D("INVALID ");
    }

    if (gps.time.isValid())
    {
        gnssData.hour = gps.time.hour();
        gnssData.minute = gps.time.minute();
        gnssData.second = gps.time.second();
        gnssData.centisecond = gps.time.centisecond();
        LOG_D("%02d:%02d:%02d.%02d\r\n",
               gnssData.hour,
               gnssData.minute,
               gnssData.second,
               gnssData.centisecond);
    }
    else
    {
        LOG_D("INVALID\r\n");
    }

    if( updatedFlag &&
        gps.time.isValid() && gps.date.isValid()
        && gps.location.isValid())
    {
        rtcOperation(STORE, &gnssData);
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

//	if (HAL_GetTick() > 5000 && gps.charsProcessed() < 10)
//	{
//	  LOG_D("No GPS detected: check wiring.\r\n");
//	  while (1)
//	  {
//		  HAL_Delay(10);
//	  }
//	}
}
