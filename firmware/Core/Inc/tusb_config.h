#pragma once

#define CFG_TUSB_MCU               OPT_MCU_STM32F0
#define CFG_TUSB_OS                OPT_OS_NONE // Assuming bare-metal

// Enable Device stack
#define CFG_TUD_ENABLED            1

// Enable 1 CDC interface and 1 HID interface
#define CFG_TUD_CDC                1
#define CFG_TUD_HID                1

// Endpoint buffer sizes
#define CFG_TUD_CDC_EP_BUFSIZE     64
#define CFG_TUD_CDC_RX_BUFSIZE     64
#define CFG_TUD_CDC_TX_BUFSIZE     64
#define CFG_TUD_HID_EP_BUFSIZE     32