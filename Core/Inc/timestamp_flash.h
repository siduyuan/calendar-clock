/**
  ******************************************************************************
  * @file    timestamp_flash.h
  * @brief   Flash 最后一页时间戳备份区库 头文件
  *
  *          功能：在 STM32F103C8Tx(64KB Flash) 的最后一页(1KB, 0x0800FC00~0x0800FFFF)
  *                顺序记录 uint32 时间戳。每次上电/复位后调用 TS_Init() 恢复指针，
  *                接续上次写入；写满后整页擦除，从头再写。
  *
  *          区域划分（最后一页 1024 字节）：
  *            槽位区  0x0800FC00 ~ 0x0800FFBB   255 个 uint32 槽位（依次写入）
  *            魔数区  0x0800FFFC ~ 0x0800FFFF   4 字节格式化标记（区分首次运行）
  *
  *          代码隔离：由链接脚本 STM32F103C8TX_FLASH.ld 强制保证——
  *                    FLASH 长度缩减为 63K，链接器不会把代码/常量放进最后一页。
  ******************************************************************************
  */
#ifndef __TIMESTAMP_FLASH_H
#define __TIMESTAMP_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ==================== 区域与容量定义 ==================== */
/* 最后一页绝对地址（F103C8 共 64 页 × 1KB，最后一页 = 0x0800FC00） */
#define TS_PAGE_BASE    0x0800FC00u   /* 备份区起始地址 */
#define LOG_PAGE_SIZE   1024u         /* 备份区大小：1 页 = 1KB */
#define TS_PAGE_END     (TS_PAGE_BASE + LOG_PAGE_SIZE)   /* 0x08010000（页尾外沿） */

/* 每条记录：一个 uint32，4 字节 */
#define TS_SLOT_BYTES   4u

/* 页尾 4 字节固定存放“已格式化”魔数，用于区分首次运行（本功能刚添加/页内是遗留数据）。
   因此实际数据槽位数 = (1024 - 4) / 4 = 255 个。 */
#define TS_FORMAT_MAGIC       0xA5A55A5Au
#define TS_FORMAT_MAGIC_ADDR  (TS_PAGE_END - 4u)          /* 0x0800FFFC */
#define TS_SLOT_COUNT         255u

/* ==================== 编译期断言 ==================== */
/* 1) 页大小必须为 1024 */
typedef char _assert_page[(LOG_PAGE_SIZE == 1024) ? 1 : -1];
/* 2) 起始地址必须固定为最后一页 0x0800FC00 */
typedef char _assert_addr[(TS_PAGE_BASE == 0x0800FC00u) ? 1 : -1];
/* 3) 起始地址必须 1KB 页对齐 */
typedef char _assert_align[((TS_PAGE_BASE & 0x3FFu) == 0u) ? 1 : -1];
/* 4) 结束地址必须紧贴 64KB Flash 末尾 */
typedef char _assert_end[(TS_PAGE_END == 0x08010000u) ? 1 : -1];
/* 5) 槽位数量与区域布局一致 */
typedef char _assert_slots[(TS_SLOT_COUNT == (LOG_PAGE_SIZE - 4u) / 4u) ? 1 : -1];

/* ==================== 返回值 ==================== */
typedef enum
{
  TS_OK         = 0,   /* 成功 */
  TS_ERR_NO_DATA,      /* 暂无有效数据（页内为空） */
  TS_ERR_FLASH,        /* 底层 FLASH 编程/擦除失败 */
  TS_ERR_PARAM         /* 参数非法 */
} TS_Status;

/* ==================== 函数原型 ==================== */
void      TS_Init(void);                          /* 上电/复位后调用：首次运行自动格式化 + 二分查找恢复指针 */
TS_Status TS_Write(uint32_t value);               /* 顺序写入一条时间戳，写满自动擦除后从头写 */
TS_Status TS_ReadLast(uint32_t *out);             /* 读取最后一条有效时间戳 */
uint32_t  TS_GetCount(void);                      /* 当前已写入条数（调试用） */

#ifdef __cplusplus
}
#endif

#endif /* __TIMESTAMP_FLASH_H */
