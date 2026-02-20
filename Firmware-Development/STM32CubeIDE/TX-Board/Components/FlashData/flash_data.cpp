#include "flash_data.h"
#include "gps.h"   // FlashData_t definition
#include "app.h"   // DAY definition
#include <string.h>  // for memcpy


void Flash_Write(uint32_t address, uint64_t data)
{
    HAL_FLASH_Unlock();

    // Erase one page
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Page = (address - FLASH_BASE) / FLASH_PAGE_SIZE;
    eraseInit.NbPages = 1;
    HAL_FLASHEx_Erase(&eraseInit, &pageError);

    // Write 32-bit word
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    HAL_Delay(150);
    HAL_FLASH_Lock();
}


void Flash_Write_Single(uint32_t address, uint64_t data)
{

    // Write 32-bit word
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    HAL_Delay(50);
}

void Flash_Multiple_Write(FlashData_t* data, DAY *data_2)
{

		uint64_t timestamp_data =
						((uint64_t)data->centisecond << 56) |
						((uint64_t)data->second << 48) |
						((uint64_t)data->minute << 40) |
						((uint64_t)data->hour << 32) |
						((uint64_t)data->year << 16) |
						((uint64_t)data->month << 8) |
						((uint64_t)data->day);

		uint64_t calendar_data[15];

		HAL_FLASH_Unlock();
		    // Erase one page
		FLASH_EraseInitTypeDef eraseInit;
		uint32_t pageError;
		eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
		eraseInit.Page = (FLASH_CALENDAR_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
		eraseInit.NbPages = 1;
		HAL_FLASHEx_Erase(&eraseInit, &pageError);

		for(int i = 0;i < 15; i++){
			calendar_data[i] =
						((uint64_t)0 << 56) |
						((uint64_t)0 << 48) |
						((uint64_t)0 << 40) |
						((uint64_t)0 << 32) |
						((uint64_t)0 << 24) |
						((uint64_t)data_2[i].Schedule << 16) |
						((uint64_t)data_2[i].Month << 8) |
						((uint64_t)data_2[i].Day);

			Flash_Write_Single((FLASH_CALENDAR_ADDR + (8*i)), calendar_data[i]);

		}
//		LOG_D("Calendar Values Stored in Flash at Page of Address: 0x%lx\n", (unsigned long)(CALENDAR_ADDR));
		HAL_FLASH_Lock();



	 	uint64_t temp_val;
		memcpy(&temp_val, &data->latitude, sizeof(temp_val));
		Flash_Write(FLASH_LATITUDE_ADDR, temp_val);
//		LOG_D("%lf Stored in Flash at 0x%lx\n", data->latitude, (unsigned long)LATITUDE_ADDR);


		memcpy(&temp_val, &data->longitude, sizeof(temp_val));
		Flash_Write(FLASH_LONGITUDE_ADDR, temp_val);
//		LOG_D("%lf Stored in Flash at 0x%lx\n", data->longitude, (unsigned long)LONGITUDE_ADDR);


		memcpy(&temp_val, &data->altitude, sizeof(temp_val));
		Flash_Write(FLASH_ALTITUDE_ADDR, temp_val);
//		LOG_D("%lf Stored in Flash at 0x%lx\n", data->altitude, (unsigned long)ALTITUDE_ADDR);


//	Flash_Write(LATITUDE_ADDR, *(double*)&data->latitude);
//	Flash_Write(LONGITUDE_ADDR, *(double*)&data->longitude);
//	Flash_Write(ALTITUDE_ADDR, *(double*)&data->altitude);

		Flash_Write(FLASH_TIMESTAMP_ADDR, timestamp_data);
//		LOG_D("%lx Stored in Flash at 0x%lx\n", (uint64_t)timestamp_data, (unsigned long)TIMESTAMP_ADDR);

//		for(int i = 0; i < 15; i++){
//			Flash_Write((CALENDAR_ADDR + (8*i)), calendar_data[i]);
//			LOG_D("%lx Stored in Flash at 0x%lx\n", (uint64_t)calendar_data[i], (unsigned long)(CALENDAR_ADDR + (8*i)));
//		}

//		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, LATITUDE_ADDR, *(double*)&data->latitude);
//		LOG_D("%lf Stored in Flash at 0x%lx\n", data->latitude, (unsigned long)LATITUDE_ADDR);
//
////		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, LONGITUDE_ADDR, *(double*)&data->longitude);
//		LOG_D("%lf Stored in Flash at 0x%lx\n", data->longitude, (unsigned long)LONGITUDE_ADDR);
//
////		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, ALTITUDE_ADDR, *(double*)&data->altitude);
//		LOG_D("%lf Stored in Flash at 0x%lx\n", data->altitude, (unsigned long)ALTITUDE_ADDR);



//		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, TIMESTAMP_ADDR, calendar_data);
//		for(int i = 0; i < 15; i++){
//			LOG_D("%lx Stored in Flash at 0x%lx\n", (uint64_t)calendar_data[i], (unsigned long)TIMESTAMP_ADDR);
//		}

//		HAL_FLASH_Lock();

	}

uint32_t Flash_Read(uint32_t address)
{
    return *(uint32_t*)address;
}


uint8_t Flash_Read(uint8_t address)
{
    return *(uint8_t*)address;
}


uint16_t Flash_Read(uint16_t address)
{
    return *(uint16_t*)address;
}


double Flash_Read(uint64_t address)
{
    return *(double*)address;
}


// Read a 64-bit double word from flash
uint64_t Flash_ReadDoubleWord(uint32_t address) {
    return *((uint64_t*)address);
}



void Flash_Read_Multiple(FlashData_t* data, DAY* data_2){

    uint64_t raw_val;

    raw_val = Flash_ReadDoubleWord(FLASH_LATITUDE_ADDR);
	memcpy(&data->latitude, &raw_val, sizeof(data->latitude));

    raw_val = Flash_ReadDoubleWord(FLASH_LONGITUDE_ADDR);
	memcpy(&data->longitude, &raw_val, sizeof(data->longitude));

    raw_val = Flash_ReadDoubleWord(FLASH_ALTITUDE_ADDR);
	memcpy(&data->altitude, &raw_val, sizeof(data->altitude));

//	data->latitude = (double)Flash_Read(LATITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->latitude, (unsigned long)FLASH_LATITUDE_ADDR);
//	data->longitude = (double)Flash_Read(LONGITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->longitude, (unsigned long)FLASH_LONGITUDE_ADDR);
//	data->altitude = (double)Flash_Read(ALTITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->altitude, (unsigned long)FLASH_ALTITUDE_ADDR);

//	uint64_t calendar_data = (uint64_t)Flash_Read(TIMESTAMP_ADDR);
//	uint64_t calendar_data = ((uint64_t)Flash_Read(TIMESTAMP_ADDR + 4) << 32) | ((uint64_t)Flash_Read(TIMESTAMP_ADDR));
//	LOG_D("Retrieved Calendar data from Flash: %lx\n",calendar_data);
//	data->day   = calendar_data & 0xFF;
//	data->month = (calendar_data >> 8) & 0xFF;
//	data->year = (calendar_data >> 16) & 0xFFFF;
//	data->hour = (calendar_data >> 32) & 0xFF;
//	data->minute = (calendar_data >> 40) & 0xFF;
//	data->second = (calendar_data >> 48) & 0xFF;
//	data->centisecond = (calendar_data >> 56) & 0xFF;


//	LOG_D("Retrieved Calendar from Flash: %d:%d:%d:%d:%d:%d:%d\n",data->day, data->month, data->year, data->hour, data->minute, data->second, data->centisecond);


	uint64_t timestamp_data = ((uint64_t)Flash_Read(FLASH_TIMESTAMP_ADDR + 4) << 32) | Flash_Read(FLASH_TIMESTAMP_ADDR);

	data->day         = timestamp_data & 0xFF;
	data->month       = (timestamp_data >> 8) & 0xFF;
	data->year        = (timestamp_data >> 16) & 0xFFFF;
	data->hour        = (timestamp_data>> 32) & 0xFF;
	data->minute      = (timestamp_data >> 40) & 0xFF;
	data->second      = (timestamp_data >> 48) & 0xFF;
	data->centisecond = (timestamp_data >> 56) & 0xFF;

	LOG_D("Retrieved Timestamp from Flash: %d:%d:%d:%d:%d:%d:%d\n",data->day, data->month, data->year, data->hour, data->minute, data->second, data->centisecond);

	uint64_t calendar_data[15];
	for(int i = 0; i < 15; i++){
		calendar_data[i] = ((uint64_t)Flash_Read((FLASH_CALENDAR_ADDR + (8 * i)) + 4) << 32) | Flash_Read(FLASH_CALENDAR_ADDR + (8 * i));
		data_2[i].Day = calendar_data[i] & 0xFF;
		data_2[i].Month = (calendar_data[i] >> 8) & 0xFF;
		data_2[i].Schedule = (calendar_data[i] >> 16) & 0xFF;
		LOG_D("Retrieved Calendar from Flash: %d:%d:%c\n",data_2[i].Month, data_2[i].Day, data_2[i].Schedule);
		if(i == 0){
			data_2[i].Day += 1;
			LOG_D("\n--------********---------For Testing---------********----------------\nAdded 1 on the retrieved Day 1\n");
			LOG_D("New Calendar Value of Day 1 will save in Flash: %d:%d:%c\n",data_2[i].Month, data_2[i].Day, data_2[i].Schedule);
			LOG_D("\n------------------------\n");

		}
	}
//		data->hour        = (timestamp_data>> 32) & 0xFF;
//		data->minute      = (timestamp_data >> 40) & 0xFF;
//		data->second      = (timestamp_data >> 48) & 0xFF;
//		data->centisecond = (timestamp_data >> 56) & 0xFF;


	//
//	data->year = Flash_Read(year_ADDR);
//
//	uint16_t timePack = Flash_Read(second_ADDR);
//	data->hour = timePack & 0xFF;
//	data->minute = (timePack >> 8) & 0xFF;
//
//	uint16_t secPack = Flash_Read(second_ADDR);
//	data->second = secPack & 0xFF;
//	data->centisecond = (secPack >> 8) & 0xFF;

}

// -----------------------------------------------------------------------------
// Convenience helpers used by the GNSS / app code
// -----------------------------------------------------------------------------

static uint64_t Flash_ReadDoubleWordSafe(uint32_t address)
{
    return *((uint64_t*)address);
}

uint32_t Flash_ReadSleepState(void)
{
    // Sleep state is stored as a 64-bit value, lower 32 bits hold the state.
    uint64_t raw = Flash_ReadDoubleWordSafe(FLASH_SLEEP_STATE_ADDR);
    return (uint32_t)(raw & 0xFFFFFFFFu);
}

void Flash_WriteSleepState(uint32_t state)
{
    uint64_t desired = (uint64_t)state;
    uint64_t existing = Flash_ReadDoubleWordSafe(FLASH_SLEEP_STATE_ADDR);
    if(existing == desired) return; // avoid extra erase/program cycles
    Flash_Write(FLASH_SLEEP_STATE_ADDR, desired);
}

uint32_t Flash_ReadGnssLockFlag(void)
{
    uint64_t raw = Flash_ReadDoubleWordSafe(FLASH_GNSS_LOCK_FLAG_ADDR);
    return (uint32_t)(raw & 0xFFFFFFFFu);
}

void Flash_WriteGnssLockFlag(uint32_t flag)
{
    uint64_t desired = (uint64_t)flag;
    uint64_t existing = Flash_ReadDoubleWordSafe(FLASH_GNSS_LOCK_FLAG_ADDR);
    if(existing == desired) return;
    Flash_Write(FLASH_GNSS_LOCK_FLAG_ADDR, desired);
}

bool Flash_ReadGnssData(FlashData_t* out)
{
    if(out == NULL) return false;

    // Read latitude / longitude as raw IEEE-754 bits.
    uint64_t raw_lat = Flash_ReadDoubleWordSafe(FLASH_LATITUDE_ADDR);
    uint64_t raw_lon = Flash_ReadDoubleWordSafe(FLASH_LONGITUDE_ADDR);
    memcpy(&out->latitude, &raw_lat, sizeof(out->latitude));
    memcpy(&out->longitude, &raw_lon, sizeof(out->longitude));

    // Read packed timestamp.
    uint64_t ts = Flash_ReadDoubleWordSafe(FLASH_TIMESTAMP_ADDR);

    // If flash is erased, fields will be 0xFF. Treat as invalid.
    if(ts == 0xFFFFFFFFFFFFFFFFULL)
    {
        return false;
    }

    out->day         = (uint8_t)(ts & 0xFF);
    out->month       = (uint8_t)((ts >> 8) & 0xFF);
    out->year        = (uint16_t)((ts >> 16) & 0xFFFF);
    out->hour        = (uint8_t)((ts >> 32) & 0xFF);
    out->minute      = (uint8_t)((ts >> 40) & 0xFF);
    out->second      = (uint8_t)((ts >> 48) & 0xFF);
    out->centisecond = (uint8_t)((ts >> 56) & 0xFF);


    LOG_D("----Location: ");
	LOG_D("%.6f,%.6f ", out->latitude, out->longitude);

	LOG_D("  Date/Time: ");
	LOG_D("%02d/%02d/%04d ", out->month, out->day, out->year);


	LOG_D("%02d:%02d:%02d.%02d\r\n",
						out->hour,
						out->minute,
						out->second,
						out->centisecond);

    return true;
}

void Flash_WriteGnssData(const FlashData_t* in)
{
    if(in == NULL) return;

    uint64_t ts =
        ((uint64_t)in->centisecond << 56) |
        ((uint64_t)in->second      << 48) |
        ((uint64_t)in->minute      << 40) |
        ((uint64_t)in->hour        << 32) |
        ((uint64_t)in->year        << 16) |
        ((uint64_t)in->month       << 8)  |
        ((uint64_t)in->day);

    uint64_t raw_lat, raw_lon;
    memcpy(&raw_lat, &in->latitude, sizeof(raw_lat));
    memcpy(&raw_lon, &in->longitude, sizeof(raw_lon));

    // Avoid unnecessary erase/program cycles if the data hasn't changed.
    if(Flash_ReadDoubleWordSafe(FLASH_LATITUDE_ADDR) != raw_lat)
        Flash_Write(FLASH_LATITUDE_ADDR, raw_lat);
    if(Flash_ReadDoubleWordSafe(FLASH_LONGITUDE_ADDR) != raw_lon)
        Flash_Write(FLASH_LONGITUDE_ADDR, raw_lon);
    if(Flash_ReadDoubleWordSafe(FLASH_TIMESTAMP_ADDR) != ts)
        Flash_Write(FLASH_TIMESTAMP_ADDR, ts);
}
