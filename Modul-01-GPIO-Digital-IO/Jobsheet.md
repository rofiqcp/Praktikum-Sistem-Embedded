# JOBSHEET BAB 01: GPIO dan Digital I/O

## 📋 Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | GPIO dan Digital I/O |
| **Platform** | STM32F103C8T6 (Blue Pill), ESP32 DevKitC |
| **Framework** | STM32Cube HAL (STM32), ESP-IDF (ESP32) |
| **Jumlah Program STM32** | 12 program |
| **Jumlah Program ESP32** | 12 program |
| **Durasi** | 3 x 50 menit |
| **Tools** | PlatformIO, VS Code, Serial Monitor |

---

## 🎯 Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa mampu:

1. Memahami konsep dasar GPIO (General Purpose Input/Output)
2. Mengkonfigurasi pin sebagai input dan output pada STM32 dan ESP32 menggunakan native framework
3. Mengimplementasikan teknik debouncing untuk push button
4. Membuat berbagai pola LED digital menggunakan GPIO
5. Memahami akses register GPIO dan drive strength
6. Menerapkan best practices dalam pemrograman embedded systems

---

## 🔧 Peralatan yang Diperlukan

### Hardware
| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 Blue Pill | 1 | Development board |
| 2 | ESP32 DevKitC | 1 | Development board |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | LED 5mm (berbagai warna) | 8 | Merah, Hijau, Kuning |
| 5 | Resistor 220Ω | 8 | Untuk LED |
| 6 | Push Button | 4 | Tactile switch |
| 7 | DIP Switch 4-bit | 1 | Untuk input multiple |
| 8 | Keypad Matrix 4x4 | 1 | Untuk matrix scanning |
| 9 | Breadboard | 1 | Full-size |
| 10 | Kabel Jumper | 20 | Male-Male |
| 11 | Kabel USB | 2 | Micro USB / USB-C |

### Software
- Visual Studio Code dengan PlatformIO Extension
- Serial Monitor (PlatformIO atau Putty)
- Git (opsional, untuk version control)

---

## 📐 Skema Rangkaian

### Rangkaian LED (untuk kedua platform)

```
                    LED OUTPUT CIRCUIT

    GPIO Pin ────┬────[220Ω]────[LED]────┐
                 │                       │
                 │      Anode   Cathode  │
                 │        (+)     (-)    │
                 │                       │
                 └───────────────────────┴──── GND

    Catatan:
    - Resistor 220Ω membatasi arus ke ~10mA
    - LED memerlukan ~2V (merah) sampai ~3V (biru/putih)
```

### Rangkaian Button dengan Pull-up Internal

```
                    BUTTON INPUT CIRCUIT

    3.3V ────[Internal Pull-up ~40kΩ]────┬──── GPIO Pin
                                         │
                                    ┌────┴────┐
                                    │  BUTTON │
                                    └────┬────┘
                                         │
                                        GND

    Status:
    - Button TIDAK ditekan: GPIO = HIGH (3.3V)
    - Button DITEKAN: GPIO = LOW (0V)
```

### Pin Assignment STM32F103C8T6

| Fungsi | Pin | Keterangan |
|--------|-----|------------|
| LED Built-in | PC13 | Active LOW |
| LED External 1 | PA0 | Active HIGH |
| LED External 2 | PA1 | Active HIGH |
| LED External 3 | PA2 | Active HIGH |
| LED External 4 | PA3 | Active HIGH |
| LED 5-8 (Binary Counter) | PA4-PA7 | Active HIGH |
| Button 1 | PB0 | Pull-up internal |
| Button 2 | PB1 | Pull-up internal |
| DIP Switch | PB12-PB15 | Pull-up internal |
| Serial TX | PA9 | USART1 |
| Serial RX | PA10 | USART1 |

### Pin Assignment ESP32 DevKitC

