# Project Modul 11: FreeRTOS Timer & Notification (Smart Kitchen Timer)

## 🎯 Tujuan Project
1.  Mahasiswa mampu merancang sistem embedded multitasking menggunakan **FreeRTOS**.
2.  Mahasiswa dapat mengimplementasikan **Software Timer** (One-shot dan Auto-reload) untuk manajemen waktu event.
3.  Mahasiswa mampu menggunakan **Task Notification** sebagai mekanasi *lightweight communication* antar Task dan dari ISR ke Task.
4.  Mahasiswa dapat membedakan penggunaan *Binary Semaphore* dengan *Task Notification* dalam konteks sinkronisasi.

## 📝 Deskripsi Project: "Sistem Pengontrol Mesin Cuci Otomatis"
Dalam project ini, Anda diminta untuk mensimulasikan logika kontrol mesin cuci sederhana menggunakan FreeRTOS. Sistem ini memiliki beberapa tahapan kerja (Washing, Rinsing, Spinning) yang durasinya diatur oleh **Software Timer**. Interaksi pengguna (Tombol) akan ditangani menggunakan interrupt yang mengirimkan sinyal ke Task controller menggunakan **Task Notification**.

### Skenario Kerja Sistem
Sistem memiliki 3 status utama yang berjalan berurutan secara otomatis setelah tombol Start ditekan:
1.  **Idle**: Menunggu input user.
2.  **Washing**: Mensimulasikan proses pencucian (Motor berputar bolak-balik / LED berkedip lambat). Durasi: 5 detik.
3.  **Rinsing**: Mensimulasikan pembilasan (Klep air buka-tutup / LED berkedip cepat). Durasi: 3 detik.
4.  **Spinning**: Mensimulasikan pengeringan (Motor putar kencang / LED menyala *breathing* atau ON terus). Durasi: 4 detik.
5.  **Finish**: Kembali ke Idle.

Tombol **Pause** dapat ditekan sewaktu-waktu untuk menghentikan timer sementara (Stop Timer), dan ditekan lagi untuk melanjutkan (Start Timer).

## 🛠️ Spesifikasi Teknis

### 1. Hardware
- **Mikrokontroller**: STM32F103 (Blue Pill) atau ESP32.
- **Actuators**:
    - 1 LED Utama (Built-in LED PC13 atau GPIO2): Indikator Status Mesin.
    - 1 LED Tambahan (Opsional): Indikator "System Ready".
- **Inputs**:
    - Button 1 (PA0 / GPIO0): **Start / Pause / Resume**.
    - Button 2 (PA1 / GPIO4): **Emergency Stop / Reset** (Langsung ke Idle).

### 2. Software Requirements (FreeRTOS Components)

#### A. Tasks
1.  **Task Controller**:
    - Priority: High.
    - Fungsi: Menerima notifikasi dari Button ISR. Mengatur *State Machine* sistem (Idle -> Wash -> Rinse -> Spin). Mengontrol Timer (Start/Stop/Reset).
2.  **Task Display/Status**:
    - Priority: Medium.
    - Fungsi: Menampilkan status saat ini ke Serial Monitor setiap detik (atau saat perubahan state). Mengontrol pola kedipan LED berdasarkan state.

#### B. Software Timers
1.  **Process Timer** (One-shot):
    - Digunakan untuk menghitung durasi setiap tahapan (Wash 5s, Rinse 3s, Spin 4s).
    - Saat timer *expire*, callback function akan mengirim notifikasi ke Task Controller untuk pindah ke state berikutnya.
2.  **Blink Timer** (Auto-reload) - *Opsional (Bisa diganti vTaskDelay di Task Status)*:
    - Digunakan untuk mengatur frekuensi kedipan LED tanpa blocking delay.

#### C. Interrupts & Notification
- **Button ISR**:
    - Tidak boleh ada logic berat (seperti delay atau print/printf) di dalam ISR.
    - Gunakan `vTaskNotifyGiveFromISR()` atau `xTaskNotifyFromISR()` untuk mengirim sinyal ke **Task Controller**.
