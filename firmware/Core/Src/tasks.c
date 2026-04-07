#include "tasks.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Global State Instance
ControllerState_t ctrl_state;

// -----------------------------------------------------------------------------
// Hardware Lookup Tables
// -----------------------------------------------------------------------------
// Ensures we can iterate 0-11 rather than writing switch statements
GPIO_TypeDef* const BTN_PORTS[12] = {
    B0_GPIO_Port, B1_GPIO_Port, B2_GPIO_Port, B3_GPIO_Port,
    B4_GPIO_Port, B5_GPIO_Port, B6_GPIO_Port, B7_GPIO_Port,
    B8_GPIO_Port, B9_GPIO_Port, B10_GPIO_Port, B11_GPIO_Port
};
const uint16_t BTN_PINS[12] = {
    B0_Pin, B1_Pin, B2_Pin, B3_Pin,
    B4_Pin, B5_Pin, B6_Pin, B7_Pin,
    B8_Pin, B9_Pin, B10_Pin, B11_Pin
};

GPIO_TypeDef* const LED_PORTS[12] = {
    L0_GPIO_Port, L1_GPIO_Port, L2_GPIO_Port, L3_GPIO_Port,
    L4_GPIO_Port, L5_GPIO_Port, L6_GPIO_Port, L7_GPIO_Port,
    L8_GPIO_Port, L9_GPIO_Port, L10_GPIO_Port, L11_GPIO_Port
};
const uint16_t LED_PINS[12] = {
    L0_Pin, L1_Pin, L2_Pin, L3_Pin,
    L4_Pin, L5_Pin, L6_Pin, L7_Pin,
    L8_Pin, L9_Pin, L10_Pin, L11_Pin
};

// HID Keycodes for B0-B11: '0'-'9', '-', '='
// Note: HID_KEY_0 is 0x27. '1'-'9' are 0x1E to 0x26.
const uint8_t DEFAULT_KEYMAP[12] = {
    HID_KEY_0, HID_KEY_1, HID_KEY_2, HID_KEY_3,
    HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7,
    HID_KEY_8, HID_KEY_9, HID_KEY_MINUS, HID_KEY_EQUAL
};

void tasks_init(void) {
    load_config(); // Replaces manual default assignments
    
    // Ensure volatile states are still cleared on boot
    for (int i = 0; i < 12; i++) {
        ctrl_state.buttons[i].com_state = 0;
        ctrl_state.buttons[i].is_pressed = 0;
        ctrl_state.buttons[i].raw_state = 0;
        ctrl_state.buttons[i].last_change = HAL_GetTick();
    }
}

