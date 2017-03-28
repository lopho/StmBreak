#include "flash.h"
#include "cmsis_device.h"
#include "diag/Trace.h"
#include <stdlib.h>

static void Flash_Error_Handler(void);

static void eraseFlashSectors(uint32_t begin, uint32_t end);
static uint32_t getSectorSize(uint32_t Sector);
static uint32_t getSector(uint32_t address);
static void testFlash(void);

static void FLASH_Init(void);

static uint32_t FLASH_SECTORS[12] =
{
  ((uint32_t)0x08000000), /* Base @ of Sector 0, 16 Kbytes */// used by program
  ((uint32_t)0x08004000), /* Base @ of Sector 1, 16 Kbytes */// used by program
  ((uint32_t)0x08008000), /* Base @ of Sector 2, 16 Kbytes */// used by program
  ((uint32_t)0x0800C000), /* Base @ of Sector 3, 16 Kbytes */// used by program
  ((uint32_t)0x08010000), /* Base @ of Sector 4, 64 Kbytes */
  ((uint32_t)0x08020000), /* Base @ of Sector 5, 128 Kbytes */
  ((uint32_t)0x08040000), /* Base @ of Sector 6, 128 Kbytes */
  ((uint32_t)0x08060000), /* Base @ of Sector 7, 128 Kbytes */
  ((uint32_t)0x08080000), /* Base @ of Sector 8, 128 Kbytes */
  ((uint32_t)0x080A0000), /* Base @ of Sector 9, 128 Kbytes */
  ((uint32_t)0x080C0000), /* Base @ of Sector 10, 128 Kbytes */
  ((uint32_t)0x080E0000), /* Base @ of Sector 11, 128 Kbytes */
};

static uint32_t FLASH_USER_BEGIN = 0x08020000; // sector 5-11
static uint32_t FLASH_USER_END = 0x080FFFFF; // last K address of flash block

static uint32_t bankFreeSector = 0;
static uint32_t bankNextSector = 5;
static uint32_t bankNextId = 0;


void setupFlash(void)
{
  FLASH_Init();
}

FlashBank createFlashBank(const uint32_t size, const FlashSize itemSize)
{
  FlashBank bank;
  bank.id = bankNextId;
  bankNextId++;
  bank.sector = bankNextSector;
  bank.size = size;
  bank.itemSize = itemSize;

  bankNextSector++;
  bankFreeSector--;

  return bank;
}

void readFromFlashBank(void* data, const uint32_t size, const FlashBank* bank)
{
  if ((size * bank->itemSize) > (bank->size * bank->itemSize))
  {
    return;
  }

  __IO uint32_t begin = FLASH_SECTORS[bank->sector];
  const uint32_t end = begin + (size * bank->itemSize);

  while (begin < end)
  {
    switch (bank->itemSize)
    {
      case FLASH_8B:
        *((uint8_t*)data) = *(uint8_t*)begin;
        break;
      case FLASH_16B:
        *((uint16_t*)data) = *(uint16_t*)begin;
        break;
      case FLASH_32B:
        *((uint32_t*)data) = *(uint32_t*)begin;
        break;
      case FLASH_64B:
        *((uint64_t*)data) = *(uint64_t*)begin;
        break;
      default:
        break;
    }
    begin = begin + bank->itemSize;
    data = (void*)(((uint8_t*)data) + bank->itemSize);
  }
}

void writeToFlashBank(void* data, const uint32_t size,
                      const FlashBank* bank)
{
  if ((size * bank->itemSize) > (bank->size * bank->itemSize))
  {
    return;
  }

  __IO uint32_t begin = FLASH_SECTORS[bank->sector];
  const uint32_t end = begin + (size * bank->itemSize);

  eraseFlashSectors(begin, end);

  HAL_StatusTypeDef unlockstat = HAL_FLASH_Unlock();
  if (unlockstat == HAL_ERROR)
  {
    Flash_Error_Handler();
  }

  HAL_StatusTypeDef flashStatus = HAL_ERROR;
  while (begin < end)
  {
    switch (bank->itemSize)
    {
      case FLASH_8B:
        flashStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, begin, *(uint8_t*)data);
        break;
      case FLASH_16B:
        flashStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, begin, *(uint16_t*)data);
        break;
      case FLASH_32B:
        flashStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, begin, (*(uint32_t*)data));
        break;
      case FLASH_64B:
        flashStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, begin, *(uint64_t*)data);
        break;
      default:
      {
        trace_printf("Unknown item size %i\n", bank->itemSize);
        Flash_Error_Handler();
        break;
      }
    }
    if (HAL_OK == flashStatus)
    {
      begin = begin + bank->itemSize;
      data = (void*)(((uint8_t*)data) + bank->itemSize);
    }
    else
    {
      Flash_Error_Handler();
    }
  }

  HAL_FLASH_Lock();
}


