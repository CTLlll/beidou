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
#include "bd_gnss_uart.h"
#include <stdio.h>

/** 主循环节拍（毫秒）；略增大可降低轮询频率，NMEA 约 1Hz 时 30~100 均可 */
#ifndef BD_MAIN_LOOP_DELAY_MS
#define BD_MAIN_LOOP_DELAY_MS 50u
#endif
#include <string.h>
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
 * 置 1：每次得到有效定位时经 USART2 PA2 TX 打印解析后的十进制度（TTS 已用 USART1，可与调试共存）。
 */
#define BD_DEBUG_UART_POS 0
/**
 * 置 1：上电后 TTS（USART1 PA9）播报一次 "OK"，验证模块链路；量产可改 0。
 */
#ifndef BD_TTS_BOOT_SAY_OK
#define BD_TTS_BOOT_SAY_OK 1
#endif
/**
 * 置 1：仅测 MCU 片内 USART1 收发。请先断开北斗模块 TX→PA10，用杜邦线短接 PA9(TX) 与 PA10(RX)，
 * 上电后看屏上 LB OK / LB FAIL；测完改回 0。注意：现 TTS 占用 USART1，测回环时请断开 TTS RX。
 */
#define BD_USART1_LOOPBACK_SELFTEST 0
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
static void bd_poll_nav_key(void);
#if BD_USART1_LOOPBACK_SELFTEST && !BD_USE_USART2_FOR_GNSS
static void bd_uart1_loopback_demo(void);
#endif
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

#if BD_USART1_LOOPBACK_SELFTEST && !BD_USE_USART2_FOR_GNSS
static void bd_uart1_loopback_demo(void)
{
  const uint8_t pat[] = { 0x55U, 0xAAU, 0x30U, 0x31U, 0x32U };
  char msg[48];
  uint32_t r0;
  uint32_t i0;
  uint32_t got;

  LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
  LCD_ShowString(2, 4, (const uint8_t *)"Disconnect GPS TX", YELLOW, BLACK, 16);
  LCD_ShowString(2, 22, (const uint8_t *)"Short PA9 to PA10", WHITE, BLACK, 16);
  LCD_ShowString(2, 40, (const uint8_t *)"TX=PA9 RX=PA10", CYAN, BLACK, 16);
  HAL_Delay(2500);

  r0 = bd_uart1_rx_bytes;
  i0 = bd_uart1_irq_entries;
  (void)HAL_UART_Transmit(&huart1, (uint8_t *)pat, sizeof(pat), 200);
  HAL_Delay(80);

  got = bd_uart1_rx_bytes - r0;
  LCD_Fill(0, 48, LCD_W, LCD_H, BLACK);
  snprintf(msg, sizeof(msg), "%s R+%lu I+%lu", (got >= sizeof(pat)) ? "LB OK" : "LB FAIL",
           (unsigned long)got, (unsigned long)(bd_uart1_irq_entries - i0));
  LCD_ShowString(2, 52, (const uint8_t *)msg, (got >= sizeof(pat)) ? GREEN : RED, BLACK, 16);

  snprintf(msg, sizeof(msg), "[LB] got=%lu irq_delta=%lu\r\n", (unsigned long)got,
           (unsigned long)(bd_uart1_irq_entries - i0));
  (void)HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)strlen(msg), 200);

  while (1) {
    HAL_Delay(500);
  }
}
#endif

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

  // 2. Initialize TTS module（USART1 PA9 TX → TTS RX）
  tts_init();
  tts_set_volume(6);
#if BD_TTS_BOOT_SAY_OK
  HAL_Delay(150);
  (void)tts_speak("OK");
#endif

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

  /* 上电默认关闭：按 PA0 才开始地标判定与播报 */
  bd_app_set_landmark_enabled(&bd_app_instance, false);

  // 4. Add landmarks（实验点：117.12780039, 31.82965785；半径 1000m）
  bd_app_add_landmark(&bd_app_instance, &(bd_landmark_t){
      .id = 1,
      .ll = {31.82965785, 117.12780039},
      .enter_radius_m = 1000.0f,
      .title = "USTC IAT",
      .speech_text = "这是科大先进技术研究院"
  });

  /* 转发环初始化须早于 USART1 收数 */
  bd_debug_uart_forward_init();
  /* 环形缓冲已就绪后再开 RXNE */
#if !BD_USE_USART2_FOR_GNSS
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
#else
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
#endif
  /* 确认 PA2→USB-TTL→PC 是否通；原始字节流见 bd_debug_uart RAW_FORWARD */
  bd_debug_uart_boot_line();
#if BD_USART1_LOOPBACK_SELFTEST && !BD_USE_USART2_FOR_GNSS
  bd_uart1_loopback_demo(); /* 不返回；测完将宏改 0 再编 */
#endif
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
    /* 收数仅在 USART1_IRQHandler：读 DR、bd_ring_push、转发；此处只刷 UI/轮询业务 */
    bd_update_rx_rate();
    bd_debug_uart_forward_flush();
    bd_poll_nav_key();
    bd_app_poll(&bd_app_instance);
    bd_debug_uart_forward_flush();
    {
      const uint32_t now = HAL_GetTick();
      if ((uint32_t)(now - bd_last_ui_tick) >= 500u) {
        ui_show_bd_live(&bd_app_instance.pos,
                        bd_app_instance.landmark_enabled,
                        (bd_app_instance.stat_parse_ok > 0u));
        bd_last_ui_tick = now;
      }
    }
    HAL_Delay(BD_MAIN_LOOP_DELAY_MS);
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

/** PA0 按键：内部上拉、按下为低；每次短按切换地标+TTS 功能（NAV RUN/STOP） */
static void bd_poll_nav_key(void) {
  static uint8_t s_inited;
  static uint8_t s_prev_down;
  static uint32_t s_quiet_until;
  const uint32_t t = HAL_GetTick();
  if (t < s_quiet_until) {
    return;
  }
  const uint8_t down = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) ? 1u : 0u;
  if (!s_inited) {
    s_inited = 1u;
    s_prev_down = down;
    return;
  }
  if (s_prev_down == 0u && down == 1u) {
    s_quiet_until = t + 280u;
    bd_app_set_landmark_enabled(&bd_app_instance, !bd_app_instance.landmark_enabled);
    ui_bd_live_reset();
  }
  s_prev_down = down;
}

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
