# Materi Modul 14: Power Management & Low-Power Design

## 1. Pengantar Power Management pada Embedded Systems
Dalam sistem embedded, terutama yang berjalan dengan baterai (wearable, IoT sensor node, remote logger), **konsumsi daya** menjadi faktor kritis. Sebuah ESP32 yang selalu aktif dengan WiFi bisa menghabiskan ~240mA, sementara dalam mode deep sleep hanya ~10µA — perbedaan **24.000x lipat**! Memahami teknik low-power design memungkinkan perangkat bertahan berminggu-minggu bahkan bertahun-tahun dengan satu baterai coin cell.

### Prinsip Dasar Konsumsi Daya
Daya yang dikonsumsi mikrokontroler ditentukan oleh:
$$P = C \times V^2 \times f + I_{leak} \times V$$
Dimana:
- $C$ = Kapasitansi switching (proporsional dengan jumlah gate aktif)
- $V$ = Tegangan supply
- $f$ = Frekuensi clock
- $I_{leak}$ = Arus bocor (leakage current)

Dari rumus ini, ada 3 strategi utama:
1. **Turunkan frekuensi** ($f$) → Clock gating, dynamic frequency scaling
2. **Turunkan tegangan** ($V$) → Dynamic voltage scaling
3. **Matikan modul tidak terpakai** → Peripheral clock gating, sleep modes

---

## 2. Low-Power Modes

### STM32 Low-Power Modes (STM32Cube HAL)
STM32 menyediakan beberapa level low-power yang progressif, dikonfigurasi melalui **STM32Cube HAL Power API**:

| Mode | Konsumsi | CPU | SRAM | Peripheral | Wake-up Time |
|------|----------|-----|------|------------|--------------|
| **Run** | ~30-50 mA | Aktif | Aktif | Aktif | - |
| **Sleep** | ~10-15 mA | Stop | Aktif | Aktif | ~1 µs |
| **Stop** | ~2-20 µA | Stop | Aktif (retained) | Stop | ~5 µs |
| **Standby** | ~2-3 µA | Stop | Lost | Stop | ~50 µs (reset) |

- **Sleep Mode**: CPU berhenti via `HAL_PWR_EnterSLEEPMode()`, peripheral tetap jalan. Dibangunkan oleh interrupt apapun.
- **Stop Mode**: Semua clock berhenti via `HAL_PWR_EnterSTOPMode()`, regulator low-power. SRAM & register dipertahankan. Dibangunkan oleh EXTI, RTC alarm. **Setelah wake-up, clock harus dikonfigurasi ulang** karena PLL dimatikan.
- **Standby Mode**: Daya terhemat via `HAL_PWR_EnterSTANDBYMode()`, tapi SRAM hilang (seperti reset). Hanya wake-up pin, RTC alarm, atau IWDG reset yang bisa membangunkan. Data bisa disimpan di **Backup Registers**.

### ESP32 Low-Power Modes (ESP-IDF)
ESP-IDF menyediakan API lengkap melalui header `esp_sleep.h` dan `esp_pm.h`:

| Mode | Konsumsi | CPU | WiFi/BT | RTC | Wake-up Time |
|------|----------|-----|---------|-----|--------------|
| **Active** | ~160-240 mA | Aktif | Aktif | Aktif | - |
| **Modem Sleep** | ~20 mA | Aktif | Off | Aktif | Instant |
| **Light Sleep** | ~0.8 mA | Paused | Off | Aktif | ~1 ms |
| **Deep Sleep** | ~10 µA | Off | Off | Aktif | ~200 ms |
| **Hibernation** | ~5 µA | Off | Off | Minimal | ~200 ms |

- **Modem Sleep**: WiFi/BT radio dimatikan via `esp_wifi_stop()`, CPU masih aktif.
- **Light Sleep**: CPU di-pause via `esp_light_sleep_start()`, RTC dan ULP masih jalan. SRAM dipertahankan.
- **Deep Sleep**: Hanya RTC controller, RTC memory (8KB), dan ULP co-processor yang aktif. Dikonfigurasi via `esp_deep_sleep_start()`. Entry point setelah wake-up adalah `app_main()` (seperti cold boot).
- **Hibernation**: Dikonfigurasi dengan `esp_sleep_pd_config()` untuk mematikan RTC memory juga. Konsumsi terendah (~5µA).

---

## 3. Wake-up Sources (Sumber Pembangun)

