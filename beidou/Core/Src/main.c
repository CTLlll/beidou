/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bd_app.h"
#include "bd_landmarks.h"
#include "tts_driver.h"
#include "ui_display.h"
#include "lcd_hal.h"
#include "bd_debug_uart.h"
#include "bd_uart_stats.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/** 置 1：仅跑 LCD 测试，不跑业务；测北斗时请改为 0 */
#define BD_HW_SELFTEST 0
/** BD_HW_SELFTEST=1 时：0=分区色块；1=MADCTL 扫描；2=文字测试 ui_lcd_text_demo */
#define BD_LCD_TEST_KIND 2
/** 置 1：定位回调里用 ui_show_position；与屏幕实时调试 ui_show_bd_live 二选一 */
#define BD_DEBUG_LCD_POSITION 0
/**
 * 置 1：每次得到有效定位时经 USART2(PA2) 打印解析后的十进制度（测 MCU 解析链用）。
 * 与 TTS 共串口时请断开 TTS 的 RX，或只把 PA2 接 PC USB-TTL 的 RX。
 */
#define BD_DEBUG_UART_POS 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* USER CODE BEGIN PV */
bd_app_t bd_app_instance;
static uint32_t bd_rx_rate_per_sec;
static uint32_t bd_last_rate_tick;
static uint32_t bd_last_rate_count;
static uint32_t bd_last_ui_tick;
static uint8_t bd_rx_buf[256];
static char bd_nmea_line[128];
#define LANDMARK_CAP 10
static bd_landmark_t landmark_storage[LANDMARK_CAP];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void bd_update_rx_rate(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Enter landmark callback
static void on_enter_landmark(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos) {
    ui_show_landmark(lm->title);
    if (lm->speech_text) {
        tts_speak(lm->speech_text);
    }
}

// Exit landmark callback
static void on_exit_landmark(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos) {
    ui_clear();
}

static void on_position_update(const bd_nmea_position_t *pos) {
#if BD_DEBUG_LCD_POSITION
    if (pos && pos->fix == BD_NMEA_FIX_VALID && !bd_app_instance.in_landmark) {
        ui_show_position(pos->ll.lat_deg, pos->ll.lon_deg, pos->sats);
    }
#else
    (void)pos;
#endif
#if BD_DEBUG_UART_POS
    if (pos && pos->fix == BD_NMEA_FIX_VALID) {
        bd_debug_uart_print_pos(pos);
    }
#endif
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
  MX_USART1_UART_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

#if BD_HW_SELFTEST
  LCD_Init();
  while (1) {
#if BD_LCD_TEST_KIND == 2
    ui_lcd_text_demo();
#elif BD_LCD_TEST_KIND == 1
    ui_lcd_madctl_probe();
#else
    ui_lcd_region_map_test();
#endif
    HAL_Delay(400);
  }
#else
  // 1. Initialize LCD display
  ui_display_init();
  ui_bd_live_reset();

  // 2. Initialize TTS module
  tts_init();
  tts_set_volume(6);

  // 3. Initialize bd_app
  bd_app_callbacks_t cbs = {
      .on_enter = on_enter_landmark,
      .on_exit = on_exit_landmark,
      .on_pos  = on_position_update
  };

  bd_app_init(&bd_app_instance,
              bd_rx_buf, sizeof(bd_rx_buf),
              bd_nmea_line, sizeof(bd_nmea_line),
              landmark_storage, LANDMARK_CAP,
              cbs);

  // 4. Add landmarks
  bd_app_add_landmark(&bd_app_instance, &(bd_landmark_t){
      .id = 1,
      .ll = {31.2304, 121.4737},
      .enter_radius_m = 50.0f,
      .title = "Shanghai",
      .speech_text = "Welcome to Shanghai People's Square"
  });

  bd_app_add_landmark(&bd_app_instance, &(bd_landmark_t){
      .id = 2,
      .ll = {31.1899, 121.3197},
      .enter_radius_m = 80.0f,
      .title = "Shanghai South",
      .speech_text = "Welcome to Shanghai South Station"
  });

  /* 转发环初始化须早于 USART1 收数 */
  bd_debug_uart_forward_init();
  /* 环形缓冲已就绪后再开 RXNE；否则模块一发数就进中断，cap=0 会跑飞/黑屏 */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  /* 确认 PA2→USB-TTL→PC 是否通；原始字节流见 bd_debug_uart RAW_FORWARD */
  bd_debug_uart_boot_line();
#endif

  bd_last_rate_tick = HAL_GetTick();
  bd_last_rate_count = bd_uart1_rx_bytes;
  bd_last_ui_tick = bd_last_rate_tick;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

#if BD_HW_SELFTEST
    HAL_Delay(500);
#else
    bd_update_rx_rate();
    bd_debug_uart_forward_flush();
    bd_app_poll(&bd_app_instance);
    bd_debug_uart_forward_flush();
    {
      const uint32_t now = HAL_GetTick();
      if ((uint32_t)(now - bd_last_ui_tick) >= 500u) {
        ui_show_bd_live(&bd_app_instance.pos, bd_uart1_rx_bytes, bd_rx_rate_per_sec);
        bd_last_ui_tick = now;
      }
    }
    HAL_Delay(30);
#endif

    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

static void bd_update_rx_rate(void) {
  const uint32_t now = HAL_GetTick();
  const uint32_t dt = (uint32_t)(now - bd_last_rate_tick);
  /*
   * NMEA 多为约 1Hz 突发；若用 500ms 窗口，容易这一窗赶上突发、下一窗几乎没有，
   * 换算成 “/s” 就会在两个数之间跳（例如 9 与 0）。改用约 1s 窗口更稳。
   */
  if (dt < 1000u) {
    return;
  }
  bd_rx_rate_per_sec = (bd_uart1_rx_bytes - bd_last_rate_count) * 1000u / dt;
  bd_last_rate_count = bd_uart1_rx_bytes;
  bd_last_rate_tick = now;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
