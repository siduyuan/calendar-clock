# 日历时钟 (Calendar Clock)

基于 STM32F103C8T6 的日历时钟项目，支持公历、农历、天干地支、生肖显示，使用 OLED 屏幕输出，基于 FreeRTOS 实时操作系统。

## 功能特性

- **公历显示**：年、月、日、时、分、秒、星期
- **农历显示**：农历月日、闰月识别
- **天干地支**：年柱、月柱、日柱、时柱（四柱）
- **生肖显示**：自动计算当年生肖
- **OLED 显示**：SSD1306 128x64 I2C 接口
- **按钮交互**：三个按钮（加 / 模式切换 / 减），支持时间设置
- **FreeRTOS 多任务**：时间任务、OLED 显示任务、按钮扫描任务分离

## 硬件需求

| 组件 | 型号 |
|------|------|
| 主控 | STM32F103C8T6 (Blue Pill) |
| 显示屏 | SSD1306 OLED 128x64 (I2C) |
| 按钮 | 3 个独立按键 |

### 引脚分配

| 功能 | 引脚 |
|------|------|
| I2C SCL | PB6 |
| I2C SDA | PB7 |
| 按钮 + (ADD) | PB12 |
| 按钮 模式 (SW) | PB13 |
| 按钮 - (SUB) | PB14 |

## 软件架构

项目基于 **FreeRTOS** 运行，包含三个任务：

| 任务 | 优先级 | 栈大小 | 说明 |
|------|--------|--------|------|
| TimeTask | Normal | 256 words | 时间计算与日历刷新 |
| OLEDTask | Low | 128 words | OLED 屏幕刷新显示 |
| ButtonTask | Low | 128 words | 按钮扫描与消抖 |

## 开发环境

- **IDE**: STM32CubeIDE
- **HAL 库**: STM32F1xx HAL Driver
- **RTOS**: FreeRTOS (CMSIS-RTOS V2)
- **编译器**: GCC (ARM)

## 项目结构

```
├── Core/
│   ├── Inc/          # 头文件
│   │   ├── calendar.h    # 日历模块（公历/农历/天干地支）
│   │   ├── oled.h        # OLED 驱动
│   │   ├── button.h      # 按钮驱动
│   │   ├── font.h        # 字库
│   │   └── ...
│   ├── Src/          # 源文件
│   │   ├── calendar.c    # 日历核心逻辑
│   │   ├── oled.c        # OLED 驱动实现
│   │   ├── button.c      # 按钮驱动实现
│   │   ├── freertos.c    # FreeRTOS 任务定义
│   │   ├── main.c        # 主程序入口
│   │   └── ...
│   └── Startup/      # 启动文件
├── Drivers/          # STM32 HAL 驱动 & CMSIS
├── Middlewares/      # FreeRTOS 中间件
├── STM32F103C8TX_FLASH.ld  # 链接脚本
└── testcount.ioc    # STM32CubeMX 配置文件
```

## 构建与烧录

1. 使用 STM32CubeIDE 打开本项目
2. 导入 `testcount.ioc` 配置
3. 编译项目 (Project -> Build All)
4. 通过 ST-Link 烧录到 STM32F103C8T6

## 许可证

本项目采用 [GNU General Public License v3.0](LICENSE) 许可证。

Copyright (C) 2026 思渡鸢

## 致谢

- OLED 驱动库基于 [波特律动](https://github.com/keysking) 的 SSD1306 驱动
- 使用 STMicroelectronics STM32Cube HAL 库
- 基于 FreeRTOS 实时操作系统