- **Timer Callback**:
    - Callback fungsi timer mengirim notifikasi ke Task Controller bahwa waktu tahapan telah habis.

### 3. Logika State Machine & Indikator LED

| State | Durasi | Pola LED | Serial Output (Contoh) |
| :--- | :--- | :--- | :--- |
| **IDLE** | - | Mati (OFF) | `[SISTEM] Siap. Tekan Start.` |
| **WASH** | 5 Detik | Blink Lambat (500ms ON, 500ms OFF) | `[STATUS] Mencuci... Sisa 4s` |
| **RINSE** | 3 Detik | Blink Cepat (100ms ON, 100ms OFF) | `[STATUS] Membilas... Sisa 2s` |
| **SPIN** | 4 Detik | Nyala Terus (ON) | `[STATUS] Mengeringkan... ` |
| **PAUSED** | - | Blink Sangat Pendek (100ms ON, 1000ms OFF) | `[SISTEM] Terjeda.` |

---

## 📋 Deliverables (Yang Dikumpulkan)

1.  **Source Code**:
    - File `main.c` (STM32) atau `main.cpp` (ESP32).
    - Kode harus rapi, diberi komentar penjelas pada bagian konfigurasi Timer dan Notifikasi.
2.  **Video Demo** (Maksimal 2 menit):
    - Tunjukkan hardware setup yang digunakan.
    - Demokan alur normal: Idle -> Start -> Wash -> Rinse -> Spin -> Idle.
    - Demokan fitur Pause dan Resume di tengah proses.
    - Demokan fitur Reset/Emergency Stop.
    - Tampilkan Serial Monitor yang berjalan sinkron dengan hardware.
3.  **Laporan Singkat (README.md / PDF)**:
    - Penjelasan singkat alur program (State Diagram).
    - Penjelasan mengapa menggunakan Task Notification lebih baik daripada Semaphore/Queue untuk kasus tombol ini.

## 💡 Tips Pengerjaan
1.  **Debouncing**: Ingat bahwa tombol mekanik membutuhkan debouncing. Anda bisa menggunakan delay sederhana di Task Controller setelah menerima notifikasi, atau menggunakan *Software Timer* tambahan untuk debouncing yang lebih elegan.
2.  **Struct State**: Gunakan `enum` untuk mendefinisikan State mesin cuci agar kode lebih mudah dibaca (misal: `typedef enum { IDLE, WASH, RINSE, SPIN, PAUSE } MachineState_t;`).
3.  **Timer ID**: Jika menggunakan satu timer handle untuk berbagai durasi, gunakan `xTimerChangePeriod()` untuk mengubah durasi sebelum memulai timer untuk tahap berikutnya. Atau, gunakan *Timer ID* (`pvTimerGetTimerID`) jika Anda membuat banyak timer instance.

## ⛔ Larangan
- Menggunakan `HAL_Delay()` atau `delay()` yang bersifat blocking di dalam Task utama lebih dari 10ms. Gunakan `vTaskDelay()`.
- Melakukan `printf` atau operasi String di dalam **ISR** atau **Timer Callback**. Lakukan operasi berat tersebut di Task biasa.

---

## 🏗️ Skeleton Code Reference

### ESP32 (ESP-IDF)
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "MESIN_CUCI";

/* Pin Definitions */
#define BUTTON_START    GPIO_NUM_0
#define BUTTON_RESET    GPIO_NUM_4
#define LED_STATUS      GPIO_NUM_2

/* State Machine */
typedef enum {
    STATE_IDLE = 0,
    STATE_WASH,
    STATE_RINSE,
    STATE_SPIN,
    STATE_PAUSED,
    STATE_FINISH
} MachineState_t;

/* Duration per state (ms) */
#define WASH_DURATION_MS    5000
#define RINSE_DURATION_MS   3000
#define SPIN_DURATION_MS    4000

volatile MachineState_t currentState = STATE_IDLE;
volatile MachineState_t pausedState = STATE_IDLE;

TaskHandle_t xControllerTask = NULL;
TimerHandle_t xProcessTimer = NULL;
TimerHandle_t xBlinkTimer = NULL;

