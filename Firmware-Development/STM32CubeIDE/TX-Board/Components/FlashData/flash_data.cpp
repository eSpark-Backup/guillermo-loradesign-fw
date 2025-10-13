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

    HAL_FLASH_Lock();
}

void Flash_Multiple_Write(FlashData_t* data)
{
    HAL_FLASH_Unlock();

    // Erase one page
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Page = (secondsstart_page_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
    eraseInit.NbPages = 1;
    HAL_FLASHEx_Erase(&eraseInit, &pageError);


	// Write doubles (8 bytes each)
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, longitude_ADDR, *(uint64_t*)&data->longitude);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, latitude_ADDR, *(uint64_t*)&data->latitude);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, altitude_ADDR, *(uint64_t*)&data->altitude);

	uint32_t dateTimeWord1 = (data->month << 8) | data->day;  // month:8 bits, day:8 bits
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, day_ADDR, dateTimeWord1);  // address is aligned

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, year_ADDR, data->year);

	uint32_t timeBytePack =
		(data->minute << 8) | data->hour;  // minute:8 bits, hour:8 bits
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, hour_ADDR, timeBytePack);

	uint32_t secPack =
		(data->centisecond << 8) | data->second;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, second_ADDR, secPack);

    HAL_FLASH_Lock();
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


double Flash_Read(double address)
{
    return *(double*)address;
}

void Flash_Read_Multiple(FlashData_t* data){
	data->longitude = Flash_Read(longitude_ADDR);
	data->latitude = Flash_Read(latitude_ADDR);
	data->altitude = Flash_Read(altitude_ADDR);

	uint16_t dateTimeWord1 = Flash_Read<uint16_t>(day_ADDR);
	data->day   = dateTimeWord1 & 0xFF;
	data->month = (dateTimeWord1 >> 8) & 0xFF;

	data->year = Flash_Read(year_ADDR);

	uint16_t timePack = Flash_Read(second_ADDR);
	data->hour = timePack & 0xFF;
	data->minute = (timePack >> 8) & 0xFF;

	uint16_t secPack = Flash_Read(second_ADDR);
	data->second = secPack & 0xFF;
	data->centisecond = (secPack >> 8) & 0xFF;

}
