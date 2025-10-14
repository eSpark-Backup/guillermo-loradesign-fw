#ifndef FLASHDATA_H
#define FLASHDATA_H

#include <stdint.h>
#include "stm32wlxx_hal.h"
#include "gps.h"


#define PAGE_SIZE         0x800  // 2 KB page size
#define PAGE0_BASE_ADDR   0x0801F800UL // Start of second last 2KB flash page
#define PAGE1_BASE_ADDR   0x0801F000UL
#define PAGE2_BASE_ADDR   0x0801E800UL
#define PAGE3_BASE_ADDR   0x0801E000UL
#define PAGE4_BASE_ADDR   0x0801D800UL
#define PAGE5_BASE_ADDR   0x0801D000UL
#define PAGE6_BASE_ADDR   0x0801C800UL


#define SLEEP_ADDR PAGE1_BASE_ADDR // Start of second last 2KB flash page

#define GNSS_LOCK_FLAG_ADDR PAGE2_BASE_ADDR

#define SECONDSSTART_PAGE_ADDR  PAGE0_BASE_ADDR // Start of  last 2KB flash page

#define LATITUDE_ADDR        PAGE1_BASE_ADDR
#define LONGITUDE_ADDR       PAGE3_BASE_ADDR
#define ALTITUDE_ADDR        PAGE4_BASE_ADDR

#define CALENDAR_ADDR        PAGE5_BASE_ADDR

//#define day_ADDR             0x0801F020UL  // uint8_t (1 byte)
//#define month_ADDR           0x0801F021UL  // uint8_t (1 byte)
//#define year_ADDR            0x0801F022UL  // uint16_t (2 bytes)
//
//#define hour_ADDR            0x0801F024UL  // uint8_t (1 byte)
//#define minute_ADDR          0x0801F025UL  // uint8_t (1 byte)
//#define second_ADDR          0x0801F026UL  // uint8_t (1 byte)
//#define centisecond_ADDR     0x0801F027UL  // uint8_t (1 byte)

void Flash_Write(uint32_t address, uint32_t data);
void Flash_Multiple_Write(FlashData_t* data);

uint32_t Flash_Read(uint32_t address);
uint16_t Flash_Read(uint16_t address);
uint8_t Flash_Read(uint8_t address);
double Flash_Read(double address);
void Flash_Read_Multiple(FlashData_t* data);


#endif
