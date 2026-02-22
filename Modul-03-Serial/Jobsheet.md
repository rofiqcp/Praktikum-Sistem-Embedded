# JOBSHEET BAB 03: Serial UART Communication

## 📋 Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | Serial UART Communication |
| **Platform** | STM32F103C8T6 (Blue Pill), ESP32 DevKitC |
| **Framework** | ESP-IDF (ESP32), STM32Cube HAL (STM32) |
| **Jumlah Program STM32** | 12 |
| **Jumlah Program ESP32** | 12 |
| **Durasi** | 3 x 50 menit |

---

## 🎯 Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa mampu:

1. Memahami struktur frame UART dan parameter konfigurasi
2. Mengkonfigurasi UART pada STM32 dan ESP32
3. Mengirim dan menerima data melalui Serial Monitor
4. Membangun komunikasi MCU-to-MCU via UART
5. Mengimplementasikan protokol komunikasi sederhana
6. Menerapkan teknik buffer dan error handling

---

## 🔧 Alat dan Bahan

### Hardware

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | ARM Cortex-M3, 72MHz |
| 2 | ESP32 DevKitC | 1 | Dual-core, 240MHz |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | USB Cable Micro | 2 | Power & programming |
| 5 | USB-TTL Converter | 1 | CH340/CP2102/FT232 |
| 6 | LED 5mm | 3 | Status indicator |
| 7 | Resistor 330Ω | 3 | Current limiting LED |
| 8 | Push Button | 2 | User input |
| 9 | Breadboard | 1 | 830 tie-points |
| 10 | Kabel Jumper | 20 | Male-Male & Male-Female |
| 11 | Logic Analyzer | 1 | Opsional, untuk debugging |

### Software

| No | Software | Versi | Keterangan |
|----|----------|-------|------------|
| 1 | VS Code | Latest | IDE utama |
| 2 | PlatformIO | Latest | Build system |
| 3 | STM32 Platform | ststm32 | Platform STM32 |
| 4 | ESP32 Platform | espressif32 | Platform ESP32 |
| 5 | Serial Monitor | Built-in | VS Code terminal |
| 6 | HTerm / RealTerm | Latest | Advanced serial terminal |
| 7 | Saleae Logic | Latest | Opsional, logic analysis |

---

## 📐 Konfigurasi Pin

### STM32F103C8T6 UART Pins

```
┌─────────────────────────────────────────────────────────────┐
│                  STM32F103C8T6 UART Pinout                   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    USART1:                    USART2:                       │
│    PA9  ─── TX1               PA2  ─── TX2                  │
│    PA10 ─── RX1               PA3  ─── RX2                  │
│                                                             │
│    USART3:                                                  │
│    PB10 ─── TX3               LED Status:                   │
│    PB11 ─── RX3               PC13 ─── Built-in LED         │
│                               PB3  ─── Status LED 1         │
│                               PB4  ─── Status LED 2         │
│                                                             │
│    Button Input:                                            │
│    PA0  ─── Button 1                                        │
│    PA1  ─── Button 2                                        │
│                                                             │
│    ⚠️ USART1 shares pins with ST-Link (PA9/PA10)            │
│       Use USART2/USART3 when ST-Link is connected           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### ESP32 DevKitC UART Pins

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 UART Pinout                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    UART0 (USB Serial):        UART2 (User):                 │
│    GPIO1  ─── TX0             GPIO17 ─── TX2                │
│    GPIO3  ─── RX0             GPIO16 ─── RX2                │
│                                                             │
│    UART1 (User):              LED Status:                   │
│    GPIO10 ─── TX1*            GPIO2  ─── Built-in LED       │
│    GPIO9  ─── RX1*            GPIO4  ─── Status LED 1       │
│                               GPIO5  ─── Status LED 2       │
│    * UART1 default pins are used by flash                   │
│      Remap to other GPIO if using UART1                     │
│                                                             │
│    Button Input:                                            │
│    GPIO0  ─── BOOT Button                                   │
│    GPIO13 ─── User Button                                   │
│                                                             │
│    ⚠️ UART0 digunakan untuk USB Serial Monitor              │
│       Gunakan UART2 untuk komunikasi dengan device lain     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔌 Skema Rangkaian

### Rangkaian 1: STM32 ke PC via USB-TTL

```
    STM32F103C8T6              USB-TTL Converter           PC
   ┌─────────────┐            ┌─────────────┐        ┌─────────┐
   │             │            │             │        │         │
   │  PA2 (TX2) ─┼────────────┼─► RX        │        │ Serial  │
   │  PA3 (RX2) ◄┼────────────┼── TX        │◄══USB══│ Monitor │
   │  GND ───────┼────────────┼── GND       │        │         │
   │             │            │             │        │         │
   └─────────────┘            └─────────────┘        └─────────┘
   
   Konfigurasi:
   • Baud Rate: 115200
   • Data Bits: 8
   • Parity: None
   • Stop Bits: 1
