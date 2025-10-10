#ifndef ROM_FLASH_IO_H
#define ROM_FLASH_IO_H

#include <stddef.h>
#include <stdbool.h>
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

const esp_partition_t* findRomPartition(const char* name);
bool eraseRomPartition(const esp_partition_t* part, size_t bytes_to_write);
bool copyFileToPartition(const char* srcPath, const esp_partition_t* part, size_t* outSize);

#ifdef __cplusplus
}
#endif

#endif /* ROM_FLASH_IO_H */
