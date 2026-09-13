# Smart Weather Clock / 智能天气时钟

> STM32F407VET6-based smart weather clock with local environmental sensing, Wi-Fi weather retrieval, SNTP time synchronization, and an ST7789 display.  
> 基于 STM32F407VET6 的智能天气时钟，集成本地温湿度采集、Wi-Fi 天气获取、SNTP 网络校时和 ST7789 显示。

---

## 中文

### 1. 项目简介

本项目是一套运行于 **STM32F407VET6** 的嵌入式天气时钟。系统通过 ESP32 接入网络，获取当前天气及网络时间；通过 AHT20 获取室内温湿度；通过 ST7789 彩色屏幕显示时间、日期、室内环境、室外天气和 Wi-Fi 状态。

项目采用 **FreeRTOS** 将界面刷新与网络通信拆分为独立任务，以避免 HTTP 请求、ESP32 AT 指令等待等耗时操作造成页面卡顿。

### 2. 功能清单

| 功能 | 说明 |
| --- | --- |
| 时间与日期显示 | 从 STM32 RTC 每秒读取并局部刷新屏幕 |
| 网络校时 | ESP32 通过 SNTP 获取网络时间，启动后及每 24 小时校时一次 |
| 室内环境监测 | AHT20 通过 I2C2 采集温度、湿度，默认每 3 秒更新一次 |
| 室外天气 | ESP32 通过 HTTP 请求心知天气 API，默认每分钟更新一次 |
| Wi-Fi 状态 | 周期查询 ESP32 Wi-Fi 连接状态，默认每 5 秒更新一次 |
| 图形显示 | ST7789 通过 SPI2 刷新；大块 16 位像素数据使用 DMA 传输 |
| 调试日志 | `printf` 重定向至 USART1，输出至电脑串口助手 |

### 3. 硬件与接口

| 模块 | 接口 | 用途 |
| --- | --- | --- |
| STM32F407VET6 | Cortex-M4F | 主控，运行 FreeRTOS 与应用逻辑 |
| ST7789 | SPI2 | 彩色 LCD 显示；SPI 时钟约 10.5 MHz |
| AHT20 | I2C2 | 室内温湿度传感器 |
| ESP32 | USART2，115200-8-N-1 | Wi-Fi、HTTP 天气请求、SNTP 网络时间 |
| 电脑串口助手 | USART1，115200-8-N-1 | `printf` 日志输出 |
| RTC | LSE 优先，LSI 备选 | 本地持续计时 |

已在代码中配置的部分引脚如下：

| 外设 | 引脚 |
| --- | --- |
| USART1 | PA9 (TX), PA10 (RX) |
| USART2 | PA2 (TX), PA3 (RX) |
| I2C2 | PB10 (SCL), PB11 (SDA) |
| SPI2 | PB13 (SCK), PC2 (MISO), PC3 (MOSI) |

> ST7789 的片选、数据/命令、复位及背光等控制引脚由 `bsp/st7789_port/` 管理，请以该目录实现为准。

### 4. 软件架构

```text
main.c
  └─ application_start()
      ├─ 创建 application_task          ── UI、RTC、AHT20 的唯一操作任务
      ├─ 创建 network_task              ── ESP32 的唯一运行期通信任务
      ├─ 创建软件定时器                 ── 仅发送周期事件
      └─ 启动 FreeRTOS 调度器

application_task  ──请求通知──>  network_task
application_task  <──结果队列────  network_task
       │                                  │
       ├─ RTC / AHT20 / ST7789            └─ USART2 / ESP32 / HTTP / SNTP
       └─ USART1 printf 日志
```

#### 任务职责

| 任务 | 主要职责 | 不负责 |
| --- | --- | --- |
| `application_task` | 初始化页面、读取 RTC/AHT20、刷新 ST7789、消费网络结果 | HTTP 请求、等待 ESP32 AT 响应 |
| `network_task` | 查询 Wi-Fi、获取天气、获取 SNTP 时间、解析 ESP32 响应 | 直接刷新 LCD |

#### 定时任务

| 周期 | 事件 | 最终行为 |
| --- | --- | --- |
| 1 秒 | `APP_EVT_TIME_UPDATE` | 读取 RTC 并刷新时间/日期 |
| 3 秒 | `APP_EVT_INDOOR_UPDATE` | 采集并刷新室内温湿度 |
| 5 秒 | `APP_EVT_WIFI_UPDATE` | 请求查询 Wi-Fi 状态 |
| 1 分钟 | `APP_EVT_OUTDOOR_UPDATE` | 请求获取室外天气 |
| 1 天 | `APP_EVT_TIME_SYNC` | 请求 SNTP 网络校时 |

#### 任务通信

