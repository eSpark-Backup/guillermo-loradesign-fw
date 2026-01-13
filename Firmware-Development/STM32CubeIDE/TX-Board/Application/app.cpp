
#include "app.h"
#include "main.h"
#include "usart.h"
#include "gps.h"
#include "rtc.h"
#include "subghz_phy_app.h"
#include "app_subghz_phy.h"
#include <string.h>
#include "stm32_lpm_if.h"
#include "radio.h"
#include "timer.h"
#include "MCP7940.h"
#include <cstdio>
#include "flash_data.h"

BoardState state = BOOTUP_STATE;
static uint8_t currentIndex = 0;  // holds the RTC schedule index globally

static const uint16_t days_before_month[12] =
{
    0,   // Jan
    31,  // Feb
    59,  // Mar
    90,  // Apr
    120, // May
    151, // Jun
    181, // Jul
    212, // Aug
    243, // Sep
    273, // Oct
    304, // Nov
    334  // Dec
};

static uint32_t datetime_to_year_seconds(const DateTime& t)
{
    uint32_t days =
        days_before_month[t.month() - 1] +
        (t.day() - 1);

    return days * 86400UL +
           t.hour() * 3600UL +
           t.minute() * 60UL +
           t.second();
}

static uint32_t schedule_to_year_seconds(const DAY& d, uint8_t start_hour)
{
    uint32_t days =
        days_before_month[d.Month - 1] +
        (d.Day - 1);

    return days * 86400UL +
           start_hour * 3600UL;
}

#define RTC_BKP_MAGIC      RTC_BKP_DR0
#define RTC_BKP_INDEX      RTC_BKP_DR1
#define RTC_MAGIC_VALUE    0x5A5A


static GPS gps;
TimerEvent_t SleepTimeoutTimer;
static uint32_t shutoffCount = 0;
uint32_t shutOffStartTime = 0;
MCP7940_Class  MCP7940;
int Received_Data = 0;

DAY days[15] = {{1,16,'A'},{1,1,'B'},{1,3,'A'},{1,4,'A'},{1,5,'A'},{1,6,'A'},{1,7,'A'},{1,8,'A'},{1,9,'A'},{1,10,'A'},{1,11,'A'},{1,12,'A'},{1,13,'A'},{1,14,'A'},{1,15,'A'}};
//DAY days[15];

static uint8_t rtc_index_read(void)
{
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_MAGIC) != RTC_MAGIC_VALUE)
    {
        // First ever boot
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_MAGIC, RTC_MAGIC_VALUE);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_INDEX, 0);
        return 0;
    }

    return (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_INDEX);
}

static void rtc_index_write(uint8_t index)
{
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_MAGIC, RTC_MAGIC_VALUE);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_INDEX, index);
}






uint32_t calculate_sleep_from_index(uint8_t index)
{
    DateTime now = MCP7940.now();

    DAY* d = &days[index];
    uint8_t start_hour = 0;
    switch (d->Schedule)
    {
        case 'A': start_hour = SCHEDULE_TIME_RANGE_A_1; break;
        case 'B': start_hour = SCHEDULE_TIME_RANGE_B_1; break;
        case 'C': start_hour = SCHEDULE_TIME_RANGE_C_1; break;
        case 'D': start_hour = SCHEDULE_TIME_RANGE_D_1; break;
        default:  return 60; // safety
    }

    // Construct target time for this schedule
    DateTime targetTime(now.year(), d->Month, d->Day, start_hour, 0, 0);

    // If target already passed, use next year
    if (targetTime.unixtime() <= now.unixtime())
    {
        targetTime = DateTime(now.year() + 1, d->Month, d->Day, start_hour, 0, 0);
    }

    uint32_t sleep_sec = targetTime.unixtime() - now.unixtime();
    return sleep_sec;
}




