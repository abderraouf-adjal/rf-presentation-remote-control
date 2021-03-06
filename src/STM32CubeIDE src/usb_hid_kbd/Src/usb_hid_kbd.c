/* SPDX-License-Identifier: ISC */
/**
 * Licensing and copyright: See the file `LICENSE.txt`
 */

#include "usb_hid_kbd.h"

volatile uint8_t USB_HID_kbd_flag;
__ALIGN_BEGIN static uint8_t USB_HID_KBD_buf[USB_KEYBOARD_REPORT_SIZE] __ALIGN_END = {
        0x00 };

static inline USBD_StatusTypeDef USB_HID_send_report(USBD_HandleTypeDef *pdev,
                                                     uint8_t ep_addr,
                                                     uint8_t *report,
                                                     uint16_t len)
{
    USBD_CUSTOM_HID_HandleTypeDef *hhid;

    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        hhid = pdev->pClassData;
        if (hhid == NULL) {
            return USBD_FAIL;
        }

        if (hhid->state == CUSTOM_HID_IDLE) {
            hhid->state = CUSTOM_HID_BUSY;
            if (HAL_PCD_EP_Transmit(pdev->pData, ep_addr, report, len) == HAL_OK) {
                return USBD_OK;
            } else {
                return USBD_FAIL;
            }
        } else {
            return USBD_BUSY;
        }
    }
    return USBD_FAIL;
}

static inline USBD_StatusTypeDef usb_cfg_desc(USBD_ClassTypeDef *pclass,
                                              uint16_t pos,
                                              uint8_t value)
{
    /* Change USB description option */
    uint16_t tmp_desc_sz;
    uint8_t *tmp_desc_ptr;

    if ((pclass == NULL) || (pos >= USBD_CUSTOM_HID_REPORT_DESC_SIZE)) {
        return USBD_FAIL;
    }

    tmp_desc_ptr = pclass->GetHSConfigDescriptor(&tmp_desc_sz);
    if (tmp_desc_ptr == NULL) {
        return USBD_FAIL;
    }
    tmp_desc_ptr[pos] = value;

    tmp_desc_ptr = pclass->GetFSConfigDescriptor(&tmp_desc_sz);
    if (tmp_desc_ptr == NULL) {
        return USBD_FAIL;
    }
    tmp_desc_ptr[pos] = value;

    tmp_desc_ptr = pclass->GetOtherSpeedConfigDescriptor(&tmp_desc_sz);
    if (tmp_desc_ptr == NULL) {
        return USBD_FAIL;
    }
    tmp_desc_ptr[pos] = value;

    return USBD_OK;
}

USBD_StatusTypeDef USB_init_desc(USBD_HandleTypeDef *pdev,
                                 USBD_DescriptorsTypeDef *pdesc,
                                 uint8_t id,
                                 USBD_CUSTOM_HID_ItfTypeDef *fops,
                                 USBD_ClassTypeDef *pclass)
{
    /* Check if handles are valid */
    if ((pdev == NULL) || (pdesc == NULL) || (fops == NULL) || (pclass == NULL)) {
        return USBD_FAIL;
    }

    USB_HID_kbd_flag = KEY_NONE;

    /* Config USB desc. on runtime (dev stage) because of CubeMX replacing content. */
#define USB_CFG_MAXPOWER_POS_ 8U
#define USB_CFG_NINTERFACEPROTOCOL_POS_ 16U
    /* Change USB MaxPower option: (2 * USBD_MAX_POWER) mA */
    if (usb_cfg_desc(pclass, USB_CFG_MAXPOWER_POS_, USBD_MAX_POWER) != USBD_OK) {
        return USBD_FAIL;
    }
    /* Change USB nInterfaceProtocol option: 1=keyboard */
    if (usb_cfg_desc(pclass, USB_CFG_NINTERFACEPROTOCOL_POS_,
    USBD_NINTERFACEPROTOCOL) != USBD_OK) {
        return USBD_FAIL;
    }

//    /* Init and add supported class desc. */
//    /*if (USBD_Init(pdev, pdesc, id) != USBD_OK) {
//     return USBD_FAIL;
//     }*/
//    pdev->pClass = NULL;
//    pdev->pDesc = pdesc;
//    /* Set Device initial State */
//    pdev->dev_state = USBD_STATE_DEFAULT;
//    pdev->id = id;
//    /* Initialize low level driver */
//    USBD_LL_Init(pdev);
//
//    pdev->pClass = pclass; /* eq. to USBD_RegisterClass(pdev, pclass) */
//    pdev->pUserData = fops; /* eq. to USBD_CUSTOM_HID_RegisterInterface(pdev, fops) */

    pdev->pClass = pclass;
    pdev->pUserData = fops;
    pdev->pDesc = pdesc;
    pdev->dev_state = USBD_STATE_DEFAULT;
    pdev->id = id;
    (void) USBD_LL_Init(pdev);

    return USBD_OK;
}

