#include "gps.h"
#include "rtc.h"
#include "app.h"

static inline void apply_timezone_offset(RtcData_t* data, int tz_hour, int tz_minute) {
    int y = data->year;
    int mo = data->month;
    int d = data->day;
    int h = data->hour;
    int m = data->minute + tz_minute;
    int s = data->second;

    h += tz_hour + m / 60;
    m %= 60;
    if (m < 0) { m += 60; h -= 1; }

    d += h / 24;
    h %= 24;
    if (h < 0) { h += 24; d -= 1; }

    static const uint8_t daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

    while (1) {
        if (d < 1) {
            mo -= 1;
            if (mo < 1) { mo = 12; y -= 1; }
            bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
            int max_day = daysInMonth[mo - 1];
            if (leap && mo == 2) max_day = 29;
            d += max_day;
        } else {
            bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
            int max_day = daysInMonth[mo - 1];
            if (leap && mo == 2) max_day = 29;
            if (d > max_day) {
                d -= max_day;
                mo += 1;
                if (mo > 12) { mo = 1; y += 1; }
            } else break;
        }
    }

    data->year = y;
    data->month = mo;
    data->day = d;
    data->hour = h;
    data->minute = m;
    // data->second and centisecond are unchanged
}


#define IS_UPDATED_LOCATION() \
    ({ bool _updated = gps.location.isUpdated(); \
       if (_updated) { \
           gnssData.latitude = gps.location.lat(); \
           gnssData.longitude = gps.location.lng(); \
       } \
       _updated; })

#define IS_UPDATED_ALTITUDE() \
    ({ bool _updated = gps.altitude.isUpdated(); \
       if (_updated) { \
    	   gnssData.altitude = gps.altitude.meters(); \
       } \
       _updated; })

#define IS_UPDATED_DATE() \
    ({ bool _updated = gps.date.isUpdated(); \
        if (_updated) { \
            gnssData.day = gps.date.day(); \
            gnssData.month = gps.date.month(); \
            gnssData.year = gps.date.year(); \
            gnssData.hour = gps.time.hour(); \
            gnssData.minute = gps.time.minute(); \
            gnssData.second = gps.time.second(); \
            gnssData.centisecond = gps.time.centisecond(); \
            apply_timezone_offset(&gnssData, TIMEZONE_HOUR_OFFSET, TIMEZONE_MINUTE_OFFSET); \
        } \
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
RTC_HandleTypeDef* GPS::rtcMemory = NULL;
TinyGPSPlus GPS::gps;
RtcData_t GPS::gnssData;

#define GPS_RX_BUFFER_SIZE 1024

uint8_t gpsRxBuffer[GPS_RX_BUFFER_SIZE];
volatile uint16_t gpsLastPos = 0;

uint32_t GPS::lastGnssDisplay = 0;
uint32_t GPS::lastStreamProcess = 0;
uint64_t GPS::gnssStreamTimestamp = 0;
uint64_t GPS::validGnssStreamTimestamp = 0;

bool GPS::gnssSignalReceived = false;

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
        // LOG_D("Storing data in RTC bank: Addr: %X, Data: %X\r\n", adr, *data);
        HAL_RTCEx_BKUPWrite(rtcMemory, adr, *data);  // Store single value at the given address
    }
    else if (opr == RETRIEVE)
    {
        *data = HAL_RTCEx_BKUPRead(rtcMemory, adr);  // Retrieve value from the given address
        // LOG_D("Retrieving data From RTC bank: Addr: %X, Data: %X\r\n", adr, *data);
    }
    else
    {
        LOG_D("Invalid RTC operation.\r\n");
    }
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

    for (uint16_t i = 0; i < len; i++) {
        gps.encode(data[i]);
        LOG_GNSS_STREAM("%c", data[i]);
    }

    bool locUpdated  = IS_UPDATED_LOCATION();
    bool dateUpdated = IS_UPDATED_DATE();
    bool altUpdated  = IS_UPDATED_ALTITUDE();

    if (locUpdated && dateUpdated && has3DFix()) {
        setGnssData(&gnssData);

        uint32_t val;
        rtcOperation(RETRIEVE, GNSS_LOCK_FLAG_ADDR, &val);
        if (val != GNSS_LOCK_FLAG) {
            val = GNSS_LOCK_FLAG;
            rtcOperation(STORE, GNSS_LOCK_FLAG_ADDR, &val);
        }

        gnssSignalReceived = true;

    }


//	previous logic with altitude
	/*    for (uint16_t i = 0; i < len; i++)
    {
        gps.encode(data[i]);
        LOG_GNSS_STREAM("%c", data[i]);
    }

    bool locUpdated  = IS_UPDATED_LOCATION();
    bool dateUpdated = IS_UPDATED_DATE();
    bool altUpdated  = IS_UPDATED_ALTITUDE();   // <--- NEW
    // bool timeUpdated = IS_UPDATED_TIME();

    if (locUpdated && dateUpdated && altUpdated)
    {
        setGnssData(&gnssData);

        uint32_t val;
        rtcOperation(RETRIEVE, GNSS_LOCK_FLAG_ADDR, &val);
        if (val != GNSS_LOCK_FLAG)
        {
            val = GNSS_LOCK_FLAG;
            rtcOperation(STORE, GNSS_LOCK_FLAG_ADDR, &val);
        }

        gnssSignalReceived = true;
    }*/


    // else if((HAL_GetTick() - validGnssStreamTimestamp > VALID_GNSS_SIGNAL_TIMEOUT) && !gnssSignalThresholdExceeded)
    // {
    //     LOG_D("Valid gnss signal not received and threshold exceeded.\r\n");
    //     gnssSignalThresholdExceeded = true;
    //     uint32_t val;
    //     rtcOperation(RETRIEVE, GNSS_LOCK_FLAG_ADDR, &val);
    //     if(val == GNSS_LOCK_FLAG)
    //     {
    //         LOG_D("Using previous stored Coordinates.\r\n");
    //         getGnssData(&gnssData);
    //     }
    // }

    // RtcData_t readOut;
    // getGnssData(&readOut);
    // LOG_GNSS_STREAM("%.6f,%.6f   XX  ", readOut.latitude, readOut.longitude);
    // LOG_GNSS_STREAM("%02d/%02d/%04d ", readOut.month, readOut.day, readOut.year);
    // LOG_GNSS_STREAM("%02d:%02d:%02d.%02d\r\n",
    //     readOut.hour,
    //     readOut.minute,
    //     readOut.second,
    //     readOut.centisecond);
}

void GPS::displayInfo(void)
{
    LOG_D("Location: ");
    if (gps.location.isValid())
    {
        LOG_D("%.6f,%.6f", gnssData.latitude, gnssData.longitude);
    }
    else
    {
        LOG_D("INVALID");
    }

    LOG_D("Altitude: ");
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
        LOG_D("INVALID\r\n");
    }
}

void GPS::setup(RTC_HandleTypeDef* rtc, UART_HandleTypeDef* uart)
{
    if(uart && rtc)
    {
        port = uart;
        rtcMemory = rtc;
    }
    else
        return;

    HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_SET);
	HAL_Delay(10);

	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(400);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_SET);

	// SET_BIT(port->hdmarx->Instance->CCR, DMA_CCR_CIRC);
    HAL_UART_Receive_DMA(port, gpsRxBuffer, GPS_RX_BUFFER_SIZE);

    INIT_TIMESTAMPS_GNSS;
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
