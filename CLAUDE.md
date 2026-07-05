# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This is an STM32G030XX embedded project using CMake with ARM toolchain.

```bash
# Debug build with ARM GCC
cd /Users/zulin/Dev/BLINK32
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Release build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

**Note**: Build requires `arm-none-eabi-gcc` toolchain installed and in PATH. Native macOS clang will fail to compile.

## Architecture

### Hardware Peripherals
- **TIM14** (PA4): LEDA PWM output -灯光A
- **TIM17** (PA7): LEDB PWM output - 灯光B
- **TIM16** (PA6): BEEP PWM output - 蜂鸣器
- **RTC**: LSE crystal (32768Hz) - 实时时钟+闹铃
- **USART1** (PA9/PA10): 串口通信
- **PA0**: 按键输入 + 待机唤醒 (WKUP1)

### State Machine
Three light states defined in `Core/Inc/main.h`:
- `LIGHT_OFF`: PWM off, system in standby
- `LIGHT_DIM`: 10% duty cycle, auto-off after 30min (DIM_TIMEOUT_MS)
- `LIGHT_BRIGHT`: 50% duty cycle, manual off only

Key timing constants:
- `SHORT_PRESS_MIN_MS`: 500ms minimum
- `SHORT_PRESS_MAX_MS`: 2000ms maximum
- `LONG_PRESS_MS`: 1500ms for bright mode
- `DIM_TIMEOUT_MS`: 1800000 (30 minutes)

### Low-Power Design
System uses **Standby mode** for power savings. Wakeup sources:
1. PA0 rising edge (按键唤醒)
2. RTC alarm (闹铃唤醒)
3. Other sources

**Important**: On wakeup from standby, system reinitializes all peripherals and starts in LIGHT_DIM mode.

### Key Files
- `Core/Src/main.c`: Main loop, state machine, `PlayBeepSound()`, serial commands
- `Core/Src/rtc.c`: RTC init, `HAL_RTC_AlarmAEventCallback()`
- `Core/Src/gpio.c`: PA0 EXTI interrupt for button
- `Core/Src/tim.c`: TIM14/16/17 PWM configuration
- `Core/Inc/main.h`: Pin definitions, timing constants, state enum

### Serial Protocol
Commands via UART1 (115200 baud):
- `TIME=HH:MM:SS` - Set time
- `DATE=YYYY-MM-DD` - Set date
- `ALARM=HH:MM:SS` - Set daily alarm
- `ALARM=OFF` - Cancel alarm
- `BEEP=1` - Test buzzer

### Wakeup Detection
`wakeupSource` variable tracks how system started:
- `WAKEUP_SOURCE_RESET`: Cold boot
- `WAKEUP_SOURCE_BUTTON`: Wake from standby via PA0
- `WAKEUP_SOURCE_ALARM`: Wake from standby via RTC alarm
- `WAKEUP_SOURCE_OTHER`: Other wakeup source

On startup, check `__HAL_PWR_GET_FLAG(PWR_FLAG_SB)` to detect standby wake.

### Beep Playback
`PlayBeepSound()` plays a 5-note melody (C5-E5-G5-C6-G5). Must NOT be called from interrupt context - uses `playBeepFlag` + main loop pattern.

## Code Style
- HAL library usage (STM32Cube)
- Chinese comments in user code sections (`/* USER CODE BEGIN */`)
- State machine pattern for light control