void generatePayload(uint8_t* buffer, uint32_t size)
{
    char* payload = (char*) buffer;

    // Compose header
    memcpy(payload, MY_IDENTIFIER, strlen(MY_IDENTIFIER));

    // Build CommPacket
    CommPacket packet;
#if CUSTOM_BOARD_ID
    packet.ids[0] = BOARD_ID_1;
    packet.ids[1] = BOARD_ID_2;
    packet.ids[2] = BOARD_ID_2;
#else
    uint32_t boardIds[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    memcpy(packet.ids, boardIds, sizeof(packet.ids));
#endif

    // Read GNSS snapshot from flash (Flash_Read_Multiple fills both FlashData_t and days[])
    FlashData_t flashStoredData;
    DAY tmpDays[15]; // temporary, not used here
    Flash_Read_Multiple(&flashStoredData, tmpDays);

    // Populate timestamp & coordinates from flash snapshot
    packet.timestamp.hour = flashStoredData.hour;
    packet.timestamp.minute = flashStoredData.minute;
    packet.timestamp.second = flashStoredData.second;

    packet.geoCoordinates.lattitude = flashStoredData.latitude;
    packet.geoCoordinates.longitude  = flashStoredData.longitude;
    packet.geoCoordinates.altitude   = flashStoredData.altitude;

    // Copy packet after identifier
    memcpy(payload + strlen(MY_IDENTIFIER), &packet, sizeof(packet));
}



//void generatePayload(uint8_t* buffer, uint32_t size)
//{
//	char* payload = (char*) buffer;
//
//	CommPacket packet;
//	memcpy(payload, MY_IDENTIFIER, strlen(MY_IDENTIFIER));
//#if CUSTOM_BOARD_ID
//	packet.ids[0] = BOARD_ID_1;
//	packet.ids[1] = BOARD_ID_2;
//	packet.ids[2] = BOARD_ID_2;
//#else
//	uint32_t boardIds[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
//	memcpy(packet.ids, boardIds, sizeof(packet.ids));
//#endif
////	 RtcData_t rtcStoredData;
////	 GPS::getGnssData(&rtcStoredData);
//
//
//	 FlashData_t flashStoredData;
//	 GPS::getGnssData(&flashStoredData);
//
//	packet.timestamp.hour = gps.gnssData.hour;
//	packet.timestamp.minute = gps.gnssData.minute;
//	packet.timestamp.second = gps.gnssData.second;
//	packet.geoCoordinates.lattitude = gps.gnssData.latitude;
//	packet.geoCoordinates.longitude = gps.gnssData.longitude;
//	packet.geoCoordinates.altitude = gps.gnssData.altitude;
//	memcpy(payload + strlen(MY_IDENTIFIER), &packet, sizeof(packet));
//}

void UpTimerEventCallback( void *context )
{
    // This callback is executed when the RTC wakeup timer expires
    LOG_D("Woke up from STOP mode via RTC Wakeup Timer\r\n");
}

void sleep(uint32_t timeout)
{
//	uint32_t seconds = timeout/1000;
	uint32_t seconds = timeout;
	//seconds = 20;
	LOG_D("Going to sleep for %d seconds\r\n", seconds);
	HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	Radio.Sleep();

	extern TimerEvent_t RxTimeoutTimer;
	extern TimerEvent_t TxTimeoutTimer;
	TimerStop( &RxTimeoutTimer );
	TimerStop( &TxTimeoutTimer );
	TimerStop( &SleepTimeoutTimer );

	extern SUBGHZ_HandleTypeDef hsubghz;
	HAL_SUBGHZ_DeInit(&hsubghz);

	DateTime now = MCP7940.now();
	DateTime alarmTime = now + TimeSpan(seconds);

	MCP7940.setAlarm(0, matchAll, alarmTime, true);  // Set one-time alarm

	LOG_D("Alarm 0 set for: %04d-%02d-%02d %02d:%02d:%02d\r\n",
		   alarmTime.year(), alarmTime.month(), alarmTime.day(),
		   alarmTime.hour(), alarmTime.minute(), alarmTime.second());

	PWR_EnterOffMode();
	PWR_ExitOffMode();

	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

	LOG_D("Starts Receiving Data\r\n");
	Radio.Rx(RCV_TIMEOUT_VALUE);
}

extern "C" void receiveTimeout(void)
{
	switch(state)
	{
	case BOOTUP_STATE:
	{
		LOG_D("No Signal Received\r\n");
//		 sData_t rtcStoredData;
//		 GPS::getGnssData(&rtcStoredData);
//		 gps.gnssData.hour;


		FlashData_t flashStoredData;
		GPS::getGnssData(&flashStoredData);
		gps.gnssData.hour;

		DateTime now = MCP7940.now();

		if(now.hour() >= HOUR_THRESHOLD)
		{
			LOG_D("Hour threshold exceeded. ");
			LOG_D("Sleeping for a day\r\n");
			uint32_t val = SLEEP_STATE_DAY;
//			gps.rtcOperation(STORE, SLEEP_ADDR, &val);
			Flash_Write(SLEEP_ADDR, val);
	        LOG_D("%lu Stored in Flash in SLEEP_ADDR\n", val);
//
//			LOG_D("Value stored in Flash: %lu \n", val);
			sleep(calculate_sleep_from_index(currentIndex));
		}
		else
		{
			uint32_t val = SLEEP_STATE_25S;
//			gps.rtcOperation(STORE, SLEEP_ADDR, &val);
			Flash_Write(SLEEP_ADDR, val);
	        LOG_D("%lu Stored in Flash in SLEEP_ADDR\n", val);

//			LOG_D("Value stored in Flash: %lu \n", val);
			sleep(SLEEP_TIMEOUT);
		}
		break;
	}

	default:
		break;
	}

}

extern "C" void dataReceived(char* buffer, uint32_t size)
{
    LOG_D("Data received, size=%lu\n", size);

    if (size == sizeof(days)) {
        // Assume binary calendar update from relay
        memcpy(days, buffer, sizeof(days));
        FlashData_t snapshot;
        DAY oldCalendar[15];
        Flash_Read_Multiple(&snapshot, oldCalendar);
        Flash_Multiple_Write(&snapshot, days);
        LOG_D("Calendar updated from relay\n");

        uint8_t index = rtc_index_read();

        uint32_t sleep_sec = calculate_sleep_from_index(index);

        index = (index + 1) % 15;
        rtc_index_write(index);

        sleep(sleep_sec);

        return;
    }

    if (strcmp(buffer, "HELLO") == 0) {
        LOG_D("HELLO received — replying with HERE+ID\n");
        char reply[32];
        snprintf(reply, sizeof(reply), "%sHERE", MY_IDENTIFIER);
        Radio.Send((uint8_t*)reply, strlen(reply));
        while (!isTXDone());
        MX_SubGHz_Phy_Process();
        SwitchToRxMode();
        return;
    }

    LOG_D("Unhandled message: %s\n", buffer);
}


void setRtcTime()
{
    DateTime currentTime(gps.gnssData.year, gps.gnssData.month, gps.gnssData.day, gps.gnssData.hour, gps.gnssData.minute, gps.gnssData.second);
    MCP7940.adjust(currentTime);
    LOG_D("RTC time set to: %04d-%02d-%02d %02d:%02d:%02d\r\n",
               currentTime.year(), currentTime.month(), currentTime.day(),
               currentTime.hour(), currentTime.minute(), currentTime.second());
    rtc_index_write(0);


}

void rtcModuleInit() {
    // Initialize I2C communication with RTC module
    while (!MCP7940.begin()) {
        HAL_Delay(3000);
    }

    // Ensure the oscillator is running
    while (!MCP7940.deviceStatus()) {
        bool deviceStatus = MCP7940.deviceStart();
        if (!deviceStatus) {
            HAL_Delay(1000);
        }
    }

    // Set the RTC module to the compile time					// This should be changed according to the time received from GNSS module
//     DateTime compileTime(__DATE__, __TIME__);
//     MCP7940.adjust(compileTime);
//     LOG_D("RTC time set to: %04d-%02d-%02d %02d:%02d:%02d\r\n",
//                compileTime.year(), compileTime.month(), compileTime.day(),
//                compileTime.hour(), compileTime.minute(), compileTime.second());

    MCP7940.setAlarmPolarity(false);
}

void loop_test() {
    static uint8_t secs = 0;
    DateTime now = MCP7940.now();  // Get the current time

    if (secs != now.second()) {
        secs = now.second();
        LOG_D("Current RTC time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
                       now.year(), now.month(), now.day(),
                       now.hour(), now.minute(), now.second());
    }

    if (MCP7940.isAlarm(0)) {
            LOG_D("*Alarm 0 Triggered!*\r\n");
            MCP7940.clearAlarm(0);
        }
}



void App::setup(void)
{
    TimerInit(&SleepTimeoutTimer, UpTimerEventCallback);
    TimerStop(&SleepTimeoutTimer);
    rtcModuleInit();
//    Flash_Write(SLEEP_ADDR, 123);
    // Read current schedule index from RTC backup
    currentIndex = rtc_index_read();

    // Read sleep flag (from flash)
    uint32_t flag = Flash_Read(SLEEP_ADDR);
    LOG_D("\nValue Read from Flash: %lu \n", flag);

    // Detect first boot or wake from day sleep
      bool firstBoot = ((flag != SLEEP_STATE_25S) && (flag != SLEEP_STATE_DAY));
    bool wakeFromDaySleep = (flag == SLEEP_STATE_DAY);

    if (firstBoot || wakeFromDaySleep)
    {
        // Setup GPS and acquire valid GNSS fix
        gps.setup(&hrtc, &huart1);
        do
        {
            gps.loop();
        } while ((!gps.gnssSignalReceived) || (!gps.has3DFix()));

        gps.displayInfo();

        // Save GNSS snapshot to flash
        FlashData_t snapshot;
        snapshot.latitude      = gps.gnssData.latitude;
        snapshot.longitude     = gps.gnssData.longitude;
        snapshot.altitude      = gps.gnssData.altitude;
        snapshot.day           = gps.gnssData.day;
        snapshot.month         = gps.gnssData.month;
        snapshot.year          = gps.gnssData.year;
        snapshot.hour          = gps.gnssData.hour;
        snapshot.minute        = gps.gnssData.minute;
        snapshot.second        = gps.gnssData.second;
        snapshot.centisecond   = gps.gnssData.centisecond;

        // Set RTC to current GNSS time
        setRtcTime();

        HAL_GPIO_WritePin(GNSS_PWR_GPIO_Port, GNSS_PWR_Pin, GPIO_PIN_RESET); // Turn off GNSS
    }

    // Store correct sleep flag for next boot
    if (firstBoot)
    {
        Flash_Write(SLEEP_ADDR, SLEEP_STATE_DAY);
        LOG_D("%lu Stored in Flash in SLEEP_ADDR\n", SLEEP_STATE_DAY);
        LOG_D("First boot complete — entering sleep\n");
    }
    else
    {
        // Normal wakeup — prepare for radio receive
        Radio.Rx(RCV_TIMEOUT_VALUE);
    }

    // Calculate sleep duration based on schedule
     uint32_t sleep_seconds = calculate_sleep_from_index(currentIndex);
    // Move to next schedule index for sleep
    currentIndex = (currentIndex + 1) % 15;
    rtc_index_write(currentIndex);

    // Sleep until next scheduled time
    sleep(sleep_seconds);
}


void App::loop(void)
{
	// gps.loop();		// no need to process GNSS data, valid coordinates have been acquired
//	LOG_D("Data Received: %d",Received_Data);

	uint32_t currentTime = HAL_GetTick();
	uint8_t data[64];
	switch(state)
	{
	case SHUTOFF_STATE:
		if((currentTime - shutOffStartTime) > 30000)
		{
			LOG_D("Sleeping for a day\r\n");
			state = BOOTUP_STATE;
			uint32_t val = SLEEP_STATE_DAY;
//			gps.rtcOperation(STORE, SLEEP_ADDR, &val);
			Flash_Write(SLEEP_ADDR, val);
	        LOG_D("%lu Stored in Flash in SLEEP_ADDR\n", val);
//			LOG_D("Value stored in Flash: %lu \n", val);
			sleep(calculate_sleep_from_index(currentIndex));
		}
		else if( ( (currentTime - shutOffStartTime) / 5000U ) > shutoffCount)
		{
			LOG_D("SHUTOFF_STATE Count : %d\r\n", shutoffCount);
			shutoffCount++;
			generatePayload(data, sizeof(data));
			SwitchToTxMode(data, sizeof(data));
			while(!isTXDone());
			MX_SubGHz_Phy_Process();
			SwitchToRxMode();
		}
		break;

	case TRANSMIT_STATE:
	{
		LOG_D("Entering shutoff state\r\n");
		shutOffStartTime = HAL_GetTick();
		shutoffCount = 0;
		uint8_t data[64];
		generatePayload(data, sizeof(data));
		SwitchToTxMode(data, sizeof(data));
		while(!isTXDone());
		MX_SubGHz_Phy_Process();
		SwitchToRxMode();
		state = SHUTOFF_STATE;
	}
		break;

	default:
		break;
	}
}
