# Calendar Clock

A calendar clock project based on STM32F103C8T6, featuring Gregorian calendar, Chinese Lunar calendar, Heavenly Stems and Earthly Branches (Tian Gan Di Zhi), and Chinese Zodiac display on an OLED screen. Built on FreeRTOS.

## Features

- **Gregorian Calendar**: Year, month, day, hour, minute, second, day of week
- **Chinese Lunar Calendar**: Lunar month/day with leap month support
- **Heavenly Stems & Earthly Branches**: Year, month, day, and hour pillars (Four Pillars / Ba Zi)
- **Chinese Zodiac**: Automatic zodiac year calculation
- **OLED Display**: SSD1306 128x64 via I2C
- **Button Input**: 3 buttons (Add / Mode Switch / Sub) for time setting
- **FreeRTOS Multitasking**: Separate tasks for time, display, and button scanning

## Hardware Requirements

| Component | Model |
|-----------|-------|
| MCU | STM32F103C8T6 (Blue Pill) |
| Display | SSD1306 OLED 128x64 (I2C) |
| Buttons | 3x tactile push buttons |

### Pin Assignment

| Function | Pin |
|----------|-----|
| I2C SCL | PB6 |
| I2C SDA | PB7 |
| Button + (ADD) | PB12 |
| Button Mode (SW) | PB13 |
| Button - (SUB) | PB14 |

## Software Architecture

The project runs on **FreeRTOS** with three tasks:

| Task | Priority | Stack Size | Description |
|------|----------|------------|-------------|
| TimeTask | Normal | 256 words | Time calculation and calendar refresh |
| OLEDTask | Low | 128 words | OLED screen display update |
| ButtonTask | Low | 128 words | Button scanning with debouncing |

## Development Environment

- **IDE**: STM32CubeIDE
- **HAL Library**: STM32F1xx HAL Driver
- **RTOS**: FreeRTOS (CMSIS-RTOS V2)
- **Compiler**: GCC (ARM)

## Project Structure

```
├── Core/
│   ├── Inc/          # Header files
│   │   ├── calendar.h    # Calendar module (Gregorian/Lunar/Stems-Branches)
│   │   ├── oled.h        # OLED driver
│   │   ├── button.h      # Button driver
│   │   ├── font.h        # Font data
│   │   └── ...
│   ├── Src/          # Source files
│   │   ├── calendar.c    # Calendar core logic
│   │   ├── oled.c        # OLED driver implementation
│   │   ├── button.c      # Button driver implementation
│   │   ├── freertos.c    # FreeRTOS task definitions
│   │   ├── main.c        # Main entry point
│   │   └── ...
│   └── Startup/      # Startup files
├── Drivers/          # STM32 HAL Drivers & CMSIS
├── Middlewares/      # FreeRTOS middleware
├── STM32F103C8TX_FLASH.ld  # Linker script
└── testcount.ioc    # STM32CubeMX configuration
```

## Build & Flash

1. Open the project in STM32CubeIDE
2. Import the `testcount.ioc` configuration
3. Build the project (Project -> Build All)
4. Flash to STM32F103C8T6 via ST-Link

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).

Copyright (C) 2026 思渡鸢 (Siduyuan)

## Acknowledgments

- OLED driver based on [波特律动](https://github.com/keysking)'s SSD1306 library
- Uses STMicroelectronics STM32Cube HAL Library
- Built on FreeRTOS real-time operating system
