# ⚡ QUICK REFERENCE CARD
## STM32 (HAL) vs ESP32 (ESP-IDF) - Side by Side Comparison

**Platform:** PlatformIO | **Framework:** STM32Cube HAL vs Espressif ESP-IDF

---

## 📌 BASIC SETUP

### PlatformIO platformio.ini

**STM32 (Blue Pill - STM32F103C8T6):**
```ini
[env:bluepill_f103c8]
platform = ststm32
board = bluepill_f103c8
framework = stm32cube
upload_protocol = stlink
debug_tool = stlink
```

**ESP32 (DevKit v1):**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = espidf
monitor_speed = 115200
```

---

## 💡 GPIO - BASIC OUTPUT

### LED Blink

**STM32 (HAL):**
```c
// Setup
GPIO_InitTypeDef GPIO_InitStruct = {0};
__HAL_RCC_GPIOC_CLK_ENABLE();
GPIO_InitStruct.Pin = GPIO_PIN_13;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

// Toggle
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
HAL_Delay(1000);
```

**ESP32 (ESP-IDF):**
```c
// Setup
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_2),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};
gpio_config(&io_conf);

// Toggle
gpio_set_level(GPIO_NUM_2, 1);
vTaskDelay(pdMS_TO_TICKS(1000));
gpio_set_level(GPIO_NUM_2, 0);
```

---

## 🔴 GPIO - BASIC INPUT

### Button Reading

**STM32 (HAL):**
```c
// Setup (with pull-up)
GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

// Read
if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
    // Button pressed (active LOW)
}
```

**ESP32 (ESP-IDF):**
```c
// Setup (with pull-up)
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};
gpio_config(&io_conf);

// Read
if (gpio_get_level(GPIO_NUM_0) == 0) {
    // Button pressed (active LOW)
}
```

---

## ⚡ EXTERNAL INTERRUPT

### Button Interrupt

**STM32 (HAL):**
```c
// Setup
GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);

// ISR Handler
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // Handle button press
    }
}
```

**ESP32 (ESP-IDF):**
```c
// Setup
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .intr_type = GPIO_INTR_NEGEDGE
};
gpio_config(&io_conf);

gpio_install_isr_service(0);
gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, NULL);

// ISR Handler
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    // Handle button press (keep short!)
    // Use task notification or queue
}
```

---

## 📡 UART/SERIAL

### Printf Redirect

**STM32 (HAL):**
```c
// In syscalls.c or main.c
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

// Usage
printf("Hello STM32! Value: %d\n", value);
```

**ESP32 (ESP-IDF):**
```c
// Already configured by default on UART0
// Just use:
printf("Hello ESP32! Value: %d\n", value);
ESP_LOGI("TAG", "Formatted: %d", value); // With logging level
```

### UART Transmit

**STM32 (HAL):**
```c
// Setup in CubeMX or manually:
huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;
HAL_UART_Init(&huart1);

// Transmit
uint8_t data[] = "Hello\n";
HAL_UART_Transmit(&huart1, data, strlen((char*)data), HAL_MAX_DELAY);
```

**ESP32 (ESP-IDF):**
```c
// Setup
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};
uart_param_config(UART_NUM_1, &uart_config);
uart_set_pin(UART_NUM_1, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);

// Transmit
const char* data = "Hello\n";
uart_write_bytes(UART_NUM_1, data, strlen(data));
```

---

## 🎚️ ADC - ANALOG INPUT

### Single Channel Read

**STM32 (HAL):**
```c
// Setup (in CubeMX)
hadc1.Instance = ADC1;
hadc1.Init.Resolution = ADC_RESOLUTION_12B;
hadc1.Init.ContinuousConvMode = DISABLE;
HAL_ADC_Init(&hadc1);

// Read
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
float voltage = (adc_value / 4096.0) * 3.3;
```

**ESP32 (ESP-IDF):**
```c
// Setup
adc1_config_width(ADC_WIDTH_BIT_12);
adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // 0-3.3V

// Read
int raw = adc1_get_raw(ADC1_CHANNEL_0); // 0-4095
float voltage = (raw / 4095.0) * 3.3;
```

---

## ⏱️ TIMER & PWM

### PWM Output

**STM32 (HAL):**
```c
// Setup (in CubeMX: TIM2, CH1, 1kHz, 50% duty)
htim2.Instance = TIM2;
htim2.Init.Prescaler = 71; // 72MHz / 72 = 1MHz
htim2.Init.Period = 999;   // 1MHz / 1000 = 1kHz
HAL_TIM_PWM_Init(&htim2);

