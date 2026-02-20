#ifndef FLASHDATA_H
#define FLASHDATA_H

#include <stdint.h>
#include "stm32wlxx_hal.h"

// Pull in the project's canonical type definitions.
// (Avoid re-defining these types here to prevent conflicts.)
#include "gps.h"   // FlashData_t
#include "app.h"   // DAY

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

// NOTE: These are *flash* addresses. Use FLASH_* names to avoid collisions with
// RTC backup-register macros (e.g., GNSS_LOCK_FLAG_ADDR in gps.h).
#define FLASH_SLEEP_STATE_ADDR    PAGE1_BASE_ADDR

#define FLASH_LATITUDE_ADDR       PAGE2_BASE_ADDR
#define FLASH_GNSS_LOCK_FLAG_ADDR PAGE3_BASE_ADDR
#define FLASH_LONGITUDE_ADDR      PAGE4_BASE_ADDR
#define FLASH_ALTITUDE_ADDR       PAGE5_BASE_ADDR

#define FLASH_TIMESTAMP_ADDR      PAGE6_BASE_ADDR

#define FLASH_CALENDAR_ADDR       PAGE7_BASE_ADDR

// Backwards-compatible aliases (except GNSS_LOCK_FLAG_ADDR which collides with gps.h)
#define SLEEP_ADDR        FLASH_SLEEP_STATE_ADDR
#define LATITUDE_ADDR     FLASH_LATITUDE_ADDR
#define LONGITUDE_ADDR    FLASH_LONGITUDE_ADDR
#define ALTITUDE_ADDR     FLASH_ALTITUDE_ADDR
#define TIMESTAMP_ADDR    FLASH_TIMESTAMP_ADDR
#define CALENDAR_ADDR     FLASH_CALENDAR_ADDR

// -----------------------------------------------------------------------------
// Base flash primitives (existing)
// -----------------------------------------------------------------------------
void Flash_Write(uint32_t address, uint64_t data);
void Flash_Write_Single(uint32_t address, uint64_t data);

void Flash_Multiple_Write(FlashData_t* data, DAY* data_2);

uint32_t Flash_Read(uint32_t address);
uint16_t Flash_Read(uint16_t address);
uint8_t  Flash_Read(uint8_t address);
double   Flash_Read(uint64_t address);
void Flash_Read_Multiple(FlashData_t* data, DAY* data_2);

// -----------------------------------------------------------------------------
// Convenience helpers used by the GNSS / app code
// -----------------------------------------------------------------------------

// Sleep state persistence (SLEEP_STATE_DAY / SLEEP_STATE_25S)
uint32_t Flash_ReadSleepState(void);
void Flash_WriteSleepState(uint32_t state);

// GNSS lock flag persistence (GNSS_LOCK_FLAG)
uint32_t Flash_ReadGnssLockFlag(void);
void Flash_WriteGnssLockFlag(uint32_t flag);

// GNSS last-fix persistence (latitude/longitude + timestamp)
bool Flash_ReadGnssData(FlashData_t* out);
void Flash_WriteGnssData(const FlashData_t* in);

#endif // FLASHDATA_H
