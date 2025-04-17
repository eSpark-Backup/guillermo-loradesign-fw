#include "gps.h"
#include "rtc.h"

TinyGPSPlus GPS::gps;
uint8_t GPS::data;

#define GPS_RX_BUFFER_SIZE 1024

uint8_t gpsRxBuffer[GPS_RX_BUFFER_SIZE];
volatile uint16_t gpsLastPos = 0;

uint32_t GPS::lastGnssDisplay = 0;
uint32_t GPS::lastStreamProcess = 0;
uint64_t GPS::gnssStreamTimestamp = 0;

void GPS::getGnssData(RtcData_t* data)
{
    uint32_t* rtc_words = reinterpret_cast<uint32_t*>(data);
    
    // Loop to retrieve data from the RTC registers
    for (int i = 0; i < RTC_BKP_WORD_COUNT; i++)
    {
        uint32_t addr = RTC_BKP_BASE_ADDR + i;
        rtcOperation(RETRIEVE, addr, &rtc_words[i]);
    }
}

void GPS::setGnssData(RtcData_t* data)
{
    uint32_t* rtc_words = reinterpret_cast<uint32_t*>(data);
    
    // Loop to store data to the RTC registers
    for (int i = 0; i < RTC_BKP_WORD_COUNT; i++)
    {
        uint32_t addr = RTC_BKP_BASE_ADDR + i;
        rtcOperation(STORE, addr, &rtc_words[i]);
    }
}

void GPS::rtcOperation(RtcOperation opr, uint32_t adr, uint32_t* data)
{
    if (opr == STORE)
    {
        HAL_RTCEx_BKUPWrite(&hrtc, adr, *data);  // Store single value at the given address
    }
    else if (opr == RETRIEVE)
    {
        *data = HAL_RTCEx_BKUPRead(&hrtc, adr);  // Retrieve value from the given address
    }
    else
    {
        LOG_D("Invalid RTC operation.\r\n");
    }
}

void GPS::processGpsStream(void)
{
    uint16_t currentPos = GPS_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx);

    if (currentPos != gpsLastPos)
    {
        if (currentPos > gpsLastPos)
        {
            // Simple case: new data is ahead
            processData(&gpsRxBuffer[gpsLastPos], currentPos - gpsLastPos);
        }
        else
        {
            // Wrapped around buffer
            processData(&gpsRxBuffer[gpsLastPos], GPS_RX_BUFFER_SIZE - gpsLastPos);
            processData(&gpsRxBuffer[0], currentPos);
        }

        gpsLastPos = currentPos;
        gnssStreamTimestamp = HAL_GetTick();
    }
    else if(HAL_GetTick() - gnssStreamTimestamp > UART_DMA_REINTI_TIMEOUT)
    {
        LOG_D("GNSS Stream stuck. ReInitializing UART DMA.\r\n");
        HAL_UART_AbortReceive(&huart1);  // Stop current DMA

        // Restart DMA in circular mode
        HAL_UART_Receive_DMA(&huart1, gpsRxBuffer, GPS_RX_BUFFER_SIZE); 

        // OR: Use idle mode for better sentence-level parsing
        // HAL_UARTEx_ReceiveToIdle_DMA(&huartX, dmaBuffer, DMA_BUFFER_SIZE);
        // __HAL_DMA_DISABLE_IT(huartX.hdmarx, DMA_IT_HT | DMA_IT_TC);

        gnssStreamTimestamp = HAL_GetTick(); // Reset timestamp
    }
}

void GPS::processData(uint8_t *data, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
    {
		gps.encode(data[i]);
		LOG_GNSS_STREAM("%c", data[i]);
    }
}

void GPS::setup(void)
{
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(400);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_SET);

	// HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, GPS::processData);
	// HAL_UART_Receive_IT(&huart1, &data, 1);

	// SET_BIT(huart1.hdmarx->Instance->CCR, DMA_CCR_CIRC);
    HAL_UART_Receive_DMA(&huart1, gpsRxBuffer, GPS_RX_BUFFER_SIZE);

    INIT_TIMESTAMPS_GNSS;
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
        && gps.location.isValid() )
    {
        getGnssData(&gnssData);
        uint32_t val = GNSS_LOCK_FLAG;
        rtcOperation(STORE, GNSS_LOCK_FLAG_ADDR, &val);
    }
}

void GPS::loop(void)
{

	if ((HAL_GetTick() - lastGnssDisplay) > 500)
	{
		lastGnssDisplay = HAL_GetTick();
		displayInfo();
	}
	if ((HAL_GetTick() - lastStreamProcess) > 50)
	{
		lastStreamProcess = HAL_GetTick();
		processGpsStream();
	}

	// if (HAL_GetTick() > 5000 && gps.charsProcessed() < 10)
	// {
	//   LOG_D("No GPS detected: check wiring.\r\n");
	//   while (1)
	//   {
	// 	  HAL_Delay(10);
	//   }
	// }
}
