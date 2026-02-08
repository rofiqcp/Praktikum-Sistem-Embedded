# Prompt untuk Pembuatan PPT - Bagian 2
## Modul 01: GPIO Digital I/O - Praktikum dan Implementasi

> **Instruksi untuk AI/Designer:** Gunakan prompt di bawah ini untuk membuat slide presentasi. Bagian 2 fokus pada implementasi praktikum. Total target: 25-30 slide. Semua kode menggunakan ESP-IDF (ESP32) dan STM32Cube HAL (STM32), BUKAN Arduino.

---

## Slide 1: Judul Bagian 2

```
Buatkan slide judul dengan:
- Judul: "GPIO Digital I/O - Praktikum"
- Subtitle: "Implementasi pada STM32 (HAL) dan ESP32 (ESP-IDF)"
- Visual: Foto breadboard dengan LED dan button
- Badge: "Hands-On Session"
```

---

## Slide 2: Tujuan Praktikum

```
Buatkan slide tujuan dengan format checklist:
- Judul: "Apa yang Akan Kita Capai"
□ Mengkonfigurasi GPIO output untuk LED (ESP-IDF & HAL)
□ Mengkonfigurasi GPIO input untuk button
□ Mengimplementasikan software debouncing
□ Membuat binary counter dan berbagai pola LED digital
□ Memahami akses register GPIO langsung
□ Menggunakan Serial Monitor untuk debugging
□ Mengimplementasikan matrix keypad scanning
□ Menerapkan emergency stop dengan interrupt
```

---

## Slide 3-4: Tools dan Setup

```
Slide 3 - Hardware Setup:
- Judul: "Peralatan yang Diperlukan"
- Foto annotated:
  * STM32 Blue Pill + ST-Link
  * ESP32 DevKitC + USB cable
  * Breadboard
  * LED 8 buah (berbagai warna)
  * Resistor 220Ω
  * Push button (4)
  * DIP switch 4-bit
  * Keypad matrix 4x4
  * Jumper wires

Slide 4 - Software Setup:
- Judul: "Persiapan Software"
- Steps dengan screenshot:
  1. Install VS Code
  2. Install PlatformIO Extension
  3. Clone repository praktikum
  4. Verify board detection
- Framework: ESP-IDF untuk ESP32, STM32Cube HAL untuk STM32
```

---

## Slide 5-6: Skema Rangkaian

```
Slide 5 - Rangkaian LED:
- Judul: "Wiring Diagram - LED"
- Diagram Fritzing style untuk STM32:
  * PA0-PA3 → Resistor 220Ω → LED → GND (4 LED utama)
  * PA4-PA7 → Resistor 220Ω → LED → GND (binary counter)
  * PC13 (built-in LED, active LOW)
- Diagram untuk ESP32:
  * GPIO4, 5, 18, 19 → Resistor 220Ω → LED → GND
  * GPIO23, 25, 26, 27 → LED → GND (binary counter)
- Color coding untuk kabel

Slide 6 - Rangkaian Button:
- Judul: "Wiring Diagram - Button & Input"
- Diagram dengan internal pull-up enabled:
  * Button antara GPIO dan GND
  * Tidak perlu resistor eksternal
- DIP Switch wiring diagram
- Catatan: "Pull-up internal ~40kΩ"
```

---

## Slide 7-9: Program 1 - LED Blink

```
Slide 7 - Konsep LED Blink:
- Judul: "Program 01: LED Blink"
- Flowchart sederhana:
  START → Init GPIO → Toggle LED → Delay → Loop
- Foto expected result: LED berkedip

Slide 8 - Kode ESP32 (ESP-IDF):
- Judul: "Kode: LED Blink ESP32 (ESP-IDF)"
- Syntax highlighted code:
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

Slide 9 - Kode STM32 (HAL):
- Judul: "Kode: LED Blink STM32 (HAL)"
- Highlight perbedaan dengan ESP32:
  * PC13 adalah Active LOW
  * HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET) = LED ON
- Tabel perbandingan API:
  | ESP-IDF | STM32 HAL |
  | gpio_set_level() | HAL_GPIO_WritePin() |
  | vTaskDelay() | HAL_Delay() |
  | gpio_set_direction() | HAL_GPIO_Init() |
```

