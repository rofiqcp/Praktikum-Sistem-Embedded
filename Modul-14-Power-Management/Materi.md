# Modul 14: Power Management

## Daftar Isi
1. [Pendahuluan](#1-pendahuluan)
2. [Konsep Dasar Power Management](#2-konsep-dasar-power-management)
3. [Mode Daya pada Mikrokontroler](#3-mode-daya-pada-mikrokontroler)
4. [ESP32 — Power Management](#4-esp32--power-management)
5. [STM32F103 — Power Management](#5-stm32f103--power-management)
6. [Sumber Wakeup (Wakeup Sources)](#6-sumber-wakeup-wakeup-sources)
7. [RTC Memory & Data Retention](#7-rtc-memory--data-retention)
8. [Dynamic Frequency Scaling (DFS)](#8-dynamic-frequency-scaling-dfs)
9. [Peripheral Clock Gating & Power Gating](#9-peripheral-clock-gating--power-gating)
10. [ULP Coprocessor (ESP32)](#10-ulp-coprocessor-esp32)
11. [Desain Sistem Battery-Powered](#11-desain-sistem-battery-powered)
12. [Power Budget Analysis](#12-power-budget-analysis)
13. [Pengukuran Arus (Current Measurement)](#13-pengukuran-arus-current-measurement)
14. [Perbandingan ESP32 vs STM32](#14-perbandingan-esp32-vs-stm32)
15. [Best Practices](#15-best-practices)

---

## 1. Pendahuluan

**Power Management** adalah aspek krusial dalam desain sistem embedded, terutama untuk perangkat yang beroperasi dengan baterai atau energy harvesting. Manajemen daya yang efektif menentukan masa pakai baterai, keandalan sistem, dan biaya operasional.

### Mengapa Power Management Penting?
- **Battery Life**: Perangkat IoT harus bertahan berbulan-bulan atau bertahun-tahun dengan satu baterai
- **Thermal Management**: Konsumsi daya berlebih menghasilkan panas yang merusak komponen
- **Regulatory Compliance**: Standar seperti Energy Star membatasi konsumsi daya
- **Cost Reduction**: Daya lebih rendah = baterai lebih kecil = biaya lebih rendah
- **Environmental**: Efisiensi energi mengurangi dampak lingkungan

### Komponen Konsumsi Daya
Konsumsi daya total pada mikrokontroler:

$$P_{total} = P_{CPU} + P_{peripheral} + P_{memory} + P_{IO} + P_{leakage}$$

Di mana:
- $P_{CPU} = C \cdot V^2 \cdot f$ (dynamic power)
- $P_{leakage}$ = arus bocor (static power, selalu ada)

---

## 2. Konsep Dasar Power Management

### 2.1 Daya Dinamis vs Daya Statis

| Parameter | Daya Dinamis | Daya Statis |
|-----------|-------------|------------|
| Penyebab | Switching transistor | Arus bocor (leakage) |
| Rumus | $P = C \cdot V_{DD}^2 \cdot f$ | $P = V_{DD} \cdot I_{leak}$ |
| Pengaruh | Frekuensi, tegangan | Temperatur, teknologi |
| Pengurangan | Clock gating, DFS, sleep | Power gating, body biasing |

### 2.2 Strategi Pengurangan Daya

1. **Clock Gating**: Mematikan clock ke peripheral yang tidak digunakan
2. **Power Gating**: Mematikan tegangan ke blok yang tidak aktif
3. **Voltage Scaling**: Menurunkan tegangan operasi (mengurangi daya kuadratik)
4. **Frequency Scaling**: Menurunkan frekuensi clock (mengurangi daya linear)
5. **Sleep Modes**: Mematikan sebagian atau seluruh sistem saat idle
6. **Duty Cycling**: Aktif sesaat, tidur sebagian besar waktu

### 2.3 Duty Cycle dan Battery Life

$$T_{battery} = \frac{C_{battery}}{I_{avg}}$$

$$I_{avg} = I_{active} \cdot D + I_{sleep} \cdot (1-D)$$

Di mana $D$ = duty cycle (rasio waktu aktif terhadap total waktu).

**Contoh**: Baterai 2000mAh, $I_{active}$ = 80mA, $I_{sleep}$ = 10μA, duty cycle 1%:

$$I_{avg} = 80 \times 0.01 + 0.01 \times 0.99 = 0.8 + 0.0099 ≈ 0.81\text{mA}$$

$$T_{battery} = \frac{2000}{0.81} ≈ 2469 \text{ jam} ≈ 103 \text{ hari}$$

---

## 3. Mode Daya pada Mikrokontroler

### 3.1 Hirarki Mode Daya

Secara umum, mikrokontroler memiliki mode daya bertingkat:

```
Active Mode (Normal)
  ↓  Hemat daya rendah, bangun cepat
Light Sleep / Sleep Mode
  ↓  Hemat daya sedang, bangun sedang
Deep Sleep / Stop Mode
  ↓  Hemat daya tinggi, bangun lambat
Hibernate / Standby Mode
  ↓  Hemat daya tertinggi, data hilang
Power Off
```

### 3.2 Trade-off

| Mode | Konsumsi | Wakeup Time | Data Retention | Peripheral |
|------|----------|-------------|----------------|------------|
| Active | Tertinggi | — | Penuh | Semua aktif |
| Light Sleep | Sedang | μs | Penuh | Sebagian |
| Deep Sleep | Rendah | ms | Terbatas | Minimal |
| Hibernate | Sangat rendah | Detik | Tidak ada | Off |

---

## 4. ESP32 — Power Management

### 4.1 Mode Daya ESP32

ESP32 memiliki 5 mode daya utama:

| Mode | Konsumsi | Deskripsi |
|------|----------|-----------|
| **Active** | 80-260mA | CPU aktif, WiFi/BT bisa aktif |
| **Modem Sleep** | 20-30mA | WiFi/BT off, CPU aktif |
| **Light Sleep** | 0.8mA | CPU paused, RAM retained, wakeup cepat |
| **Deep Sleep** | 10-150μA | CPU off, RTC aktif, RTC memory retained |
| **Hibernation** | 2.5-5μA | RTC off, hanya RTC timer & EXT wakeup |

### 4.2 Light Sleep (ESP-IDF)

Light sleep mempertahankan semua state CPU dan RAM. CPU berhenti tapi tidak reset.

```c
#include "esp_sleep.h"

// Konfigurasi wakeup dari timer
esp_sleep_enable_timer_wakeup(5000000);  // 5 detik

// Konfigurasi wakeup dari GPIO
esp_sleep_enable_gpio_wakeup();
gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL);

// Masuk light sleep
esp_light_sleep_start();

// Setelah wakeup, eksekusi dilanjutkan dari sini
esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
```

### 4.3 Deep Sleep (ESP-IDF)

Deep sleep mematikan CPU, sebagian besar RAM, dan peripheral. Hanya RTC controller dan RTC memory yang tetap aktif.

```c
#include "esp_sleep.h"

// Simpan data ke RTC memory (tetap bertahan saat deep sleep)
RTC_DATA_ATTR int boot_count = 0;

void app_main(void) {
    boot_count++;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);
    
    // Konfigurasi wakeup sources
    esp_sleep_enable_timer_wakeup(10000000);  // 10 detik
    
    // Konfigurasi ext0 wakeup (1 GPIO)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);  // HIGH level
    
    // Konfigurasi ext1 wakeup (multiple GPIO)
    esp_sleep_enable_ext1_wakeup(BIT(GPIO_NUM_25) | BIT(GPIO_NUM_26), 
                                  ESP_EXT1_WAKEUP_ANY_HIGH);
    
    // Masuk deep sleep (fungsi tidak return!)
    esp_deep_sleep_start();
}
```

**Penting**: Setelah deep sleep, ESP32 melakukan full reset — eksekusi dimulai dari `app_main()`.

### 4.4 Touch Pad Wakeup

ESP32 mendukung wakeup dari kapasitif touch pad:

```c
#include "driver/touch_pad.h"

touch_pad_init();
touch_pad_config(TOUCH_PAD_NUM0, 40);  // GPIO4, threshold 40
touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);

esp_sleep_enable_touchpad_wakeup();
esp_deep_sleep_start();
```

### 4.5 Hibernation Mode

Mode paling hemat daya, hanya RTC timer dan ext0/ext1 yang bisa membangunkan:

```c
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

esp_sleep_enable_timer_wakeup(60000000);  // 60 detik
esp_deep_sleep_start();
```

### 4.6 ESP32 Power Management Framework

ESP-IDF menyediakan framework PM untuk automatic frequency scaling:

```c
#include "esp_pm.h"

esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = 240,
    .min_freq_mhz = 80,
    .light_sleep_enable = true
};
esp_pm_configure(&pm_config);
```

---

## 5. STM32F103 — Power Management

### 5.1 Mode Daya STM32F103

STM32F103 memiliki 3 mode low-power:

| Mode | Konsumsi | Regulator | Clock | RAM | Wakeup |
|------|----------|-----------|-------|-----|--------|
| **Run** | ~30mA @72MHz | ON | Semua | OK | — |
| **Sleep** | ~10-15mA | ON | CPU off | OK | Interrupt/Event |
| **Stop** | ~20μA | Low-power | HSI/HSE off | OK | EXTI |
| **Standby** | ~2μA | OFF | Semua off | Hilang | WKUP pin, RTC |

### 5.2 Sleep Mode (WFI / WFE)

Sleep mode paling ringan — hanya menghentikan CPU clock:

```c
// Wait For Interrupt — bangun dari interrupt apapun
HAL_SuspendTick();
HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
HAL_ResumeTick();

// Wait For Event — bangun dari event
HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFE);
```

### 5.3 Stop Mode

Stop mode mematikan semua clock kecuali LSI/LSE. RAM dan register dipertahankan:

```c
// Konfigurasi EXTI untuk wakeup
HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);

// Masuk Stop mode
HAL_SuspendTick();
HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

// Setelah wakeup, konfigurasi ulang system clock!
SystemClock_Config();
HAL_ResumeTick();
```

**Penting**: Setelah keluar dari Stop mode, system clock kembali ke HSI (8MHz). Harus rekonfigurasi ke HSE/PLL.

### 5.4 Standby Mode

Mode paling hemat daya. Semua RAM hilang, hanya backup register yang tersimpan:

```c
// Enable PWR clock
__HAL_RCC_PWR_CLK_ENABLE();

// Enable Wakeup Pin (PA0)
HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

// Clear wakeup flag
__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

// Masuk Standby (tidak return, reset setelah wakeup)
HAL_PWR_EnterSTANDBYMode();
```

### 5.5 Backup Register

STM32F103 memiliki backup register yang bertahan di Standby mode (jika VBAT ada):

```c
// Enable akses ke Backup domain
__HAL_RCC_PWR_CLK_ENABLE();
__HAL_RCC_BKP_CLK_ENABLE();
HAL_PWR_EnableBkUpAccess();

// Tulis ke backup register
HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x1234);

// Baca dari backup register
uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
```

### 5.6 Peripheral Clock Control

Mematikan clock ke peripheral yang tidak digunakan:

```c
// Matikan clock GPIOB jika tidak dipakai
__HAL_RCC_GPIOB_CLK_DISABLE();

// Matikan clock TIM3 jika tidak dipakai
__HAL_RCC_TIM3_CLK_DISABLE();

// Matikan clock USART2
__HAL_RCC_USART2_CLK_DISABLE();

// Matikan clock SPI1
__HAL_RCC_SPI1_CLK_DISABLE();
```

---

## 6. Sumber Wakeup (Wakeup Sources)

### 6.1 ESP32 Wakeup Sources

| Sumber | Light Sleep | Deep Sleep | Hibernation |
|--------|------------|------------|-------------|
| Timer | ✅ | ✅ | ✅ |
| GPIO (ext0) | — | ✅ (1 pin, RTC) | ✅ |
| GPIO (ext1) | — | ✅ (multi pin, RTC) | ✅ |
| GPIO wakeup | ✅ (any GPIO) | — | — |
| Touch Pad | ✅ | ✅ | — |
| ULP Coprocessor | ✅ | ✅ | — |
| UART | ✅ | — | — |

### 6.2 STM32F103 Wakeup Sources

| Sumber | Sleep | Stop | Standby |
|--------|-------|------|---------|
| Any NVIC Interrupt | ✅ | — | — |
| EXTI Line (GPIO) | ✅ | ✅ | — |
| WKUP Pin (PA0) | — | — | ✅ |
| RTC Alarm | ✅ | ✅ | ✅ |
| NRST Pin | ✅ | ✅ | ✅ |
| IWDG Reset | ✅ | ✅ | ✅ |

### 6.3 Mengidentifikasi Wakeup Cause

**ESP32:**
```c
esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:    // Timer
    case ESP_SLEEP_WAKEUP_EXT0:     // ext0
    case ESP_SLEEP_WAKEUP_EXT1:     // ext1
    case ESP_SLEEP_WAKEUP_TOUCHPAD: // Touch
    case ESP_SLEEP_WAKEUP_ULP:      // ULP
    default:                        // Power-on / reset
}
```

**STM32:**
```c
if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB)) {
    // Wakeup dari Standby mode
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
}
if (__HAL_PWR_GET_FLAG(PWR_FLAG_WU)) {
    // Wakeup dari WKUP pin
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}
```

---

## 7. RTC Memory & Data Retention

### 7.1 ESP32 RTC Memory

ESP32 memiliki 8KB RTC SLOW memory dan 8KB RTC FAST memory yang bertahan saat deep sleep:

```c
// RTC_DATA_ATTR — disimpan di RTC SLOW memory
RTC_DATA_ATTR int counter = 0;
RTC_DATA_ATTR float last_reading = 0.0;
RTC_DATA_ATTR char message[64] = {0};

// RTC_NOINIT_ATTR — tidak di-initialize saat deep sleep reset
RTC_NOINIT_ATTR int persistent_data;
```

### 7.2 STM32 Backup Domain

STM32F103 memiliki Backup Register (10 x 16-bit = 20 bytes) dan RTC:

```c
// Backup Register Data Map
// BKP_DR1: Boot counter (16-bit)
// BKP_DR2-DR3: Last sensor value (32-bit, split)
// BKP_DR4: Status flags

#define BOOT_COUNT_REG  RTC_BKP_DR1
#define DATA_HIGH_REG   RTC_BKP_DR2
#define DATA_LOW_REG    RTC_BKP_DR3
```

### 7.3 Perbandingan

| Fitur | ESP32 RTC Memory | STM32 Backup Register |
|-------|-----------------|----------------------|
| Ukuran | 8KB + 8KB | 20 bytes (10 reg × 16-bit) |
| Tipe Data | Apapun (array, struct) | Integer 16-bit per register |
| Bertahan | Deep Sleep | Standby (jika VBAT ada) |
| Akses | Langsung (variable) | HAL API (BKUPWrite/Read) |

---

## 8. Dynamic Frequency Scaling (DFS)

### 8.1 Konsep DFS

DFS menurunkan frekuensi CPU saat beban rendah untuk menghemat daya:

$$P_{dynamic} = C \cdot V^2 \cdot f$$

Menurunkan $f$ secara linear mengurangi daya. Dalam beberapa kasus, tegangan juga bisa diturunkan (DVFS).

### 8.2 ESP32 DFS

```c
#include "esp_pm.h"

esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = 240,    // Frekuensi maksimum saat busy
    .min_freq_mhz = 40,     // Frekuensi minimum saat idle
    .light_sleep_enable = false
};
ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

// Power management lock (prevent DFS during critical section)
esp_pm_lock_handle_t lock;
esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "high_freq", &lock);
esp_pm_lock_acquire(lock);
// ... critical code ...
esp_pm_lock_release(lock);
```

### 8.3 STM32 Clock Switching

STM32F103 tidak memiliki DFS otomatis, tetapi bisa mengubah clock secara manual:

```c
void SetSystemClock_48MHz(void) {
    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1);
}

void SetSystemClock_8MHz(void) {
    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;  // Switch to internal 8MHz
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}
```

---

## 9. Peripheral Clock Gating & Power Gating

### 9.1 Clock Gating

Mematikan clock ke peripheral yang tidak aktif mengurangi dynamic power secara signifikan.

**ESP32:**
```c
#include "driver/periph_ctrl.h"

// Matikan WiFi
esp_wifi_stop();
esp_wifi_deinit();

// Matikan Bluetooth
esp_bt_controller_disable();
esp_bt_controller_deinit();

// Matikan peripheral tertentu
periph_module_disable(PERIPH_I2C0_MODULE);
periph_module_disable(PERIPH_SPI2_MODULE);
```

**STM32:**
```c
// Matikan clock GPIO yang tidak terpakai
__HAL_RCC_GPIOB_CLK_DISABLE();
__HAL_RCC_GPIOC_CLK_DISABLE();

// Matikan timer yang tidak terpakai
__HAL_RCC_TIM2_CLK_DISABLE();
__HAL_RCC_TIM3_CLK_DISABLE();

// Matikan ADC
__HAL_RCC_ADC1_CLK_DISABLE();
```

### 9.2 Power Gating (ESP32)

ESP32 mendukung power domain control:

```c
// Kontrol power domain saat deep sleep
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_AUTO);
esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
```

---

## 10. ULP Coprocessor (ESP32)

### 10.1 Apa itu ULP?

Ultra-Low-Power (ULP) Coprocessor adalah prosesor sederhana di dalam ESP32 yang dapat beroperasi saat main CPU dalam deep sleep. Konsumsi daya hanya ~150μA.

### 10.2 Kemampuan ULP

- Membaca sensor analog (ADC)
- Mengontrol GPIO
- Berkomunikasi via I2C (ULP RISC-V pada ESP32-S2/S3)
- Membangunkan main CPU berdasarkan kondisi
- Operasi timer periodik

### 10.3 Skenario Penggunaan

```
Main CPU (Deep Sleep, 10μA)
    ↕
ULP Coprocessor (Active, 150μA)
    ↓
Membaca sensor setiap 1 detik
    ↓
Jika sensor > threshold → bangunkan Main CPU
Jika tidak → lanjut tidur, cek lagi nanti
```

### 10.4 Contoh ULP FSM (Assembly)

```
// ULP membaca ADC dan membangunkan CPU jika > threshold
    move r3, threshold_val
    adc r0, 0, 6       // Baca ADC channel 6
    sub r0, r0, r3     // r0 = r0 - threshold
    jump wake_up, ov   // Jika overflow (r0 > threshold), wakeup
    halt                // Jika tidak, kembali tidur

wake_up:
    wake                // Bangunkan main CPU
    halt
```

---

## 11. Desain Sistem Battery-Powered

### 11.1 Komponen Sistem Battery-Powered

```
┌──────────────┐    ┌───────────┐    ┌──────────────┐
│   Baterai     │───→│ Regulator │───→│   MCU        │
│ (Li-Ion/LiPo)│    │  (LDO/    │    │ (ESP32/STM32)│
│  3.7V nom    │    │   Buck)   │    │              │
└──────────────┘    └───────────┘    └──────────────┘
       ↑                                    │
       │            ┌───────────┐           │
       └────────────│  Charger  │←──────────┘
                    │ (TP4056)  │     Monitoring
                    └───────────┘     (ADC → VBAT)
```

### 11.2 Monitoring Baterai

**ESP32 — ADC Reading:**
```c
// Baca tegangan baterai via voltage divider
// VBAT → [R1=100K] → ADC → [R2=100K] → GND
// V_adc = VBAT × R2/(R1+R2) = VBAT/2

#include "esp_adc/adc_oneshot.h"

adc_oneshot_unit_handle_t adc_handle;
adc_oneshot_unit_init_cfg_t cfg = { .unit_id = ADC_UNIT_1 };
adc_oneshot_new_unit(&cfg, &adc_handle);

int raw;
adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &raw);
float voltage = (raw / 4095.0) * 3.3 * 2;  // × 2 untuk voltage divider

// Estimasi level baterai (Li-Ion discharge curve)
int percent;
if (voltage >= 4.2) percent = 100;
else if (voltage >= 3.7) percent = (int)((voltage - 3.7) / 0.5 * 70 + 30);
else if (voltage >= 3.3) percent = (int)((voltage - 3.3) / 0.4 * 30);
else percent = 0;
```

**STM32 — ADC Baterai:**
```c
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 100);
uint32_t raw = HAL_ADC_GetValue(&hadc1);
float voltage = (raw / 4095.0f) * 3.3f * 2.0f;  // voltage divider
```

### 11.3 Solar Energy Harvesting

Untuk sistem outdoor, panel surya dapat digunakan:

```
Panel Surya (5V/1W) → MPPT/Charge Controller → Baterai → LDO → MCU
```

Strategi:
1. Charge baterai saat matahari bersinar
2. MCU deep sleep dengan wakeup periodik
3. Transmisi data saat baterai cukup
4. Low-power mode saat baterai kritis

---

## 12. Power Budget Analysis

### 12.1 Metodologi

Power Budget Analysis adalah proses menghitung total konsumsi daya sistem untuk menentukan kapasitas baterai dan masa pakai:

1. **Identifikasi** semua mode operasi
2. **Ukur/estimasi** arus pada setiap mode
3. **Tentukan** durasi setiap mode (duty cycle)
4. **Hitung** rata-rata arus

### 12.2 Template Power Budget

| Mode | Arus (mA) | Durasi (detik) | % Waktu | I × D (mA) |
|------|-----------|---------------|---------|------------|
| Deep Sleep | 0.01 | 290 | 96.67% | 0.00967 |
| Sensor Read | 25 | 2 | 0.67% | 0.167 |
| WiFi TX | 180 | 5 | 1.67% | 3.0 |
| Processing | 40 | 3 | 1.00% | 0.4 |
| **Total** | — | **300** | **100%** | **3.577** |

### 12.3 Menghitung Battery Life

$$I_{avg} = \sum_{i} I_i \times D_i = 3.577 \text{ mA}$$

Dengan baterai 18650 (3000mAh):

$$T_{life} = \frac{3000}{3.577} = 838.7 \text{ jam} ≈ 35 \text{ hari}$$

Dengan efisiensi regulator 85%:

$$T_{life,real} = 838.7 \times 0.85 = 712.9 \text{ jam} ≈ 29.7 \text{ hari}$$

### 12.4 Optimasi

Dari tabel, WiFi TX mengonsumsi 83.9% energi meskipun hanya 1.67% waktu. Strategi:
- Kurangi durasi WiFi (batch data, kompres)
- Gunakan protokol lebih hemat (MQTT vs HTTP)
- Kurangi frekuensi transmisi

---

## 13. Pengukuran Arus (Current Measurement)

### 13.1 Metode Pengukuran

| Metode | Range | Resolusi | Biaya |
|--------|-------|----------|-------|
| Multimeter | mA-A | 0.1mA | Rendah |
| Shunt Resistor + ADC | μA-A | 1μA | Rendah |
| INA219 (I2C) | μA-A | 10μA | Sedang |
| Power Profiler Kit | nA-A | 0.2μA | Tinggi |
| Oscilloscope + Probe | μA-A | Bervariasi | Tinggi |

### 13.2 Shunt Resistor Method

```
VCC ──[R_shunt=0.1Ω]──→ MCU (VDD)
         │
         │ V_shunt = I × R_shunt
         │ I = V_shunt / R_shunt
         ↓
      ADC / Multimeter
```

### 13.3 INA219 Current Sensor

INA219 adalah sensor arus/tegangan I2C yang dapat mengukur arus shunt hingga ±3.2A:

```c
// ESP32: Baca INA219 via I2C
uint8_t data[2];
i2c_master_read_from_device(I2C_NUM_0, INA219_ADDR, data, 2, 100);
int16_t raw_current = (data[0] << 8) | data[1];
float current_mA = raw_current * 0.1;  // tergantung kalibrasi
```

---

## 14. Perbandingan ESP32 vs STM32

### 14.1 Tabel Perbandingan Power Management

| Fitur | ESP32 | STM32F103 |
|-------|-------|-----------|
| Mode Sleep | 5 level | 3 level |
| Deep Sleep Current | 10μA | 2μA (Standby) |
| Light Sleep Current | 0.8mA | 10-15mA (Sleep) |
| Stop Mode Current | — | 20μA |
| RTC Memory | 8KB + 8KB | 20 bytes (BKP reg) |
| ULP Coprocessor | ✅ | ❌ |
| DFS | ✅ (otomatis) | ❌ (manual) |
| Wakeup Sources | Timer, GPIO, Touch, ULP | EXTI, WKUP pin, RTC |
| WiFi Power Save | ✅ | ❌ (via modul eksternal) |
| Power Domains | ✅ (per-domain) | ❌ (global) |

### 14.2 Kapan Menggunakan Apa?

- **ESP32**: Cocok untuk IoT battery-powered dengan WiFi/BLE, membutuhkan DFS dan ULP
- **STM32F103**: Cocok untuk ultra-low-power tanpa wireless, standby 2μA sangat rendah

---

## 15. Best Practices

### 15.1 Desain Hardware
1. Gunakan LDO/Buck converter dengan low quiescent current (<10μA)
2. Tambahkan power switch untuk sensor/modul eksternal
3. Pull-up/pull-down resistor pada GPIO saat sleep (hindari floating)
4. Voltage divider untuk monitoring baterai
5. Decoupling capacitor pada setiap VCC pin

### 15.2 Desain Firmware
1. **Matikan peripheral** yang tidak digunakan sebelum sleep
2. **Konfigurasi GPIO** ke mode analog atau output low sebelum sleep (reduce leakage)
3. **Gunakan RTC memory** untuk state persistence (ESP32)
4. **Batch data** sebelum transmisi wireless
5. **Implementasikan watchdog** untuk recovery dari hang
6. **Adaptive duty cycling** — sesuaikan frekuensi sampling dengan kebutuhan
7. **Error handling** — masuk deep sleep jika error persisten

### 15.3 Pengukuran dan Verifikasi
1. Selalu **ukur arus real** (jangan hanya andalkan datasheet)
2. Ukur arus di **setiap mode** operasi
3. Hitung **power budget** sebelum memilih baterai
4. Pertimbangkan **self-discharge** baterai (2-5% per bulan untuk Li-Ion)
5. Tambahkan **margin 20-30%** pada power budget

---

## 16. ULP Coprocessor — Detail Instruction Set

ULP (Ultra Low Power) coprocessor ESP32 dapat menjalankan program sederhana saat main CPU tidur, mengonsumsi hanya ~150 µA.

### 16.1 ULP Assembly Programming

ULP memiliki instruction set sendiri (bukan Xtensa/RISC-V):

```
ULP Assembly Instructions:
┌─────────────────────────────────────────────────────┐
│ Arithmetic:  ADD, SUB, AND, OR, LSH, RSH            │
│ Memory:      ST (store), LD (load)                   │
│ Branch:      JUMP, JUMPR, JUMPS                      │
│ I/O:         REG_RD, REG_WR (register access)       │
│ ADC:         ADC (read ADC in ULP)                   │
│ Wakeup:      WAKE (wakeup main CPU)                  │
│ Control:     HALT (stop ULP until next timer)        │
│ Counter:     STAGE_INC, STAGE_DEC, STAGE_RST         │
└─────────────────────────────────────────────────────┘
```

### 16.2 Contoh ULP: Monitoring Sensor Saat Deep Sleep

```c
#include "esp_sleep.h"
#include "ulp.h"
#include "ulp_main.h"  /* Generated from ULP assembly */

/* ULP Assembly Program (ulp_program.S) */
/*
    .global entry
entry:
    // Baca ADC
    adc r0, 0, 6        // ADC1_CH6 (GPIO34)
    
    // Simpan hasil ke RTC memory
    move r1, adc_value
    st r0, r1, 0
    
    // Bandingkan dengan threshold
    move r1, threshold
    ld r1, r1, 0
    sub r0, r0, r1
    jump wake_cpu, ov    // Jika > threshold, bangunkan CPU
    
    halt                 // Kembali tidur
    
wake_cpu:
    wake                 // Bangunkan main CPU
    halt
    
    .global adc_value
adc_value: .long 0
    .global threshold
threshold: .long 2000   // ~1.6V pada 12-bit ADC
*/

void setup_ulp_monitoring(void)
{
    /* Load ULP program */
    ulp_load_binary(0, ulp_main_bin_start,
                    (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    
    /* Set ULP wakeup period: setiap 500ms */
    ulp_set_wakeup_period(0, 500000);  /* microseconds */
    
    /* Set threshold via RTC memory */
    ulp_threshold = 2000;
    
    /* Jalankan ULP dan tidurkan main CPU */
    ulp_run(&ulp_entry - RTC_SLOW_MEM);
    esp_deep_sleep_start();
}

/* Saat main CPU bangun, baca data ULP */
void app_main(void)
{
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP) {
        uint16_t adc_val = ulp_adc_value & 0xFFFF;
        printf("ULP detected threshold! ADC=%d\n", adc_val);
        /* Kirim alert via WiFi, dll */
    }
    
    setup_ulp_monitoring();  /* Kembali tidur */
}
```

---

## 17. Security — Secure Boot dan Flash Encryption

### 17.1 ESP32 Secure Boot

Secure Boot memastikan hanya firmware yang sah yang bisa dijalankan:

```
Secure Boot Flow:
┌──────────────────────────────────────────────┐
│ 1. eFuse stores public key hash              │
│ 2. ROM verifies 2nd stage bootloader         │
│ 3. Bootloader verifies app partition         │
│ 4. If verification fails → refuse to boot    │
└──────────────────────────────────────────────┘

Enable via menuconfig:
  Security features →
    [*] Enable hardware Secure Boot in bootloader
    [*] Sign binaries during build
```

### 17.2 ESP32 Flash Encryption

Flash encryption melindungi firmware dan data dari pembacaan:

```
Flash Encryption:
┌──────────────────────────────────────────────┐
│ Plain firmware → AES-256 → Encrypted flash   │
│ Decryption key stored in eFuse (one-time)    │
│ Transparent to software (decrypt on read)    │
│                                              │
│ ⚠️ WARNING: Sekali di-enable, tidak bisa     │
│    di-disable! (eFuse bersifat one-time)     │
└──────────────────────────────────────────────┘
```

### 17.3 STM32 Read-Out Protection

STM32 memiliki 3 level Read-Out Protection (RDP):

| Level | Proteksi | Reversible? |
|-------|----------|-------------|
| **Level 0** | Tidak ada proteksi | — |
| **Level 1** | Flash tidak bisa dibaca via debugger | Ya (erase flash) |
| **Level 2** | Permanent — debug port disabled | **Tidak!** |

```c
/* Enable RDP Level 1 */
void enable_rdp(void)
{
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    
    FLASH_OBProgramInitTypeDef ob = {0};
    ob.OptionType = OPTIONBYTE_RDP;
    ob.RDPLevel   = OB_RDP_LEVEL_1;
    HAL_FLASHEx_OBProgram(&ob);
    
    HAL_FLASH_OB_Launch();  /* Reset required */
}
```

> ⚠️ **HATI-HATI!** Level 2 bersifat **permanent** dan tidak bisa dibatalkan. Chip akan terkunci selamanya.

---

## 18. Advanced Debugging

### 18.1 Serial Wire Viewer (SWV) — STM32

SWV memungkinkan trace output real-time tanpa mengganggu eksekusi program:

```c
/* ITM (Instrumentation Trace Macrocell) untuk printf via SWO */
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

/* Sekarang printf() keluar via SWO pin, bukan UART */
/* Di debugger, enable SWV di frekuensi SWO yang sesuai */
```

### 18.2 ESP32 Core Dump

ESP32 dapat menyimpan core dump saat crash untuk analisis post-mortem:

```c
/* menuconfig → ESP System Settings →
   Core dump destination: Flash */

/* Saat crash terjadi, core dump otomatis tersimpan di flash */
/* Untuk menganalisis: */
/* $ espcoredump.py info_corefile -t elf build/project.elf core.bin */
```

### 18.3 Fault Analysis STM32

```c
/* Hard Fault Handler untuk debugging */
void HardFault_Handler(void)
{
    __asm volatile(
        "TST LR, #4    \n"
        "ITE EQ        \n"
        "MRSEQ R0, MSP \n"
        "MRSNE R0, PSP \n"
        "B hard_fault_handler_c \n"
    );
}

void hard_fault_handler_c(uint32_t *stack)
{
    printf("=== HARD FAULT ===\n");
    printf("R0  = 0x%08lX\n", stack[0]);
    printf("R1  = 0x%08lX\n", stack[1]);
    printf("R2  = 0x%08lX\n", stack[2]);
    printf("R3  = 0x%08lX\n", stack[3]);
    printf("R12 = 0x%08lX\n", stack[4]);
    printf("LR  = 0x%08lX\n", stack[5]);
    printf("PC  = 0x%08lX\n", stack[6]);  /* ← Alamat crash */
    printf("xPSR= 0x%08lX\n", stack[7]);
    
    /* Baca Fault Status Registers */
    printf("CFSR = 0x%08lX\n", SCB->CFSR);
    printf("HFSR = 0x%08lX\n", SCB->HFSR);
    printf("BFAR = 0x%08lX\n", SCB->BFAR);
    
    while (1);  /* Halt for debugger */
}
```

---

## 19. Daftar Program Praktikum

| No | Platform | Nama Program | Topik | Tingkat |
|----|----------|-------------|-------|---------|
| 01 | ESP32 | Light_Sleep | Light sleep + GPIO wakeup | Dasar |
| 02 | ESP32 | Deep_Sleep | Deep sleep + timer wakeup | Dasar |
| 03 | ESP32 | Touch_Wakeup | Deep sleep + touch pad wakeup | Menengah |
| 04 | ESP32 | ULP_Monitor | ULP ADC monitoring saat deep sleep | Lanjut |
| 05 | ESP32 | DFS_Dynamic | Dynamic Frequency Scaling | Menengah |
| 06 | ESP32 | Battery_Monitor | ADC battery voltage + power budget | Menengah |
| 07 | STM32 | Sleep_Mode | CPU sleep mode + EXTI wakeup | Dasar |
| 08 | STM32 | Stop_Mode | Stop mode + RTC wakeup | Menengah |
| 09 | STM32 | Standby_Mode | Standby + WKUP pin wakeup | Menengah |
| 10 | STM32 | Clock_Gating | Peripheral clock enable/disable | Menengah |
| 11 | STM32 | RTC_Backup | RTC + backup register | Menengah |
| 12 | STM32 | Current_Measure | INA219 power measurement | Lanjut |

---

## Referensi API

### ESP32 ESP-IDF
- `esp_sleep.h` — Sleep modes & wakeup configuration
- `esp_pm.h` — Power management framework & DFS
- `driver/touch_pad.h` — Touch pad driver
- `esp_adc/adc_oneshot.h` — ADC oneshot reading
- `driver/periph_ctrl.h` — Peripheral clock control
- `esp_ota_ops.h` — OTA update operations
- `ulp.h` — ULP coprocessor programming

### STM32 HAL
- `stm32f1xx_hal_pwr.h` — Power control (Sleep, Stop, Standby)
- `stm32f1xx_hal_rcc.h` — Clock configuration & peripheral clocks
- `stm32f1xx_hal_rtc.h` — RTC & backup registers
- `stm32f1xx_hal_adc.h` — ADC for battery monitoring
- `stm32f1xx_hal_gpio.h` — GPIO configuration for low-power
- `stm32f1xx_hal_flash_ex.h` — Flash & Option Bytes (RDP)

### Buku Referensi
1. *Mastering STM32* — Chapters: Power Management, Booting, Advanced Debug
2. *Kolban's Book on ESP32* — Sections: ULP, Security, OTA
3. ESP-IDF Programming Guide — Security, Deep Sleep, ULP
