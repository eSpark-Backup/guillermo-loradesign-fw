#ifndef GPS_H
#define GPS_H

#include "usart.h"
#include "TinyGPS++.h"

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