static void FLASH_Init(void)
{
  bankFreeSector = 11-5;
  bankNextSector = 5;
}

static void eraseFlashSectors(uint32_t begin, uint32_t end)
{
  uint32_t FirstSector = 0, NbOfSectors = 0;
  uint32_t SectorError = 0;
  FLASH_EraseInitTypeDef EraseInitStruct;

  /* Get the 1st sector to erase */
  FirstSector = getSector(begin);
  /* Get the number of sector to erase from 1st sector*/
  NbOfSectors = getSector(end) - FirstSector + 1;

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = FirstSector;
  EraseInitStruct.NbSectors = NbOfSectors;

  HAL_FLASH_Unlock();

  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
  {
    Flash_Error_Handler();
  }

  FLASH_FlushCaches();

  HAL_FLASH_Lock();
}

static uint32_t getSector(uint32_t address)
{
  uint32_t sector = 0;

  if((address < FLASH_SECTORS[1]) && (address >= FLASH_SECTORS[0]))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((address < FLASH_SECTORS[2]) && (address >= FLASH_SECTORS[1]))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((address < FLASH_SECTORS[3]) && (address >= FLASH_SECTORS[2]))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((address < FLASH_SECTORS[4]) && (address >= FLASH_SECTORS[3]))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((address < FLASH_SECTORS[5]) && (address >= FLASH_SECTORS[4]))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((address < FLASH_SECTORS[6]) && (address >= FLASH_SECTORS[5]))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((address < FLASH_SECTORS[7]) && (address >= FLASH_SECTORS[6]))
  {
    sector = FLASH_SECTOR_6;
  }
  else if((address < FLASH_SECTORS[8]) && (address >= FLASH_SECTORS[7]))
  {
    sector = FLASH_SECTOR_7;
  }
  else if((address < FLASH_SECTORS[9]) && (address >= FLASH_SECTORS[8]))
  {
    sector = FLASH_SECTOR_8;
  }
  else if((address < FLASH_SECTORS[10]) && (address >= FLASH_SECTORS[9]))
  {
    sector = FLASH_SECTOR_9;
  }
  else if((address < FLASH_SECTORS[11]) && (address >= FLASH_SECTORS[10]))
  {
    sector = FLASH_SECTOR_10;
  }
  else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11) */
  {
    sector = FLASH_SECTOR_11;
  }
  return sector;
}

/**
  * @brief  Gets sector Size
  * @param  None
  * @retval The size of a given sector
  */
static uint32_t getSectorSize(uint32_t Sector)
{
  uint32_t sectorsize = 0x00;

  if((Sector == FLASH_SECTOR_0) || (Sector == FLASH_SECTOR_1) || (Sector == FLASH_SECTOR_2) || (Sector == FLASH_SECTOR_3))
  {
    sectorsize = 16 * 1024;
  }
  else if(Sector == FLASH_SECTOR_4)
  {
    sectorsize = 64 * 1024;
  }
  else
  {
    sectorsize = 128 * 1024;
  }
  return sectorsize;
}

static void testFlash(void)
{
  uint32_t begin = 0;
  uint32_t end = 0;
  __IO uint32_t data32 = 0;

  FlashBank testBank = createFlashBank(1000, FLASH_32B);
  uint32_t testData[1000];

  for (uint32_t i = 0; 1000 > i; ++i)
  {
    testData[i] = (uint32_t)0xFF;
  }
  trace_printf("Writing test data to flash...0xff\n");
  writeToFlashBank(testData, 1000, &testBank);

  trace_printf("Testing data from flash...\n");

  begin = FLASH_USER_BEGIN;
  end = begin + 1000*4;
  data32 = 0;

  while (begin < end)
  {
    data32 = *(__IO uint32_t*)begin;
    if (data32 != (uint32_t)0xFF)
    {
      trace_printf("%X", data32);
      trace_printf("Wrong data read at %X\n", begin);
      Flash_Error_Handler();
    }

    begin = begin + 4;
  }

  for (uint32_t i = 0; 1000 > i; ++i)
  {
    testData[i] = (uint32_t)0x12345678;
  }
  trace_printf("Writing test data to flash...0x12345678\n");
  writeToFlashBank(testData, 1000, &testBank);

  trace_printf("Testing data from flash...\n");

  begin = FLASH_USER_BEGIN;
  end = begin + 1000*4;
  data32 = 0;

  while (begin < end)
  {
    data32 = *(__IO uint32_t*)begin;
    if (data32 != (uint32_t)0x12345678)
    {
      trace_printf("%X", data32);
      trace_printf("Wrong data read at %X\n", begin);
      Flash_Error_Handler();
    }

    begin = begin + 4;
  }
}

static void Flash_Error_Handler(void)
{
  trace_printf("Flash error: %i", HAL_FLASH_GetError());
  while (1)
  {}
}
