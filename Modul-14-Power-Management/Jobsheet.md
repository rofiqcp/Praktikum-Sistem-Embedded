# Jobsheet Modul 14: Power Management & Low-Power Design

## 🎯 Tujuan Praktikum
1. Mahasiswa mampu mengkonfigurasi berbagai mode low-power pada STM32 dan ESP32.
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

---

## 🛠️ Percobaan 1: Basic Sleep Mode
**Tujuan:** Memahami perbedaan antara mode active dan sleep pada kedua platform.

### A. ESP32 — Light Sleep & Deep Sleep
```cpp
#include <Arduino.h>

#define LED_PIN 2
#define SLEEP_DURATION_US  5000000  // 5 detik

RTC_DATA_ATTR int bootCount = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    bootCount++;
    Serial.printf("\n=== Boot #%d ===\n", bootCount);
    Serial.printf("Wake-up cause: %d\n", esp_sleep_get_wakeup_cause());
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    
    // Konfigurasi timer wake-up
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    
    Serial.println("Entering deep sleep for 5 seconds...");
    Serial.flush();
    
    esp_deep_sleep_start();
}

void loop() {
    // Tidak pernah sampai sini pada deep sleep
}
```

### B. STM32 — Sleep & Stop Mode
```c
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;

void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

// Konfigurasi RTC Alarm sebagai wake-up source
void Enter_Stop_Mode(void) {
    UART_SendString("[PWR] Entering STOP mode...\r\n");
    HAL_Delay(100);
    
    // Suspend SysTick
    HAL_SuspendTick();
    
    // Enter Stop mode, wake via EXTI
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    // Setelah bangun, konfigurasi ulang clock
    SystemClock_Config();
    HAL_ResumeTick();
    
    UART_SendString("[PWR] Woke up from STOP mode!\r\n");
}

void Enter_Sleep_Mode(void) {
    UART_SendString("[PWR] Entering SLEEP mode...\r\n");
    HAL_Delay(100);
    
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_ResumeTick();
    
    UART_SendString("[PWR] Woke up from SLEEP mode!\r\n");
}
```

---

## 🛠️ Percobaan 2: Wake-up Sources
**Tujuan:** Mengkonfigurasi berbagai sumber wake-up untuk membangunkan MCU dari mode sleep.

### A. ESP32 — Timer + External GPIO Wake-up
```cpp
#include <Arduino.h>

#define WAKEUP_PIN GPIO_NUM_33  // Touch atau button
#define LED_PIN 2

RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
    switch(reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wake-up: External signal (ext0)");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Wake-up: External signal (ext1)");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wake-up: Timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("Wake-up: Touchpad");
            break;
        default:
            Serial.printf("Wake-up: Other (%d)\n", reason);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    bootCount++;
    Serial.printf("\n=== Boot #%d ===\n", bootCount);
    print_wakeup_reason();
    
    // Konfigurasi multiple wake-up sources
    esp_sleep_enable_timer_wakeup(10 * 1000000);    // 10 detik timer
    esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, LOW);  // GPIO LOW trigger
    
    Serial.println("Going to deep sleep (wake: timer 10s OR GPIO33 LOW)...");
    Serial.flush();
    
    esp_deep_sleep_start();
}

void loop() {}
```

### B. ESP32 — Touch Pad Wake-up
```cpp
#include <Arduino.h>

#define TOUCH_PIN T0       // GPIO4
#define TOUCH_THRESHOLD 40
#define LED_PIN 2

RTC_DATA_ATTR int bootCount = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    bootCount++;
    Serial.printf("\n=== Touch Wake-up Boot #%d ===\n", bootCount);
    
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TOUCHPAD) {
        Serial.printf("Woke up by touch pad! Pin: %d\n", 
                      esp_sleep_get_touchpad_wakeup_status());
    }
    
    // Blink LED
    pinMode(LED_PIN, OUTPUT);
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(200);
    }
    
    // Setup touch wake-up
    touchAttachInterrupt(TOUCH_PIN, [](){}, TOUCH_THRESHOLD);
    esp_sleep_enable_touchpad_wakeup();
    
    Serial.println("Going to deep sleep... Touch GPIO4 to wake up!");
    Serial.flush();
    
    esp_deep_sleep_start();
}

void loop() {}
```

---

