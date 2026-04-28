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

/** ‰∏????ÁÆ??©???ç??????Áß???????Á????æ?§ß?è?È?ç‰?Æ??????È??ÁÆ????NMEA Á?? 1Hz ??? 30~100 ?ù??è? */
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
/** Á?? 1??π‰????? LCD ?????????‰∏ç???‰∏π?©??????????????????????‰∏? 0 */
#define BD_HW_SELFTEST 0
/** BD_HW_SELFTEST=1 ?????π0=??????????ù????1=MADCTL ????èè???2=????≠??????? ui_lcd_text_demo */
#define BD_LCD_TEST_KIND 2
/** Á?? 1??π??π‰?ç??æ?∞?È??Á?? ui_show_position???‰∏Æ??è?????æ????∞???? ui_show_bd_live ‰??È??‰∏? */
#define BD_DEBUG_LCD_POSITION 0
/**
 * Á?? 1??π??è????????∞????????π‰?ç???Á?? USART2 PA2 TX ????ç∞?ß??æê?êÆÁπ??çÅ????????????TTS ???Á?® USART1????è?‰∏Æ?∞????????≠???????
 */
#define BD_DEBUG_UART_POS 0
/**
 * Á?? 1??π‰∏©Á???êÆ TTS???USART1 PA9?????≠?©?‰∏???? "OK"???È????Å?®??ù?È????????È?è‰?ß?è???? 0???
 */
#ifndef BD_TTS_BOOT_SAY_OK
#define BD_TTS_BOOT_SAY_OK 1
#endif
/**
 * Á?? 1??π‰????? MCU Á????? USART1 ????è????????????≠??????????®??ù? TX‚??PA10???Á?®?ù?È??Á??Á?≠?Æ? PA9(TX) ‰∏? PA10(RX)???
 * ‰∏©Á???êÆÁ????è‰∏© LB OK / LB FAIL??????????????? 0?????®??è??πÁÆ? TTS ?ç†Á?® USART1????????æÁÆ?????????≠??? TTS RX???
 */
#define BD_USART1_LOOPBACK_SELFTEST 0
/** ???(HMI)???? */
#ifndef BD_TJC_HMI_ENABLE
#define BD_TJC_HMI_ENABLE 1
#endif
/* Page switching: use page index (most robust). If you prefer names, change to "main"/etc. */
#define BD_TJC_PAGE_BOOT  "0"
#define BD_TJC_PAGE_1     "1"
#define BD_TJC_PAGE_2     "2"

/* PA0???????????????????(???)=1?????????0 */
#ifndef BD_KEY_ACTIVE_LOW
#define BD_KEY_ACTIVE_LOW 1
#endif

