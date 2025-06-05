#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>
//#define CUSTOM_BOARD_ID 1
#define BOARD_ID 0x1234
#define WAKEUP_TIME_HOUR		8	 // 24 hour format		// 8 AM
#define WAKEUP_TIME_MINS		0
#define DAY_TIMEOUT (get_wrapped_seconds_diff(WAKEUP_TIME_HOUR, WAKEUP_TIME_MINS, 0)*1000)
// #define DAY_TIMEOUT 86400000UL // (1000U * 60U * 60U * 24U) // 1 Day Timeout
#define SLEEP_TIMEOUT 25000
#define HOUR_THRESHOLD 10		// 24-hour format // device should sleep if current hour is more than threshold and no signal is received from relay board
#define MY_IDENTIFIER "TX:"
#define RELAY_BOARD_IDENTIFIER "RELAY:"
#define HELLO_MESSAGE "HELLO"
#define PING_MESSAGE RELAY_BOARD_IDENTIFIER HELLO_MESSAGE
#define TIMEZONE_HOUR_OFFSET   -7    // For UTC-7
#define TIMEZONE_MINUTE_OFFSET  0    // For UTC-7:00

#define RCV_TIMEOUT_VALUE	5000	// should be synced with "RX_TIMEOUT_VALUE" value in "subghz_phy_app.c" file

enum BoardState
{
	BOOTUP_STATE,
	SHUTOFF_STATE,
	TRANSMIT_STATE
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
