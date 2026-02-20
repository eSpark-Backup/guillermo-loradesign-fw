#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>
#define CUSTOM_BOARD_ID 1
#define BOARD_ID_1 0x00000020
#define BOARD_ID_2 0x00000020
#define BOARD_ID_3 0x00000020
#define WAKEUP_TIME_HOUR		8	 // 24 hour format		// 8 AM
#define WAKEUP_TIME_MINS		0
#define DAY_TIMEOUT (get_wrapped_seconds_diff(WAKEUP_TIME_HOUR, WAKEUP_TIME_MINS, 0)*1000)
// #define DAY_TIMEOUT 86400000UL // (1000U * 60U * 60U * 24U) // 1 Day Timeout
#define SLEEP_TIMEOUT 55000
#define HOUR_THRESHOLD 10		// 24-hour format // device should sleep if current hour is more than threshold and no signal is received from relay board
#define MY_IDENTIFIER "TX:"
#define RELAY_BOARD_IDENTIFIER "RELAY:"
#define HELLO_MESSAGE "HELLO"
#define CAL_PREFIX "CAL:" //Calendar
#define PING_MESSAGE RELAY_BOARD_IDENTIFIER HELLO_MESSAGE
#define TIMEZONE_HOUR_OFFSET   -7    // For UTC-7
#define TIMEZONE_MINUTE_OFFSET  0    // For UTC-7:00

#define RCV_TIMEOUT_VALUE	5000	// should be synced with "RX_TIMEOUT_VALUE" value in "subghz_phy_app.c" file

//#define BOARD_SLEEP_STATE_ADDR		RTC_BKP_DR1
#define SLEEP_STATE_DAY				0xABCD
#define SLEEP_STATE_25S				0xDCBA

uint32_t get_next_schedule_sleep_seconds();
void sleep(uint32_t timeout);

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
	double altitude;
};

struct CommPacket
{
	uint32_t ids[3];
	Timestamp timestamp;
	GeoCoordinates geoCoordinates;
};


struct DAY{
	unsigned short Month; // 2 bytes
	unsigned short Day; // 2 bytes
	char Schedule; // 1 byte
};
extern DAY days[15];

#define SCHEDULE_TIME_RANGE_A_1 	6	 // 06-08
#define SCHEDULE_TIME_RANGE_A_2 	8	 // 06-08
#define SCHEDULE_TIME_RANGE_B_1 	7	 // 07-09
#define SCHEDULE_TIME_RANGE_B_2 	9	 // 07-09
#define SCHEDULE_TIME_RANGE_C_1 	8	 // 08-10
#define SCHEDULE_TIME_RANGE_C_2 	10	 // 08-10
#define SCHEDULE_TIME_RANGE_D_1 	16	 // 16-18
#define SCHEDULE_TIME_RANGE_D_2 	18	 // 16-18


class App
{
public:
	void setup(void);
	void loop(void);
};
#endif // APPLICATION_H