USBD_StatusTypeDef USB_wait_idle(USBD_HandleTypeDef *pdev, uint32_t delay_max)
{
    USBD_CUSTOM_HID_HandleTypeDef *hhid;
    uint32_t wait = delay_max + (uint32_t) (HAL_GetTickFreq());
    volatile uint32_t tickstart;

    if (pdev == NULL) {
        return USBD_FAIL;
    }
    tickstart = HAL_GetTick();
    while (pdev->dev_state != USBD_STATE_CONFIGURED) {
        if ((HAL_GetTick() - tickstart) >= wait) {
            return USBD_FAIL;
        }
        __NOP();
    }
    hhid = pdev->pClassData;
    if (hhid == NULL) {
        return USBD_FAIL;
    }
    while (hhid->state != CUSTOM_HID_IDLE) {
        if ((HAL_GetTick() - tickstart) >= wait) {
            return USBD_BUSY;
        }
        __NOP();
    }

    return USBD_OK;
}

USBD_StatusTypeDef USB_presskey(USBD_HandleTypeDef *pdev,
                                uint8_t ep_addr,
                                uint8_t modifier,
                                uint8_t keycode1)
{
    uint32_t wait = USB_MAX_DELAY_MS + (uint32_t) (HAL_GetTickFreq());
    volatile uint32_t tickstart;

    /* Build 'key press' USB data */
    USB_HID_KBD_buf[0] = modifier; /* MODIFIER */
    USB_HID_KBD_buf[2] = keycode1; /* KEYCODE 1 */
    /* Transmit data */
    tickstart = HAL_GetTick();
    while (USB_HID_send_report(pdev, ep_addr, USB_HID_KBD_buf,
                               (uint16_t) USB_KEYBOARD_REPORT_SIZE) != USBD_OK) {
        if ((HAL_GetTick() - tickstart) >= wait) {
            return USBD_FAIL;
        }
        __NOP();
    }

    /* PRESS delay */
    HAL_Delay(KEY_PRESS_DELAY_MS);
    wait += KEY_PRESS_DELAY_MS;

    /* Build 'key release' USB data  */
    USB_HID_KBD_buf[0] = KEY_MODIF_NONE; /* MODIFIER */
    USB_HID_KBD_buf[2] = KEY_NONE; /* KEYCODE 1 */
    /* Transmit data */
    while (USB_HID_send_report(pdev, ep_addr, USB_HID_KBD_buf,
                               (uint16_t) USB_KEYBOARD_REPORT_SIZE) != USBD_OK) {
        if ((HAL_GetTick() - tickstart) >= wait) {
            return USBD_FAIL;
        }
        __NOP();
    }

    return USBD_OK;
}

void USB_presskey_handle()
{
    if (USB_HID_kbd_flag != KEY_NONE) {
        LED_IND_ON(); /* LED ON */
        /* Press key */
        if (USB_presskey(&hUsbDeviceFS, CUSTOM_HID_EPIN_ADDR, KEY_MODIF_NONE,
                         USB_HID_kbd_flag) != USBD_OK) {
            NVIC_SystemReset(); /* System Reset */
            //Error_Handler();
        }
        USB_HID_kbd_flag = KEY_NONE;
        /* Debounce delay */
        HAL_Delay(KEY_DEBOUNCE_DELAY_MS);
        /* Reset kbd_flag value to get new order */
        LED_IND_OFF(); /* LED OFF */
    }
}

/* =================================== EOF ================================== */