/* ???????????? 5s ???? page2 ???????? */
#ifndef BD_KEY_LONG_PRESS_MS
#define BD_KEY_LONG_PRESS_MS 2000u
#endif
#ifndef BD_TEST_ACTION_DELAY_MS
#define BD_TEST_ACTION_DELAY_MS 5000u
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

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
static uint8_t s_key_prev_down;
static uint32_t s_key_down_tick;
static uint8_t s_test_pending;
static uint32_t s_test_deadline_tick;
static uint8_t s_test_active;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void bd_update_rx_rate(void);
static void bd_poll_nav_key(void);
#if BD_TJC_HMI_ENABLE
static void bd_tjc_send_cmd(const char *cmd);
static void bd_tjc_show_page(const char *page_name);
#endif
static void bd_poll_test_action(void);
#if BD_USART1_LOOPBACK_SELFTEST && !BD_USE_USART2_FOR_GNSS
static void bd_uart1_loopback_demo(void);
#endif
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Enter landmark callback
static void on_enter_landmark(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos) {
#if BD_TJC_HMI_ENABLE
    /* ??1??? page1 */
    bd_tjc_show_page(BD_TJC_PAGE_1);
#endif
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

#if BD_TJC_HMI_ENABLE
static void bd_tjc_send_cmd(const char *cmd) {
    static const uint8_t end3[3] = {0xFF, 0xFF, 0xFF};
    if (!cmd) {
        return;
    }
    (void)HAL_UART_Transmit(&huart3, (uint8_t *)cmd, (uint16_t)strlen(cmd), 200);
    (void)HAL_UART_Transmit(&huart3, (uint8_t *)end3, 3, 100);
}

static void bd_tjc_show_page(const char *page_name) {
    char cmd[32];
    if (!page_name) {
        return;
    }
    (void)snprintf(cmd, sizeof(cmd), "page %s", page_name);
    bd_tjc_send_cmd(cmd);
}
#endif

static void bd_poll_test_action(void) {
    if (!s_test_pending) {
        return;
    }
    const uint32_t now = HAL_GetTick();
    if ((int32_t)(now - s_test_deadline_tick) < 0) {
        return;
    }
    /* ???????? page2 + ??????? */
    s_test_pending = 0u;
    s_test_active = 1u;
#if BD_TJC_HMI_ENABLE
    bd_tjc_show_page(BD_TJC_PAGE_2);
#endif
    /* GBK bytes: ????????????????????? */
    (void)tts_speak("\xB2\xE2\xCA\xD4\xBE\xB0\xB5\xE3\xA3\xBA\xD5\xE2\xCA\xC7\xD2\xBB\xB8\xF6\xB0\xB2\xBE\xB2\xB5\xC4\xD0\xA1\xC7\xF8\xA3\xAC\xBB\xB7\xBE\xB3\xD5\xFB\xBD\xE0\xD2\xCB\xBE\xD3");
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
  MX_USART3_UART_Init();
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

  // 2. Initialize TTS module???USART1 PA9 TX ‚?? TTS RX???
  tts_init();
  tts_set_volume(6);
#if BD_TTS_BOOT_SAY_OK
  HAL_Delay(150);
  (void)tts_speak("OK");
#endif
#if BD_TJC_HMI_ENABLE
  bd_tjc_send_cmd("bkcmd=0");
  bd_tjc_send_cmd("sleep=0");
  bd_tjc_send_cmd("dim=100");
  bd_tjc_show_page(BD_TJC_PAGE_BOOT);
#endif
  /* key/test state */
  s_key_prev_down = 0u;
  s_key_down_tick = 0u;
  s_test_pending = 0u;
  s_test_deadline_tick = 0u;
  s_test_active = 0u;

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

  /* ‰∏©Á??È????§???È?≠??π??? PA0 ??ç????ß???∞?†???§??π‰∏Æ??≠?©? */
  bd_app_set_landmark_enabled(&bd_app_instance, false);

  // 4. Add landmarks?????æÈ??Á?????117.12780039, 31.82965785????ç©??? 1000m???
  bd_app_add_landmark(&bd_app_instance, &(bd_landmark_t){
      .id = 1,
      .ll = {31.82965785, 117.12780039},
      .enter_radius_m = 1000.0f,
      .title = "Harbin Music Park",
      /* GBK bytes: ??????????? */
      .speech_text = "\xBB\xB6\xD3\xAD\xC0\xB4\xB5\xBD\xB9\xFE\xB6\xFB\xB1\xF5\xD2\xF4\xC0\xD6\xB9\xAB\xD4\xB0"
  });

  /* ????è?ÁÆ???ù?ß????È?????‰?? USART1 ?????∞ */
  bd_debug_uart_forward_init();
  /* ÁÆ????Á?????????∞?Á???êÆ??ç??? RXNE */
#if !BD_USE_USART2_FOR_GNSS
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
#else
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
#endif
  /* Á????§ PA2‚??USB-TTL‚??PC ????ê?È?π????Æ??ß??≠??©???Å?ßÅ bd_debug_uart RAW_FORWARD */
  bd_debug_uart_boot_line();
#if BD_USART1_LOOPBACK_SELFTEST && !BD_USE_USART2_FOR_GNSS
  bd_uart1_loopback_demo(); /* ‰∏ç?????æ??????????∞???è??? 0 ??çÁ?? */
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
    /* ?????∞‰????® USART1_IRQHandler??π??? DR??Åbd_ring_push??Å????è?????≠§?§??è???? UI/??????‰∏π?©? */
    bd_update_rx_rate();
    bd_debug_uart_forward_flush();
    bd_poll_nav_key();
    bd_poll_test_action();
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
  /* USER CODE END WHILE */
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

/** PA0 ???È????π???È?®‰∏©?????Å???‰∏?‰∏?‰?Æ?????è???Á?≠???????ç???∞?†?+TTS ?©???????NAV RUN/STOP??? */
static void bd_poll_nav_key(void) {
  static uint32_t s_quiet_until;
  const uint32_t now = HAL_GetTick();
  if (now < s_quiet_until) {
    return;
  }
  const GPIO_PinState pin = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
#if BD_KEY_ACTIVE_LOW
  const uint8_t down = (pin == GPIO_PIN_RESET) ? 1u : 0u;
#else
  const uint8_t down = (pin == GPIO_PIN_SET) ? 1u : 0u;
#endif

  /* press edge */
  if (!s_key_prev_down && down) {
    s_key_down_tick = now;
  }

  /* release edge */
  if (s_key_prev_down && !down) {
    const uint32_t held = (uint32_t)(now - s_key_down_tick);
    s_quiet_until = now + 120u;
    if (held >= BD_KEY_LONG_PRESS_MS) {
      /* long press: enter test mode (no immediate page/tts), schedule after 5s */
      s_test_active = 0u;
      s_test_pending = 1u;
      s_test_deadline_tick = now + BD_TEST_ACTION_DELAY_MS;

      /* entering test mode implies leaving nav mode */
      bd_app_set_landmark_enabled(&bd_app_instance, false);
      ui_bd_live_reset();
#if BD_TJC_HMI_ENABLE
      bd_tjc_show_page(BD_TJC_PAGE_BOOT);
#endif
    } else {
      /* short press */
      if (s_test_pending || s_test_active) {
        /* exit test mode -> standby */
        s_test_pending = 0u;
        s_test_active = 0u;
        bd_app_set_landmark_enabled(&bd_app_instance, false);
        ui_bd_live_reset();
#if BD_TJC_HMI_ENABLE
        bd_tjc_show_page(BD_TJC_PAGE_BOOT);
#endif
      } else {
        /* toggle NAV RUN/STOP (??????) */
        const bool next = !bd_app_instance.landmark_enabled;
        bd_app_set_landmark_enabled(&bd_app_instance, next);
        ui_bd_live_reset();
        if (!next) {
          /* leaving RUN -> standby shows page0 */
#if BD_TJC_HMI_ENABLE
          bd_tjc_show_page(BD_TJC_PAGE_BOOT);
#endif
        }
      }
    }
  }

  s_key_prev_down = down;
}

static void bd_update_rx_rate(void) {
  const uint32_t now = HAL_GetTick();
  const uint32_t dt = (uint32_t)(now - bd_last_rate_tick);
  /*
   * NMEA ?§π‰∏?Á?? 1Hz Á?Å?è???????Á?? 500ms Á???è?????????????‰∏?Á?????‰∏©Á?Å?è???Å‰∏?‰∏?Á????†‰?Æ?????????
   * ?ç?Á????? ‚??/s‚?? ?∞?‰?π??®‰∏§‰∏???∞‰??È?¥??????‰????? 9 ‰∏? 0?????????Á?®Á?? 1s Á???è???¥Á®????
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