| Fungsi | Pin | Keterangan |
|--------|-----|------------|
| LED Built-in | GPIO2 | Active HIGH |
| LED External 1 | GPIO4 | Active HIGH |
| LED External 2 | GPIO5 | Active HIGH |
| LED External 3 | GPIO18 | Active HIGH |
| LED External 4 | GPIO19 | Active HIGH |
| LED 5-8 (Binary Counter) | GPIO23, GPIO25, GPIO26, GPIO27 | Active HIGH |
| Button 1 | GPIO21 | Pull-up internal |
| Button 2 | GPIO22 | Pull-up internal |
| DIP Switch | GPIO32, GPIO33, GPIO34, GPIO35 | Pull-up (ext jika input-only) |
| Serial TX | GPIO1 | UART0 |
| Serial RX | GPIO3 | UART0 |

---

## 📝 Daftar Program Praktikum

### STM32F103C8T6 Programs (STM32Cube HAL)

| No | Nama Program | Deskripsi | Konsep yang Dipelajari |
|----|--------------|-----------|------------------------|
| 1 | STM32_01_LED_Blink | LED berkedip dasar | GPIO output, HAL_Delay |
| 2 | STM32_02_Multi_LED_Running | Running LED pattern | Array, sequencing |
| 3 | STM32_03_LED_Binary_Counter | Counter biner 4-bit LED | Bit manipulation, binary |
| 4 | STM32_04_Button_Debounce | Debouncing state machine | State machine, HAL_GetTick |
| 5 | STM32_05_Long_Short_Press | Deteksi durasi tekan | Timing, press detection |
| 6 | STM32_06_Toggle_Latch | Toggle on/off | Latch logic |
| 7 | STM32_07_GPIO_Drive_Strength | Konfigurasi drive strength | GPIO speed config |
| 8 | STM32_08_DIP_Switch_Reader | Baca DIP switch | Multiple input |
| 9 | STM32_09_GPIO_Port_Register | Akses register langsung | ODR, IDR, BSRR register |
| 10 | STM32_10_GPIO_Matrix_Keypad | Scanning keypad matrix | Matrix scanning |
| 11 | STM32_11_Emergency_Stop | Safety interlock | Interrupt, safety systems |
| 12 | STM32_12_LED_Test_Pattern | Pola diagnostik LED | Testing patterns |

### ESP32 Programs (ESP-IDF)

| No | Nama Program | Deskripsi | Konsep yang Dipelajari |
|----|--------------|-----------|------------------------|
| 1 | ESP32_01_LED_Blink | LED berkedip dasar | gpio_set_level, vTaskDelay |
| 2 | ESP32_02_Multi_LED_Running | Running LED pattern | Array, sequencing |
| 3 | ESP32_03_LED_Binary_Counter | Counter biner 4-bit LED | Bit manipulation, binary |
| 4 | ESP32_04_Button_Debounce | Debouncing state machine | State machine, esp_timer |
| 5 | ESP32_05_Long_Short_Press | Deteksi durasi tekan | Timing, press detection |
| 6 | ESP32_06_Toggle_Latch | Toggle on/off | Latch logic |
| 7 | ESP32_07_GPIO_Drive_Strength | Konfigurasi drive strength | gpio_set_drive_capability |
| 8 | ESP32_08_DIP_Switch_Reader | Baca DIP switch | Multiple input |
| 9 | ESP32_09_GPIO_Port_Register | Akses register GPIO | GPIO_OUT_REG, GPIO_IN_REG |
| 10 | ESP32_10_GPIO_Matrix_Keypad | Scanning keypad matrix | Matrix scanning |
| 11 | ESP32_11_Emergency_Stop | Safety interlock | gpio_isr, safety systems |
| 12 | ESP32_12_LED_Test_Pattern | Pola diagnostik LED | Testing patterns |

---

## 📋 Langkah Praktikum

### Persiapan Awal (15 menit)

1. **Clone/Download Repository**
   ```bash
   cd ~/Documents
   git clone [repository-url] praktikum-embedded
   ```

