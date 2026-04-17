#include "data_storage.h"
#include "main.h"
#include "app.h"
#include "MCP7940.h"
#include <string.h>
#include <stddef.h>

#define MAX_RECORDS   (PAGE_SIZE / sizeof(storage_page_t))

static storage_page_t g_storage;
static bool g_storage_valid = false;
static bool g_dirty = false;
static uint32_t g_write_index = 0;

// ---------------- FLASH HELPERS ----------------

static void flash_unlock(void)
{
    HAL_FLASH_Unlock();
}

static void flash_lock(void)
{
    HAL_FLASH_Lock();
}

static bool flash_erase_page(uint32_t address)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Page = (address - FLASH_BASE) / FLASH_PAGE_SIZE;
    erase.NbPages = 1;

    return (HAL_FLASHEx_Erase(&erase, &error) == HAL_OK);
}

static bool flash_write_buffer(uint32_t address, uint8_t *data, uint32_t len)
{
    if (len % 8 != 0) return false;         // must align

    for (uint32_t i = 0; i < len; i += 8)
    {
        uint64_t d;
        memcpy(&d, &data[i], 8);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                              address + i,
                              d) != HAL_OK)
        {
            return false;
        }
    }
    return true;
}

// ---------------- CRC ----------------

static uint32_t simple_crc32(uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
    return crc;
}

// ---------------- SCAN PAGE ----------------

static bool storage_scan(storage_page_t *latest, uint32_t *next_index)
{
    storage_page_t *ptr = (storage_page_t*)STORAGE_PAGE_ADDR;

    storage_page_t *last_valid = NULL;
    uint32_t i;

    for (i = 0; i < MAX_RECORDS; i++)
    {
        // Empty slot → stop
        if (ptr[i].magic == 0xFFFFFFFF)
            break;

        // Invalid record → stop
        if (ptr[i].magic != STORAGE_MAGIC)
            break;

        uint32_t crc = simple_crc32((uint8_t*)&ptr[i],
                                    offsetof(storage_page_t, crc));

        if (crc == ptr[i].crc)
        {
            last_valid = &ptr[i];
        }
        else
        {
            // Corrupted write → stop
            break;
        }
    }

    if (next_index) *next_index = i;

    if (!last_valid)
        return false;

    memcpy(latest, last_valid, sizeof(*latest));
    return true;
}

// ---------------- PRINT ----------------

void storage_print_metadata(void)
{
    if (!g_storage_valid)
    {
        LOG_D("STORAGE: INVALID\r\n");
        return;
    }

    LOG_D("----- STORAGE DUMP -----\r\n");

    LOG_D("Sleep State: 0x%08lX\r\n", g_storage.sleep_state);
    LOG_D("Lat: %.6f Lon: %.6f Alt: %.2f\r\n",
          g_storage.meta.latitude,
          g_storage.meta.longitude,
          g_storage.meta.altitude);

    LOG_D("Date: %02u/%02u/%04u Time: %02u:%02u:%02u.%02u\r\n",
          g_storage.meta.day,
          g_storage.meta.month,
          g_storage.meta.year,
          g_storage.meta.hour,
          g_storage.meta.minute,
          g_storage.meta.second,
          g_storage.meta.centisecond);

    LOG_D("------------------------\r\n");
}

// ---------------- FORMAT ----------------

bool storage_format(void)
{
    memset(&g_storage, 0, sizeof(g_storage));       // no need to set to 0xFF, we write full pages and increment index on next commit

    g_storage.magic = STORAGE_MAGIC;
    g_storage.sleep_state = STORAGE_DEFAULT_SLEEP_STATE;

    DateTime t(__DATE__, __TIME__);                 // Compile time as default value for metadata
    g_storage.meta.day = t.day();
    g_storage.meta.month = t.month();
    g_storage.meta.year = t.year();
    g_storage.meta.hour = t.hour();
    g_storage.meta.minute = t.minute();
    g_storage.meta.second = t.second();

    apply_timezone_offset(&g_storage.meta, TIMEZONE_HOUR_OFFSET, TIMEZONE_MINUTE_OFFSET);

    g_storage.crc = simple_crc32((uint8_t*)&g_storage,
                                 offsetof(storage_page_t, crc));

    flash_unlock();

    if (!flash_erase_page(STORAGE_PAGE_ADDR))
    {
        flash_lock();
        return false;
    }

    bool ok = flash_write_buffer(STORAGE_PAGE_ADDR,
                                 (uint8_t*)&g_storage,
                                 sizeof(storage_page_t));

    flash_lock();

    if (ok)
    {
        g_storage_valid = true;
        g_dirty = false;
        g_write_index = 1; // next write slot
    }

    return ok;
}

// ---------------- INIT ----------------

bool storage_init(void)
{
    storage_page_t latest;

    if (storage_scan(&latest, &g_write_index))
    {
        g_storage = latest;
        g_storage_valid = true;
    }
    else
    {
        g_storage_valid = false;
        g_write_index = 0;
        return false;
    }

    g_dirty = false;
    return true;
}

// ---------------- COMMIT ----------------

bool storage_commit(void)
{
    if (!g_dirty) return true;

    storage_page_t rec;

    rec.magic = STORAGE_MAGIC;
    rec.sleep_state = g_storage.sleep_state;
    rec.meta = g_storage.meta;

    rec.crc = simple_crc32((uint8_t*)&rec,
                           offsetof(storage_page_t, crc));

    flash_unlock();

    // If page full → erase and restart
    if (g_write_index >= MAX_RECORDS)
    {
        if (!flash_erase_page(STORAGE_PAGE_ADDR))
        {
            flash_lock();
            return false;
        }

        g_write_index = 0;
    }

    uint32_t addr = STORAGE_PAGE_ADDR +
                    g_write_index * sizeof(storage_page_t);

    bool ok = flash_write_buffer(addr,
                                 (uint8_t*)&rec,
                                 sizeof(storage_page_t));

    flash_lock();

    if (ok)
    {
        g_write_index++;
        g_dirty = false;
        g_storage_valid = true;
    }

    return ok;
}

// ---------------- HIGH LEVEL ----------------

uint32_t storage_get_sleep_state(void)
{
    return g_storage.sleep_state;
}

void storage_set_sleep_state(uint32_t state)
{
    if (g_storage.sleep_state != state)
    {
        g_storage.sleep_state = state;
        g_dirty = true;
    }
}

bool storage_get_metadata(Storage_data_t *out)
{
    if (!g_storage_valid) return false;

    *out = g_storage.meta;
    return true;
}

void storage_set_metadata(const Storage_data_t *meta)
{
    if (memcmp(&g_storage.meta, meta, sizeof(*meta)) != 0)
    {
        g_storage.meta = *meta;
        g_dirty = true;
    }
}