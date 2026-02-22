# JOBSHEET BAB 09: FreeRTOS Task Management

## 📋 Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | FreeRTOS Task Management |
| **Platform** | STM32F103C8T6 (Blue Pill), ESP32 DevKit V1 |
| **Jumlah Program STM32** | 10 |
| **Jumlah Program ESP32** | 10 |
| **Durasi** | 3 x 50 menit |
| **Prasyarat** | Modul 1-8 (GPIO, Interrupt, Timer, UART) |

---

## 🎯 Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa mampu:

1. Memahami konsep Real-Time Operating System (RTOS)
2. Membuat dan mengelola task di FreeRTOS
3. Mengatur prioritas dan state task
4. Mengimplementasikan multi-tasking pada STM32 dan ESP32
5. Melakukan debugging task menggunakan runtime statistics
6. Menerapkan pattern task design yang baik

---

## 🛠️ Komponen yang Dibutuhkan

| Komponen | Jumlah | Keterangan |
|----------|--------|------------|
| STM32F103C8T6 | 1 | Blue Pill Board |
| ESP32 DevKit V1 | 1 | 30/38 pin |
| LED | 4 | Merah, Kuning, Hijau, Biru |
| Push Button | 2 | Momentary switch |
| Resistor 220Ω | 4 | Untuk LED |
| Resistor 10kΩ | 2 | Pull-up button |
| Potensiometer 10kΩ | 1 | Untuk input analog |
| Breadboard | 1 | Full size |
| Kabel Jumper | 20 | Male-to-male |
| USB-TTL | 1 | Untuk debugging |

---

## 🔌 Skema Koneksi

### STM32F103C8T6

