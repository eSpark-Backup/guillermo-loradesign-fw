#ifndef GPS_H
#define GPS_H

#include "usart.h"
#include "TinyGPS++.h"

#ifdef  ENABLE_GNSS_STREAM_DEBUG
#define LOG_GNSS_STREAM(fmt, ...)		itm_printf(GNSS_STREAM_PORT, fmt, ##__VA_ARGS__)
#else
#define LOG_GNSS_STREAM(fmt, ...)
#endif

#define INIT_TIMESTAMPS_GNSS				 		\
		lastGnssDisplay = HAL_GetTick();	 		\
		lastStreamProcess = HAL_GetTick();	 		\
		gnssStreamTimestamp = HAL_GetTick(); 		\
		validGnssStreamTimestamp = HAL_GetTick(); 	\

#define UART_DMA_REINT_TIMEOUT		1000
#define VALID_GNSS_SIGNAL_TIMEOUT	30000

#define GNSS_LOCK_FLAG		  0x9669
#define GNSS_LOCK_FLAG_ADDR   RTC_BKP_DR0
#define RTC_BKP_BASE_ADDR     RTC_BKP_DR14   // Starting from DR14
#define RTC_BKP_WORD_COUNT    6              // 6 x 4-byte backup registers

enum RtcOperation
{
	STORE,
	RETRIEVE,
};

typedef struct __attribute__((packed, aligned(1))) {
    double longitude;      // 8 bytes
    double latitude;       // 8 bytes
    uint8_t day;           // 1 byte
    uint8_t month;         // 1 byte
    uint16_t year;         // 2 bytes
    uint8_t hour;          // 1 byte
    uint8_t minute;       // 1 byte
    uint8_t second;       // 1 byte
    uint8_t centisecond;  // 1 byte
} RtcData_t;

class GPS
{
public:
	void setup(RTC_HandleTypeDef* rtc, UART_HandleTypeDef* uart);
	void loop(void);
	void displayInfo();

	static void processGpsStream(void);

	static void processData(uint8_t *data, uint16_t len);
	// static void processData(UART_HandleTypeDef *huart);

	static void getGnssData(RtcData_t* data);
	static void setGnssData(RtcData_t* data);
	static void rtcOperation(RtcOperation opr, uint32_t adr, uint32_t* data);

	static UART_HandleTypeDef* port;
	static RTC_HandleTypeDef* rtcMemory;
	static TinyGPSPlus gps;
	static RtcData_t gnssData;

	static uint32_t lastGnssDisplay;
	static uint32_t lastStreamProcess;

	static uint64_t gnssStreamTimestamp;
	static uint64_t validGnssStreamTimestamp;

	static bool gnssSignalThresholdExceeded;
};
#endif // GPS_H
