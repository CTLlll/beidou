/**
 * @file app_main.c
 * @brief 北斗定位+地标播报系统 - 主应用初始化
 * @note 复制此文件内容到 CubeMX 生成的 main.c 的 USER CODE 区域
 */

/* ============================================
   第一步：在 main.c 文件顶部添加以下 include
   ============================================ */
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "spi.h"

// 业务模块头文件
#include "bd_app.h"
#include "bd_geo.h"
#include "bd_landmarks.h"
#include "bd_nmea.h"
#include "bd_uart_ring.h"
#include "tts_driver.h"
#include "ui_display.h"

// LCD 驱动（来自你的示例代码）
extern void LCD_Init(void);
extern void LCD_Fill(u16 xsta, u16 ysta, u16 xend, u16 yend, u16 color);
extern void LCD_ShowString(u16 x, u16 y, const u8 *p, u16 fc, u16 bc, u8 sizey);

/* ============================================
   第二步：定义缓冲区（放在 USER CODE BEGIN PV）
   ============================================ */

/* USER CODE BEGIN PV */

// 北斗模块接收缓冲区
static uint8_t bd_rx_buf[256];
static char bd_nmea_line[128];

// 地标存储
#define LANDMARK_CAP 10
static bd_landmark_t landmark_storage[LANDMARK_CAP];

// bd_app 实例
static bd_app_t bd_app_instance;

/* USER CODE END PV */

/* ============================================
   第三步：实现回调函数（放在 USER CODE BEGIN 0）
   ============================================ */

/* USER CODE BEGIN 0 */

// 进入地标回调：触发语音播报 + 屏幕显示
static void on_enter_landmark(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos) {
    // 屏幕显示地标标题
    ui_show_landmark(lm->title);
    
    // 触发 TTS 播报
    if (lm->speech_text) {
        tts_speak(lm->speech_text);
    }
}

// 离开地标回调：清屏
static void on_exit_landmark(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos) {
    ui_clear();
}

// 位置更新回调：调试显示经纬度（可选）
static void on_position_update(const bd_nmea_position_t *pos) {
    if (pos->fix == BD_NMEA_FIX_VALID) {
        // 可选：实时显示位置信息
        // ui_show_position(pos->ll.lat_deg, pos->ll.lon_deg, pos->sats);
    }
}

/* USER CODE END 0 */

/* ============================================
   第四步：在 main() 的 USER CODE BEGIN 2 初始化
   ============================================ */

void SystemClock_Config(void);
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_SPI2_Init();

    /* USER CODE BEGIN 2 */
    
    // 1. 初始化 UI 显示（LCD）
    ui_display_init();
    
    // 2. 初始化 TTS 模块
    tts_init();
    tts_set_volume(6);  // 设置默认音量
    
    // 3. 初始化 bd_app 业务模块
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
    
    // 4. 添加地标数据（示例）
    bd_app_add_landmark(&bd_app_instance, &(bd_landmark_t){
        .id = 1,
        .ll = {31.2304, 121.4737},  // 上海人民广场
        .enter_radius_m = 50.0f,
        .title = "上海人民广场",
        .speech_text = "欢迎来到上海人民广场，这里是上海市中心"
    });
    
    bd_app_add_landmark(&bd_app_instance, &(bd_landmark_t){
        .id = 2,
        .ll = {31.1899, 121.3197},  // 上海南站
        .enter_radius_m = 80.0f,
        .title = "上海南站",
        .speech_text = "上海南站到了"
    });
    
    // 可继续添加更多地标...
    
    /* USER CODE END 2 */

    while (1) {
        /* USER CODE BEGIN 3 */
        
        // 主循环：每 100ms 调用一次 bd_app_poll
        bd_app_poll(&bd_app_instance);
        HAL_Delay(100);
        
        /* USER CODE END 3 */
    }
}

/* ============================================
   第五步：串口中断处理（复制到 stm32f1xx_it.c）
   ============================================ */

/*
   在 stm32f1xx_it.c 的 USART1_IRQHandler() 里添加：
   
   uint8_t byte;
   if (READ_BIT(USART1->SR, USART_SR_RXNE)) {
       byte = (uint8_t)(USART1->DR & 0xFF);
       bd_app_on_uart_rx_byte(&bd_app_instance, byte);
   }
*/

/* ============================================
   总结：复制到 CubeMX 工程的步骤
   ============================================

1. 复制 include/ 和 src/ 下的业务代码到工程目录
2. 在 Keil 里把 bd_*.c, tts_driver.c, ui_display.c 添加到工程
3. 把示例代码的 HARDWARE/LCD/ 复制过去（lcd.c, lcd.h, lcd_init.c, lcdfont.h 等）
4. 在 Keil 的 Include Path 里添加 ..\..\include
5. 修改 main.c 如上所示（替换 USER CODE 区域）
6. 修改 stm32f1xx_it.c 添加 USART1 中断处理
7. 编译、下载、运行
*/