#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <stdint.h>
#include "gps.h"   // Storage_data_t

#define PAGE_SIZE         0x800         // 2 KB page size
#define PAGE0_BASE_ADDR   0x0801F800UL  // Start of second last 2KB flash page
#define PAGE1_BASE_ADDR   0x0801F000UL
#define PAGE2_BASE_ADDR   0x0801E800UL
#define PAGE3_BASE_ADDR   0x0801E000UL
#define PAGE4_BASE_ADDR   0x0801D800UL
#define PAGE5_BASE_ADDR   0x0801D000UL
#define PAGE6_BASE_ADDR   0x0801C800UL
#define PAGE7_BASE_ADDR   0x0801C000UL



#define STORAGE_PAGE_ADDR             PAGE7_BASE_ADDR
#define STORAGE_MAGIC                 0xDEADBEEF
#define STORAGE_DEFAULT_SLEEP_STATE   0xFFFFFFFF  // or choose DAY/25S

typedef struct __attribute__((packed, aligned(8))) {
    uint32_t magic;
    uint32_t sleep_state;
    Storage_data_t meta;
    uint32_t crc;
} storage_page_t;


void storage_print_metadata(void);
bool storage_format(void);
bool storage_init(void);
bool storage_commit(void);


uint32_t storage_get_sleep_state(void);
void storage_set_sleep_state(uint32_t state);
bool storage_get_metadata(Storage_data_t *out);
void storage_set_metadata(const Storage_data_t *meta);


#endif // DATASTORAGE_H
