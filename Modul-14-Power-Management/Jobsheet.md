# Jobsheet Modul 14: Power Management & Low-Power Design

## 🎯 Tujuan Praktikum
1. Mahasiswa mampu mengkonfigurasi berbagai mode low-power pada STM32 (HAL) dan ESP32 (ESP-IDF).
2. Mahasiswa memahami perbedaan konsumsi daya pada setiap sleep mode.
3. Mahasiswa mampu mengimplementasikan wake-up sources (RTC, EXTI, Timer, Touch).
4. Mahasiswa dapat mendesain sistem embedded hemat daya dengan teknik duty cycling.
5. Mahasiswa memahami penggunaan ULP co-processor pada ESP32.
6. Mahasiswa mampu melakukan monitoring dan optimasi konsumsi baterai.

## ⚠️ Peringatan
- Gunakan **multimeter** untuk mengukur arus aktual jika tersedia.
- Pastikan Serial Monitor dengan baudrate **115200**.
- Beberapa mode sleep akan memutus koneksi Serial — amati output sebelum tidur.
- Pada STM32 Standby mode, SRAM hilang — seperti reset. Jangan simpan data penting di RAM.
- Pada ESP32 Deep Sleep, WiFi dan Bluetooth harus di-inisialisasi ulang setelah bangun.
- **ESP32**: Menggunakan framework **ESP-IDF** (bukan Arduino). Entry point adalah `app_main()`.
- **STM32**: Menggunakan framework **STM32Cube HAL** dengan FreeRTOS. Entry point adalah `main()`.

---

## 🛠️ Percobaan 1: ESP32 Deep Sleep dengan Timer Wake-up (ESP-IDF)
**Tujuan:** Memahami deep sleep dan timer wake-up pada ESP32 menggunakan ESP-IDF API.

### Kode Program
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define LED_GPIO    GPIO_NUM_2
#define SLEEP_US    5000000  // 5 detik

static const char *TAG = "DEEP_SLEEP";

RTC_DATA_ATTR int boot_count = 0;

void app_main(void)
{
    boot_count++;
    ESP_LOGI(TAG, "=== Boot #%d ===", boot_count);

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wake-up cause: TIMER");
            break;
        default:
            ESP_LOGI(TAG, "Wake-up cause: POWER ON / RESET (%d)", cause);
            break;
    }

    // Blink LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(LED_GPIO, 0);

    // Konfigurasi timer wake-up
    esp_sleep_enable_timer_wakeup(SLEEP_US);
    ESP_LOGI(TAG, "Entering deep sleep for %d seconds...", SLEEP_US / 1000000);

    esp_deep_sleep_start();
    // Tidak pernah sampai sini
}
```

---

## 🛠️ Percobaan 2: STM32 Sleep & Stop Mode (STM32Cube HAL)
**Tujuan:** Mengkonfigurasi Sleep dan Stop mode pada STM32 menggunakan HAL Power API.

### Kode Program
```c
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef huart1;

void SystemClock_Config(void);
void UART_SendString(const char *str);

void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

void Enter_Sleep_Mode(void) {
    UART_SendString("[PWR] Entering SLEEP mode...\r\n");
    HAL_Delay(100);
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_ResumeTick();
    UART_SendString("[PWR] Woke up from SLEEP mode!\r\n");
}

void Enter_Stop_Mode(void) {
    UART_SendString("[PWR] Entering STOP mode...\r\n");
    HAL_Delay(100);
    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    // Setelah wake-up dari STOP, reconfigure system clock
    SystemClock_Config();
    HAL_ResumeTick();
    UART_SendString("[PWR] Woke up from STOP mode!\r\n");
}
```
*Lihat program lengkap di folder praktikum STM32_01 dan STM32_02.*

---

## 🛠️ Percobaan 3: ESP32 Multiple Wake-up Sources (ESP-IDF)
**Tujuan:** Mengkonfigurasi timer + GPIO external wake-up pada ESP32.

### Kode Program
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"

#define WAKEUP_GPIO  GPIO_NUM_33
#define SLEEP_US     10000000  // 10 detik

static const char *TAG = "MULTI_WAKE";
RTC_DATA_ATTR int boot_count = 0;

static void print_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wake-up: TIMER"); break;
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Wake-up: EXT0 (GPIO%d)", WAKEUP_GPIO); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            ESP_LOGI(TAG, "Wake-up: TOUCHPAD"); break;
        default:
            ESP_LOGI(TAG, "Wake-up: OTHER (%d)", cause); break;
    }
}

void app_main(void)
{
    boot_count++;
    ESP_LOGI(TAG, "=== Boot #%d ===", boot_count);
    print_wakeup_reason();

    // Konfigurasi multiple wake-up sources
    esp_sleep_enable_timer_wakeup(SLEEP_US);
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);  // LOW trigger

    ESP_LOGI(TAG, "Going to deep sleep (timer 10s OR GPIO33 LOW)...");
    esp_deep_sleep_start();
}
```

---

## 🛠️ Percobaan 4: ESP32 Touch Pad Wake-up (ESP-IDF)
**Tujuan:** Membangunkan ESP32 dari deep sleep menggunakan sensor kapasitif (touch pad).

### Kode Program
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/touch_pad.h"

#define TOUCH_PAD_NO    TOUCH_PAD_NUM0  // GPIO4
#define TOUCH_THRESHOLD 400

static const char *TAG = "TOUCH_WAKE";
RTC_DATA_ATTR int boot_count = 0;

