# STM32 Temperature Monitor for Statistical Anomaly Detection

A bare-metal embedded project on the STM32F103 Blue Pill that reads temperature from a DS18B20 sensor (KY-001 module) every few seconds, streams the readings over UART, and triggers an LED and buzzer alert when a reading is statistically unusual (Z-score detector).

> The sensor was switched from the DHT11 (KY-015) to the DS18B20 (KY-001): it measures up to 125 °C (the DHT11 stops at 50 °C) and resolves 0.0625 °C instead of whole degrees. The old DHT11 driver is kept in the repo for reference but is no longer built.

This project is now being extended into a research project on low-cost anomaly detection. See [Roadmap: Statistical Anomaly Detection](#roadmap-statistical-anomaly-detection).

DEMO Link: https://www.linkedin.com/posts/noormustafa715_project-link-httpslnkdindgzxnrej-descrption-ugcPost-7476007107285209089-Gs78/?utm_source=share&utm_medium=member_desktop&rcm=ACoAAD3VztoBqnFH6rlJuo7mzXSpDRqDyTKvQXc

---

## Hardware

| Component | Part | Notes |
|-----------|------|-------|
| Microcontroller | STM32F103C8T6 "Blue Pill" | Clone chip — actual silicon is F103C6T6 (10 KB RAM / 32 KB Flash) |
| Sensor | KY-001 (DS18B20) | Digital temperature sensor, −55 to +125 °C, 0.0625 °C resolution, on-board pull-up resistor |
| LED | KY-011 (bi-color LED) | Red/green, connected to PA2 |
| Buzzer | KY-012 (active buzzer) | Sounds on HIGH signal, connected to PA3 |
| Programmer | ST-Link V2 | Flashes and debugs over SWD |

### Pin Connections

| MCU Pin | Connected To | Purpose |
|---------|-------------|---------|
| `PA1` | KY-001 DATA | DS18B20 1-Wire data line (open-drain) |
| `PA2` | KY-011 SIGNAL | LED — ON when temp > threshold |
| `PA3` | KY-012 SIGNAL | Buzzer — ON when temp > threshold |
| `PA9` | USB-TTL TX | UART transmit (115200 baud) |
| `SWDIO` | ST-Link SWDIO | SWD debug/flash |
| `SWCLK` | ST-Link SWCLK | SWD clock |
| `3.3V` | ST-Link 3.3V | Board power via ST-Link |
| `GND` | ST-Link GND | Common ground |

---

## How It Works

### DS18B20 (1-Wire) Protocol

The DS18B20 uses the Dallas/Maxim 1-Wire bus: one data line shared by the MCU and the sensor. Neither side drives it HIGH; each can only pull it LOW, and the pull-up resistor on the KY-001 board brings it back HIGH. PA1 is therefore set as an **open-drain** output, and its level is read back through the input register.

1. **Reset** — MCU pulls the line LOW for 480 µs, releases it, and the sensor answers with a **presence pulse** (LOW) about 70 µs later
2. **Commands** — sent as bytes, least-significant bit first, one ~70 µs time slot per bit:
   - `0xCC` SKIP ROM (only one sensor on the bus)
   - `0x44` CONVERT T (start measuring)
3. **Wait** — the sensor answers read slots with 0 while measuring and 1 when done (up to ~750 ms at 12-bit resolution)
4. **Read** — reset, `0xCC`, then `0xBE` READ SCRATCHPAD, and read 9 bytes:

   | Byte | Content |
   |------|---------|
   | 0–1 | Temperature, signed 16-bit, 1/16 °C per step |
   | 2–3 | Alarm registers |
   | 4 | Configuration (resolution) |
   | 5–7 | Reserved |
   | 8 | CRC-8 of bytes 0–7 |

5. **Verify** — the Dallas CRC-8 must match byte 8; the driver also rejects all-zero data and the 85 °C power-on value

Timing-critical parts of each slot run with interrupts briefly disabled, so a SysTick interrupt cannot stretch them.

### Timing with TIM2

TIM2 is configured with a prescaler of 7, giving 1 tick = 1 µs at 8 MHz HSI. The 1-Wire driver uses the TIM2 counter for its microsecond delays (reset pulse, bit slots, sampling point). The conversion wait has a 1 s timeout so the program never hangs if the sensor does not respond.

### Alert Logic

- Temperature reading arrives every 2 seconds
- If `temperature > 20°C`: PA2 (LED) and PA3 (buzzer) are driven HIGH
- Otherwise both are driven LOW
- Reading and result are transmitted over UART so they can be monitored on a PC

---

## Tools Used

| Tool | Purpose |
|------|---------|
| **STM32CubeMX** | Generated peripheral initialization code (GPIO, TIM2, USART1) and the Makefile |
| **arm-none-eabi-gcc** | Cross-compiler for ARM Cortex-M3 |
| **GNU Make** | Build system |
| **st-flash** (stlink-tools) | Flashes the `.bin` to the MCU over ST-Link |
| **VS Code** + C/C++ extension | Code editing |

---

## Project Structure

```
├── Core/
│   ├── Inc/
│   │   ├── ky_001.h          # ds18b20_data_t struct, KY_001_init / KY_001_data_reader
│   │   ├── ky_015.h          # old DHT11 driver header (not built)
│   │   └── main.h            # HAL handle externs, peripheral declarations
│   └── Src/
│       ├── ky_001.c          # DS18B20 1-Wire implementation (open-drain GPIO + TIM2)
│       ├── ky_015.c          # old DHT11 driver (kept for reference, not built)
│       ├── main.c            # Peripheral init, main loop, alert logic
│       ├── stm32f1xx_hal_msp.c  # Low-level peripheral clock / GPIO config
│       └── stm32f1xx_it.c    # Interrupt handlers
├── Drivers/
│   ├── STM32F1xx_HAL_Driver/ # ST HAL driver sources
│   └── CMSIS/                # ARM CMSIS headers
├── startup_stm32f103xb.s     # Startup assembly (vector table, stack init)
├── STM32F103XX_FLASH.ld      # Linker script — 10 KB RAM / 32 KB Flash
└── Makefile                  # Build rules generated by CubeMX
```

---

## Build & Flash

**Requirements:** `arm-none-eabi-gcc`, `make`, `stlink-tools`

```bash
# Build
make

# Flash
st-flash write build/STM32_projects.bin 0x8000000
```

Build output goes to `build/`: `.elf`, `.hex`, `.bin`

---

## Monitor UART Output

Connect a USB-TTL adapter to `PA9` (TX) and `GND`, then open a serial terminal at **115200 baud**:

```bash
# Linux
screen /dev/ttyUSB0 115200
```

Expected output:
```
Temp: 38.4C  StdDev: 0.2C  Z-Score: 0.5
Temp: 38.5C  StdDev: 0.2C  Z-Score: 1.0
read failed        ← if the sensor doesn't respond or the CRC fails
```

---

## Key Implementation Notes

**Linker script fix** — Blue Pill clones are often relabeled F103C6T6 chips (10 KB RAM / 32 KB Flash) sold as C8T6 (20 KB / 64 KB). Using the wrong memory sizes in the linker script causes the stack pointer to land outside valid RAM, triggering an instant HardFault on boot. The linker script in this repo is corrected for the actual hardware.

**SWD kept enabled** — CubeMX defaults to `__HAL_AFIO_REMAP_SWJ_DISABLE()` which disables SWD after the first flash, permanently locking the chip out. This project uses `__HAL_AFIO_REMAP_SWJ_NONJTRST()` instead, which disables JTAG but keeps SWD available.

**PA1 as open-drain** — The 1-Wire line is shared, so PA1 is configured once as an open-drain output in `KY_001_init()`. Writing 1 releases the line (the KY-001's pull-up brings it HIGH) and the pin level is read from `GPIOA->IDR`, so no runtime mode switching is needed. A bare DS18B20 without the module needs an external 4.7 kΩ pull-up to 3.3 V.

---

## Roadmap: Statistical Anomaly Detection

> **Status: planned / in progress.** Nothing in this section is implemented yet.

### Goal

Extend this monitor from a fixed-threshold alarm into a **statistical anomaly detector** that learns what "normal" looks like for a device and flags readings that are unusually far from it, running entirely on the STM32 with no operating system, no cloud and no PC in the loop.

The work will be written up as a research paper. It builds on recent work that ran an on-device Z-score detector on a more capable Cortex-M4 board with a floating-point unit ([arXiv:2604.08581](https://arxiv.org/pdf/2604.08581)). This project tests the same idea on a much cheaper and smaller chip, and adds the comparisons that paper left as future work.

### Research questions

1. Does on-device statistical anomaly detection still work on a Cortex-M3 with **no floating-point unit and only 10 KB RAM**?
2. How does it compare with the simple fixed-threshold alarm this project already uses?
3. How sensitive are the results to the choice of threshold?

### Monitored device

An old **pedestal fan**. Its motor housing gets very hot while running and it has no built-in temperature monitoring, which is the gap a low-cost monitor fills.

- The DS18B20 (KY-001) is mounted a few centimetres from the motor housing so it reads the warm air near the motor. Its 125 °C range leaves plenty of headroom (the DHT11 used earlier stopped at 50 °C).
- Anomalies will be created only through safe controls (for example, changing speed settings). The blades will not be stalled and the motor will not be blocked.
- The fan will not be left running unattended for long periods.

### How the detector works

1. **Learning phase:** for a fixed period at start-up, the MCU collects readings and computes their **mean** and **standard deviation**.
2. **Detection phase:** every new reading gets a **Z-score**, the number of standard deviations it lies from the mean.
3. **Alert:** if the Z-score's size exceeds a chosen threshold (starting around 3), the reading is flagged and the existing LED and buzzer are triggered.

The existing `temperature > 20 °C` logic is kept as the **fixed-threshold baseline** for comparison.

### What will be measured

| Metric | How |
|--------|-----|
| Detection rate | Fraction of induced anomalies that were flagged |
| False-alarm rate | Flags raised during normal operation |
| Detection delay | Time from the start of an anomaly to the first flag |
| Flash and RAM use | From the build output / map file |
| Compute time per reading | Measured on the MCU at 8 MHz |

### Steps

- [x] DHT11 driver, UART output and fault handling (sensor timeout, checksum)
- [x] Switched to the DS18B20 (KY-001): 1-Wire driver with CRC check, 0.0625 °C resolution
- [x] Fixed-threshold alert with LED and buzzer (baseline)
- [ ] Cold-start log on the fan: find when the temperature levels off (sets the warm-up time)
- [ ] Define what counts as normal and as an anomaly for the fan
- [ ] Design the detector on paper (learning-phase length, threshold, behaviour after an alert)
- [ ] Add timestamps to UART output and save logs as CSV on the PC
- [ ] Write and test the Z-score detector in C on a PC with made-up data
- [ ] Port the detector to the STM32 and measure flash, RAM and timing
- [ ] Record a long normal run plus anomaly sessions, noting the time of each event
- [ ] Analyse results and compare against the fixed threshold at several threshold values
- [ ] Write the paper

---

## License

Application code (files in `Core/`) is written by the project author.
HAL and CMSIS sources in `Drivers/` are licensed by STMicroelectronics under their standard terms (see individual file headers).