---

## Slide 10-12: Program 02 - Running LED & Program 03 - Binary Counter

```
Slide 10 - Running LED Pattern:
- Judul: "Program 02: Multi LED Running Pattern"
- Animasi GIF atau sequence diagram:
  LED1 → LED2 → LED3 → LED4 → repeat
- Kode ESP-IDF:
  const gpio_num_t leds[] = {GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_18, GPIO_NUM_19};
  for (int i = 0; i < 4; i++) {
      gpio_set_level(leds[i], 1);
      vTaskDelay(pdMS_TO_TICKS(100));
      gpio_set_level(leds[i], 0);
  }

Slide 11 - Binary Counter:
- Judul: "Program 03: LED Binary Counter"
- Diagram 4 LED showing binary values 0-15
- Kode:
  for (uint8_t count = 0; count <= 15; count++) {
      for (int bit = 0; bit < 4; bit++) {
          gpio_set_level(led_pins[bit], (count >> bit) & 0x01);
      }
      vTaskDelay(pdMS_TO_TICKS(500));
  }
- Truth table: 0000 → 0001 → 0010 → ... → 1111

Slide 12 - Variasi Pattern:
- Judul: "Variasi Pola LED Digital"
- Grid 2x2 dengan pattern:
  * Forward: → → → →
  * Reverse: ← ← ← ←
  * Ping-pong: → ← → ←
  * Binary count: 0000 → 1111
```

---

## Slide 13-15: Program 04 - Button Debounce

```
Slide 13 - Demo Bouncing:
- Judul: "Program 04: Mengapa Perlu Debouncing?"
- Screenshot Serial output TANPA debouncing:
  Button pressed!
  Button pressed!
  Button pressed!  ← Multiple triggers!
- Grafik bouncing

Slide 14 - State Machine Debouncing (ESP-IDF):
- Judul: "Implementasi Debounce (ESP-IDF)"
- Diagram state:
  IDLE → PRESSED_PENDING → PRESSED → RELEASED_PENDING → IDLE
- Kode snippet:
  int reading = gpio_get_level(BUTTON_PIN);
  if (reading != last_state) {
      last_debounce = xTaskGetTickCount();
  }
  if ((xTaskGetTickCount() - last_debounce) > pdMS_TO_TICKS(DEBOUNCE_MS)) {
      button_state = reading;
  }

Slide 15 - Debounce STM32 HAL:
- Judul: "Implementasi Debounce (STM32 HAL)"
- Kode:
  GPIO_PinState reading = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
  if (HAL_GetTick() - last_debounce > DEBOUNCE_MS) {
      if (reading != button_state) {
          button_state = reading;
      }
  }
- Perbandingan before/after
```

---

## Slide 16-18: Program 05 - Long/Short Press & Program 06 - Toggle Latch

```
Slide 16 - Long Press vs Short Press:
- Judul: "Program 05: Deteksi Long Press vs Short Press"
- Timeline diagram:
  |----SHORT----|
  |----------LONG----------|
  0ms        500ms       1500ms

Slide 17 - Implementasi Press Duration:
- Judul: "Kode Deteksi Press Duration"
- Logic flowchart (ESP-IDF):
  if (gpio_get_level(BUTTON_PIN) == 0) {  // pressed
      press_start = xTaskGetTickCount();
  }
  // on release:
  duration = xTaskGetTickCount() - press_start;
  if (duration < pdMS_TO_TICKS(500)) short_press_action();
  else long_press_action();
- Aplikasi: Short press = next, Long press = power off

Slide 18 - Toggle Latch:
- Judul: "Program 06: Toggle Latch Behavior"
- Diagram: Setiap tekan button → state LED berubah
- Konsep: Satu variabel boolean menyimpan state
  static bool led_state = false;
  if (button_pressed) {
      led_state = !led_state;
      gpio_set_level(LED_PIN, led_state);
  }
- Perbedaan toggle vs momentary
```

