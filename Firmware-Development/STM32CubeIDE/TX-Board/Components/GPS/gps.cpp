#include "gps.h"
#include "rtc.h"
#include "app.h"
#include "data_storage.h"



#define IS_UPDATED_LOCATION()                       \
    ({ bool _updated = gps.location.isUpdated();    \
       if (_updated) {                              \
           gnssData.latitude = gps.location.lat();  \
           gnssData.longitude = gps.location.lng(); \
       } \
       _updated; })

#define IS_UPDATED_ALTITUDE()                           \
    ({ bool _updated = gps.altitude.isUpdated();        \
       if (_updated) {                                  \
    	   gnssData.altitude = gps.altitude.meters();   \
       }                                                \
       _updated; })

#define IS_UPDATED_DATE()                                   \
    ({ bool _updated = gps.date.isUpdated();                \
        if (_updated) {                                     \
            gnssData.day = gps.date.day();                  \
            gnssData.month = gps.date.month();              \
            gnssData.year = gps.date.year();                \
            gnssData.hour = gps.time.hour();                \
            gnssData.minute = gps.time.minute();            \
            gnssData.second = gps.time.second();            \
            gnssData.centisecond = gps.time.centisecond();  \
            apply_timezone_offset(&gnssData, TIMEZONE_HOUR_OFFSET, TIMEZONE_MINUTE_OFFSET); \
        }                                                   \
        _updated; })

// #define IS_UPDATED_TIME() \
//     ({ bool _updated = gps.time.isUpdated(); \
//         if (_updated) { \
//             gnssData.hour = gps.time.hour(); \
//             gnssData.minute = gps.time.minute(); \
//             gnssData.second = gps.time.second(); \
//             gnssData.centisecond = gps.time.centisecond(); \
//             gnssData.day = gps.date.day(); \
//             gnssData.month = gps.date.month(); \
//             gnssData.year = gps.date.year(); \
//             apply_timezone_offset(&gnssData, TIMEZONE_HOUR_OFFSET, TIMEZONE_MINUTE_OFFSET); \
//         } \
//         _updated; })


UART_HandleTypeDef* GPS::port = NULL;
TinyGPSPlus GPS::gps;
Storage_data_t GPS::gnssData;

#define GPS_RX_BUFFER_SIZE 1024

uint8_t gpsRxBuffer[GPS_RX_BUFFER_SIZE];
volatile uint16_t gpsLastPos = 0;

uint32_t GPS::lastGnssDisplay = 0;
uint32_t GPS::lastStreamProcess = 0;
uint64_t GPS::gnssStreamTimestamp = 0;
uint64_t GPS::validGnssStreamTimestamp = 0;

bool GPS::gnssSignalReceived = false;

void GPS::getGnssData(Storage_data_t* data)
{
   storage_get_metadata(data);
}

void GPS::setGnssData(Storage_data_t* data)
{
   storage_set_metadata(data);
}

void GPS::processGpsStream(void)
{
    uint16_t currentPos = GPS_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(port->hdmarx);

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
    else if(HAL_GetTick() - gnssStreamTimestamp > UART_DMA_REINT_TIMEOUT)
    {
        LOG_D("GNSS Stream stuck. ReInitializing UART DMA.\r\n");
        HAL_UART_AbortReceive(port);  // Stop current DMA

        // Restart DMA in circular mode
        HAL_UART_Receive_DMA(port, gpsRxBuffer, GPS_RX_BUFFER_SIZE); 

        // OR: Use idle mode for better sentence-level parsing
        // HAL_UARTEx_ReceiveToIdle_DMA(port, dmaBuffer, DMA_BUFFER_SIZE);
        // __HAL_DMA_DISABLE_IT(port->hdmarx, DMA_IT_HT | DMA_IT_TC);

        gnssStreamTimestamp = HAL_GetTick(); // Reset timestamp
    }
}

bool GPS::has3DFix()
{
    // Require valid location, altitude, and HDOP
    if (gps.location.isValid() &&
        gps.altitude.isValid() &&
        gps.satellites.value() >= 4 &&     // minimum for 3D
        gps.hdop.isValid() &&
        gps.hdop.hdop() <= 5.0)
    {
        LOG_D("Strong 3D Fix acquired: sats=%d hdop=%.2f alt=%.2f m",
              gps.satellites.value(),
              gps.hdop.hdop(),
              gps.altitude.meters());
        return true;
    }

    return false;
}

void GPS::processData(uint8_t *data, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
    {
		gps.encode(data[i]);
		LOG_GNSS_STREAM("%c", data[i]);
    }

    bool locUpdated  = IS_UPDATED_LOCATION();
    bool dateUpdated = IS_UPDATED_DATE();
    bool altUpdated  = IS_UPDATED_ALTITUDE();
    // bool timeUpdated = IS_UPDATED_TIME();

    // if( locUpdated ||
    //     dateUpdated ||
    //     altUpdated )
    // {
    //     setGnssData(&gnssData);
    // }

	if (has3DFix() && gps.date.isValid())
	{
        // IMPORTANT: Flash has limited erase/program cycles. Avoid writing on
        // every NMEA update. We only persist the first valid fix per boot.
        if(!gnssSignalReceived)
        {
            setGnssData(&gnssData);
//            Flash_WriteGnssLockFlag(GNSS_LOCK_FLAG);
        }
        gnssSignalReceived = true;
    }
}


void GPS::displayInfo(void)
{
    LOG_D("Location: ");
    if (gps.location.isValid())
    {
        LOG_D("%.6f, %.6f", gnssData.latitude, gnssData.longitude);
    }
    else
    {
        LOG_D("INVALID");
    }

    LOG_D("  Altitude: ");
    if (gps.altitude.isValid())
    {
        LOG_D("%.2f m", gnssData.altitude);
    }
    else
    {
        LOG_D("INVALID");
    }

    LOG_D("  Date/Time: ");
    if (gps.date.isValid())
    {
        LOG_D("%02d/%02d/%04d ", gnssData.month, gnssData.day, gnssData.year);
    }
    else
    {
        LOG_D("INVALID ");
    }

    if (gps.time.isValid())
    {
        LOG_D("%02d:%02d:%02d.%02d\r\n",
               gnssData.hour,
               gnssData.minute,
               gnssData.second,
               gnssData.centisecond);
    }
    else
    {
        LOG_D("INVALID\r");
    }
}

void GPS::setup(UART_HandleTypeDef* uart)
{
    if(!uart)
    {
        LOG_D("GPS setup failed: UART handle is null.\r\n");
        return;
    }

    port = uart;

    HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_SET);
	HAL_Delay(10);

	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(400);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_SET);

	// SET_BIT(port->hdmarx->Instance->CCR, DMA_CCR_CIRC);
    HAL_UART_Receive_DMA(port, gpsRxBuffer, GPS_RX_BUFFER_SIZE);

    INIT_TIMESTAMPS_GNSS;

    getGnssData(&gnssData);
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