```

### Rangkaian 2: ESP32 ke PC (Built-in USB)

```
    ESP32 DevKitC                                        PC
   ┌─────────────┐                                  ┌─────────┐
   │             │                                  │         │
   │  GPIO1 (TX) │◄══════════════════════════USB═══│ Serial  │
   │  GPIO3 (RX) │                                  │ Monitor │
   │             │                                  │         │
   └─────────────┘                                  └─────────┘
   
   Konfigurasi:
   • Baud Rate: 115200
   • USB CDC built-in
   • No external converter needed
```

### Rangkaian 3: STM32 ↔ ESP32 Communication

```
    STM32F103C8T6                               ESP32 DevKitC
   ┌─────────────┐                             ┌─────────────┐
   │             │                             │             │
   │  PA2 (TX2) ─┼─────────────────────────────┼─► GPIO16(RX)│
   │  PA3 (RX2) ◄┼─────────────────────────────┼── GPIO17(TX)│
   │  GND ───────┼─────────────────────────────┼── GND       │
   │             │                             │             │
   │  PB3 (LED) ─┼──[330Ω]──►|── GND          │  GPIO4(LED)─┼──[330Ω]──►|── GND
   │             │                             │             │
   └─────────────┘                             └─────────────┘
   
   ⚠️ PENTING:
   1. TX ke RX (crossed connection)
   2. GND HARUS terhubung (common ground)
   3. Kedua device menggunakan 3.3V logic (compatible)
   4. Baud rate HARUS sama di kedua sisi
