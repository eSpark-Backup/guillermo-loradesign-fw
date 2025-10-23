#include "flash_data.h"
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
		eraseInit.Page = (CALENDAR_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
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

			Flash_Write_Single((CALENDAR_ADDR + (8*i)), calendar_data[i]);

		}
		LOG_D("Calendar Values Stored in Flash at Page of Address: 0x%lx\n", (unsigned long)(CALENDAR_ADDR));
		HAL_FLASH_Lock();



	 	uint64_t temp_val;
		memcpy(&temp_val, &data->latitude, sizeof(temp_val));
		Flash_Write(LATITUDE_ADDR, temp_val);
		LOG_D("%lf Stored in Flash at 0x%lx\n", data->latitude, (unsigned long)LATITUDE_ADDR);


		memcpy(&temp_val, &data->longitude, sizeof(temp_val));
		Flash_Write(LONGITUDE_ADDR, temp_val);
		LOG_D("%lf Stored in Flash at 0x%lx\n", data->longitude, (unsigned long)LONGITUDE_ADDR);


		memcpy(&temp_val, &data->altitude, sizeof(temp_val));
		Flash_Write(ALTITUDE_ADDR, temp_val);
		LOG_D("%lf Stored in Flash at 0x%lx\n", data->altitude, (unsigned long)ALTITUDE_ADDR);



		Flash_Write(TIMESTAMP_ADDR, timestamp_data);
		LOG_D("%lx Stored in Flash at 0x%lx\n", (uint64_t)timestamp_data, (unsigned long)TIMESTAMP_ADDR);



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

	raw_val = Flash_ReadDoubleWord(LATITUDE_ADDR);
	memcpy(&data->latitude, &raw_val, sizeof(data->latitude));

	raw_val = Flash_ReadDoubleWord(LONGITUDE_ADDR);
	memcpy(&data->longitude, &raw_val, sizeof(data->longitude));

	raw_val = Flash_ReadDoubleWord(ALTITUDE_ADDR);
	memcpy(&data->altitude, &raw_val, sizeof(data->altitude));


	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->latitude, (unsigned long)LATITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->longitude, (unsigned long)LONGITUDE_ADDR);
	LOG_D("%lf Retrieved from Flash of Register Address: 0x%lx\n", data->altitude, (unsigned long)ALTITUDE_ADDR);



	uint64_t timestamp_data = ((uint64_t)Flash_Read(TIMESTAMP_ADDR + 4) << 32) | Flash_Read(TIMESTAMP_ADDR);

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
		calendar_data[i] = ((uint64_t)Flash_Read((CALENDAR_ADDR + (8 * i)) + 4) << 32) | Flash_Read(CALENDAR_ADDR + (8 * i));
		data_2[i].Day = calendar_data[i] & 0xFF;
		data_2[i].Month = (calendar_data[i] >> 8) & 0xFF;
		data_2[i].Schedule = (calendar_data[i] >> 16) & 0xFF;
		LOG_D("Retrieved Calendar from Flash: %d:%d:%c\n",data_2[i].Month, data_2[i].Day, data_2[i].Schedule);

	}


}

// -----------------------------------------------------------------------------
//  CALENDAR-ONLY FLASH FUNCTIONS
// -----------------------------------------------------------------------------

void Flash_Multiple_Write_Calendar(DAY *data_2)
{
    HAL_FLASH_Unlock();

    // Erase calendar page
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Page = (CALENDAR_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
    eraseInit.NbPages = 1;
    HAL_FLASHEx_Erase(&eraseInit, &pageError);

    // Write all 15 DAY entries
    for (int i = 0; i < 15; i++)
    {
        uint64_t calendar_data =
            ((uint64_t)0 << 56) |
            ((uint64_t)0 << 48) |
            ((uint64_t)0 << 40) |
            ((uint64_t)0 << 32) |
            ((uint64_t)0 << 24) |
            ((uint64_t)data_2[i].Schedule << 16) |
            ((uint64_t)data_2[i].Month << 8) |
            ((uint64_t)data_2[i].Day);

        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                          (CALENDAR_ADDR + (8 * i)),
                          calendar_data);

        LOG_D("Calendar[%d]: %02d/%02d '%c' written @0x%lx\n",
              i, data_2[i].Month, data_2[i].Day, data_2[i].Schedule,
              (unsigned long)(CALENDAR_ADDR + (8 * i)));
    }

    HAL_FLASH_Lock();
    LOG_D(" Calendar-only Flash write complete.\n");
}

void Flash_Read_Multiple_Calendar(DAY *data_2)
{
    uint64_t calendar_data[15];

    for (int i = 0; i < 15; i++)
    {
        calendar_data[i] =
            ((uint64_t)Flash_Read((CALENDAR_ADDR + (8 * i)) + 4) << 32) |
            Flash_Read(CALENDAR_ADDR + (8 * i));

        data_2[i].Day      = calendar_data[i] & 0xFF;
        data_2[i].Month    = (calendar_data[i] >> 8) & 0xFF;
        data_2[i].Schedule = (calendar_data[i] >> 16) & 0xFF;

        LOG_D("Calendar[%d] read: %02d/%02d '%c'\n",
              i, data_2[i].Month, data_2[i].Day, data_2[i].Schedule);
    }

    LOG_D(" Calendar-only Flash read complete.\n");
}