TIM_OC_InitTypeDef sConfigOC = {0};
sConfigOC.OCMode = TIM_OCMODE_PWM1;
sConfigOC.Pulse = 500; // 50% duty cycle
HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

// Start
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

// Change duty cycle
__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 750); // 75%
```

**ESP32 (ESP-IDF):**
```c
// Setup (LEDC - LED Controller)
ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_13_BIT, // 8192 steps
    .freq_hz = 1000,
    .clk_cfg = LEDC_AUTO_CLK
};
ledc_timer_config(&ledc_timer);

ledc_channel_config_t ledc_channel = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .intr_type = LEDC_INTR_DISABLE,
    .gpio_num = GPIO_NUM_5,
    .duty = 4096, // 50% of 8192
    .hpoint = 0
};
ledc_channel_config(&ledc_channel);

// Change duty cycle
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 6144); // 75%
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
```

---

## 🔄 I²C COMMUNICATION

### I²C Device Scan

**STM32 (HAL):**
```c
// Setup (in CubeMX: I2C1, 100kHz)
hi2c1.Instance = I2C1;
hi2c1.Init.ClockSpeed = 100000;
hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
HAL_I2C_Init(&hi2c1);

// Scan
printf("I2C Scanner:\n");
for (uint8_t addr = 1; addr < 128; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
        printf("Device found at 0x%02X\n", addr);
    }
}
```

**ESP32 (ESP-IDF):**
```c
// Setup
i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000
};
i2c_param_config(I2C_NUM_0, &conf);
i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

// Scan
printf("I2C Scanner:\n");
for (uint8_t addr = 1; addr < 128; addr++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS) == ESP_OK) {
        printf("Device found at 0x%02X\n", addr);
    }
    i2c_cmd_link_delete(cmd);
}
```

---

## 🚀 SPI COMMUNICATION

### SPI Basic Transfer

**STM32 (HAL):**
```c
// Setup (in CubeMX: SPI1, 1Mbps, CPOL=0, CPHA=0)
hspi1.Instance = SPI1;
hspi1.Init.Mode = SPI_MODE_MASTER;
hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
HAL_SPI_Init(&hspi1);

// CS pin (manual control)
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // CS LOW
uint8_t tx_data = 0xAA;
uint8_t rx_data;
HAL_SPI_TransmitReceive(&hspi1, &tx_data, &rx_data, 1, HAL_MAX_DELAY);
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // CS HIGH
```

**ESP32 (ESP-IDF):**
```c
// Setup
spi_bus_config_t buscfg = {
    .miso_io_num = GPIO_NUM_19,
    .mosi_io_num = GPIO_NUM_23,
    .sclk_io_num = GPIO_NUM_18,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
};
spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 1000000,
    .mode = 0, // CPOL=0, CPHA=0
    .spics_io_num = GPIO_NUM_5,
    .queue_size = 7
};
spi_device_handle_t spi;
spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

// Transfer
uint8_t tx_data = 0xAA;
uint8_t rx_data;
spi_transaction_t t = {
    .length = 8,
    .tx_buffer = &tx_data,
    .rx_buffer = &rx_data
};
spi_device_transmit(spi, &t);
```

---

## 🧠 FREERTOS BASICS

### Task Creation

**STM32 (HAL + CMSIS-RTOS):**
```c
// Task function
void vTask1(void *pvParameters) {
    while (1) {
        printf("Task 1 running\n");
        osDelay(1000);
    }
}