/* Notification Bits */
#define NOTIF_BUTTON_START  (1 << 0)
#define NOTIF_BUTTON_RESET  (1 << 1)
#define NOTIF_TIMER_EXPIRE  (1 << 2)

/* ISR Handlers */
static void IRAM_ATTR startISR(void *arg) {
    BaseType_t woken = pdFALSE;
    xTaskNotifyFromISR(xControllerTask, NOTIF_BUTTON_START, eSetBits, &woken);
    if(woken) portYIELD_FROM_ISR();
}

static void IRAM_ATTR resetISR(void *arg) {
    BaseType_t woken = pdFALSE;
    xTaskNotifyFromISR(xControllerTask, NOTIF_BUTTON_RESET, eSetBits, &woken);
    if(woken) portYIELD_FROM_ISR();
}

/* Timer Callbacks */
void vProcessTimerCallback(TimerHandle_t xTimer) {
    xTaskNotify(xControllerTask, NOTIF_TIMER_EXPIRE, eSetBits);
}

/* TODO: Implement ControllerTask, StatusTask, app_main */
void app_main(void) {
    // 1. Configure GPIO pins
    // 2. Create tasks
    // 3. Create timers
    // 4. Install ISR service
    ESP_LOGI(TAG, "Sistem Mesin Cuci Otomatis Ready!");
}
```

### STM32 (STM32Cube HAL)
```c
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

/* Pin Definitions - Blue Pill */
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13     // Active LOW
#define BTN_START_PORT  GPIOA
#define BTN_START_PIN   GPIO_PIN_0
#define BTN_RESET_PORT  GPIOA
#define BTN_RESET_PIN   GPIO_PIN_1

typedef enum {
    STATE_IDLE = 0,
    STATE_WASH,
    STATE_RINSE,
    STATE_SPIN,
    STATE_PAUSED,
    STATE_FINISH
} MachineState_t;

volatile MachineState_t currentState = STATE_IDLE;

TaskHandle_t xControllerTask = NULL;
TimerHandle_t xProcessTimer = NULL;

#define NOTIF_BUTTON_START  (1 << 0)
#define NOTIF_BUTTON_RESET  (1 << 1)
#define NOTIF_TIMER_EXPIRE  (1 << 2)

/* EXTI Callback (HAL ISR) */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BaseType_t woken = pdFALSE;
    if(GPIO_Pin == BTN_START_PIN) {
        xTaskNotifyFromISR(xControllerTask, NOTIF_BUTTON_START, eSetBits, &woken);
    } else if(GPIO_Pin == BTN_RESET_PIN) {
        xTaskNotifyFromISR(xControllerTask, NOTIF_BUTTON_RESET, eSetBits, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

/* TODO: Implement ControllerTask, StatusTask, main */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    // GPIO, UART init...
    
    printf("[SISTEM] Mesin Cuci Otomatis Ready!\r\n");
    
    // Create tasks and timers
    // ...
    
    vTaskStartScheduler();
    while(1) {}
}
```

---

## 📊 Kriteria Penilaian

| Komponen | Bobot | Kriteria |
| :--- | :---: | :--- |
| **State Machine** | 25% | Alur IDLE→WASH→RINSE→SPIN→FINISH berjalan benar dan otomatis |
| **Software Timer** | 25% | Timer one-shot mengatur durasi tiap tahap, timer auto-reload untuk LED blink |
| **Task Notification** | 20% | Button ISR mengirim notifikasi dengan benar, tidak ada logic berat di ISR |
| **Pause/Resume** | 15% | Fitur pause/resume bekerja di semua state, timer di-stop/start dengan benar |
| **Emergency Stop** | 10% | Reset langsung ke IDLE, semua timer dihentikan, LED mati |
| **Kode & Dokumentasi** | 5% | Kode rapi, komentar jelas, README/laporan lengkap |

### Penilaian Bonus (+10%)
- Implementasi debounce menggunakan software timer (bukan delay)
- Menampilkan countdown timer di serial monitor
- Dual-platform (ESP32 + STM32) dengan fitur lengkap
- Menambahkan buzzer notification saat state berubah

---
**Selamat Mengerjakan!**