### STM32 Wake-up Sources (HAL API)
1. **EXTI (External Interrupt)**: Pin tertentu (WKUP pin) via `HAL_PWR_EnableWakeUpPin()`.
2. **RTC Alarm**: Timer RTC via `HAL_RTC_SetAlarm_IT()` bisa dijadwalkan untuk membangunkan MCU.
3. **RTC Wakeup Timer**: Periodic wake-up menggunakan sub-second timer RTC.
4. **IWDG (Independent Watchdog)**: Reset dari watchdog bisa membangunkan dari Standby.
5. **NRST Pin**: Reset eksternal.

### ESP32 Wake-up Sources (ESP-IDF API)
1. **Timer**: `esp_sleep_enable_timer_wakeup()` — RTC timer dengan resolusi microsecond.
2. **Touch Pad**: `esp_sleep_enable_touchpad_wakeup()` — sensor kapasitif built-in.
3. **External Wake-up ext0**: `esp_sleep_enable_ext0_wakeup()` — satu GPIO RTC, level HIGH/LOW.
4. **External Wake-up ext1**: `esp_sleep_enable_ext1_wakeup()` — beberapa GPIO RTC, logic ANY/ALL.
5. **ULP Co-processor**: `esp_sleep_enable_ulp_wakeup()` — program di ULP bisa membangunkan main CPU.
6. **GPIO**: Light sleep bisa dibangunkan oleh perubahan GPIO via `gpio_wakeup_enable()`.

---

## 4. Clock Gating & Dynamic Frequency Scaling

### Clock Gating
Konsep: matikan clock peripheral yang tidak digunakan. Tanpa clock, transistor tidak switching, sehingga **tidak ada dynamic power consumption**.

**STM32 (HAL):**
```c
// Aktifkan clock hanya saat dibutuhkan
__HAL_RCC_GPIOA_CLK_ENABLE();
// ... gunakan GPIOA ...
__HAL_RCC_GPIOA_CLK_DISABLE();  // Matikan saat tidak dipakai

// Matikan clock ADC saat tidak dipakai
__HAL_RCC_ADC1_CLK_DISABLE();
// Matikan clock SPI saat tidak dipakai
__HAL_RCC_SPI1_CLK_DISABLE();
```

**ESP32 (ESP-IDF):**
```c
#include "driver/periph_ctrl.h"

// Disable peripheral clock via periph_ctrl
periph_module_disable(PERIPH_I2C0_MODULE);
periph_module_disable(PERIPH_SPI2_MODULE);

// Disable WiFi & Bluetooth
esp_wifi_stop();
esp_wifi_deinit();
esp_bt_controller_disable();
```

### Dynamic Frequency Scaling (DFS)
Menurunkan frekuensi CPU saat beban rendah, menaikkan saat butuh performa tinggi.

**ESP32 (ESP-IDF)** mendukung DFS secara native:
```c
#include "esp_pm.h"

esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = 240,
    .min_freq_mhz = 10,
    .light_sleep_enable = true
};
esp_pm_configure(&pm_config);
```

**STM32 (HAL)** bisa menurunkan clock melalui reconfiguration:
```c
// Switch dari PLL (72MHz) ke HSI (8MHz) untuk hemat daya
RCC_ClkInitTypeDef clk = {0};
clk.ClockType = RCC_CLOCKTYPE_SYSCLK;
clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;  // 8MHz internal
HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
```

---

## 5. ULP Co-Processor (ESP32)
ESP32 memiliki **Ultra-Low Power co-processor** — sebuah prosesor kecil yang berjalan saat main CPU dalam deep sleep. Diprogram menggunakan assembly ULP atau ULP-RISC-V (pada ESP32-S2/S3).

### Kemampuan ULP:
- Baca sensor via ADC (`adc` instruction)
- Baca/tulis GPIO (`reg_rd`, `reg_wr`)
- Operasi aritmatika sederhana
- Akses RTC memory (8KB shared dengan main CPU)
- Wake-up main CPU berdasarkan kondisi (`wake` instruction)

### Penggunaan via ESP-IDF:
```c
#include "esp_sleep.h"
#include "ulp.h"

// Load ULP program
ulp_load_binary(0, ulp_main_bin_start, ...);
// Set ULP wake-up period
ulp_set_wakeup_period(0, 1000000);  // 1 detik
// Enable ULP wake-up
esp_sleep_enable_ulp_wakeup();
// Start ULP program
ulp_run(&ulp_entry - RTC_SLOW_MEM);
```

### Konsumsi ULP:
- ULP aktif: ~150 µA (jauh lebih hemat dari main CPU ~50mA)
- ULP + deep sleep: ~10-150 µA tergantung aktivitas

---