- **任务通知（Task Notification）**：传递“该执行什么工作”或“结果已准备好”等事件位。
- **长度为 1 的队列**：`weather_result_queue`、`wifi_result_queue`、`time_sync_result_queue` 仅保留最新结果，适合状态类数据，避免网络任务堆积过期数据。
- **屏幕所有权**：仅 `application_task` 调用 UI/ST7789 刷新函数，避免多任务同时写屏造成闪烁或显示冲突。

### 5. 时间同步机制

RTC 并不直接联网。系统启动并成功连接 Wi-Fi 后，ESP32 配置 SNTP；`network_task` 通过 `AT+CIPSNTPTIME?` 获取网络时间，解析后通过队列交给 `application_task`。后者调用 `RTC_Set_Time()`，最终由 `RTC_SetDate()` 与 `RTC_SetTime()` 写入 STM32 RTC 寄存器。

后续时间显示只读取本地 RTC。RTC 优先使用外部 32.768 kHz LSE 晶振；LSE 未就绪时自动退回内部 LSI。若未连接 VBAT 备份电源，断电后 RTC 时间会丢失，需要重新联网校时。

### 6. 目录结构

```text
app/
├─ main.c                    程序入口与调度器启动
├─ application/              FreeRTOS 任务、定时器、队列与业务调度
├─ bsp_module/               BSP 统一初始化入口
├─ ui/                       页面绘制与局部刷新
├─ weather/                  天气 JSON 文本解析
└─ wifi/                     ESP32 初始化、连接 Wi-Fi、SNTP 配置

bsp/
├─ i2c/                      I2C2 驱动
├─ rtc/                      STM32 RTC 配置、读写时间
├─ spi/                      SPI2 与显示 DMA 发送
├─ st7789_port/              ST7789 板级控制引脚
└─ uart/                     USART1 日志、USART2 ESP32 通信与接收环形缓冲区

drivers/
├─ aht20/                    AHT20 驱动
├─ esp32/                    ESP-AT 命令、HTTP、SNTP 时间解析
└─ st7789/                   ST7789 显示驱动

third_lib/                   FreeRTOS、SPL、字体和图片资源
utilities/                   通用环形缓冲区
Firmware/                    STM32 CMSIS 与 SPL
```

### 7. 构建与烧录

#### 环境要求

- CMake 3.15 或更高版本
- `arm-none-eabi-gcc` 工具链
- 支持 STM32F4 的烧录/调试工具，例如 ST-LINK

#### 构建示例

```bash
cmake -S . -B build
cmake --build build
```

构建完成后，`build/` 目录中将生成：

- `Smart_Weather_Clock.elf`
- `Smart_Weather_Clock.hex`
- `Smart_Weather_Clock.bin`
- `Smart_Weather_Clock.s19`

使用 ST-LINK 或你习惯的烧录工具，将 `.elf`、`.hex` 或 `.bin` 写入芯片即可。

### 8. 配置说明与安全提示

以下配置目前位于源码中，克隆项目后必须替换为自己的值：

| 配置 | 位置 | 说明 |
| --- | --- | --- |
| Wi-Fi SSID 和密码 | `app/wifi/wifi.c` | 用于 ESP32 连接网络 |
| 心知天气 API Key 和城市 | `app/application/application.c` 的 `WEATHER_URL` | 用于室外天气请求 |
| SNTP 时区 | `drivers/esp32/esp32.c` | 当前配置为 `8`，即 UTC+8 |

**请不要将真实 Wi-Fi 密码或 API Key 提交到公开仓库。** 推荐将其移至未追踪的本地配置头文件，并在 `.gitignore` 中忽略该文件。

### 9. 已知设计边界

- SPI DMA 当前为“DMA 传输 + 等待完成”的同步实现；它降低了 CPU 搬运像素的负担，但调用者仍会等待传输结束。
- ESP32 AT 命令等待过程可能阻塞 `network_task`，但不会阻塞 UI 任务；因此页面刷新不会被 HTTP 请求直接拖慢。
- LSI 的频率误差较大，建议硬件上安装 LSE 晶振，并使用 VBAT 保持 RTC。
- 天气响应采用轻量文本解析，不是完整 JSON 解析器；若 API 返回格式变化，需要同步调整解析代码。

---

## English

### 1. Overview

Smart Weather Clock is an embedded weather-clock firmware for the **STM32F407VET6**. It displays local time, date, indoor temperature/humidity, outdoor weather, and Wi-Fi status on an ST7789 LCD. An ESP32 provides Wi-Fi connectivity, HTTP weather retrieval, and SNTP time synchronization.

The firmware uses **FreeRTOS** to isolate UI updates from potentially slow ESP-AT, HTTP, and SNTP operations. This keeps the display responsive while network requests are in progress.

### 2. Features

