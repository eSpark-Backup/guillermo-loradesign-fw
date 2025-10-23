#ifndef FLASHDATA_H
#define FLASHDATA_H

#include <stdint.h>
#include "stm32wlxx_hal.h"
#include "gps.h"
#include "app.h"


#define PAGE_SIZE         0x800  // 2 KB page size
#define PAGE0_BASE_ADDR   0x0801F800UL // Start of second last 2KB flash page
#define PAGE1_BASE_ADDR   0x0801F000UL
#define PAGE2_BASE_ADDR   0x0801E800UL
#define PAGE3_BASE_ADDR   0x0801E000UL
#define PAGE4_BASE_ADDR   0x0801D800UL
#define PAGE5_BASE_ADDR   0x0801D000UL
#define PAGE6_BASE_ADDR   0x0801C800UL
#define PAGE7_BASE_ADDR   0x0801C000UL


#define SECONDSSTART_PAGE_ADDR  PAGE0_BASE_ADDR // Start of  last 2KB flash page

#define SLEEP_ADDR 				PAGE1_BASE_ADDR // Start of second last 2KB flash page


#define LATITUDE_ADDR       	PAGE2_BASE_ADDR
#define GNSS_LOCK_FLAG_ADDR  	PAGE3_BASE_ADDR
#define LONGITUDE_ADDR       	PAGE4_BASE_ADDR
#define ALTITUDE_ADDR        	PAGE5_BASE_ADDR

#define TIMESTAMP_ADDR       	PAGE6_BASE_ADDR

#define CALENDAR_ADDR       	PAGE7_BASE_ADDR


//struct DAY{
//	unsigned short Month; // 2 bytes
//	unsigned short Day; // 2 bytes
//	char Schedule; // 1 byte
//};


//DAY calendar[30];

// the below variable will be use for saving calendar in Flash Memoryx
//uint64_t schedule_data =  ((uint64_t)calendar[0].Month << 16)
//						| ((uint64_t)calendar[0].Day << 8)
//						| (calendar[0].Schedule);

//#define SCHEDULE_TIME_RANGE_A 	6	 // 06-08
//#define SCHEDULE_TIME_RANGE_B 	7	 // 07-09
//#define SCHEDULE_TIME_RANGE_C 	8	 // 08-10
//#define SCHEDULE_TIME_RANGE_D 	16	 // 16-18

//#define day_ADDR             0x0801F020UL  // uint8_t (1 byte)
//#define month_ADDR           0x0801F021UL  // uint8_t (1 byte)
//#define year_ADDR            0x0801F022UL  // uint16_t (2 bytes)
//
//#define hour_ADDR            0x0801F024UL  // uint8_t (1 byte)
//#define minute_ADDR          0x0801F025UL  // uint8_t (1 byte)
//#define second_ADDR          0x0801F026UL  // uint8_t (1 byte)
//#define centisecond_ADDR     0x0801F027UL  // uint8_t (1 byte)

void Flash_Write(uint32_t address, uint64_t data);
void Flash_Write_Single(uint32_t address, uint64_t data);

void Flash_Multiple_Write(FlashData_t* data, DAY *data_2);

uint32_t Flash_Read(uint32_t address);
uint16_t Flash_Read(uint16_t address);
uint8_t Flash_Read(uint8_t address);
double Flash_Read(double address);
void Flash_Read_Multiple(FlashData_t* data, DAY* data_2);

// Lightweight calendar-only functions
void Flash_Multiple_Write_Calendar(DAY *data_2);
void Flash_Read_Multiple_Calendar(DAY *data_2);

#endif
