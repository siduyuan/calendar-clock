/**
  ******************************************************************************
  * @file    timestamp_flash.c
  * @brief   Flash 最后一页时间戳备份区库
  *
  *          区域：0x0800FC00 ~ 0x0800FFFF（64KB Flash 最后一页，1KB）
  *          布局：页首 255 个 uint32 槽位顺序写入，页尾 4B 存放格式化魔数
  *          满页：整页擦除 -> 写回魔数 -> 指针复位页首
  *          上电/复位：TS_Init() 二分查找最后一个非全1槽位，接续写入
  *
  *          安全约定：
  *            - 每次读/写/擦除前均校验目标地址严格落在 [TS_PAGE_BASE, TS_PAGE_END)
  *            - 代码隔离由链接脚本强制（FLASH LENGTH=63K），本库绝不越界访问
  ******************************************************************************
  */

#include "timestamp_flash.h"
#include "stm32f1xx_hal_flash.h"

/* 下一个空闲槽位索引，范围 [0, TS_SLOT_COUNT]。
   仅在主循环上下文访问（无中断修改），无需 volatile。 */
static uint16_t ts_slot_ptr = 0;
static uint8_t  ts_inited   = 0;

/* 槽位 idx 的绝对地址。调用方必须保证 idx < TS_SLOT_COUNT，否则地址越出数据区 */
static uint32_t ts_slot_addr(uint16_t idx)
{
  return TS_PAGE_BASE + ((uint32_t)idx << 2u);
}

/* 读 32 位，使用 volatile 指针避免编译器缓存 */
static uint32_t ts_read32(uint32_t addr)
{
  return *(volatile uint32_t *)addr;
}

/* F1 无 32 位整字编程，按两个半字(16bit)顺序写入：先低半字，后高半字。
   写入前强制校验地址落在备份区内，杜绝野指针/越界。 */
static TS_Status ts_program_word(uint32_t addr, uint32_t value)
{
  TS_Status st = TS_OK;

  /* 严格边界检查：目标区间 [addr, addr+4) 必须完全位于本页内 */
  if ((addr < TS_PAGE_BASE) || ((addr + TS_SLOT_BYTES) > TS_PAGE_END))
  {
    return TS_ERR_PARAM;
  }

  HAL_FLASH_Unlock();

  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr,     (uint64_t)(value & 0xFFFFu)) != HAL_OK)
  {
    st = TS_ERR_FLASH;
  }
  else if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + 2u, (uint64_t)(value >> 16u)) != HAL_OK)
  {
    st = TS_ERR_FLASH;
  }

  HAL_FLASH_Lock();
  return st;
}

/* 整页擦除，并把格式化魔数写回页尾。
   擦除期间关中断：Flash 擦除时若发生中断，CPU 取指会阻塞到擦除结束，关闭可避免时序被拉长。 */
static TS_Status ts_page_erase(void)
{
  FLASH_EraseInitTypeDef erase;
  uint32_t page_err = 0xFFFFFFFFu;
  TS_Status st;

  erase.TypeErase   = FLASH_TYPEERASE_PAGES;
  erase.PageAddress = TS_PAGE_BASE;
  erase.NbPages     = 1;

  HAL_FLASH_Unlock();
  __disable_irq();
  HAL_StatusTypeDef hal_st = HAL_FLASHEx_Erase(&erase, &page_err);
  __enable_irq();

  if ((hal_st != HAL_OK) || (page_err != 0xFFFFFFFFu))
  {
    HAL_FLASH_Lock();
    return TS_ERR_FLASH;
  }
  HAL_FLASH_Lock();

  /* 擦除完成后页内全 1，写回魔数标记本页已被本库格式化 */
  st = ts_program_word(TS_FORMAT_MAGIC_ADDR, TS_FORMAT_MAGIC);
  return st;
}

/* 二分查找最后一个非全 1（有效）槽位，返回下一个空闲槽位索引。
   前提：写入严格按序从索引 0 开始，有效槽位构成连续前缀。
   半开区间 [lo, hi)，hi 初始为 TS_SLOT_COUNT。 */
static uint16_t ts_find_next_free(void)
{
  uint16_t lo = 0;
  uint16_t hi = TS_SLOT_COUNT;

  while (lo < hi)
  {
    uint16_t mid = (uint16_t)((uint16_t)(lo + hi) >> 1u);
    if (ts_read32(ts_slot_addr(mid)) != 0xFFFFFFFFu)
    {
      lo = (uint16_t)(mid + 1u);
    }
    else
    {
      hi = mid;
    }
  }
  return lo;
}

/**
  * @brief 初始化：上电/复位后调用。
  *        1) 首次运行检测——页尾魔数不存在（本功能刚添加、或页内为遗留数据）
  *           时，整页擦除保证页内全 1，再写魔数，从头开始；
  *        2) 二分查找恢复指针，接续上次写入位置。
  * @note  重复调用安全（幂等）。
  */
void TS_Init(void)
{
  if (ts_read32(TS_FORMAT_MAGIC_ADDR) != TS_FORMAT_MAGIC)
  {
    if (ts_page_erase() != TS_OK)
    {
      /* 擦除失败：指针置满，屏蔽后续写入，避免越界 */
      ts_slot_ptr = TS_SLOT_COUNT;
      ts_inited   = 1;
      return;
    }
  }

  ts_slot_ptr = ts_find_next_free();
  ts_inited   = 1;
}

/**
  * @brief  擦除检测（内部）：若指针已指向末尾（全部槽位写满），
  *        则整页擦除、写回魔数，并把指针复位到页首第一个地址。
  *        未满则不做任何操作，返回 TS_OK。
  * @note  仅内部调用，调用前必须保证已初始化（TS_Write 中已确保）。
  */
static TS_Status ts_erase_if_full(void)
{
  if (ts_slot_ptr >= TS_SLOT_COUNT)
  {
    TS_Status st = ts_page_erase();
    if (st != TS_OK)
    {
      return st;
    }
    ts_slot_ptr = 0;
  }
  return TS_OK;
}

/**
  * @brief  顺序写入一条 uint32 时间戳到当前指针位置。
  *        写满时自动先整页擦除、再从头写。
  */
TS_Status TS_Write(uint32_t value)
{
  TS_Status st;

  if (!ts_inited)
  {
    TS_Init();
  }

  /* 满页检测：写满 -> 擦除重来 */
  st = ts_erase_if_full();
  if (st != TS_OK)
  {
    return st;
  }

  st = ts_program_word(ts_slot_addr(ts_slot_ptr), value);
  if (st != TS_OK)
  {
    return st;
  }

  ts_slot_ptr++;
  return TS_OK;
}

/**
  * @brief  读取最后一条有效时间戳。
  *        页内尚无数据时返回 TS_ERR_NO_DATA。
  */
TS_Status TS_ReadLast(uint32_t *out)
{
  if (!ts_inited)
  {
    TS_Init();
  }

  if (out == NULL)
  {
    return TS_ERR_PARAM;
  }
  if (ts_slot_ptr == 0u)
  {
    return TS_ERR_NO_DATA;
  }

  *out = ts_read32(ts_slot_addr((uint16_t)(ts_slot_ptr - 1u)));
  return TS_OK;
}

/**
  * @brief  当前已写入条数（0 ~ TS_SLOT_COUNT）。
  */
uint32_t TS_GetCount(void)
{
  if (!ts_inited)
  {
    TS_Init();
  }
  return (uint32_t)ts_slot_ptr;
}
