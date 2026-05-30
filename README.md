# STM32 Temperature Monitor / Alert Prototype

This repository contains a temperature-monitor and alert prototype for the STM32F103C8T6 "Blue Pill" board. The project reads a DHT11/KY-015 temperature sensor every 2 seconds, sends the measured value over UART, and triggers an LED and buzzer when the temperature crosses a threshold.

## Project Description

The system is designed to demonstrate core embedded engineering concepts through a small practical project:

- Read temperature data from a DHT11-compatible sensor on **PA1**
- Output the reading to a PC terminal over **USART1** at **115200 baud**
- Turn on an **LED** connected to **PA2** when the temperature exceeds a limit
- Drive a **buzzer** on **PA3** for an audible alert
- Use **TIM2** as a microsecond-precision timer because the DHT11 protocol requires timing accuracy below 1 ms

## Hardware Setup

- MCU: **STM32F103C8T6 Blue Pill**
- Sensor: **DHT11 / KY-015** temperature sensor module
- Communication: **ST-Link V2** via SWD for programming/debugging
- Breadboard power rails: 3.3V and GND tied to the Blue Pill rails

### Physical Connections

- **DHT11 / KY-015**
  - `VCC` → `3.3V`
  - `GND` → `GND`
  - `DATA` → `PA1`
- **LED**
  - `PA2` → 220Ω resistor → LED anode
  - LED cathode → `GND`
- **Buzzer**
  - `VCC` → `3.3V`
  - `GND` → `GND`
  - `SIGNAL` → `PA3`
- **ST-Link V2**
  - `SWDIO` → `SWDIO`
  - `SWCLK` → `SWCLK`
  - `GND` → `GND`
  - `3.3V` → `3.3V`

### Breadboard Wiring Diagram

```
      PC / USB                 Breadboard                Blue Pill
  +----------------+    +---------------------------+    +-----------+
  | ST-Link V2 USB |    | 3.3V + rail  |  GND - rail |    |           |
  |                |    |             |             |    |  Blue Pill|
  |  SWDIO  -----> |----|-------------|             |    |           |
  |  SWCLK  -----> |----|-------------|             |    |           |
  |   3.3V  -----> |----| + 3.3V rail |             |    |  PA1 DHT11|
  |   GND   -----> |----| -  GND rail |             |    |  PA2 LED  |
  +----------------+    |             |             |    |  PA3 BUZZ |
                        |  DHT11 DATA |             |    |           |
                        +-------------+             |    +-----------+
```

## Software Setup

Use VS Code with STM32 development tools:

- Install the **ARM GCC toolchain** (`arm-none-eabi-gcc`)
- Install **STM32 CLI tools** for build, flash, and debugging
- Install VS Code extensions:
  - **C/C++** (Microsoft)
  - **Cortex-Debug**
- Optionally use **STM32CubeMX** to inspect or regenerate project initialization code

### Project Generation Notes

The original PDF workflow uses STM32CubeMX to configure:

- **PA1** as `GPIO_Input` for the DHT11 data line
- **PA2** as `GPIO_Output` for LED control
- **PA3** as `GPIO_Output` for buzzer control
- **USART1** on `PA9`/`PA10` at `115200` baud
- **TIM2** with prescaler set so each timer tick is `1 µs` and period `65535`

CubeMX generates the HAL initialization code and a Makefile, then the user adds application logic inside the `USER CODE` blocks.

## Build Instructions

From the project root:

```bash
make clean
make -j$(nproc)
```

Build artifacts are generated in `build/`:

- `build/STM32_projects.elf`
- `build/STM32_projects.hex`
- `build/STM32_projects.bin`

## Flashing

Example with `st-flash`:

```bash
st-flash write build/STM32_projects.bin 0x8000000
```

Example with `openocd`:

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/STM32_projects.elf verify reset exit"
```

## Implementation Guidance

The PDF describes the DHT11 protocol and implementation strategy for this project:

1. Drive the data pin low for **18 ms** to send the DHT11 start signal
2. Release the pin high and wait **20 µs**
3. Switch the pin to input mode and wait for the sensor response
4. Read the sensor acknowledgment: **80 µs LOW**, then **80 µs HIGH**
5. Read **40 bits** of data
   - each bit begins with **50 µs LOW**
   - a short **HIGH** pulse (~26 µs) = `0`
   - a long **HIGH** pulse (~70 µs) = `1`
6. Assemble 5 bytes: humidity integer, humidity decimal, temperature integer, temperature decimal, checksum
7. Verify the checksum by adding the first four bytes and comparing to the fifth

The project should use the HAL `GPIO_InitTypeDef` and `HAL_GPIO_Init()` calls to switch the DHT11 data pin between output and input modes at runtime.

## Troubleshooting

Common issues from the PDF:

- **Flashes fine but no output**: the chip may be running; check UART setup and use the debugger
- **DHT11 reads all zeros**: verify `PA1` wiring and add a **10kΩ pull-up** if the module does not already include one
- **Reads once then stops**: DHT11 needs at least **1 second between readings**; use `HAL_Delay(2000)` for a 2 second cycle
- **Buzzer silent**: verify whether it is **active** or **passive**; passive buzzer may require PWM, active buzzer only needs a HIGH signal
- **LED does not light**: check LED polarity, resistor value, and whether `PA2` is actually being driven HIGH
- **Checksum fails**: DHT11 timing is critical; ensure microsecond delays are accurate and the timer prescaler is configured correctly

## Project Structure

- `Core/Src/` — application source and HAL initialization code
- `Core/Inc/` — application headers
- `Drivers/STM32F1xx_HAL_Driver/` — HAL driver sources and headers
- `Drivers/CMSIS/` — CMSIS headers and startup files
- `startup_stm32f103xb.s` — startup assembly
- `STM32F103XX_FLASH.ld` — linker script
- `Makefile` — build rules and toolchain settings

## Notes

- The `KY_015_data_reader()` function in `Core/Src/ky_015.c` is the placeholder for the DHT11 read logic.
- `delay_execution_in_microseconds_using_TIM2()` in `Core/Src/ky_015.c` provides the timer-based microsecond delays needed for the DHT11 protocol.
- This project currently has the hardware initialization and build framework; the main sensor and alert application logic is intended to be added in the source files.

## License

This project follows the licensing terms in the STM32 HAL distribution. If no additional license is provided, assume the HAL code is used under ST’s standard license terms.
