#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


int xip_map_rom_partition(const char *part_name, size_t rom_size_effective);
const uint8_t* get_rom_ptr(void);
size_t get_rom_size(void);
void xip_unmap(void);

#ifdef __cplusplus
}
#endif