---

## Slide 19-21: Program 07 - Drive Strength & Program 08 - DIP Switch

```
Slide 19 - GPIO Drive Strength:
- Judul: "Program 07: GPIO Drive Strength"
- ESP32 drive strength levels:
  gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_0);  // ~5mA
  gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_1);  // ~10mA
  gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_2);  // ~20mA (default)
  gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_3);  // ~40mA
- STM32 GPIO speed:
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;       // 2 MHz
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;      // 50 MHz

Slide 20 - DIP Switch Reader:
- Judul: "Program 08: DIP Switch Reader"
- Diagram DIP switch 4-bit
- Kode membaca kombinasi:
  uint8_t dip_value = 0;
  for (int i = 0; i < 4; i++) {
      dip_value |= (gpio_get_level(dip_pins[i]) << i);
  }
  ESP_LOGI(TAG, "DIP Value: %d (0b%04b)", dip_value, dip_value);

Slide 21 - Aplikasi DIP Switch:
- Judul: "Menggunakan DIP Switch untuk Konfigurasi"
- Contoh: Memilih mode operasi berdasarkan kombinasi DIP
  * 0000 → Mode 0: Normal
  * 0001 → Mode 1: Debug
  * 0010 → Mode 2: Test
  * dll.
```

---

## Slide 22-24: Program 09 - GPIO Port Register & Program 10 - Matrix Keypad

```
Slide 22 - GPIO Port Register Access:
- Judul: "Program 09: Akses Register GPIO"
- STM32:
  GPIOA->BSRR = (1 << 0);       // Set PA0 HIGH (atomic)
  GPIOA->BSRR = (1 << 16);      // Set PA0 LOW (atomic)
  uint32_t input = GPIOB->IDR;   // Read all Port B
- ESP32:
  REG_WRITE(GPIO_OUT_W1TS_REG, (1 << pin));  // Set HIGH
  REG_WRITE(GPIO_OUT_W1TC_REG, (1 << pin));  // Set LOW
  uint32_t input = REG_READ(GPIO_IN_REG);

Slide 23 - Keypad Matrix Konsep:
- Judul: "Program 10: Scanning Keypad Matrix"
- Diagram 4x4 keypad matrix
- Penjelasan row-column scanning
- Mengapa menghemat pin GPIO (8 pin untuk 16 tombol)

Slide 24 - Keypad Implementasi:
- Judul: "Kode Scanning Matrix (ESP-IDF)"
- Pseudocode:
  for (int row = 0; row < 4; row++) {
      gpio_set_level(row_pins[row], 0);
      for (int col = 0; col < 4; col++) {
          if (gpio_get_level(col_pins[col]) == 0) {
              key = keymap[row][col];
          }
      }
      gpio_set_level(row_pins[row], 1);
  }
- Tips: Debouncing dan ghost key prevention
```

---

## Slide 25-27: Program 11 - Emergency Stop & Program 12 - LED Test Pattern

```
Slide 25 - Emergency Stop:
- Judul: "Program 11: Emergency Stop (Safety Interlock)"
- Diagram: E-Stop → GPIO Interrupt → Disable All Outputs
- Pentingnya safety dalam industri

Slide 26 - Implementasi E-Stop:
- Judul: "Kode Safety Interlock"
- ESP-IDF interrupt:
  gpio_install_isr_service(0);
  gpio_isr_handler_add(ESTOP_PIN, estop_isr, NULL);

  void IRAM_ATTR estop_isr(void* arg) {
      emergency_flag = true;
  }
- STM32 HAL interrupt:
  void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
      emergency_flag = 1;
  }
- Warning: "Software safety ≠ hardware safety!"

Slide 27 - LED Test Pattern:
- Judul: "Program 12: LED Test Pattern"
- Pola diagnostik:
  * All ON → verifikasi semua LED bekerja
  * Sequential → verifikasi urutan wiring
  * Binary count → verifikasi bit mapping
  * Alternating → verifikasi tidak ada short
- Digunakan untuk hardware verification sebelum project
```

