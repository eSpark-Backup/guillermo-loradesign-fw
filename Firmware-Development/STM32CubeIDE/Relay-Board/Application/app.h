#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>

//#define CUSTOM_BOARD_ID 1
#define BOARD_ID 0x1234
#define DAY_TIMEOUT 60000U // (1000U * 60U * 60U * 24U) // 1 Day Timeout

#define TX_BOARD_IDENTIFIER "TX:"
#define MY_IDENTIFIER "RELAY:"
#define PACKET_IDENTIFIER "PACKET:"
#define MY_PACKET_IDENTIFIER MY_IDENTIFIER PACKET_IDENTIFIER

#define MCU_1 1
#define MCU_2 2

#define RELAYBOARD MCU_2

enum BoardState
{
	BOOTUP_STATE,
	SEQUENCE_END_STATE,
	SEND_SHUTOFF_STATE
};

struct Timestamp
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

struct GeoCoordinates
{
	double lattitude;
	double longitude;
	double altitude;
};

struct CommPacket
{
	uint32_t ids[3];
	Timestamp timestamp;
	GeoCoordinates geoCoordinates;
};

class App
{
public:
	void setup(void);
	void loop(void);
};
#endif // APPLICATION_H
