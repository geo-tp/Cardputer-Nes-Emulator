#include "rom_flash_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_spi_flash.h"

const esp_partition_t* findRomPartition(const char* name) {
  esp_vfs_spiffs_unregister(NULL);
  if (!name) name = "spiffs";
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                  ESP_PARTITION_SUBTYPE_ANY,
                                  name);
}

bool eraseRomPartition(const esp_partition_t* part, size_t bytes_to_write) {
  if (!part) return false;
  const size_t ERASE_BLOCK = 0x10000; // 64 KiB
  size_t to_erase = (bytes_to_write + (ERASE_BLOCK - 1)) & ~(ERASE_BLOCK - 1);
  if (to_erase > part->size) to_erase = part->size;
  esp_err_t err = esp_partition_erase_range(part, 0, to_erase);
  return (err == ESP_OK);
}

bool copyFileToPartition(const char* srcPath, const esp_partition_t* part, size_t* outSize) {
  if (outSize) *outSize = 0;
  if (!srcPath || !part) return false;

  FILE* f = fopen(srcPath, "rb");
  if (!f) return false;

  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
  long fsz = ftell(f);
  if (fsz <= 0) { fclose(f); return false; }
  if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return false; }
  if ((size_t)fsz > part->size) { fclose(f); return false; }

  if (!eraseRomPartition(part, (size_t)fsz)) { fclose(f); return false; }

  uint8_t* buf = (uint8_t*)malloc(8192);
  if (!buf) { fclose(f); return false; }

  size_t written = 0;
  while (written < (size_t)fsz) {
    size_t toRead = (size_t)fsz - written;
    if (toRead > 8192) toRead = 8192;
    size_t r = fread(buf, 1, toRead, f);
    if (r == 0) { free(buf); fclose(f); return false; }

    esp_err_t err = esp_partition_write(part, written, buf, r);
    if (err != ESP_OK) { free(buf); fclose(f); return false; }

    written += r;
  }

  free(buf);
  fclose(f);
  if (outSize) *outSize = written;
  return true;
}