## 6. RTC Memory & Data Persistence

### ESP32 RTC Memory (ESP-IDF)
ESP32 memiliki **8KB RTC SLOW memory** yang tetap aktif selama deep sleep:
```c
// Variabel di RTC memory — bertahan selama deep sleep
RTC_DATA_ATTR int boot_count = 0;
RTC_DATA_ATTR float last_temperature = 0.0f;
RTC_DATA_ATTR uint8_t data_buffer[256];
```
- Variabel dengan `RTC_DATA_ATTR` ditempatkan di RTC SLOW memory.
- Hilang hanya saat power cycle atau hibernation mode (jika RTC memory dimatikan).

### STM32 Backup Domain (HAL)
STM32 memiliki **Backup registers** dan **VBAT domain**:
```c
// Aktifkan akses Backup Domain
__HAL_RCC_PWR_CLK_ENABLE();
__HAL_RCC_BKP_CLK_ENABLE();
HAL_PWR_EnableBkpAccess();

// Tulis ke backup register
HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, boot_count);

// Baca dari backup register
uint32_t saved = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
```
- Backup registers (20 x 16-bit pada F1) tetap tersimpan selama VBAT ada tegangan.
- RTC tetap berjalan dengan baterai coin cell di VBAT pin.
- Data hilang hanya jika VBAT dan VDD sama-sama hilang.

---

## 7. Battery Monitoring & Optimization

### Teknik Monitoring Baterai:
1. **Voltage Divider + ADC**: Bagi tegangan baterai dengan resistor divider, baca via ADC.
2. **Internal VREFINT (STM32)**: Bandingkan Vref internal (1.2V) via `HAL_ADC_Start()` untuk kalibrasi.
3. **ESP32 ADC (ESP-IDF)**: Gunakan `adc1_get_raw()` dengan attenuation dan kalibrasi eFuse via `esp_adc_cal`.

### Strategi Optimasi Baterai:
1. **Duty Cycling**: Aktif sebentar, tidur lama (contoh: 2 detik aktif, 5 menit tidur).
2. **Adaptive Sampling**: Tingkatkan frekuensi sampling hanya saat ada event penting.
3. **Data Batching**: Kumpulkan beberapa pembacaan sensor di RTC memory, kirim sekaligus.
4. **WiFi Power Save**: Gunakan DTIM interval tinggi, modem sleep antar transmisi.
5. **Peripheral Power Control**: Matikan sensor/modul eksternal via MOSFET switch.

### Estimasi Umur Baterai:
$$T_{battery} = \frac{C_{battery}}{I_{avg}}$$

Dimana $I_{avg}$ untuk duty cycling:
$$I_{avg} = \frac{I_{active} \times t_{active} + I_{sleep} \times t_{sleep}}{t_{active} + t_{sleep}}$$

**Contoh:**
- Baterai 2000mAh, active 100mA selama 5 detik, sleep 10µA selama 295 detik
- $I_{avg} = \frac{100 \times 5 + 0.01 \times 295}{300} = 1.68\text{ mA}$
- $T = \frac{2000}{1.68} \approx 1190\text{ jam} \approx 50\text{ hari}$

---

## 8. Hardware Implementation

### ESP32 (ESP-IDF Framework via PlatformIO)
- **Platform**: `espressif32`
- **Framework**: `esp-idf`
- **Board**: `esp32doit-devkit-v1`
- Entry point: `void app_main(void)`
- FreeRTOS terintegrasi dan otomatis berjalan
- Power API: `esp_sleep.h`, `esp_pm.h`, `driver/rtc_io.h`
- Logging: `ESP_LOGI()`, `ESP_LOGW()`, `ESP_LOGE()`

### STM32 (STM32Cube HAL Framework via PlatformIO)
- **Platform**: `ststm32`
- **Framework**: `stm32cube`
- **Board**: `bluepill_f103c8`
- Entry point: `int main(void)` → `vTaskStartScheduler()`
- FreeRTOS dikompilasi via `extra_script_F103.py`
- Power API: `stm32f1xx_hal_pwr.h`, `stm32f1xx_hal_rtc.h`
- Logging: `UART_SendString()` via USART1

### Pertimbangan Hardware:
- **Decoupling capacitor** pada VBAT untuk backup domain STM32
- **Voltage regulator** efisiensi tinggi (LDO vs switching regulator)
- **Power MOSFET** untuk mematikan peripheral eksternal
- **Pull-up/pull-down** pada pin floating untuk menghindari arus bocor
- **LED** current limiter — LED status bisa menghabiskan 10-20mA!
