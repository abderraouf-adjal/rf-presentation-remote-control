/* SPDX-License-Identifier: ISC */
/**
 * Licensing and copyright: See the file `LICENSE.txt`
 */

#ifndef USB_HID_KBD_H
#define USB_HID_KBD_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h> /* For `NULL`. */

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"

#define USE_CUSTOM_HID
#ifdef USE_CUSTOM_HID
# include "usbd_custom_hid_if.h"
#else
# include "usbd_hid.h"
#endif

#include "led_ind.h"

/******************************************************************************/

#define DELAY_1000_MS ((uint32_t) 1000U) /* 1 sec. */

#define USB_MAX_DELAY_MS ((uint32_t) 10U*DELAY_1000_MS) /* 10 sec. */
//#define USB_BOOT_DELAY_MS (500U) /* 500 ms */
#define KEY_PRESS_DELAY_MS ((uint32_t) 30U) /* 30 ms */
#define KEY_DEBOUNCE_DELAY_MS ((uint32_t) 150U) /* 150 ms */

#define USB_KEYBOARD_REPORT_SIZE 8U

/******************************************************************************/
/* USB HID keyboard MODIFIER KEYS bits data (Index 0) for none pressed. */
#define KEY_MODIF_NONE 0x00
/* Keyboard IDLE (None) */
#define KEY_NONE  0x00
/******************************************************************************/
/* USB HID keyboard KEYCODE-1 byte data (Index 2). */
/* Keyboard arrows: Up, Down, Left, Right */
#define KEY_ARW_U 0x52
#define KEY_ARW_D 0x51
#define KEY_ARW_L 0x50
#define KEY_ARW_R 0x4F
/* Keyboard Return (ENTER) */
#define KEY_ENTER 0x28
/* Keyboard DELETE (Backspace) */
#define KEY_BACKSPACE 0x2A
/* Keyboard Spacebar */
#define KEY_SPACEBAR 0x2C
/* Keyboard PageUp & PageDown */
#define KEY_PG_UP 0x4B
#define KEY_PG_DW 0x4E
/******************************************************************************/

extern volatile uint8_t USB_HID_kbd_flag;
extern USBD_HandleTypeDef hUsbDeviceFS;

/******************************************************************************/

USBD_StatusTypeDef USB_init_desc(USBD_HandleTypeDef *pdev,
                                 USBD_DescriptorsTypeDef *pdesc,
                                 uint8_t id,
                                 USBD_CUSTOM_HID_ItfTypeDef *fops,
                                 USBD_ClassTypeDef *pclass);
USBD_StatusTypeDef USB_wait_idle(USBD_HandleTypeDef *pdev, uint32_t delay_max);
USBD_StatusTypeDef USB_presskey(USBD_HandleTypeDef *pdev,
                                uint8_t ep_addr,
                                uint8_t modifier,
                                uint8_t keycode1);
void USB_presskey_handle();

#ifdef __cplusplus
}
#endif

#endif /* USB_HID_KBD_H */
