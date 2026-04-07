#ifndef TASKS_H
#define TASKS_H

#include "main.h" // Pulls in GPIO definitions (B0_Pin, etc.)
#include "tusb.h"

// -----------------------------------------------------------------------------
// NKRO Report Definition
// -----------------------------------------------------------------------------
typedef struct TU_ATTR_PACKED {
    uint8_t modifiers;
    uint8_t keys[16];
} nkro_keyboard_report_t;

// -----------------------------------------------------------------------------
// Controller State Definitions
// -----------------------------------------------------------------------------
typedef struct {
    uint8_t mode;           
    uint8_t brightness;     
    uint8_t com_state;      
    uint8_t is_pressed;     // The debounced, "official" state
    uint8_t raw_state;      // The immediate, unverified hardware state
    uint32_t last_change;   // Timestamp (in ms) of the last raw state change
} ButtonState_t;

typedef struct {
    uint8_t global_led_enable; 
    uint32_t debounce_time_ms; // Global debounce time
    uint8_t keycodes[12];      // Configurable keycodes for B0-B11
    ButtonState_t buttons[12];
} ControllerState_t;

// -----------------------------------------------------------------------------
// Flash & Bootloader Definitions
// -----------------------------------------------------------------------------
// Address of the last 2KB page in a 64KB STM32F072 (Adjust if using 128KB version)
#define FLASH_CONFIG_ADDR   0x0800F800 
#define CONFIG_MAGIC_WORD   0x00670727 // Used to check if flash is formatted

// The memory address where ST's ROM bootloader lives on the F072
#define BOOTLOADER_ADDR     0x1FFFC800 
#define DFU_MAGIC_ADDR      0x20003800
#define DFU_MAGIC_WORD      0x45554555

// Struct containing only the variables we want to save
typedef struct {
    uint32_t magic;
    uint32_t debounce_time_ms;
    uint8_t  global_led_enable;
    uint8_t  keycodes[12];
    uint8_t  modes[12];
    uint8_t  brightness[12];
} PersistentConfig_t;

// -----------------------------------------------------------------------------
// Task Prototypes
// -----------------------------------------------------------------------------
void tasks_init(void);
void gpio_task(void);
void hid_task(void);
void cdc_task(void);
void reset_config(void);
void save_config(void);
void load_config(void);
void jump_to_dfu(void);

#endif // TASKS_H