2. **Buka Project di VS Code**
   ```bash
   code praktikum-embedded
   ```

3. **Install PlatformIO Extension** (jika belum)
   - Buka Extensions (Ctrl+Shift+X)
   - Cari "PlatformIO IDE"
   - Install dan restart VS Code

4. **Rangkai Hardware**
   - Pasang LED dan resistor sesuai skema
   - Hubungkan button ke pin yang ditentukan
   - Hubungkan ST-Link ke STM32 (SWDIO, SWCLK, GND, 3.3V)
   - Hubungkan ESP32 via USB

### Praktikum 1: LED Blink (20 menit)

**Tujuan:** Memahami dasar GPIO output dan timing

**Langkah:**

1. Buka folder `praktikum/ESP32/ESP32_01_LED_Blink`

2. Perhatikan struktur project:
   ```
   ├── src/
   │   └── main.c         # Program utama (ESP-IDF)
   ├── include/
   │   └── config.h       # Konfigurasi pin
   └── platformio.ini     # Konfigurasi PlatformIO
   ```

3. Pelajari kode `main.c` (ESP-IDF):
   ```c
   #include "driver/gpio.h"
   #include "freertos/FreeRTOS.h"
   #include "freertos/task.h"

   #define LED_PIN GPIO_NUM_2

   void app_main(void) {
       gpio_reset_pin(LED_PIN);
       gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

       while (1) {
           gpio_set_level(LED_PIN, 1);
           vTaskDelay(pdMS_TO_TICKS(500));
           gpio_set_level(LED_PIN, 0);
           vTaskDelay(pdMS_TO_TICKS(500));
       }
   }
   ```

4. Build dan Upload:
   - Klik tombol Build (✓) di status bar
   - Klik tombol Upload (→) untuk upload ke board
   - Buka Serial Monitor (baud rate: 115200)

5. **Modifikasi yang harus dilakukan:**
   - Ubah interval blink menjadi 250ms
   - Tambahkan counter untuk menghitung jumlah blink via `ESP_LOGI()`
   - Dokumentasikan hasilnya

**Untuk STM32 (STM32Cube HAL):**
- Buka folder `praktikum/STM32/STM32_01_LED_Blink`
- Contoh kode:
  ```c
  #include "stm32f1xx_hal.h"

  // PC13 adalah Active LOW pada Blue Pill
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  // LED ON
  HAL_Delay(500);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);    // LED OFF
  HAL_Delay(500);
  ```
- Build dan upload menggunakan ST-Link

### Praktikum 2: Multi LED Running Pattern (20 menit)

**Tujuan:** Memahami array dan sequencing dalam embedded

**Langkah:**

1. Buka program `ESP32_02_Multi_LED_Running` atau `STM32_02_Multi_LED_Running`

2. **Pelajari Konsep (ESP-IDF):**
   ```c
   const gpio_num_t led_pins[] = {GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_18, GPIO_NUM_19};
   const int num_leds = sizeof(led_pins) / sizeof(led_pins[0]);

   for (int i = 0; i < num_leds; i++) {
       gpio_set_level(led_pins[i], 1);
       vTaskDelay(pdMS_TO_TICKS(100));
       gpio_set_level(led_pins[i], 0);
   }
   ```

3. **Modifikasi:**
   - Buat pola ping-pong (maju-mundur)
   - Buat pola blink semua LED bersamaan
   - Buat pola random

4. **Dokumentasikan** setiap pola dalam laporan

### Praktikum 3: LED Binary Counter (20 menit)

**Tujuan:** Memahami representasi biner dan bit manipulation

**Langkah:**

1. Buka program `ESP32_03_LED_Binary_Counter` atau `STM32_03_LED_Binary_Counter`

2. **Pelajari Konsep:**
   ```c
   // 4 LED merepresentasikan nilai biner 0-15
   for (uint8_t count = 0; count <= 15; count++) {
       for (int bit = 0; bit < 4; bit++) {
           gpio_set_level(led_pins[bit], (count >> bit) & 0x01);
       }
       vTaskDelay(pdMS_TO_TICKS(500));
   }
   ```

