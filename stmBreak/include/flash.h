#ifndef __FLASH_H
#define __FLASH_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

typedef enum FlashSize
{
 FLASH_8B = 1,
 FLASH_16B = 2,
 FLASH_32B = 4,
 FLASH_64B = 8
} FlashSize;

typedef struct FlashBank
{
  uint32_t id;        // bank id
  uint32_t sector;    // flash sector
  uint32_t size;      // in items
  FlashSize itemSize; // FLASH_8B, FLASH_16B, FLASH_32B, FLASH_64B
} FlashBank;

void setupFlash(void);
FlashBank createFlashBank(const uint32_t size, const FlashSize itemSize);
void readFromFlashBank(void* data, const uint32_t size,
                       const FlashBank* bank);
void writeToFlashBank(void* data, const uint32_t size,
                      const FlashBank* bank);
//void eraseFromFlashBank(const uint32_t size, const FlashBank* bank, const uint32_t offset);

#ifdef __cplusplus
 }
#endif

#endif /* __FLASH_H */
