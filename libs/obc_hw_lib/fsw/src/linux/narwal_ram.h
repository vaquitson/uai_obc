#ifndef NARWAL_RAM_C
#define NARWAL_RAM_C

#define NARWAL_RAM_MEMINFO_PATH "/proc/meminfo"
#define NARWAL_RAM_ENTRY_VALUE_MAX 15

typedef long int MemSize_t;

MemSize_t narwal_ram_size(void);
MemSize_t narwal_ram_usage(void);
float narwal_ram_usage_percentage(void);

#define NARWAL_READ_SIZE 250

#define NARWAL_RAM_SUCCESS         0
#define NARWAL_RAM_OPEN_ERR       -1
#define NARWAL_RAM_READ_ERR       -2
#define NARWAL_RAM_KEY_NOT_FOUND  -3
#define NARWAL_RAM_TO_LARGE_ENTRY -4
#define NARWAL_RAM_NULL_BUF       -5
#define NARWAL_RAM_SEEK_RESET_ERR -6



#endif
