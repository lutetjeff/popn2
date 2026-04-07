#include "tusb.h"

// -----------------------------------------------------------------------------
// Endpoint & Interface Definitions
// -----------------------------------------------------------------------------
#define ITF_NUM_CDC       0
#define ITF_NUM_CDC_DATA  1
#define ITF_NUM_HID       2
#define ITF_TOTAL         3

// Endpoint Addresses (STM32F072 has up to 8 bidirectional EPs)
// IN = 0x80 | EP_NUM, OUT = EP_NUM
#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82
#define EPNUM_HID_IN      0x83

// -----------------------------------------------------------------------------
// Device Descriptor
// -----------------------------------------------------------------------------
// Must use MISC/IAD class when combining CDC with other classes
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xFFFF, // Replace with your VID
    .idProduct          = 0x0727, // Replace with your PID
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

// -----------------------------------------------------------------------------
// NKRO HID Report Descriptor
// -----------------------------------------------------------------------------
// 1 byte modifiers + 16 bytes key bitmap (128 keys total) = 17 byte report
uint8_t const desc_hid_report[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    
    // Modifiers (8 bits: Ctrl, Shift, Alt, GUI - Left and Right)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0xE0,       //   Usage Minimum (224)
    0x29, 0xE7,       //   Usage Maximum (231)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    
    // LEDs (5 bits for Num Lock, Caps Lock, etc. + 3 bits padding)
    0x05, 0x08,       //   Usage Page (LEDs)
    0x19, 0x01,       //   Usage Minimum (1)
    0x29, 0x05,       //   Usage Maximum (5)
    0x95, 0x05,       //   Report Count (5)
    0x75, 0x01,       //   Report Size (1)
    0x91, 0x02,       //   Output (Data, Variable, Absolute)
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x03,       //   Report Size (3)
    0x91, 0x03,       //   Output (Constant)
    
    // NKRO Key Bitmap (128 keys: keycodes 0x00 to 0x7F)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x7F,       //   Usage Maximum (127)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1 bit per key)
    0x95, 0x80,       //   Report Count (128 keys)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    
    0xC0              // End Collection
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    return desc_hid_report;
}

// -----------------------------------------------------------------------------
// Configuration Descriptor
// -----------------------------------------------------------------------------
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // CDC Descriptor: Interface number, string index, EP notification, EP out, EP in, EP size
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

    // HID Descriptor: Interface number, string index, protocol, report descriptor len, EP In address, size, polling interval
    // NOTE: Protocol is set to NONE (0) because NKRO violates the strict Boot Keyboard format
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_IN, 32, 1)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    return desc_configuration;
}

// -----------------------------------------------------------------------------
// String Descriptors
// -----------------------------------------------------------------------------
char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: Supported language (English)
    "lutet industries",          // 1: Manufacturer
    "12 button controller f072",        // 2: Product
    "727",                      // 3: Serial
    "12bcf072 COM Port",            // 4: CDC Interface
    "12bcf072 Keyboard"                // 5: HID Interface
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    uint8_t chr_count;

    if ( index == 0 ) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        for ( uint8_t i=0; i<chr_count; i++ ) {
            _desc_str[1+i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);
    return _desc_str;
}

// -----------------------------------------------------------------------------
// TinyUSB HID Callbacks
// -----------------------------------------------------------------------------

// Invoked when the host sends a GET_REPORT request.
// The application should fill the buffer with the current report and return its length.
// Returning 0 tells the stack to STALL the request (which is fine for most modern OSs
// as they rely on the interrupt endpoint for data instead of polling this).
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    // For a simple keyboard, we don't usually need to respond to this directly.
    return 0; 
}

// Invoked when the host sends a SET_REPORT request.
// This is typically used by the PC to set the keyboard LEDs (Caps Lock, Num Lock, etc.)
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;

    // Ensure it's an output report (which is what LEDs use)
    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        // The buffer contains the LED state bitmask
        // Bit 0: Num Lock
        // Bit 1: Caps Lock
        // Bit 2: Scroll Lock
        // Bit 3: Compose
        // Bit 4: Kana
        if (bufsize > 0) {
            uint8_t kbd_leds = buffer[0];

            if (kbd_leds & KEYBOARD_LED_CAPSLOCK) {
                // Turn ON Caps Lock LED on your board
                // HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            } else {
                // Turn OFF Caps Lock LED
                // HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            }
        }
    }
}