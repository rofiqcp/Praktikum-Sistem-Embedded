# Prompt untuk Pembuatan PPT - Bagian 1
## Modul 01: GPIO Digital I/O - Teori dan Konsep Dasar

> **Instruksi untuk AI/Designer:** Gunakan prompt di bawah ini untuk membuat slide presentasi. Setiap section mewakili 1-3 slide. Total target: 25-30 slide untuk Bagian 1.

---

## Slide 1: Judul

```
Buatkan slide judul dengan:
- Judul: "GPIO dan Digital I/O"
- Subtitle: "Modul 01 - Praktikum Sistem Embedded"
- Visual: Gambar mikrokontroler STM32 dan ESP32 berdampingan
- Warna tema: Biru teknologi dengan aksen hijau
- Logo institusi di pojok
- Nama pengajar dan tanggal
- Catatan: Framework yang digunakan: STM32Cube HAL & ESP-IDF
```

---

## Slide 2: Agenda/Outline

```
Buatkan slide agenda dengan ikon untuk setiap topik:
1. 📚 Pengenalan GPIO
2. 🔧 Arsitektur GPIO pada STM32 dan ESP32
3. ⚡ Mode Operasi GPIO
4. 🔘 Input: Button dan Debouncing
5. 💡 Output: LED Control (Digital On/Off)
6. 🔢 Binary Counter dan Bit Manipulation
7. 📝 Register Access (ODR, BSRR, GPIO_OUT_REG)
8. 🛡️ Best Practices dan Safety
9. 🔬 Demo dan Praktikum
```

---

## Slide 3-4: Apa itu GPIO?

```
Slide 3 - Definisi GPIO:
- Judul: "Apa itu GPIO?"
- Definisi: General Purpose Input/Output - pin serbaguna pada mikrokontroler
- Analogi: "GPIO seperti saklar dan sensor universal pada robot"
- Gambar: Diagram blok sederhana MCU dengan GPIO pins
- Highlight bahwa GPIO adalah building block paling dasar

Slide 4 - Aplikasi GPIO:
- Judul: "Aplikasi GPIO dalam Dunia Nyata"
- Grid 2x3 dengan gambar dan caption:
  * LED indicator (lampu status)
  * Push button (input user)
  * Relay control (on/off beban besar)
  * Sensor digital (PIR, limit switch)
  * DIP switch (konfigurasi)
  * Keypad matrix (input keyboard)
```

---

## Slide 5-6: Perbandingan STM32 vs ESP32

```
Slide 5 - Spesifikasi:
- Judul: "STM32F103C8T6 vs ESP32 - GPIO Comparison"
- Tabel perbandingan side-by-side:
  | Fitur | STM32F103 | ESP32 |
  |-------|-----------|-------|
  | Core | ARM Cortex-M3 | Xtensa LX6 |
  | GPIO Count | 37 pins | 34 pins |
  | Voltage | 3.3V (5V tolerant*) | 3.3V only |
  | Max Current | 25mA/pin | 40mA/pin |
  | Built-in LED | PC13 (Active LOW) | GPIO2 (Active HIGH) |
  | Framework | STM32Cube HAL | ESP-IDF |

Slide 6 - Visualisasi Pinout:
- Judul: "Pinout Diagram"
- Gambar pinout STM32 Blue Pill di kiri
- Gambar pinout ESP32 DevKitC di kanan
- Highlight pin-pin penting dengan warna berbeda
- Catatan: "Perhatikan pin yang tidak boleh digunakan"
```

---

## Slide 7-8: Struktur Internal GPIO

```
Slide 7 - Blok Diagram GPIO:
- Judul: "Struktur Internal GPIO Pin"
- Diagram blok menunjukkan:
  * Input buffer
  * Output driver (P-MOS dan N-MOS)
  * Pull-up/Pull-down resistor internal
  * Protection diodes
- Gunakan animasi untuk menjelaskan aliran sinyal

Slide 8 - Register GPIO STM32:
- Judul: "GPIO Register pada STM32"
- Tabel register dengan fungsi:
  * CRL/CRH - Configuration
  * IDR - Input Data Register (baca input)
  * ODR - Output Data Register (set output)
  * BSRR - Bit Set/Reset Register (atomic operation)
- Contoh kode akses register:
  GPIOC->BSRR = (1 << 13);     // Set PC13
  GPIOC->BSRR = (1 << 29);     // Reset PC13
```

---

## Slide 9-11: Mode GPIO

