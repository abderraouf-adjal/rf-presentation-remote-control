/* USER CODE BEGIN Header */
/* SPDX-License-Identifier: ISC */
/**
 * Licensing and copyright: See the file `LICENSE.txt`
 *
 * Hardware:
 *   UART RX (Input) Pin: GPIO_PIN_11 GPIOB (PB11)
 *   Indication LED Pin: GPIO_PIN_13 GPIOC (PC13)
 *   USB FS interface in device mode.
 *   USB Host (Personal Computer) must be able to provide 200mA
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_hid_kbd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//extern USBD_HandleTypeDef hUsbDeviceFS;
//volatile uint8_t sleep_lock = 0x00;
/* Buffer used for reception */
uint8_t USART3_RxBuffer[USART3_RXBUFFERSIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Send key-press signal. */
    if (USB_HID_kbd_flag == KEY_NONE) {
        if (USART3_RxBuffer[0] == 'U') {
            USB_HID_kbd_flag = KEY_ARW_U;
        } else if (USART3_RxBuffer[0] == 'D') {
            USB_HID_kbd_flag = KEY_ARW_D;
        }
    }

    if (HAL_UART_Receive_IT(&huart3, (uint8_t*) USART3_RxBuffer,
    USART3_RXBUFFERSIZE) != HAL_OK) {
        Error_Handler();
    }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART3_UART_Init();
    /* USER CODE BEGIN 2 */
    LED_IND_ON(); /* LED ON */
    /* Init & config USB desc. */
    if (USB_init_desc(&hUsbDeviceFS, &FS_Desc, DEVICE_FS,
                      &USBD_CustomHID_fops_FS, &USBD_CUSTOM_HID) != USBD_OK) {
        NVIC_SystemReset(); /* System Reset */
        //Error_Handler();
    }
    /* Give it time to stable connector wires with the host */
    //HAL_Delay(USB_BOOT_DELAY_MS);
    /*while (HAL_GetTick() <= USB_BOOT_DELAY_MS) {
     __NOP();
     }*/
    /* Start USB */
    HAL_PCD_Start(hUsbDeviceFS.pData);
    if (USB_wait_idle(&hUsbDeviceFS, USB_MAX_DELAY_MS) != USBD_OK) {
        NVIC_SystemReset(); /* System Reset */
        //Error_Handler();
    }
    //HAL_DBGMCU_EnableDBGSleepMode();
    //HAL_DBGMCU_EnableDBGStopMode();

    if (HAL_UART_Receive_IT(&huart3, (uint8_t*) USART3_RxBuffer,
    USART3_RXBUFFERSIZE) != HAL_OK) {
        NVIC_SystemReset(); /* System Reset */
        //Error_Handler();
    }

    LED_IND_OFF(); /* LED OFF */

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
//        /* SLEEP MODE */
//        sleep_lock = 0xFF;
//        HAL_SuspendTick();
//        HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
//        while (sleep_lock != 0x00) {
//        }
//        /* Out from SLEEP MODE */
//        HAL_ResumeTick();

        USB_presskey_handle();
        (void) USB_wait_idle(&hUsbDeviceFS, USB_MAX_DELAY_MS);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {
    }
    LL_RCC_HSE_Enable();

    /* Wait till HSE is ready */
    while (LL_RCC_HSE_IsReady() != 1) {

    }
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_6);
    LL_RCC_PLL_Enable();

    /* Wait till PLL is ready */
    while (LL_RCC_PLL_IsReady() != 1) {

    }
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_4);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {

    }
    LL_SetSystemCoreClock(12000000);

    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* Try to stop USB */
    (void) USBD_Stop(&hUsbDeviceFS);
    LED_IND_ON(); /* LED ON */
    //HAL_NVIC_SystemReset();

    /* Implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
        __NOP();
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
