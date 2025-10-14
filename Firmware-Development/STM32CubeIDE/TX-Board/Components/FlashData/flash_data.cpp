#include "flash_data.h"

void Flash_Write(uint32_t address, uint32_t data)
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

//void Flash_Multiple_Write(FlashData_t* data)
//{
//	HAL_FLASH_Unlock();
//
//	// Erase one page
//	FLASH_EraseInitTypeDef eraseInit;
//	uint32_t pageError;
//	eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
//	eraseInit.Page = (SECONDSSTART_PAGE_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
//	eraseInit.NbPages = 1;
//	HAL_FLASHEx_Erase(&eraseInit, &pageError);
//
//
//	// Write doubles (8 bytes each)
//	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, LATITUDE_ADDR, *(double*)&data->latitude);
//	LOG_D("%lf Stored in Flash at 0x%lx\n", data->latitude, (unsigned long)LATITUDE_ADDR);
//
//	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, LONGITUDE_ADDR, *(double*)&data->longitude);
//	LOG_D("%lf Stored in Flash at 0x%lx\n", data->longitude, (unsigned long)LONGITUDE_ADDR);
//
//	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, ALTITUDE_ADDR, *(double*)&data->altitude);
//	LOG_D("%lf Stored in Flash at 0x%lx\n", data->altitude, (unsigned long)ALTITUDE_ADDR);
//
//	uint64_t calendar_data =
//			((uint64_t)data->centisecond << 56) |
//			((uint64_t)data->second << 48) |
//			((uint64_t)data->minute << 40) |
//			((uint64_t)data->hour << 32) |
//			((uint64_t)data->year << 16) |
//			((uint64_t)data->month << 8) |
//			((uint64_t)data->day);
//
//	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, CALENDAR_ADDR, calendar_data);
//	LOG_D("%lx Stored in Flash at 0x%lx\n", (uint64_t)calendar_data, (unsigned long)CALENDAR_ADDR);
//
//	HAL_FLASH_Lock();
//}

void Flash_Multiple_Write(FlashData_t* data)
{

	uint64_t calendar_data =
			        ((uint64_t)data->centisecond << 56) |
			        ((uint64_t)data->second << 48) |
			        ((uint64_t)data->minute << 40) |
			        ((uint64_t)data->hour << 32) |
			        ((uint64_t)data->year << 16) |
			        ((uint64_t)data->month << 8) |
			        ((uint64_t)data->day);

	Flash_Write(LATITUDE_ADDR, *(double*)&data->latitude);
	Flash_Write(LONGITUDE_ADDR, *(double*)&data->longitude);
	Flash_Write(ALTITUDE_ADDR, *(double*)&data->altitude);
	Flash_Write(CALENDAR_ADDR, calendar_data);

//		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, LATITUDE_ADDR, *(double*)&data->latitude);
		LOG_D("%lf Stored in Flash at 0x%lx\n", data->latitude, (unsigned long)LATITUDE_ADDR);

//		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, LONGITUDE_ADDR, *(double*)&data->longitude);
		LOG_D("%lf Stored in Flash at 0x%lx\n", data->longitude, (unsigned long)LONGITUDE_ADDR);

//		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, ALTITUDE_ADDR, *(double*)&data->altitude);
		LOG_D("%lf Stored in Flash at 0x%lx\n", data->altitude, (unsigned long)ALTITUDE_ADDR);



//		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, CALENDAR_ADDR, calendar_data);
		LOG_D("%lx Stored in Flash at 0x%lx\n", (uint64_t)calendar_data, (unsigned long)CALENDAR_ADDR);

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

void Flash_Read_Multiple(FlashData_t* data){
	data->latitude = (double)Flash_Read(LATITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->latitude, (unsigned long)LATITUDE_ADDR);
	data->longitude = (double)Flash_Read(LONGITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->longitude, (unsigned long)LONGITUDE_ADDR);
	data->altitude = (double)Flash_Read(ALTITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->altitude, (unsigned long)ALTITUDE_ADDR);

//	uint64_t calendar_data = (uint64_t)Flash_Read(CALENDAR_ADDR);
//	uint64_t calendar_data = ((uint64_t)Flash_Read(CALENDAR_ADDR + 4) << 32) | ((uint64_t)Flash_Read(CALENDAR_ADDR));
//	LOG_D("Retrieved Calendar data from Flash: %lx\n",calendar_data);
//	data->day   = calendar_data & 0xFF;
//	data->month = (calendar_data >> 8) & 0xFF;
//	data->year = (calendar_data >> 16) & 0xFFFF;
//	data->hour = (calendar_data >> 32) & 0xFF;
//	data->minute = (calendar_data >> 40) & 0xFF;
//	data->second = (calendar_data >> 48) & 0xFF;
//	data->centisecond = (calendar_data >> 56) & 0xFF;


//	LOG_D("Retrieved Calendar from Flash: %d:%d:%d:%d:%d:%d:%d\n",data->day, data->month, data->year, data->hour, data->minute, data->second, data->centisecond);


	uint64_t calendar_data = ((uint64_t)Flash_Read(CALENDAR_ADDR + 4) << 32) | Flash_Read(CALENDAR_ADDR);

	data->day         = calendar_data & 0xFF;
	data->month       = (calendar_data >> 8) & 0xFF;
	data->year        = (calendar_data >> 16) & 0xFFFF;
	data->hour        = (calendar_data >> 32) & 0xFF;
	data->minute      = (calendar_data >> 40) & 0xFF;
	data->second      = (calendar_data >> 48) & 0xFF;
	data->centisecond = (calendar_data >> 56) & 0xFF;

	LOG_D("Retrieved Calendar from Flash: %d:%d:%d:%d:%d:%d:%d\n",data->day, data->month, data->year, data->hour, data->minute, data->second, data->centisecond);


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
