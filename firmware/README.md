# popn2_firmware

STM32F072CBT6 firmware for a 12-input, 12-output rhythm game controller.

## Hardware

- **MCU**: STM32F072CBT6 (ARM Cortex-M0, 48MHz)
- **Inputs**: 12 buttons (B0-B11), active-low with pull-ups
- **Outputs**: 12 LEDs (L0-L11)
- **USB**: 1kHz HID + CDC

## Features

- **NKRO Keyboard HID**: All 12 buttons report simultaneously as HID keyboard keys
- **CDC Serial Config**: Virtual COM port for runtime configuration
- **Flash Persistence**: All settings saved to internal flash
- **DFU Bootloader**: Jump to ST's built-in ROM bootloader for firmware updates

## Building & Flashing

Requires [STM32-for-VSCode](https://marketplace.visualstudio.com/items?item=bmd.stm32-for-vscode) extension.

```bash
# Build
Ctrl+Shift+B > Build STM

# Flash
Ctrl+Shift+B > Flash STM
```

## Configuration Commands (CDC)

Connect to the Virtual COM port at 115200 baud.

| Command | Description |
|---------|-------------|
| `e 0/1` | Disable/Enable global LEDs |
| `t <ms>` | Set global debounce time (ms) |
| `k <btn> <hex>` | Set keycode for button (e.g. `k 0 0x04`) |
| `m <btn> <mode>` | Set LED mode: 0=Direct, 1=COM Controlled |
| `d <btn> 0/1` | Set COM state for button LED control |
| `r <btn>` | Read current button state |
| `S` | Save current config to flash |
| `R` | Reset to defaults and save |
| `Z` | Reboot into DFU bootloader |

## Python Configurator

Run `configurator.py` for a graphical interface to configure all settings and monitor button states in real-time.

```bash
python configurator.py
```

## Pin Mapping

| Button | Pin | LED | Pin |
|--------|-----|-----|-----|
| B0 | PA0 | L0 | PB12 |
| B1 | PA1 | L1 | PB13 |
| B2 | PA2 | L2 | PB14 |
| B3 | PA3 | L3 | PB15 |
| B4 | PA4 | L4 | PA8 |
| B5 | PA5 | L5 | PA9 |
| B6 | PA6 | L6 | PA10 |
| B7 | PA7 | L7 | PA15 |
| B8 | PB0 | L8 | PB3 |
| B9 | PB1 | L9 | PB4 |
| B10 | PB2 | L10 | PB5 |
| B11 | PB10 | L11 | PB6 |

## Default Keymap

B0-B9 map to HID keys `0`-`9`, B10 to `-`, B11 to `=`.
