/* SPDX-License-Identifier: ISC */
/**
 * Licensing and copyright: See the file `LICENSE.txt`
 */

#include "led_ind.h"

void LED_IND_ON()
{
    /* LED ON */
    HAL_GPIO_WritePin(IND_LED_GPIO_Port, IND_LED_Pin, GPIO_PIN_RESET);
}

void LED_IND_OFF()
{
    /* LED OFF */
    HAL_GPIO_WritePin(IND_LED_GPIO_Port, IND_LED_Pin, GPIO_PIN_SET);
}

/* =================================== EOF ================================== */