```

---

## 📝 Daftar Program Praktikum

### Program ESP32

| No | Nama Program | Topik | Tingkat |
|----|--------------|-------|----------|
| 1 | ESP32_01_UART_Echo | UART Echo (Polling) | Dasar |
| 2 | ESP32_02_UART_Interrupt_RX | UART Interrupt Receive | Dasar |
| 3 | ESP32_03_UART_Ring_Buffer | Ring Buffer Management | Menengah |
| 4 | ESP32_04_UART_Printf_Redirect | Printf Redirect via UART | Menengah |
| 5 | ESP32_05_UART_Command_Parser | Serial Command Parser | Menengah |
| 6 | ESP32_06_UART_JSON_Protocol | JSON Protocol Communication | Menengah |
| 7 | ESP32_07_UART_Line_Editor | Interactive Line Editor | Lanjut |
| 8 | ESP32_08_UART_Framing_STX_ETX | STX/ETX Frame Protocol | Lanjut |
| 9 | ESP32_09_UART_CRC_Checksum | CRC Checksum Validation | Menengah |
| 10 | ESP32_10_UART_Timeout_Parser | Timeout-based Parser | Lanjut |
| 11 | ESP32_11_UART_Bridge_Multi | Multi-UART Bridge | Lanjut |
| 12 | ESP32_12_UART_Error_Statistics | Error Statistics Monitor | Lanjut |

### Program STM32

| No | Nama Program | Topik | Tingkat |
|----|--------------|-------|----------|
| 1 | STM32_01_UART_Echo | USART Echo (Polling) | Dasar |
| 2 | STM32_02_UART_Interrupt_RX | USART Interrupt Receive | Dasar |
| 3 | STM32_03_UART_Ring_Buffer | Ring Buffer Management | Menengah |
| 4 | STM32_04_UART_Printf_Redirect | Printf Redirect via USART | Menengah |
| 5 | STM32_05_UART_Command_Parser | USART Command Parser | Menengah |
| 6 | STM32_06_UART_JSON_Protocol | JSON Protocol Communication | Menengah |
| 7 | STM32_07_UART_Line_Editor | Interactive Line Editor | Lanjut |
| 8 | STM32_08_UART_Framing_STX_ETX | STX/ETX Frame Protocol | Lanjut |
| 9 | STM32_09_UART_CRC_Checksum | CRC Checksum Validation | Menengah |
| 10 | STM32_10_UART_Timeout_Parser | Timeout-based Parser | Lanjut |
| 11 | STM32_11_UART_Bridge_Multi | Multi-USART Bridge | Lanjut |
| 12 | STM32_12_UART_Error_Statistics | Error Statistics Monitor | Lanjut |

---

## 📚 Tugas Praktikum

### Tugas 1: Serial Communication Basic (30 menit)

**Tujuan:** Memahami konfigurasi UART dan komunikasi basic

**Langkah Kerja ESP32:**

1. **Setup Hardware (5 menit)**
   - Hubungkan ESP32 ke PC via USB
   - Buka VS Code dengan PlatformIO
   - Create new project atau buka Modul-01

2. **Serial Print Test (10 menit)**
   ```c
   #include <stdio.h>
   #include "driver/uart.h"
   #include "esp_log.h"
   #include "esp_timer.h"

   #define UART_PORT UART_NUM_0

   void app_main(void) {
       // UART0 sudah di-init untuk console
       printf("ESP32 UART Test Started!\n");

       while (1) {
           printf("Hello from ESP32!\n");
           printf("Uptime: %lld ms\n", esp_timer_get_time() / 1000);
           vTaskDelay(pdMS_TO_TICKS(1000));
       }
   }
   ```
   - Upload program
   - Buka Serial Monitor (115200 baud)
   - Verifikasi output

3. **Serial Read Test (15 menit)**
   ```c
   #include <stdio.h>
   #include <string.h>
   #include "driver/uart.h"

   #define UART_PORT UART_NUM_0
   #define BUF_SIZE  256

   void app_main(void) {
       uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
       printf("Type something:\n");

       uint8_t data[BUF_SIZE];
       while (1) {
           int len = uart_read_bytes(UART_PORT, data, BUF_SIZE - 1,
                                     100 / portTICK_PERIOD_MS);
           if (len > 0) {
               data[len] = '\0';
               printf("You typed: %s\n", (char *)data);
           }
       }
   }
   ```
   - Ketik text di Serial Monitor
   - Verifikasi echo response

**Langkah Kerja STM32:**

1. **Setup Hardware (5 menit)**
   - Hubungkan USB-TTL ke PA2(TX), PA3(RX), GND
   - Connect USB-TTL ke PC
   - Hubungkan ST-Link untuk programming

2. **Serial Print Test (10 menit)**
   - Buka program STM32_01_UART_Echo
   - Upload via ST-Link
   - Buka serial terminal (HTerm/RealTerm)
   - Pilih COM port USB-TTL, 115200 baud
   - Verifikasi output

**Pertanyaan Analisis:**
1. Apa yang terjadi jika baud rate tidak cocok?
2. Mengapa ESP32 menggunakan `uart_read_bytes()` sedangkan STM32 menggunakan `HAL_UART_Receive()`?
3. Jelaskan perbedaan mode polling dan interrupt pada UART!

---

### Tugas 2: Command Parser (30 menit)

**Tujuan:** Implementasi command parser via serial

**Langkah Kerja:**

1. **Buka program ESP32_05_UART_Command_Parser (ESP32) atau STM32_05_UART_Command_Parser**

2. **Implementasi Command Parser:**
   ```c
   // Commands: LED ON, LED OFF, STATUS, HELP
   #include <stdio.h>
   #include <string.h>
   #include "driver/gpio.h"
   #include "driver/uart.h"
   #include "esp_timer.h"

   #define LED_PIN   GPIO_NUM_2
   #define UART_PORT UART_NUM_0

   void processCommand(char *cmd) {
       // Trim whitespace
       while (*cmd == ' ' || *cmd == '\r' || *cmd == '\n') cmd++;
       char *end = cmd + strlen(cmd) - 1;
       while (end > cmd && (*end == ' ' || *end == '\r' || *end == '\n')) *end-- = '\0';

       // Case-insensitive compare
       if (strcasecmp(cmd, "LED ON") == 0) {
           gpio_set_level(LED_PIN, 1);
           printf("OK: LED is ON\n");
       }
       else if (strcasecmp(cmd, "LED OFF") == 0) {
           gpio_set_level(LED_PIN, 0);
           printf("OK: LED is OFF\n");
       }
       else if (strcasecmp(cmd, "STATUS") == 0) {
           printf("LED: %s\n", gpio_get_level(LED_PIN) ? "ON" : "OFF");
           printf("Uptime: %lld ms\n", esp_timer_get_time() / 1000);
       }
       else if (strcasecmp(cmd, "HELP") == 0) {
           printf("Commands: LED ON, LED OFF, STATUS, HELP\n");
       }
       else {
           printf("ERROR: Unknown command\n");
       }
   }
   ```

3. **Test Commands:**
   - Kirim "LED ON" → LED menyala
   - Kirim "LED OFF" → LED mati
   - Kirim "STATUS" → Tampilkan status
   - Kirim "HELP" → Tampilkan daftar command

**Tugas Modifikasi:**
Tambahkan command baru:
- `BLINK <delay>` → LED blink dengan delay tertentu
- `ADC <pin>` → Baca nilai ADC dari pin tertentu
- `INFO` → Tampilkan info chip (ESP32: chip model, revision)

---

### Tugas 3: MCU-to-MCU Communication (40 menit)

**Tujuan:** Komunikasi serial antara STM32 dan ESP32

**Langkah Kerja:**

1. **Persiapan Hardware (10 menit)**
   ```
   Wiring:
   STM32 PA2 (TX) ──────────► ESP32 GPIO16 (RX)
   STM32 PA3 (RX) ◄────────── ESP32 GPIO17 (TX)
   STM32 GND ───────────────── ESP32 GND
   ```

2. **Program STM32 sebagai Sender (15 menit)**
   - Buka STM32_10_UART_Timeout_Parser
   - Program mengirim data sensor simulasi
   ```c
   // STM32 HAL - mengirim data sensor via USART2
   extern UART_HandleTypeDef huart2;

   void sendSensorData(void) {
       float temp = 25.0f + (float)(rand() % 100 - 50) / 10.0f;
       float hum  = 60.0f + (float)(rand() % 200 - 100) / 10.0f;

       char buffer[64];
       int len = snprintf(buffer, sizeof(buffer), "$DATA,%.1f,%.1f", temp, hum);

       // Calculate XOR checksum
       uint8_t checksum = 0;
       for (int i = 1; i < len; i++) {
           checksum ^= buffer[i];
       }

       char frame[80];
       int frameLen = snprintf(frame, sizeof(frame), "%s*%02X\n", buffer, checksum);
       HAL_UART_Transmit(&huart2, (uint8_t *)frame, frameLen, HAL_MAX_DELAY);
       HAL_Delay(1000);
   }
   ```

3. **Program ESP32 sebagai Receiver (15 menit)**
   - Buka ESP32_10_UART_Timeout_Parser
   - Program menerima dan mem-parse data
   ```c
   // ESP-IDF - menerima data dari STM32 via UART2
   #include "driver/uart.h"

   #define UART_PORT UART_NUM_2
   #define BUF_SIZE  256

   void receiver_task(void *pvParam) {
       uint8_t data[BUF_SIZE];
       while (1) {
           int len = uart_read_bytes(UART_PORT, data, BUF_SIZE - 1,
                                     200 / portTICK_PERIOD_MS);
           if (len > 0) {
               data[len] = '\0';
               if (validateMessage((char *)data)) {
                   parseData((char *)data);
                   displayData();
               } else {
                   printf("Invalid message!\n");
               }
           }
       }
   }
   ```

4. **Testing dan Verifikasi**
   - Upload kedua program
   - Monitor output di ESP32 Serial Monitor
   - Verifikasi data yang diterima

**Dokumentasi:**
| No | Data Dikirim | Data Diterima | Checksum Valid |
|----|--------------|---------------|----------------|
| 1 | | | |
| 2 | | | |
| 3 | | | |

---

### Tugas 4: Protocol Implementation (30 menit)

**Tujuan:** Implementasi protokol komunikasi dengan ACK/NACK

**Spesifikasi Protokol:**
```
Request:  $CMD,<command>,<params>*<checksum>\r\n
Response: $ACK,<cmd_id>*<checksum>\r\n  (sukses)
          $NAK,<cmd_id>,<error_code>*<checksum>\r\n (gagal)