3. **Analisis:**
   - Amati pola LED saat menghitung 0-15
   - Hubungkan dengan konsep bilangan biner
   - Catat pola perubahan setiap bit (LSB vs MSB)

### Praktikum 4: Button Debouncing (25 menit)

**Tujuan:** Memahami masalah bouncing dan solusinya

**Langkah:**

1. Buka program `ESP32_04_Button_Debounce` atau `STM32_04_Button_Debounce`

2. **Eksperimen Tanpa Debouncing:**
   - Modifikasi kode untuk menghilangkan debouncing
   - Tekan button dan perhatikan output Serial
   - Catat berapa kali button terdeteksi ditekan

3. **Eksperimen Dengan Debouncing (ESP-IDF):**
   ```c
   #define DEBOUNCE_MS 50
   static uint32_t last_debounce = 0;

   int reading = gpio_get_level(BUTTON_PIN);
   if (reading != last_state) {
       last_debounce = xTaskGetTickCount();
   }
   if ((xTaskGetTickCount() - last_debounce) > pdMS_TO_TICKS(DEBOUNCE_MS)) {
       button_state = reading;
   }
   ```

4. **Analisis:**
   - Jelaskan mengapa terjadi bouncing
   - Jelaskan bagaimana state machine mengatasi bouncing
   - Gambarkan diagram state machine

### Praktikum 5: Long Press vs Short Press (20 menit)

**Tujuan:** Deteksi durasi penekanan tombol

**Langkah:**

1. Buka program `ESP32_05_Long_Short_Press` atau `STM32_05_Long_Short_Press`

2. **Pelajari implementasi timing:**
   - Short press: < 500ms
   - Long press: > 1500ms
   - Aksi berbeda untuk setiap durasi

3. **Modifikasi:**
   - Ubah threshold durasi
   - Tambahkan aksi untuk double press

### Praktikum 6: Toggle Latch (15 menit)

**Tujuan:** Memahami behavior toggle/latch

**Langkah:**

1. Buka program `ESP32_06_Toggle_Latch` atau `STM32_06_Toggle_Latch`
2. Tekan button untuk toggle LED on/off
3. Perhatikan bahwa state LED di-latch (bertahan)
4. **Analisis** perbedaan toggle vs momentary

### Praktikum 7: GPIO Drive Strength (15 menit)

**Tujuan:** Memahami konfigurasi drive strength GPIO

**Langkah:**

1. Buka program `ESP32_07_GPIO_Drive_Strength` atau `STM32_07_GPIO_Drive_Strength`

2. **ESP32 - Konfigurasi drive strength:**
   ```c
   gpio_set_drive_capability(LED_PIN, GPIO_DRIVE_CAP_0);   // ~5mA
   gpio_set_drive_capability(LED_PIN, GPIO_DRIVE_CAP_1);   // ~10mA
   gpio_set_drive_capability(LED_PIN, GPIO_DRIVE_CAP_2);   // ~20mA (default)
   gpio_set_drive_capability(LED_PIN, GPIO_DRIVE_CAP_3);   // ~40mA
   ```

3. **STM32 - Konfigurasi GPIO speed:**
   ```c
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;    // 2 MHz
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;  // 10 MHz
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    // 50 MHz
   ```

4. **Analisis** perbedaan brightness LED pada setiap level

### Praktikum 8: DIP Switch Reader (15 menit)

**Tujuan:** Membaca multiple input secara bersamaan

**Langkah:**

1. Buka program `ESP32_08_DIP_Switch_Reader` atau `STM32_08_DIP_Switch_Reader`
2. Baca kombinasi 4-bit DIP switch
3. Tampilkan nilai desimal dan biner ke Serial Monitor
4. **Implementasikan** aksi berbeda untuk setiap kombinasi

