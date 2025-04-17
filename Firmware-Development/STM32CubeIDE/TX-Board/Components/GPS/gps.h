#ifndef GPS_H
#define GPS_H

#include "usart.h"
#include "TinyGPS++.h"

#define GNSS_STREAM_PORT		1
#ifdef  ENABLE_GNSS_STREAM_DEBUG
#define LOG_GNSS_STREAM(fmt, ...)		itm_printf(GNSS_STREAM_PORT, fmt, ##__VA_ARGS__)
#else
#define LOG_GNSS_STREAM(fmt, ...)
#endif


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
	void setup(void);
	void loop(void);
	void displayInfo();

	static void processGpsStream(void);

	static void processData(uint8_t *data, uint16_t len);
	// static void processData(UART_HandleTypeDef *huart);

	static void getGnssData(RtcData_t* data);
	static void setGnssData(RtcData_t* data);
	static void rtcOperation(RtcOperation opr, uint32_t adr, uint32_t* data);

	static TinyGPSPlus gps;
	static uint8_t data;

	static uint32_t lastGnssDisplay;
	static uint32_t lastStreamProcess;
};
#endif // GPS_H