## 🛠️ Percobaan 3: Clock Gating & Frequency Scaling
**Tujuan:** Mengoptimalkan konsumsi daya dengan menonaktifkan peripheral clock dan menurunkan frekuensi CPU.

### ESP32 — Dynamic Frequency Scaling
```cpp
#include <Arduino.h>
#include "esp_pm.h"
#include "esp_wifi.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Dynamic Frequency Scaling Demo ===");
    Serial.printf("CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
    
    // Test dengan frekuensi berbeda
    Serial.println("\n--- Test CPU 240MHz ---");
    setCpuFrequencyMhz(240);
    unsigned long start = micros();
    volatile long sum = 0;
    for (long i = 0; i < 1000000; i++) sum += i;
    unsigned long elapsed = micros() - start;
    Serial.printf("1M iterations @ 240MHz: %lu us\n", elapsed);
    
    Serial.println("\n--- Test CPU 80MHz ---");
    setCpuFrequencyMhz(80);
    start = micros();
    sum = 0;
    for (long i = 0; i < 1000000; i++) sum += i;
    elapsed = micros() - start;
    Serial.printf("1M iterations @ 80MHz: %lu us\n", elapsed);
    
    Serial.println("\n--- Test CPU 10MHz ---");
    setCpuFrequencyMhz(10);
    start = micros();
    sum = 0;
    for (long i = 0; i < 1000000; i++) sum += i;
    elapsed = micros() - start;
    Serial.printf("1M iterations @ 10MHz: %lu us\n", elapsed);
    
    // Kembalikan ke 240MHz
    setCpuFrequencyMhz(240);
    Serial.println("\nCPU restored to 240MHz");
}

void loop() {
    delay(5000);
    Serial.printf("Running at %d MHz\n", getCpuFrequencyMhz());
}
```

---

## 🛠️ Percobaan 4: RTC Memory & Data Persistence
**Tujuan:** Menyimpan data yang bertahan melewati siklus deep sleep.

### ESP32 — RTC Memory Data Logger
```cpp
#include <Arduino.h>

#define MAX_READINGS 50
#define SLEEP_SECONDS 10

// Data di RTC memory - bertahan selama deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float readings[MAX_READINGS];
RTC_DATA_ATTR int readingIndex = 0;

float readBatteryVoltage() {
    // Simulasi pembacaan baterai (gunakan ADC untuk real hardware)
    return 3.3 + (random(-30, 30) / 100.0);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    bootCount++;
    Serial.printf("\n=== RTC Memory Logger - Boot #%d ===\n", bootCount);
    
    // Baca sensor
    float voltage = readBatteryVoltage();
    
    // Simpan ke RTC memory
    if (readingIndex < MAX_READINGS) {
        readings[readingIndex] = voltage;
        readingIndex++;
    }
    
    Serial.printf("Current reading: %.2f V (stored at index %d)\n", 
                  voltage, readingIndex - 1);
    
    // Tampilkan semua readings yang tersimpan
    Serial.println("\nStored readings:");
    for (int i = 0; i < readingIndex; i++) {
        Serial.printf("  [%02d] %.2f V\n", i, readings[i]);
    }
    
    // Jika buffer penuh, dump dan reset
    if (readingIndex >= MAX_READINGS) {
        Serial.println("\n*** Buffer full! Dumping data... ***");
        // Di sini bisa kirim data via WiFi atau simpan ke SD card
        readingIndex = 0;
    }
    
    // Konfigurasi wake-up timer
    esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
    
    Serial.printf("Sleeping for %d seconds...\n", SLEEP_SECONDS);
    Serial.flush();
    
    esp_deep_sleep_start();
}

void loop() {}
```

---

## 🛠️ Percobaan 5: Battery Monitoring
**Tujuan:** Membaca tegangan baterai dan mengestimasi sisa kapasitas.