Commands:
- LED,ON,<pin>   → Nyalakan LED
- LED,OFF,<pin>  → Matikan LED
- READ,ADC,<pin> → Baca ADC
- READ,GPIO,<pin>→ Baca GPIO
```

**Implementasi:**
1. STM32 mengirim command
2. ESP32 menerima, validasi, execute
3. ESP32 mengirim ACK/NAK
4. STM32 tunggu response (timeout 1 detik)

**Error Codes:**
- 01: Unknown command
- 02: Invalid parameters
- 03: Checksum error
- 04: Execution failed

---

## 📊 Rubrik Penilaian Praktikum

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| **Implementasi** | 40% | Semua tugas berjalan dengan benar |
| | | - Excellent (36-40): Semua + modifikasi |
| | | - Good (28-35): Semua tugas dasar |
| | | - Fair (20-27): Sebagian tugas |
| | | - Poor (<20): Sedikit berhasil |
| **Analisis** | 30% | Pemahaman konsep dan debugging |
| | | - Excellent (27-30): Analysis mendalam |
| | | - Good (21-26): Analysis memadai |
| | | - Fair (15-20): Analysis minimal |
| | | - Poor (<15): Tidak ada analysis |
| **Laporan** | 20% | Dokumentasi lengkap |
| | | - Excellent (18-20): Lengkap + insights |
| | | - Good (14-17): Lengkap |
| | | - Fair (10-13): Cukup lengkap |
| | | - Poor (<10): Tidak lengkap |
| **Keaktifan** | 10% | Partisipasi dan inisiatif |

---

## 📝 Format Laporan Praktikum

### Struktur Laporan

1. **Cover** (1 halaman)
   - Judul: Praktikum 03 - Serial UART Communication
   - Nama, NIM, Tanggal

2. **Tujuan** (0.5 halaman)
   - List tujuan praktikum

3. **Dasar Teori** (1-2 halaman)
   - UART frame structure
   - Baud rate calculation
   - STM32 vs ESP32 UART

4. **Metodologi** (1 halaman)
   - Alat dan bahan
   - Diagram wiring
   - Prosedur kerja

5. **Hasil dan Analisis** (3-4 halaman)
   - Screenshot setiap tugas
   - Tabel pengukuran
   - Analisis perbandingan
   - Jawaban pertanyaan

6. **Kesimpulan** (0.5 halaman)
   - Ringkasan pembelajaran
   - Challenges dan solutions

7. **Lampiran**
   - Source code modifikasi
   - Screenshot tambahan

---

## ⚠️ Troubleshooting

### Masalah Umum UART

| Masalah | Penyebab | Solusi |
|---------|----------|--------|
| Tidak ada output | TX/RX terbalik | Swap wires |
| Karakter aneh (garbage) | Baud rate mismatch | Samakan baud rate |
| Partial data | Buffer overflow | Increase buffer size |
| No response | GND not connected | Connect GND |
| ESP32 crash saat Serial2 | Wrong pins | Use correct UART2 pins |
| STM32 not printing | Wrong USART selected | Check USART number |

### Debug Tips

1. **Loopback Test:**
   - Connect TX to RX on same device
   - Send data, should receive back
   - Verifies hardware works

2. **Logic Analyzer:**
   - Capture actual signal
   - Verify timing
   - Check data bits

3. **LED Indicator:**
   - Toggle LED on TX/RX
   - Visual feedback of activity

---

## 📚 Referensi Tambahan

1. **STM32F103 Reference Manual** - Chapter 27: USART
2. **ESP32 Technical Reference** - Chapter 12: UART Controller
3. **RS-232 Standard** - EIA/TIA-232-F
4. [ESP-IDF UART API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html)
5. [STM32 HAL UART Documentation](https://wiki.st.com/stm32mcu/wiki/Getting_started_with_UART)

