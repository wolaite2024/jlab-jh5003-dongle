/**
 * @copyright Copyright (C) 2023 Realtek Semiconductor Corporation.
 *
 * @file port_macro.h
 * @version 1.0
 * @brief
 *
 * @note:
 */
#ifndef _PORT_MACRO_H_
#define _PORT_MACRO_H_

#define USB_USER_SPEC_SECTION    __attribute__((section(".text.usb.user_spec")))
#define USB_ISOC_ISR_ENTRY_SECTION    __attribute__((section(".text.usb.isoc_isr_entry")))

#endif