### ESP32 — Battery Voltage Monitor
```cpp
#include <Arduino.h>

#define BATTERY_ADC_PIN  34   // GPIO34 (ADC1_CH6)
#define VOLTAGE_DIVIDER_RATIO 2.0  // R1=R2 voltage divider
#define ADC_RESOLUTION 4095
#define ADC_VREF 3.3

// Battery lookup table (Li-Ion typical)
struct BatteryLevel {
    float voltage;
    int percentage;
};

BatteryLevel batteryTable[] = {
    {4.20, 100}, {4.15, 95}, {4.11, 90}, {4.08, 85},
    {4.02, 80},  {3.98, 75}, {3.95, 70}, {3.91, 65},
    {3.87, 60},  {3.85, 55}, {3.84, 50}, {3.82, 45},
    {3.80, 40},  {3.79, 35}, {3.77, 30}, {3.75, 25},
    {3.73, 20},  {3.71, 15}, {3.69, 10}, {3.61, 5},
    {3.27, 0}
};

int getBatteryPercentage(float voltage) {
    if (voltage >= 4.20) return 100;
    if (voltage <= 3.27) return 0;
    
    for (int i = 0; i < 20; i++) {
        if (voltage >= batteryTable[i + 1].voltage) {
            float range = batteryTable[i].voltage - batteryTable[i + 1].voltage;
            float diff = voltage - batteryTable[i + 1].voltage;
            int pctRange = batteryTable[i].percentage - batteryTable[i + 1].percentage;
            return batteryTable[i + 1].percentage + (int)(diff / range * pctRange);
        }
    }
    return 0;
}

float readBatteryVoltage() {
    long sum = 0;
    for (int i = 0; i < 64; i++) {
        sum += analogRead(BATTERY_ADC_PIN);
    }
    float avgADC = sum / 64.0;
    float voltage = (avgADC / ADC_RESOLUTION) * ADC_VREF * VOLTAGE_DIVIDER_RATIO;
    return voltage;
}

void setup() {
    Serial.begin(115200);
    analogSetAttenuation(ADC_11db);
    
    Serial.println("=== Battery Monitor ===");
}

void loop() {
    float voltage = readBatteryVoltage();
    int percentage = getBatteryPercentage(voltage);
    
    Serial.printf("Battery: %.2fV (%d%%)", voltage, percentage);
    
    if (percentage > 75) Serial.println(" [FULL]");
    else if (percentage > 50) Serial.println(" [GOOD]");
    else if (percentage > 25) Serial.println(" [LOW]");
    else if (percentage > 10) Serial.println(" [CRITICAL]");
    else Serial.println(" [SHUTDOWN IMMINENT!]");
    
    // Visualisasi bar
    Serial.print("[");
    int bars = percentage / 5;
    for (int i = 0; i < 20; i++) {
        Serial.print(i < bars ? "█" : "░");
    }
    Serial.printf("] %d%%\n\n", percentage);
    
    delay(2000);
}
```

---

## 🛠️ Percobaan 6: STM32 Standby Mode dengan RTC Wake-up
**Tujuan:** Mengkonfigurasi mode daya terendah STM32 dengan RTC alarm sebagai wake-up source.

### STM32 — Standby Mode + RTC Alarm
```c
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;
static RTC_HandleTypeDef hrtc;

void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

void RTC_Config(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkpAccess();
    
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    HAL_RTC_Init(&hrtc);
}

void Enter_Standby_With_RTC_Alarm(uint32_t seconds) {
    char buf[64];
    snprintf(buf, sizeof(buf), "[PWR] Standby for %lu seconds...\r\n", seconds);
    UART_SendString(buf);
    HAL_Delay(100);
    
    // Set RTC Alarm
    RTC_AlarmTypeDef alarm = {0};
    RTC_TimeTypeDef time;
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    
    uint32_t totalSec = time.Hours * 3600 + time.Minutes * 60 + time.Seconds + seconds;
    alarm.AlarmTime.Hours = (totalSec / 3600) % 24;
    alarm.AlarmTime.Minutes = (totalSec % 3600) / 60;
    alarm.AlarmTime.Seconds = totalSec % 60;
    HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);
    
    // Clear wake-up flag
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    
    // Enter Standby
    HAL_PWR_EnterSTANDBYMode();
    // Tidak akan kembali ke sini - MCU reset setelah wake-up
}
```

---

## 📝 Tugas Percobaan
1. **Ukur** konsumsi daya pada setiap mode sleep (gunakan multimeter jika tersedia, atau amati perilaku LED/Serial).
2. **Bandingkan** waktu wake-up dari berbagai mode (catat timestamp Serial).
3. **Implementasikan** duty cycling: baca sensor setiap 30 detik, tidur di antaranya.
4. **Hitung** estimasi umur baterai untuk skenario: baterai 1000mAh, aktif 2 detik setiap 5 menit.
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