### Praktikum 9: GPIO Port Register (20 menit)

**Tujuan:** Memahami akses register GPIO secara langsung

**Langkah:**

1. Buka program `ESP32_09_GPIO_Port_Register` atau `STM32_09_GPIO_Port_Register`

2. **STM32 - Akses Register:**
   ```c
   // Menggunakan ODR (Output Data Register)
   GPIOA->ODR |= (1 << 0);       // Set PA0 HIGH
   GPIOA->ODR &= ~(1 << 0);      // Set PA0 LOW

   // Menggunakan BSRR (Bit Set/Reset Register) - atomic
   GPIOA->BSRR = (1 << 0);       // Set PA0 HIGH
   GPIOA->BSRR = (1 << 16);      // Set PA0 LOW

   // Membaca IDR (Input Data Register)
   uint32_t input = GPIOB->IDR & (1 << 0);
   ```

3. **ESP32 - Akses Register:**
   ```c
   #include "soc/gpio_reg.h"
   REG_WRITE(GPIO_OUT_W1TS_REG, (1 << gpio_num));  // Set HIGH
   REG_WRITE(GPIO_OUT_W1TC_REG, (1 << gpio_num));  // Set LOW
   uint32_t input = REG_READ(GPIO_IN_REG);          // Read input
   ```

4. **Analisis** keuntungan akses register vs fungsi API (kecepatan, atomicity)

### Praktikum 10: GPIO Matrix Keypad (25 menit)

**Tujuan:** Memahami teknik scanning matrix

**Langkah:**

1. Buka program `ESP32_10_GPIO_Matrix_Keypad` atau `STM32_10_GPIO_Matrix_Keypad`

2. **Hubungkan keypad 4x4** ke GPIO pins

3. **Pelajari scanning algorithm:**
   ```c
   for (int row = 0; row < 4; row++) {
       gpio_set_level(row_pins[row], 0);
       for (int col = 0; col < 4; col++) {
           if (gpio_get_level(col_pins[col]) == 0) {
               key = keymap[row][col];
           }
       }
       gpio_set_level(row_pins[row], 1);
   }
   ```

4. **Tampilkan** tombol yang ditekan ke Serial Monitor

### Praktikum 11: Emergency Stop (20 menit)

**Tujuan:** Mengimplementasikan safety interlock

**Langkah:**

1. Buka program `ESP32_11_Emergency_Stop` atau `STM32_11_Emergency_Stop`

2. **ESP-IDF Interrupt:**
   ```c
   gpio_install_isr_service(0);
   gpio_isr_handler_add(ESTOP_PIN, estop_isr_handler, NULL);

   static void IRAM_ATTR estop_isr_handler(void* arg) {
       emergency_flag = true;
   }
   ```

3. **STM32 HAL Interrupt:**
   ```c
   HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
   HAL_NVIC_EnableIRQ(EXTI0_IRQn);

   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
       if (GPIO_Pin == GPIO_PIN_0) {
           emergency_flag = 1;
       }
   }
   ```

4. **Test:** Tekan E-Stop dan semua LED mati instantly
5. **Analisis** pentingnya interrupt-based safety

### Praktikum 12: LED Test Pattern (15 menit)

**Tujuan:** Membuat pola diagnostik untuk verifikasi hardware

**Langkah:**

1. Buka program `ESP32_12_LED_Test_Pattern` atau `STM32_12_LED_Test_Pattern`
2. Jalankan dan amati pola-pola:
   - All ON → All OFF
   - Sequential scan
   - Binary count
   - Alternating pattern
3. **Gunakan** untuk verifikasi semua LED dan koneksi bekerja

---

## 📊 Tugas Praktikum

### Tugas 1: Dokumentasi Program (Individu)
**Deadline:** Akhir sesi praktikum

Untuk setiap program yang dijalankan, dokumentasikan:
- Screenshot Serial Monitor output
- Foto rangkaian hardware
- Penjelasan cara kerja program (dalam komentar kode)

