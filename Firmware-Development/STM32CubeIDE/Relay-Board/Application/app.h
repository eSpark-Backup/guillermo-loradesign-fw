#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>

/* ===================== BOARD CONFIG ===================== */

#define BOARD_ID 0x1234

#define TX_BOARD_IDENTIFIER "TX:"
#define MY_IDENTIFIER "RELAY:"
#define PACKET_IDENTIFIER "PACKET:"
#define BASE_BOARD_IDENTIFIER "BASESTATION:"
#define MY_PACKET_IDENTIFIER MY_IDENTIFIER PACKET_IDENTIFIER

#define MCU_1 1
#define MCU_2 2

#define RELAYBOARD MCU_2

/* ===================== STATES ===================== */
/*  THIS IS THE ONLY PLACE BoardState IS DEFINED */

enum BoardState
{
    BOOTUP_STATE,
    TX_HELLO_STATE,
    RX_LISTEN_STATE,
    SLEEP_STATE,
    SEQUENCE_END_STATE,
    SEND_SHUTOFF_STATE
};

/* ===================== DATA STRUCTURES ===================== */

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

struct DAY
{
    unsigned short Month;
    unsigned short Day;
    char Schedule;
};

class App
{
public:
    void setup(void);
    void loop(void);
};

#endif