| Feature | Description |
| --- | --- |
| Clock and calendar | Reads the STM32 RTC and refreshes changed UI fields every second |
| Network time synchronization | ESP32 obtains SNTP time at startup and once every 24 hours |
| Indoor sensing | AHT20 temperature and humidity sampling over I2C2, every 3 seconds by default |
| Outdoor weather | Current conditions retrieved from the Seniverse API over HTTP, every minute by default |
| Wi-Fi status | ESP32 connection state queried every 5 seconds by default |
| LCD rendering | ST7789 rendering through SPI2; 16-bit bulk pixel transfers use DMA |
| Debug logging | `printf` is redirected to USART1 for a host serial terminal |

### 3. Hardware Interfaces

| Device | Interface | Purpose |
| --- | --- | --- |
| STM32F407VET6 | Cortex-M4F | Main controller and FreeRTOS host |
| ST7789 | SPI2 | Color LCD; SPI clock is approximately 10.5 MHz |
| AHT20 | I2C2 | Indoor temperature and humidity sensor |
| ESP32 | USART2, 115200-8-N-1 | Wi-Fi, HTTP, and SNTP through ESP-AT |
| Host PC | USART1, 115200-8-N-1 | `printf` debug output |
| RTC | LSE preferred, LSI fallback | Local timekeeping |

Configured MCU pins:

| Peripheral | Pins |
| --- | --- |
| USART1 | PA9 (TX), PA10 (RX) |
| USART2 | PA2 (TX), PA3 (RX) |
| I2C2 | PB10 (SCL), PB11 (SDA) |
| SPI2 | PB13 (SCK), PC2 (MISO), PC3 (MOSI) |

### 4. Architecture

```text
main.c → application_start() → FreeRTOS scheduler
                                  │
                 ┌────────────────┴────────────────┐
                 │                                 │
        application_task                    network_task
        UI / RTC / AHT20                    ESP32 / HTTP / SNTP
                 │                                 │
                 └──── requests / latest-result queues ────┘
```

`application_task` is the sole owner of LCD/UI updates. `network_task` is the sole runtime owner of ESP32 communication. Software timers only post periodic events; they do not perform slow I/O themselves.

The project uses task notifications for event signaling and one-element queues for weather, Wi-Fi, and time-sync results. Keeping only the latest value is appropriate for state data and prevents stale network results from accumulating.

### 5. Time Synchronization

The STM32 RTC does not access the network directly. After Wi-Fi is connected, the ESP32 is configured for SNTP. `network_task` sends `AT+CIPSNTPTIME?`, parses the returned time, and passes it to `application_task` through a queue. `application_task` writes it into STM32 RTC registers through `RTC_SetDate()` and `RTC_SetTime()`.

The display subsequently reads local RTC time every second. RTC uses the external 32.768 kHz LSE oscillator when available and falls back to the internal LSI oscillator otherwise. Without VBAT backup power, RTC contents are lost after a full power removal and must be synchronized again.

### 6. Project Layout

| Path | Responsibility |
| --- | --- |
| `app/application/` | FreeRTOS tasks, timers, queues, and application scheduling |
| `app/ui/` | Page drawing and partial display refresh |
| `app/weather/` | Weather-response parsing |
| `app/wifi/` | ESP32 startup, Wi-Fi connection, and SNTP setup |
| `app/bsp_module/` | Unified board-support initialization |
| `bsp/` | UART, I2C, SPI, RTC, and display-port support |
| `drivers/` | AHT20, ESP32, and ST7789 device drivers |
| `third_lib/` | FreeRTOS, SPL, fonts, and image assets |
| `utilities/` | Shared utilities, including the ring buffer |

### 7. Build

Requirements:

- CMake 3.15 or newer
- Arm GNU Toolchain (`arm-none-eabi-gcc`)
- An STM32F4-compatible programmer/debugger, such as ST-LINK

```bash
cmake -S . -B build
cmake --build build
```

The build creates ELF, HEX, BIN, and S19 images under `build/`. Flash the image format supported by your programming tool.

### 8. Configuration and Security

Replace the following project-specific settings before use:

| Setting | Location |
| --- | --- |
| Wi-Fi SSID and password | `app/wifi/wifi.c` |
| Seniverse API key and location | `WEATHER_URL` in `app/application/application.c` |
| SNTP timezone | `drivers/esp32/esp32.c`; currently UTC+8 |

Do not commit real Wi-Fi credentials or API keys to a public repository. Move them into a local, ignored configuration header for production use.

### 9. Current Limitations

- SPI DMA is synchronous at the API level: DMA handles pixel movement, but the caller waits for completion.
- ESP-AT waiting can block `network_task`; it does not block the UI task.
- LSI timekeeping is less accurate than LSE. An LSE crystal and VBAT backup are recommended.
- Weather parsing is lightweight and tied to the expected response format rather than a full JSON parser.