### Tugas 2: Modifikasi dan Analisis (Individu)
**Deadline:** H+3 setelah praktikum

Pilih 3 program dan lakukan modifikasi:
1. Ubah parameter (timing, pin, pattern)
2. Tambahkan fitur baru
3. Dokumentasikan perbedaan perilaku

### Tugas 3: Mini Project GPIO (Kelompok 2-3 orang)
**Deadline:** H+7 setelah praktikum

Buat salah satu aplikasi berikut (hanya menggunakan GPIO digital, tanpa PWM):
- **Traffic Light Controller:** 3 LED dengan timing realistic
- **Binary Counter Display:** 4 LED menampilkan hitungan 0-15 dengan kontrol button
- **Simon Says Game:** LED pattern memory game
- **Morse Code Transmitter:** Input teks via serial, output LED morse code

**Deliverables:**
- Kode program (ESP-IDF atau STM32Cube HAL) yang dapat dicompile
- Video demonstrasi (max 3 menit)
- Laporan singkat (max 2 halaman)

---

## 📊 Rubrik Penilaian Praktikum

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| **Kehadiran & Partisipasi** | 10% | Hadir tepat waktu, aktif bertanya |
| **Implementasi Program** | 30% | Semua 12 program berhasil dijalankan |
| **Modifikasi & Eksperimen** | 25% | Modifikasi kreatif dan analisis benar |
| **Dokumentasi** | 20% | Lengkap, rapi, screenshot jelas |
| **Mini Project** | 15% | Fungsional, kreatif, presentasi |

### Kriteria Penilaian Detail

**A (85-100):** Semua tugas selesai dengan modifikasi kreatif, dokumentasi lengkap
**B (70-84):** Semua tugas selesai, dokumentasi baik
**C (55-69):** Sebagian besar tugas selesai, dokumentasi cukup
**D (40-54):** Beberapa tugas selesai, dokumentasi minimal
**E (<40):** Tidak menyelesaikan tugas minimum

---

## ❓ Pertanyaan Refleksi

Jawab pertanyaan berikut dalam laporan:

1. Apa perbedaan utama GPIO STM32 dan ESP32 yang Anda temukan?
2. Mengapa debouncing penting dalam membaca input button?
3. Apa kelebihan menggunakan `HAL_GetTick()` / `xTaskGetTickCount()` dibanding `HAL_Delay()` / `vTaskDelay()`?
4. Bagaimana cara menghitung nilai resistor untuk LED?
5. Apa yang terjadi jika GPIO input dibiarkan floating?
6. Apa keuntungan akses register langsung (BSRR/ODR) dibanding fungsi HAL?

---

## 📚 Referensi Tambahan

1. [STM32 GPIO Tutorial - DeepBlue Embedded](https://deepbluembedded.com/stm32-gpio-tutorial/)
2. [ESP-IDF GPIO API Reference - Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
3. [Button Debouncing - Ganssle](http://www.ganssle.com/debouncing.htm)
4. Mastering STM32 - Chapter 6: GPIO Management
5. Kolban's Book on ESP32 - Halaman 251-257: GPIO

---

## 🆘 Troubleshooting

| Masalah | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| Upload gagal (STM32) | ST-Link tidak terdeteksi | Periksa koneksi SWDIO, SWCLK |
| Upload gagal (ESP32) | Port COM salah | Pilih port yang benar di PlatformIO |
| LED tidak menyala | Polaritas terbalik | Balik arah LED |
| Button tidak responsif | Pull-up tidak aktif | Konfigurasi `GPIO_PULLUP` di init |
| Serial tidak muncul | Baud rate salah | Set ke 115200 |
| Keypad tidak merespons | Row/column terbalik | Periksa wiring dan konfigurasi |

---

*Jobsheet Praktikum Sistem Embedded - Modul 01*
*Versi 2.0 - Februari 2026*
