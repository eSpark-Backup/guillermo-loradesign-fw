#ifndef GPS_H
#define GPS_H

#include "usart.h"
#include "TinyGPS++.h"

// #define LONGITUDE_RTC_ADDR      RTC_BKP_DR18
// #define LONGITUDE_RTC_ADDR_H    RTC_BKP_DR17
// #define LATITUDE_RTC_ADDR       RTC_BKP_DR16
// #define LATITUDE_RTC_ADDR_H     RTC_BKP_DR13
// #define DATE_RTC_ADDR           RTC_BKP_DR15
// #define TIME_RTC_ADDR           RTC_BKP_DR14

#define RTC_BKP_BASE_ADDR     RTC_BKP_DR13   // Starting from DR13
#define RTC_BKP_WORD_COUNT    6              // 6 x 4-byte backup registers

enum RtcOperation
{
	STORE,
	RETRIEVE,
};

typedef struct {//__attribute__((packed)) {
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

	static void processData(UART_HandleTypeDef *huart);
	static void rtcOperation(RtcOperation opr, RtcData_t* data);

	static TinyGPSPlus gps;
	static uint8_t data;
};
#endif // GPS_H
