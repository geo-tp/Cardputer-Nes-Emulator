#include "rom_xip.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include <stdio.h>

/* -------------------- XIP ROM access -------------------- */

static const uint8_t *g_rom_ptr = NULL;
static size_t g_rom_size = 0;
static spi_flash_mmap_handle_t g_rom_mmap = 0;


int xip_map_rom_partition(const char *part_name, size_t rom_size_effective)
{
    const esp_partition_t *p;
    const void *ptr = NULL;

    if (part_name == NULL)
        part_name = "rom";

    p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, part_name);
    if (!p) {
        ESP_LOGE("ROM_XIP", "Partition '%s' not found", part_name);
        return -1;
    }

    if (esp_partition_mmap(p, 0, p->size, SPI_FLASH_MMAP_DATA, &ptr, &g_rom_mmap) != ESP_OK) {
        ESP_LOGE("ROM_XIP", "esp_partition_mmap() failed");
        return -2;
    }

    g_rom_ptr  = (const uint8_t *)ptr;
    g_rom_size = (rom_size_effective > 0 && rom_size_effective <= p->size) ? rom_size_effective : p->size;

    return 0;
}

const uint8_t* _get_rom_ptr(void) { 
    return g_rom_ptr; 
}


size_t _get_rom_size(void) { 
    return g_rom_size; 
}

void xip_unmap(void)
{
    if (g_rom_mmap) {
        spi_flash_munmap(g_rom_mmap);
        g_rom_mmap = 0;
    }
    g_rom_ptr = NULL;
    g_rom_size = 0;
}