void app_main(void)
{
    boot_count++;
    ESP_LOGI(TAG, "=== Touch Wake-up Boot #%d ===", boot_count);

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TOUCHPAD) {
        ESP_LOGI(TAG, "Woke up by TOUCH PAD!");
    }

    // Init touch pad
    touch_pad_init();
    touch_pad_config(TOUCH_PAD_NO, TOUCH_THRESHOLD);
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_filter_start(10);

    // Enable touch wake-up
    esp_sleep_enable_touchpad_wakeup();

    ESP_LOGI(TAG, "Touch GPIO4 to wake up! Sleeping...");
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}
```

---

## 🛠️ Percobaan 5: ESP32 Dynamic Frequency Scaling (ESP-IDF)
**Tujuan:** Mengoptimalkan daya dengan menurunkan frekuensi CPU saat idle.

### Kode Program
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pm.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

static const char *TAG = "DFS";

static void benchmark(const char *label)
{
    int64_t start = esp_timer_get_time();
    volatile long sum = 0;
    for (long i = 0; i < 1000000; i++) sum += i;
    int64_t elapsed = esp_timer_get_time() - start;
    ESP_LOGI(TAG, "%s: 1M iterations in %lld us", label, elapsed);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Dynamic Frequency Scaling Demo ===");

    // Benchmark at different frequencies
    esp_pm_config_esp32_t pm_240 = { .max_freq_mhz = 240, .min_freq_mhz = 240 };
    esp_pm_configure(&pm_240);
    vTaskDelay(pdMS_TO_TICKS(100));
    benchmark("240 MHz");

    esp_pm_config_esp32_t pm_80 = { .max_freq_mhz = 80, .min_freq_mhz = 80 };
    esp_pm_configure(&pm_80);
    vTaskDelay(pdMS_TO_TICKS(100));
    benchmark("80 MHz");

    ESP_LOGI(TAG, "Done. Observe the speed difference!");
}
```

---

## 🛠️ Percobaan 6: ESP32 RTC Memory Data Logger (ESP-IDF)
**Tujuan:** Menyimpan data yang bertahan melewati siklus deep sleep menggunakan RTC memory.

### Kode Program
```c
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"

#define MAX_READINGS 20
#define SLEEP_SEC    10

static const char *TAG = "RTC_LOG";

RTC_DATA_ATTR int boot_count = 0;
RTC_DATA_ATTR float readings[MAX_READINGS];
RTC_DATA_ATTR int reading_idx = 0;

void app_main(void)
{
    boot_count++;
    ESP_LOGI(TAG, "=== RTC Logger Boot #%d ===", boot_count);

    // Simulasi pembacaan sensor
    float value = 20.0f + (esp_random() % 200) / 10.0f;

    if (reading_idx < MAX_READINGS) {
        readings[reading_idx] = value;
        reading_idx++;
    }
    ESP_LOGI(TAG, "Reading: %.1f (index %d)", value, reading_idx - 1);

    // Tampilkan semua data
    ESP_LOGI(TAG, "--- Stored Data ---");
    for (int i = 0; i < reading_idx; i++) {
        printf("  [%02d] %.1f\n", i, readings[i]);
    }

    if (reading_idx >= MAX_READINGS) {
        ESP_LOGW(TAG, "Buffer full! Resetting...");
        reading_idx = 0;
    }

    esp_sleep_enable_timer_wakeup(SLEEP_SEC * 1000000ULL);
    ESP_LOGI(TAG, "Sleeping for %d seconds...", SLEEP_SEC);
    esp_deep_sleep_start();
}
```

---

## 🛠️ Percobaan 7: STM32 Standby Mode + RTC Alarm Wake-up (HAL)
**Tujuan:** Mode daya terendah STM32 dengan RTC alarm sebagai sumber wake-up. Setelah Standby, MCU melakukan full reset.

*Lihat program lengkap di folder praktikum STM32_03_Standby_RTC_Wakeup.*

---

## 🛠️ Percobaan 8: ESP32 Battery Monitoring (ESP-IDF)
**Tujuan:** Membaca tegangan baterai via ADC dan mengestimasi sisa kapasitas.

### Kode Program (Ringkasan)
```c
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define BATT_ADC_CH    ADC1_CHANNEL_6   // GPIO34
#define V_DIV_RATIO    2.0f

static esp_adc_cal_characteristics_t adc_chars;

float read_battery_voltage(void)
{
    uint32_t raw = adc1_get_raw(BATT_ADC_CH);
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
    return (mv / 1000.0f) * V_DIV_RATIO;
}
```
*Lihat program lengkap di folder praktikum ESP32_07_Battery_Monitor.*

---

## 📝 Tugas Percobaan
1. **Ukur** konsumsi daya pada setiap mode sleep (gunakan multimeter jika tersedia).
2. **Bandingkan** waktu wake-up dari berbagai mode (catat timestamp via `esp_timer_get_time()` atau `HAL_GetTick()`).
3. **Implementasikan** duty cycling: baca sensor setiap 30 detik, deep sleep di antaranya.
4. **Hitung** estimasi umur baterai: baterai 1000mAh, aktif 2 detik setiap 5 menit.
5. **Kombinasikan** multiple wake-up sources: timer + external button pada ESP32.

---

## 📊 Tabel Pengamatan

| Mode | Platform | Konsumsi (mA) | Wake-up Time | Data Retained? |
|------|----------|---------------|--------------|----------------|
| Active | ESP32 | | | Ya |
| Modem Sleep | ESP32 | | | Ya |
| Light Sleep | ESP32 | | | Ya |
| Deep Sleep | ESP32 | | | RTC memory saja |
| Sleep | STM32 | | | Ya |
| Stop | STM32 | | | Ya |
| Standby | STM32 | | | Backup register saja |

**Catatan:** Isi tabel berdasarkan pengukuran atau observasi Anda.