```
Slide 9 - Input Modes:
- Judul: "GPIO Input Modes"
- Diagram 3 mode input:
  1. Floating (Hi-Z) - Tidak disarankan!
  2. Pull-up - Default HIGH
  3. Pull-down - Default LOW
- Warning box: "Jangan biarkan input floating!"

Slide 10 - Output Modes:
- Judul: "GPIO Output Modes"
- Diagram perbandingan:
  1. Push-Pull: Dapat drive HIGH dan LOW
  2. Open-Drain: Hanya dapat pull LOW
- Use case masing-masing mode
- Gambar transistor untuk visualisasi

Slide 11 - Memilih Mode yang Tepat:
- Judul: "Kapan Menggunakan Mode Apa?"
- Flowchart decision tree:
  * LED → Push-Pull
  * I2C → Open-Drain
  * Button → Input Pull-up
  * Sensor Active-Low → Input Pull-up
```

---

## Slide 12-14: LED Control (Digital)

```
Slide 12 - Dasar LED:
- Judul: "Mengendalikan LED (Digital On/Off)"
- Diagram rangkaian LED dengan resistor
- Formula: R = (Vcc - Vf) / If
- Contoh perhitungan untuk LED merah
- Catatan: Modul ini fokus pada kontrol digital (on/off), bukan analog/PWM

Slide 13 - Sourcing vs Sinking:
- Judul: "Current Sourcing vs Sinking"
- Diagram side-by-side:
  * Sourcing: GPIO → Resistor → LED → GND
  * Sinking: VCC → Resistor → LED → GPIO
- Kapan menggunakan masing-masing metode

Slide 14 - Active HIGH vs Active LOW:
- Judul: "Memahami Active HIGH dan Active LOW"
- Tabel truth table untuk kedua kasus
- Contoh: STM32 PC13 adalah Active LOW
- STM32 HAL: HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET) = LED ON
- ESP-IDF: gpio_set_level(GPIO_NUM_2, 1) = LED ON
```

---

## Slide 15-17: Push Button dan Debouncing

```
Slide 15 - Masalah Bouncing:
- Judul: "Fenomena Bouncing pada Button"
- Grafik oscilloscope showing bounce
- Penjelasan mengapa terjadi (kontak mekanis)
- Durasi typical: 5-50ms

Slide 16 - Solusi Hardware:
- Judul: "Hardware Debouncing"
- Rangkaian RC filter
- Perhitungan time constant: τ = R × C
- Kelebihan dan kekurangan

Slide 17 - Solusi Software:
- Judul: "Software Debouncing"
- ESP-IDF style pseudocode:
  int reading = gpio_get_level(BUTTON_PIN);
  if (reading != last_state)
      debounce_tick = xTaskGetTickCount();
  if ((xTaskGetTickCount() - debounce_tick) > pdMS_TO_TICKS(50))
      button_state = reading;
- STM32 HAL style:
  if (HAL_GetTick() - last_debounce > DEBOUNCE_MS)
      button_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
- Kelebihan: Fleksibel, tidak butuh komponen tambahan
```

---

## Slide 18-19: Binary Counter dan Bit Manipulation

```
Slide 18 - Konsep Binary Counter:
- Judul: "Binary Counter dengan 4 LED"
- Diagram 4 LED merepresentasikan 4-bit:
  * 0000 = 0  → semua LED mati
  * 0001 = 1  → LED0 nyala
  * 1010 = 10 → LED1 dan LED3 nyala
  * 1111 = 15 → semua LED nyala
- Tabel truth table 0-15

Slide 19 - Bit Manipulation:
- Judul: "Operasi Bit untuk GPIO"
- Operasi yang sering digunakan:
  * Set bit:    value |= (1 << n)
  * Clear bit:  value &= ~(1 << n)
  * Toggle bit: value ^= (1 << n)
  * Test bit:   (value >> n) & 0x01
- Contoh implementasi:
  for (int bit = 0; bit < 4; bit++) {
      gpio_set_level(led[bit], (count >> bit) & 0x01);
  }
```

---

## Slide 20-21: GPIO Register Access

```
Slide 20 - STM32 Register Access:
- Judul: "Akses Register Langsung pada STM32"
- Register penting:
  * ODR - baca/tulis semua output sekaligus
  * IDR - baca semua input sekaligus
  * BSRR - set/reset individual bit (atomic!)
- Perbandingan:
  HAL_GPIO_WritePin() → lebih mudah, overhead fungsi
  GPIOA->BSRR = ...   → lebih cepat, atomic, direct

Slide 21 - ESP32 Register Access:
- Judul: "Akses Register GPIO pada ESP32"
- Register GPIO ESP32:
  * GPIO_OUT_REG - output register
  * GPIO_OUT_W1TS_REG - write 1 to set
  * GPIO_OUT_W1TC_REG - write 1 to clear
  * GPIO_IN_REG - input register
- Contoh:
  #include "soc/gpio_reg.h"
  REG_WRITE(GPIO_OUT_W1TS_REG, (1 << pin));
- Mengapa register access penting: kecepatan, atomicity
```

