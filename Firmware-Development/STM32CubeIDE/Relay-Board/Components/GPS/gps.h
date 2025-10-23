#ifndef GPS_H
#define GPS_H

#include "usart.h"
#include "TinyGPS++.h"

typedef struct __attribute__((packed, aligned(8))) {
    double longitude;      // 8 bytes
    double latitude;       // 8 bytes
    double altitude;
    uint8_t day;           // 1 byte
    uint8_t month;         // 1 byte
    uint16_t year;         // 2 bytes
    uint8_t hour;          // 1 byte
    uint8_t minute;       // 1 byte
    uint8_t second;       // 1 byte
    uint8_t centisecond;  // 1 byte
} FlashData_t;

class GPS
{
public:
	void setup(void);
	void loop(void);
	void displayInfo();

	static void processData(UART_HandleTypeDef *huart);

	static TinyGPSPlus gps;
	static uint8_t data;
};
#endif // GPS_H