// Create task
osThreadId_t task1Handle;
const osThreadAttr_t task1_attributes = {
    .name = "Task1",
    .priority = osPriorityNormal,
    .stack_size = 128 * 4
};
task1Handle = osThreadNew(vTask1, NULL, &task1_attributes);
```

**ESP32 (ESP-IDF):**
```c
// Task function
void vTask1(void *pvParameters) {
    while (1) {
        printf("Task 1 running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Create task
TaskHandle_t task1Handle = NULL;
xTaskCreate(
    vTask1,           // Task function
    "Task1",          // Name
    2048,             // Stack size (bytes)
    NULL,             // Parameters
    5,                // Priority
    &task1Handle      // Handle
);

// Pin to core (ESP32 specific)
xTaskCreatePinnedToCore(vTask1, "Task1", 2048, NULL, 5, &task1Handle, 0); // Core 0
```

### Queue (Message Passing)

**STM32 (HAL + CMSIS-RTOS):**
```c
osMessageQueueId_t queueHandle;

// Create
const osMessageQueueAttr_t queue_attributes = {
    .name = "Queue1"
};
queueHandle = osMessageQueueNew(10, sizeof(uint32_t), &queue_attributes);

// Send
uint32_t data = 123;
osMessageQueuePut(queueHandle, &data, 0, 0);

// Receive
uint32_t received;
osMessageQueueGet(queueHandle, &received, NULL, osWaitForever);
```

**ESP32 (ESP-IDF):**
```c
QueueHandle_t queueHandle;

// Create
queueHandle = xQueueCreate(10, sizeof(uint32_t));

// Send
uint32_t data = 123;
xQueueSend(queueHandle, &data, 0);

// Receive
uint32_t received;
xQueueReceive(queueHandle, &received, portMAX_DELAY);
```

---

## 💤 POWER MANAGEMENT

### Sleep Mode

**STM32 (HAL):**
```c
// Enter Sleep mode (wake by any interrupt)
HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

// Enter Stop mode (wake by EXTI/RTC)
HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

// Enter Standby mode (wake by WKUP pin/RTC)
HAL_PWR_EnterSTANDBYMode();
```

**ESP32 (ESP-IDF):**
```c
// Light sleep (wake by timer, GPIO, UART, etc.)
esp_sleep_enable_timer_wakeup(5 * 1000000); // 5 seconds
esp_light_sleep_start();

// Deep sleep (wake by timer, EXT0/EXT1, touch, ULP)
esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 seconds
esp_deep_sleep_start();

// GPIO wakeup (EXT0 - single pin)
esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Wake on LOW
```

---

## 📶 WIFI (ESP32 Only)

### WiFi Station Mode

**ESP32 (ESP-IDF):**
```c
#include "esp_wifi.h"
#include "esp_event.h"

// Initialize
esp_netif_init();
esp_event_loop_create_default();
esp_netif_create_default_wifi_sta();

wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);

// Configure
wifi_config_t wifi_config = {
    .sta = {
        .ssid = "YourSSID",
        .password = "YourPassword"
    },
};
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

// Start
esp_wifi_start();
esp_wifi_connect();
```

---

## 📊 COMPARISON SUMMARY

| Feature | STM32 (HAL) | ESP32 (ESP-IDF) |
|---------|-------------|-----------------|
| **GPIO Speed** | Up to 50MHz | Up to 40MHz |
| **ADC Resolution** | 12-bit (16-bit on H7) | 12-bit |
| **ADC Channels** | 16-24 channels | 18 channels (+ 2 ADC) |
| **Timers** | 16 advanced timers | 4 hardware timers |
| **PWM Channels** | 16+ channels | 16 LEDC channels |
| **UART** | Up to 8 UART | 3 UART |
| **I²C** | Up to 4 I²C | 2 I²C |
| **SPI** | Up to 6 SPI | 4 SPI (VSPI, HSPI) |
| **DMA** | Full DMA support | Limited (I2S, SPI) |
| **Real-Time** | ⭐⭐⭐⭐⭐ Excellent | ⭐⭐⭐ Good (with tuning) |
| **WiFi/BLE** | ❌ External module | ✅ Built-in |
| **Dual Core** | ❌ Single core | ✅ Dual core |
| **Power (Sleep)** | <1 µA (Standby) | 5 µA (Deep sleep) |
| **Cost** | $2-10 | $2-5 |
| **Ecosystem** | Professional tools | Maker-friendly |

---

## 🎯 WHEN TO USE?

### Use STM32 When:
```
✅ Hard real-time requirements (<1µs jitter)
✅ Safety-critical applications (ISO 26262, IEC 61508)
✅ 5V sensor compatibility needed
✅ Ultra-low power (<1 µA standby)
✅ Complex timer requirements (encoder, motor control)
✅ Industrial protocols (CAN, Modbus, EtherCAT)
✅ Automotive/medical/aerospace projects
```

### Use ESP32 When:
```
✅ WiFi/Bluetooth connectivity required
✅ Rapid prototyping & development
✅ IoT applications
✅ Web server or REST API needed
✅ Budget constraints (<$3 per unit)
✅ Dual-core parallel processing
✅ Large community support needed
✅ OTA (Over-The-Air) updates
```

---

## 🔗 USEFUL LINKS

### STM32:
- Official: https://www.st.com/stm32
- HAL Documentation: Search "STM32F1 HAL User Manual"
- Forum: https://community.st.com
- PlatformIO: https://docs.platformio.org/en/latest/platforms/ststm32.html

### ESP32:
- Official: https://www.espressif.com
- ESP-IDF Programming Guide: https://docs.espressif.com/projects/esp-idf/en/latest/
- Forum: https://www.esp32.com
- PlatformIO: https://docs.platformio.org/en/latest/platforms/espressif32.html

---

**Quick Reference v1.0** | Last Updated: 2025