---

## Slide 28-29: Hands-On dan Troubleshooting

```
Slide 28 - Hands-On Time:
- Judul: "Giliran Anda! 🔧"
- Instruksi:
  1. Rangkai LED dan button
  2. Upload program 01 (LED Blink) - verifikasi
  3. Upload program 03 (Binary Counter) - amati pola
  4. Upload program 04 (Button Debounce) - eksperimen
  5. Coba program lainnya sesuai urutan
  6. Modifikasi parameter dan dokumentasikan

Slide 29 - Troubleshooting:
- Judul: "Jika Ada Masalah..."
- Tabel troubleshooting:
  | Gejala | Penyebab | Solusi |
  | LED tidak nyala | Polaritas terbalik | Balik LED |
  | Upload gagal | Port salah | Pilih port yang benar |
  | Button tidak responsif | Pull-up tidak aktif | Konfigurasi GPIO_PULLUP |
  | Serial kosong | Baud rate salah | Set 115200 |
  | Keypad error | Row/col terbalik | Cek wiring |
```

---

## Slide 30: Tugas dan Mini Project

```
Buatkan slide tugas:
- Judul: "Tugas Praktikum"
- Tugas 1: Dokumentasi 12 program (screenshot, foto)
- Tugas 2: Modifikasi 3 program pilihan
- Tugas 3: Mini Project GPIO Digital (pilih salah satu):
  * Traffic Light Controller
  * Binary Counter Display dengan kontrol button
  * Simon Says Game
  * Morse Code Transmitter
- Catatan: Semua kode harus menggunakan ESP-IDF atau STM32Cube HAL
- Deliverables: Kode + Video + Laporan
```

---

## Slide 31: Kesimpulan

```
Buatkan slide kesimpulan:
- Judul: "Apa yang Sudah Kita Pelajari"
- Summary dengan ikon:
  ✅ GPIO output untuk mengendalikan LED (digital on/off)
  ✅ GPIO input dengan debouncing (state machine)
  ✅ Binary counter dan bit manipulation
  ✅ Register access langsung (BSRR, ODR, GPIO_OUT_REG)
  ✅ Matrix keypad scanning
  ✅ Emergency stop dengan interrupt
  ✅ Perbedaan STM32 HAL vs ESP-IDF
```

---

## Slide 32: Preview & Q&A

```
Buatkan slide penutup:
- Preview Modul 02: Interrupt dan Timer
- Teaser: "Bagaimana membuat program yang responsive tanpa polling?"
- Informasi kontak dosen/asisten
- Link repository GitHub
- QR code untuk feedback form
```

---

## Catatan Tambahan untuk Presenter

```
Tips presentasi:
1. Slide 7-9: Lakukan live coding bersama mahasiswa
2. Slide 11: Tunjukkan binary counting secara live dengan LED
3. Slide 13-15: Tunjukkan perbedaan with/without debounce secara langsung
4. Slide 22: Jelaskan kenapa register lebih cepat dari API
5. Slide 25-26: Tekankan pentingnya safety

Durasi estimasi:
- Review teori: 15 menit
- Praktikum guided (Prog 1-6): 60 menit
- Praktikum mandiri (Prog 7-12): 45 menit
- Q&A & troubleshooting: 15 menit
- Total: 135 menit (2.25 jam)

PENTING: Semua contoh kode HARUS menggunakan ESP-IDF atau STM32Cube HAL.
JANGAN gunakan Arduino framework (pinMode, digitalWrite, dll).
```