---

## Slide 22-23: GPIO pada ESP32 dan STM32 - Fitur Khusus

```
Slide 22 - ESP32 Special Features:
- Judul: "Fitur Unik GPIO ESP32"
- List dengan ikon:
  * 🖐️ Touch Sensing (10 pin)
  * 💤 RTC GPIO (deep sleep wake-up)
  * 🔌 GPIO Matrix (flexible routing)
  * ⚡ Configurable drive strength (4 level)
- Pin Restrictions:
  * GPIO6-11: Flash SPI - JANGAN GUNAKAN
  * GPIO34-39: INPUT ONLY
  * GPIO0: Boot mode
  * GPIO2: LED + boot indicator

Slide 23 - STM32 GPIO Features:
- Judul: "Fitur GPIO STM32F103"
- List:
  * 5V tolerant pada banyak pin
  * Alternate function mapping
  * Lock register untuk keamanan
  * Atomic bit operations (BSRR)
  * Configurable GPIO speed (2/10/50 MHz)
```

---

## Slide 24-25: Best Practices dan Safety

```
Slide 24 - Do's and Don'ts:
- Judul: "Best Practices GPIO"
- Two columns:
  ✅ DO:
  - Gunakan resistor untuk LED
  - Enable pull-up untuk button
  - Debounce semua input mekanis
  - Gunakan HAL/ESP-IDF API untuk portabilitas

  ❌ DON'T:
  - Hubungkan 5V ke ESP32
  - Biarkan input floating
  - Exceed max current per pin
  - Gunakan GPIO6-11 pada ESP32

Slide 25 - Safety dan Coding Standards:
- Judul: "Safety dan Standar Kode"
- Safety: ESD protection, current limiting, voltage matching
- Coding standards:
  * Pin definitions di config.h
  * Meaningful variable names
  * Non-blocking code (tick-based timing)
  * Proper error checking (ESP_ERROR_CHECK)
```

---

## Slide 26-27: Rangkuman

```
Slide 26 - Key Takeaways:
- Judul: "Rangkuman Materi"
- 6 poin kunci dengan checkmark:
  ☑ GPIO adalah interface fundamental embedded
  ☑ STM32 (HAL) dan ESP32 (ESP-IDF) memiliki karakteristik berbeda
  ☑ Selalu gunakan pull resistor untuk input
  ☑ Debouncing wajib untuk button mekanis
  ☑ Binary counter mendemonstrasikan bit manipulation
  ☑ Register access memberikan kontrol langsung dan cepat

Slide 27 - Preview Praktikum:
- Judul: "12 Program yang Akan Dipelajari"
- Grid preview program:
  * 01 LED Blink           * 07 GPIO Drive Strength
  * 02 Multi LED Running    * 08 DIP Switch Reader
  * 03 LED Binary Counter   * 09 GPIO Port Register
  * 04 Button Debounce      * 10 GPIO Matrix Keypad
  * 05 Long/Short Press     * 11 Emergency Stop
  * 06 Toggle Latch         * 12 LED Test Pattern
- "Mari kita praktikkan!"
```

---

## Slide 28: Q&A

```
Buatkan slide Q&A:
- Judul besar: "Pertanyaan?"
- Gambar ilustrasi mahasiswa bertanya
- Kontak info dosen/asisten
- QR code ke material online (opsional)
```

---

## Catatan untuk Presenter

```
Sertakan speaker notes untuk setiap slide:
- Slide 7: "Gunakan animasi untuk menjelaskan aliran sinyal step-by-step"
- Slide 15: "Tunjukkan video oscilloscope jika tersedia"
- Slide 17: "Tunjukkan perbedaan kode ESP-IDF vs STM32 HAL untuk debouncing"
- Slide 18: "Demo langsung binary counting dengan 4 LED di breadboard"
- Slide 20-21: "Jelaskan mengapa BSRR lebih baik dari ODR untuk concurrent access"
- Setiap slide teknis: "Pause untuk pertanyaan"
```

---

## Design Guidelines

```
Panduan desain untuk keseluruhan PPT:
- Font: Sans-serif (Calibri, Arial, atau Roboto)
- Ukuran minimum: 24pt untuk body text
- Warna primary: #1E3A8A (dark blue)
- Warna secondary: #10B981 (green)
- Warna accent: #F59E0B (amber untuk warning)
- Maksimal 6 bullet points per slide
- Setiap slide teknis wajib ada diagram/gambar
- Gunakan ikon dari Flaticon atau similar
- Animasi: Subtle, entrance dari kiri
- Semua kode contoh menggunakan ESP-IDF atau STM32Cube HAL (BUKAN Arduino)
```