\`\`\`
STM32F103C8T6 Pinout:
┌─────────────────────────────────────┐
│                                     │
│  PA0  ──── Button 1 (dengan 10K pull-up)
│  PA1  ──── Button 2 (dengan 10K pull-up)
│  PA4  ──── LED Merah (dengan 220Ω)
│  PA5  ──── LED Kuning (dengan 220Ω)
│  PA6  ──── LED Hijau (dengan 220Ω)
│  PA7  ──── LED Biru (dengan 220Ω)
│  PA9  ──── USB-TTL TX
│  PA10 ──── USB-TTL RX
│  PB0  ──── Potentiometer (ADC input)
│  3.3V ──── VCC
│  GND  ──── GND
│                                     │
└─────────────────────────────────────┘
\`\`\`

### ESP32 DevKit

\`\`\`
ESP32 DevKit Pinout:
┌─────────────────────────────────────┐
│                                     │
│  GPIO2  ──── Built-in LED
│  GPIO4  ──── LED Merah (dengan 220Ω)
│  GPIO5  ──── LED Kuning (dengan 220Ω)
│  GPIO18 ──── LED Hijau (dengan 220Ω)
│  GPIO19 ──── LED Biru (dengan 220Ω)
│  GPIO21 ──── Button 1 (internal pull-up)
│  GPIO22 ──── Button 2 (internal pull-up)
│  GPIO34 ──── Potentiometer (ADC input)
│  3.3V   ──── VCC
│  GND    ──── GND
│                                     │
└─────────────────────────────────────┘
\`\`\`

---

## 📝 Praktikum STM32

### Program 1: Basic Task Creation - STM32

**Tujuan:** Membuat dua task sederhana yang berjalan bersamaan

**Konsep:** xTaskCreate, vTaskDelay, vTaskDelayUntil

**Langkah:**
1. Buat project PlatformIO dengan framework stm32cube
2. Tambahkan FreeRTOS library
3. Implementasikan dua task: LED blink dan UART print

**Kode:** Lihat file \`praktikum/STM32/STM32_01_Basic_Task/src/main.c\`

**Analisis:**
- Amati output UART, perhatikan tick count
- Task mana yang dieksekusi lebih sering?
- Mengapa LED blink 2x lebih cepat dari UART print?

---

### Program 2: Task Priority - STM32

**Tujuan:** Memahami pengaruh prioritas terhadap scheduling

**Konsep:** Priority-based preemptive scheduling

**Analisis:**
- Amati urutan eksekusi task
- Apa yang terjadi jika high priority task tidak yield?
- Bagaimana low priority task mendapat CPU time?

---

### Program 3: Task Suspend/Resume - STM32

**Tujuan:** Kontrol manual state task

**Konsep:** vTaskSuspend, vTaskResume

**Analisis:**
- Bagaimana state task berubah saat suspend?
- Apa yang terjadi dengan LED saat task suspended?

---

### Program 4: Dynamic Task Creation - STM32

**Tujuan:** Membuat dan menghapus task secara runtime

**Konsep:** xTaskCreate, vTaskDelete, heap management

**Analisis:**
- Amati perubahan free heap saat task dibuat/dihapus
- Apa yang terjadi jika membuat terlalu banyak task?

---

### Program 5: Runtime Statistics - STM32

**Tujuan:** Monitor CPU dan stack usage

**Konsep:** vTaskList, vTaskGetRunTimeStats, uxTaskGetStackHighWaterMark

**Analisis:**
- Task mana yang menggunakan CPU paling banyak?
- Apakah stack size sudah optimal?

---

### Program 6: vTaskDelayUntil Precision - STM32

**Tujuan:** Demonstrasi periodic task dengan timing presisi

**Konsep:** vTaskDelayUntil vs vTaskDelay

---

### Program 7: Task Parameters - STM32

**Tujuan:** Passing data ke task via parameter

**Konsep:** pvParameters, struct passing

---

### Program 8: Rate Monotonic Scheduling - STM32

**Tujuan:** Implementasi RMS scheduling

**Konsep:** Period-to-priority mapping

---

### Program 9: Idle Hook - STM32

**Tujuan:** Power management dengan idle hook

**Konsep:** vApplicationIdleHook, WFI instruction

---

### Program 10: Task Watchdog - STM32

**Tujuan:** Deteksi task yang hang

**Konsep:** Task monitoring, watchdog pattern

---

## 📝 Praktikum ESP32

### Program 1: Basic Dual Core - ESP32

**Tujuan:** Memanfaatkan dual core ESP32

**Konsep:** xTaskCreatePinnedToCore, core affinity

\`\`\`cpp
#include <Arduino.h>

// Pin definitions
#define LED_RED     4
#define LED_YELLOW  5
#define LED_GREEN   18
#define LED_BLUE    19
#define BTN1_PIN    21
#define BTN2_PIN    22

TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;

void Task1_Core0(void *pvParameters) {
    Serial.println("[Task1] Running on Core 0");
    
    for(;;) {
        digitalWrite(LED_RED, HIGH);
        Serial.printf("[Core %d] Task1: LED ON  - Tick: %lu\n", 
                      xPortGetCoreID(), xTaskGetTickCount());
        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        digitalWrite(LED_RED, LOW);
        Serial.printf("[Core %d] Task1: LED OFF - Tick: %lu\n", 
                      xPortGetCoreID(), xTaskGetTickCount());
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void Task2_Core1(void *pvParameters) {
    Serial.println("[Task2] Running on Core 1");
    
    for(;;) {
        digitalWrite(LED_YELLOW, HIGH);
        Serial.printf("[Core %d] Task2: LED ON  - Tick: %lu\n", 
                      xPortGetCoreID(), xTaskGetTickCount());
        vTaskDelay(300 / portTICK_PERIOD_MS);
        
        digitalWrite(LED_YELLOW, LOW);
        Serial.printf("[Core %d] Task2: LED OFF - Tick: %lu\n", 
                      xPortGetCoreID(), xTaskGetTickCount());
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n==========================================");
    Serial.println("ESP32 Program 1: Basic Dual Core Tasks");
    Serial.println("==========================================\n");
    
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    
    // Create task pinned to Core 0
    xTaskCreatePinnedToCore(
        Task1_Core0,      // Task function
        "Task1",          // Task name
        4096,             // Stack size (bytes)
        NULL,             // Parameters
        1,                // Priority
        &Task1Handle,     // Task handle
        0                 // Core 0
    );
    
    // Create task pinned to Core 1
    xTaskCreatePinnedToCore(
        Task2_Core1,
        "Task2",
        4096,
        NULL,
        1,
        &Task2Handle,
        1                 // Core 1
    );
    
    Serial.println("Tasks created on both cores!");
}

void loop() {
    // Loop runs on Core 1 by default
    Serial.printf("[Loop] Free Heap: %d bytes\n", ESP.getFreeHeap());
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}
\`\`\`

---

### Program 2: Task Priority with Core Affinity - ESP32

**Tujuan:** Kombinasi priority dan core affinity

---

### Program 3: Task Suspend/Resume - ESP32

**Tujuan:** Kontrol task dengan button

---

### Program 4: Dynamic Task Management - ESP32

**Tujuan:** Create/delete task dengan WiFi connected

---

### Program 5: Task Statistics - ESP32

**Tujuan:** Monitor task dengan ESP-IDF tools

---

### Program 6: Inter-Core Communication - ESP32

**Tujuan:** Komunikasi task antar core

---

### Program 7: Watchdog Timer Integration - ESP32

**Tujuan:** Task dengan hardware watchdog

---

### Program 8: Task with WiFi - ESP32

**Tujuan:** Multi-task dengan koneksi WiFi

---

### Program 9: Memory Monitoring - ESP32

**Tujuan:** PSRAM dan heap monitoring

---

### Program 10: Production Task Pattern - ESP32

**Tujuan:** Best practices untuk production

---

## 📊 Tabel Perbandingan STM32 vs ESP32

| Aspek | STM32F103 | ESP32 |
|-------|-----------|-------|
| Cores | 1 (72MHz) | 2 (240MHz) |
| RAM | 20KB | 520KB |
| Stack Unit | Words | Bytes |
| Core Affinity | N/A | Supported |
| Default Tick | 1000Hz | 100Hz |
| Heap | heap_4.c | ESP-IDF |

---

## 📝 Tugas Praktikum

### Tugas 1: Implementasi Dasar (20 poin)
1. Jalankan Program 1 pada kedua platform
2. Dokumentasikan output serial
3. Jelaskan perbedaan implementasi

### Tugas 2: Modifikasi Priority (20 poin)
1. Ubah prioritas task dan amati perilaku
2. Buat diagram timing eksekusi
3. Analisis dampak priority inversion

### Tugas 3: Dynamic Task (20 poin)
1. Implementasikan system dengan 3 worker task
2. Tambahkan monitoring task
3. Demonstrasikan memory management

### Tugas 4: Cross-Platform (20 poin)
1. Port program STM32 ke ESP32
2. Manfaatkan fitur dual-core ESP32
3. Bandingkan performance

### Tugas 5: Inovasi (20 poin)
1. Kembangkan aplikasi multi-task original
2. Dokumentasikan dengan flowchart
3. Presentasikan di kelas

---

## 📋 Rubrik Penilaian Praktikum

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| Implementasi | 40% | Program berjalan sesuai spesifikasi |
| Laporan | 25% | Lengkap, analisis mendalam |
| Pemahaman | 20% | Mampu menjawab pertanyaan |
| Keaktifan | 15% | Partisipasi dan kreativitas |

---

## 🔍 Troubleshooting

| Masalah | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| Task tidak jalan | Stack overflow | Tambah stack size |
| System freeze | Tidak ada yield | Tambah vTaskDelay |
| Timing tidak akurat | Gunakan vTaskDelay | Ganti ke vTaskDelayUntil |
| Memory habis | Terlalu banyak task | Kurangi task atau stack |

---

## 📚 Referensi

1. FreeRTOS Official Documentation
2. STM32 FreeRTOS User Guide (UM1722)
3. ESP-IDF FreeRTOS Documentation
4. Mastering the FreeRTOS Real Time Kernel - Richard Barry