// -----------------------------------------------------------------------------
// GPIO Task: Reads buttons, applies LED logic
// -----------------------------------------------------------------------------
void gpio_task(void) {
    uint32_t current_time = HAL_GetTick();

    for (int i = 0; i < 12; i++) {
        // Read physical button (Active LOW)
        uint8_t current_read = (HAL_GPIO_ReadPin(BTN_PORTS[11-i], BTN_PINS[11-i]) == GPIO_PIN_RESET) ? 1 : 0;
        
        // 1. Check if the raw hardware state changed
        if (current_read != ctrl_state.buttons[i].raw_state) {
            ctrl_state.buttons[i].last_change = current_time; // Reset the timer
            ctrl_state.buttons[i].raw_state = current_read;   // Update raw state
        }

        // 2. Check if the raw state has been stable for the debounce duration
        if ((current_time - ctrl_state.buttons[i].last_change) >= ctrl_state.debounce_time_ms) {
            // It's stable! Update the official state
            ctrl_state.buttons[i].is_pressed = ctrl_state.buttons[i].raw_state;
        }

        // 3. Determine LED state (using the debounced is_pressed state)
        uint8_t led_on = 0;
        if (ctrl_state.global_led_enable) {
            if (ctrl_state.buttons[i].mode == 0) {
                led_on = ctrl_state.buttons[i].is_pressed; 
            } else if (ctrl_state.buttons[i].mode == 1) {
                led_on = ctrl_state.buttons[i].com_state; 
            }
        }

        // Apply physical LED state
        HAL_GPIO_WritePin(LED_PORTS[i], LED_PINS[i], led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

// -----------------------------------------------------------------------------
// HID Task: Builds NKRO report based on current states
// -----------------------------------------------------------------------------
void hid_task(void) {
    static nkro_keyboard_report_t prev_report = {0};
    nkro_keyboard_report_t current_report = {0};
    
    if (!tud_hid_ready()) return;

    for (int i = 0; i < 12; i++) {
        if (ctrl_state.buttons[i].is_pressed) {
            // Read the dynamic keycode from state
            uint8_t keycode = ctrl_state.keycodes[i]; 
            current_report.keys[keycode / 8] |= (1 << (keycode % 8));
        }
    }

    if (memcmp(&current_report, &prev_report, sizeof(nkro_keyboard_report_t)) != 0) {
        tud_hid_report(0, &current_report, sizeof(current_report));
        prev_report = current_report;
    }
}

// -----------------------------------------------------------------------------
// CDC Task: Reads and parses commands, echoes input, provides feedback
// -----------------------------------------------------------------------------
void cdc_task(void) {
    static char line_buf[64];
    static uint8_t line_idx = 0;

    if (tud_cdc_connected() && tud_cdc_available()) {
        char c;
        uint32_t count = tud_cdc_read(&c, 1);
        
        if (count == 0) return;

        // 1. Echo the character back to the terminal immediately
        tud_cdc_write_char(c);
        tud_cdc_write_flush();

        // 2. Accumulate characters (ignore newline/carriage return for now)
        if (c != '\n' && c != '\r' && line_idx < (sizeof(line_buf) - 1)) {
            line_buf[line_idx++] = c;
        } 
        // 3. Parse when we hit a newline or carriage return (and have data)
        else if ((c == '\n' || c == '\r') && line_idx > 0) {
            
            // Format the terminal neatly for the upcoming response
            if (c == '\r') tud_cdc_write_str("\n"); 
            tud_cdc_write_flush();
            
            line_buf[line_idx] = '\0'; // Null-terminate the string
            
            char cmd;
            int arg0 = 0, arg1 = 0;
            
            // sscanf returns how many items it successfully matched.
            // "e 1" returns 2. "m 0 1" returns 3.
            int parsed = sscanf(line_buf, "%c %i %i", &cmd, &arg0, &arg1);
            
            bool success = false;
            char response[64];

            // Execute if we at least got a command and arg0
            if (parsed >= 2) { 
                switch (cmd) {
                    case 'e': // Global enable (arg1 ignored)
                        ctrl_state.global_led_enable = (arg0 != 0) ? 1 : 0;
                        success = true;
                        break;
                        
                    case 'm': // Mode (requires arg0 and arg1)
                        if (parsed == 3 && arg0 < 12) {
                            ctrl_state.buttons[arg0].mode = arg1;
                            success = true;
                        }
                        break;
                        
                    case 'b': // Brightness (requires arg0 and arg1)
                        if (parsed == 3 && arg0 < 12) {
                            ctrl_state.buttons[arg0].brightness = arg1;
                            success = true;
                        }
                        break;
                        
                    case 'd': // COM Data State (requires arg0 and arg1)
                        if (parsed == 3 && arg0 < 12) {
                            ctrl_state.buttons[arg0].com_state = (arg1 != 0) ? 1 : 0;
                            success = true;
                        }
                        break;
                        
                    case 'r': // Read button state (arg1 ignored)
                        if (arg0 < 12) {
                            success = true;
                        }
                        break;

                    case 't': // Global debounce time (requires arg0)
                        ctrl_state.debounce_time_ms = arg0;
                        success = true;
                        break;
                        
                    case 'k': // Set keycode (requires arg0=button_idx, arg1=keycode)
                        if (parsed == 3 && arg0 < 12 && arg1 <= 127) { // 127 is max for our NKRO map
                            ctrl_state.keycodes[arg0] = arg1;
                            success = true;
                        }
                        break;

                    case 'S': // Save to Flash (capital S, no args needed)
                        save_config();
                        success = true;
                        break;
                        
                    case 'Z': // Jump to DFU bootloader (capital Z, no args needed)
                        tud_cdc_write_str("Rebooting to DFU...\r\n");
                        tud_cdc_write_flush();
                        jump_to_dfu();
                        // Code stops here
                        break;

                    case 'R': // Reset settings and save
                        reset_config();
                        save_config();
                        success = true;
                        break;
                        
                    default:
                        break;
                }
            }
            
            // 4. Provide Feedback
            if (success) {
                if (cmd == 'r') {
                    // Send the read value back
                    snprintf(response, sizeof(response), "READ %d: %d\r\n", arg0, ctrl_state.buttons[arg0].is_pressed);
                    tud_cdc_write_str(response);
                } else {
                    tud_cdc_write_str("OK\r\n");
                }
            } else {
                tud_cdc_write_str("ERROR\r\n");
            }
            tud_cdc_write_flush();

            // Reset buffer index for the next command
            line_idx = 0;
        }
    }
}

// Loads configuration from Flash into the active controller state
void load_config(void) {
    PersistentConfig_t* flash_cfg = (PersistentConfig_t*)FLASH_CONFIG_ADDR;

    if (flash_cfg->magic == CONFIG_MAGIC_WORD) {
        // Valid config found, load it
        ctrl_state.debounce_time_ms = flash_cfg->debounce_time_ms;
        ctrl_state.global_led_enable = flash_cfg->global_led_enable;
        for (int i = 0; i < 12; i++) {
            ctrl_state.keycodes[i] = flash_cfg->keycodes[i];
            ctrl_state.buttons[i].mode = flash_cfg->modes[i];
            ctrl_state.buttons[i].brightness = flash_cfg->brightness[i];
        }
    } else {
        // No valid config, use defaults
        ctrl_state.debounce_time_ms = 5;
        ctrl_state.global_led_enable = 1;
        for (int i = 0; i < 12; i++) {
            ctrl_state.keycodes[i] = DEFAULT_KEYMAP[i]; // From your HID_MAP array
            ctrl_state.buttons[i].mode = 0;
            ctrl_state.buttons[i].brightness = 255;
        }
    }
}

// Resets the configuration to the default
void reset_config(void) {
    ctrl_state.debounce_time_ms = 5;
    ctrl_state.global_led_enable = 1;
    for (int i = 0; i < 12; i++) {
        ctrl_state.keycodes[i] = DEFAULT_KEYMAP[i]; // From your HID_MAP array
        ctrl_state.buttons[i].mode = 0;
        ctrl_state.buttons[i].brightness = 255;
    }
}

// Saves the active controller state to Flash
void save_config(void) {
    PersistentConfig_t cfg_to_save = {0};
    cfg_to_save.magic = CONFIG_MAGIC_WORD;
    cfg_to_save.debounce_time_ms = ctrl_state.debounce_time_ms;
    cfg_to_save.global_led_enable = ctrl_state.global_led_enable;
    
    for (int i = 0; i < 12; i++) {
        cfg_to_save.keycodes[i] = ctrl_state.keycodes[i];
        cfg_to_save.modes[i] = ctrl_state.buttons[i].mode;
        cfg_to_save.brightness[i] = ctrl_state.buttons[i].brightness;
    }

    // 1. Unlock Flash
    HAL_FLASH_Unlock();

    // 2. Erase the configuration page
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = FLASH_CONFIG_ADDR;
    erase_init.NbPages = 1;
    HAL_FLASHEx_Erase(&erase_init, &page_error);

    // 3. Write the struct to flash word-by-word (32 bits at a time)
    uint32_t* data_ptr = (uint32_t*)&cfg_to_save;
    size_t words_to_write = sizeof(PersistentConfig_t) / 4;
    if (sizeof(PersistentConfig_t) % 4 != 0) words_to_write++; // Catch remainder

    for (size_t i = 0; i < words_to_write; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_CONFIG_ADDR + (i * 4), data_ptr[i]);
    }

    // 4. Lock Flash
    HAL_FLASH_Lock();
}

void jump_to_dfu(void) {
    // 1. Write the magic word to RAM
    *((volatile uint32_t *)DFU_MAGIC_ADDR) = DFU_MAGIC_WORD;
    
    // 2. Trigger a violent hardware reset
    // This instantly cuts the USB connection so Windows sees a true disconnect
    NVIC_SystemReset(); 
}