#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>

//#define CUSTOM_BOARD_ID 1
#define BOARD_ID 0x1234
#define DAY_TIMEOUT 60000U // (1000U * 60U * 60U * 24U) // 1 Day Timeout

#define RELAY_BOARD_IDENTIFIER "RELAY:"
#define PACKET_IDENTIFIER "PACKET:"
#define MY_PACKET_IDENTIFIER RELAY_BOARD_IDENTIFIER PACKET_IDENTIFIER
#define MY_IDENTIFIER "BASESTATION:"

enum BoardState
{
	BOOTUP_STATE,
	SEQUENCE_END_STATE
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
