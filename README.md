# Beidou STM32F103 业务代码（可移植）

tts http://www.hellostem.cn/?ljiliechuanganqipeijiandeng/365.html=
32 https://product.abrobot.club/ABrobot%E4%BA%A7%E5%93%81%E8%B5%84%E6%96%99%E4%B8%AD%E5%BF%83/stm32%E5%BC%80%E5%8F%91%E6%9D%BF%E7%B3%BB%E5%88%97/STM32F103%20TFT%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99/%E5%BC%95%E8%84%9A%E5%9B%BE.png

这份代码提供"业务层/算法层"，不绑定 Keil 工程和 HAL/StdPeriph，后续你把它嵌入 STM32F103 工程即可。

## 硬件配置汇总

### 北斗模块 (SR2631)

| 项目 | 值 |
|------|------|
| 型号 | SR2631 |
| 协议 | NMEA-0183 |
| 波特率 | 115200 bps (8N1) |
| 引脚 | RX, TX, VCC, GND, PPS(可选) |
| 串口 | **USART2 (PA2=TX, PA3=RX)**（模块 TX→PA3） |

### TTS 语音模块 (VTX316)

| 项目 | 值 |
|------|------|
| 型号 | VTX316 (或兼容协议) |
| 波特率 | 115200 bps (8N1) |
| 编码 | GBK |
| 播放帧格式 | `0xFD 0x00 (长度+2) 0x01 0x05 + GBK文本` |
| 串口 | **USART1 (PA9=TX, PA10=RX)**（通常只接 PA9→TTS RX） |

### 板载按键（KEY）

| 项目 | 值 |
|------|------|
| GPIO | **PA0** |
| 模式 | 输入上拉，**按下为低** |
| 行为 | 短按切换 **NAV RUN/STOP**：STOP 时只解析显示，不判地标不播报；RUN 时进入地标播报一次 |

### 调试串口（可选）

- **PA2 (USART2_TX)** 可接 USB-TTL 查看启动信息与解析坐标输出（不影响 TTS，因为 TTS 已迁 USART1）。

### LCD 屏幕 (ST7735S)

| 项目 | 值 |
|------|------|
| 驱动芯片 | ST7735S |
| 接口 | **软件模拟 SPI**（GPIO 推挽输出，不依赖硬件 SPI 外设） |
| 分辨率 | 160×80 (横屏模式 USE_HORIZONTAL=2) |
| 方向 | 横屏显示 |

#### 引脚配置

| 功能 | GPIO | 说明 |
|------|------|------|
| SCLK (时钟) | PB3 | 模拟 SPI 时钟 |
| MOSI (数据) | PB5 | 模拟 SPI 数据输出 |
| RES | PB6 | 复位 |
| DC | PB4 | 数据/命令选择 |
| CS | PB7 | 片选 |
| BLK | PB8 | 背光控制 |

> 注意：PB3 与 JTAG 冲突，需要在 CubeMX 里禁用 JTAG（设 SWD_IO 或把 PB3 配为主功能）

#### CubeMX 配置

- **LCD 引脚**：PB3, PB4, PB5, PB6, PB7, PB8 全部设为 **GPIO Output**（推挽）
- **无需开启 SPI 外设**（软件模拟）
- **禁用 JTAG**：在 `SYS` 里把 `Debug` 设为 `Serial Wire` 或把 PB3 复用为普通 GPIO

## 目标功能

- MCU 预存一批地标（经纬度、进入阈值、显示文本/语音文本 ID）
- MCU 持续接收北斗 NMEA 语句，解析出当前位置（优先 RMC / GGA）
- 计算当前位置与地标距离；距离小于阈值判定"进入地标"，触发事件：
  - 屏幕显示对应信息
  - 播放对应文本（TTS 模块）
- 支持离开检测（滞回阈值，避免反复进出触发）

## 目录结构

```
include/          - 对外头文件（可直接放进 Keil 的 include path）
src/              - 实现文件
```

## 模块清单

| 模块 | 功能 |
|------|------|
| `bd_geo` | 经纬度距离计算（Haversine） |
| `bd_uart_ring` | 环形缓冲区，串口数据接收 |
| `bd_nmea` | NMEA-0183 解析（RMC/GGA） |
| `bd_landmarks` | 地标存储管理 |
| `bd_app` | 主业务状态机（位置更新→距离判断→触发回调） |
| `tts_driver` | TTS 模块发送驱动（VTX316 协议） |
| `ui_display` | LCD 显示适配层 |

## 嵌入步骤

### 1. CubeMX 配置

- **USART2**（北斗）：115200，8N1，开启 NVIC 中断
- **USART1**（TTS）：115200，8N1
- **LCD 引脚**：PB3, PB4, PB5, PB6, PB7 配置为对应功能
- **禁用 JTAG**：PB3 设为普通 GPIO（因为与 LCD SCL 冲突）

### 2. 代码集成

1. 串口中断/DMA 接收字节后，逐字节喂给 `bd_app_on_uart_rx_byte(...)`
2. 每隔 50~200ms 调一次 `bd_app_poll(...)`

### 3. 实现回调

```c
// 进入地标回调
void on_enter_callback(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos) {
    // 屏幕显示
    ui_show_landmark(lm->title);
    // 播放语音
    tts_speak(lm->speech_text);
}

// 离开地标回调（可选）
void on_exit_callback(const bd_landmark_t *lm, float distance_m, const bd_nmea_position_t *pos) {
    ui_clear();
}

// 位置更新回调（可选，用于调试）
void on_pos_callback(const bd_nmea_position_t *pos) {
    // 打印经纬度或卫星数
}
```

### 4. 添加地标数据

```c
// 添加示例地标
bd_app_add_landmark(&app, &(bd_landmark_t){
    .id = 1,
    .ll = {31.2304, 121.4737},  // 示例坐标
    .enter_radius_m = 50.0f,
    .title = "上海人民广场",
    .speech_text = "欢迎来到上海人民广场"
});
```

## 待完善

- [ ] TTS 驱动 (`tts_driver.h/c`)
- [ ] LCD 显示适配层 (`ui_display.h/c`)
- [ ] 主循环初始化代码模板