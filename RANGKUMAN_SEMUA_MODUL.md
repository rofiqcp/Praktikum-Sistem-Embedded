# 📘 RANGKUMAN LENGKAP PRAKTIKUM SISTEM EMBEDDED
## STM32F103C8T6 (Blue Pill) & ESP32 DevKitC

> **14 Modul | Semua Materi, Program & Fungsi**

---

# ═══════════════════════════════════════════════════════════
# MODUL 01: GPIO dan Digital I/O
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **General Purpose Input/Output (GPIO)** — pin pada mikrokontroler yang bisa dikonfigurasi sebagai **input** atau **output** digital. GPIO adalah interface paling dasar dan fundamental dalam sistem embedded untuk berinteraksi dengan dunia luar (LED, tombol, relay, sensor digital).

## 📖 Materi Utama
- Arsitektur dan struktur internal GPIO (Push-Pull vs Open-Drain)
- Konfigurasi pin sebagai Input (Floating, Pull-Up, Pull-Down) dan Output (Push-Pull, Open-Drain)
- Teknik **Debouncing** (hardware RC filter & software state machine) untuk push button
- Current Sourcing vs Current Sinking untuk LED
- Perhitungan resistor LED: `R = (Vcc - Vf) / If`
- Drive Strength configuration
- Best practices: non-blocking code dengan `millis()`, pin definition, wrapper functions

## 🔧 Perbandingan STM32 vs ESP32

| Fitur | STM32F103C8T6 | ESP32 |
|-------|---------------|-------|
| Arsitektur | ARM Cortex-M3 | Xtensa LX6 Dual Core |
| Tegangan I/O | 3.3V (beberapa pin 5V tolerant) | 3.3V (TIDAK 5V tolerant) |
| Jumlah GPIO | 37 pin | 34 pin |
| Drive Current | Max 25mA/pin | Max 40mA (20mA recommended) |
| Built-in LED | PC13 (Active LOW) | GPIO2 (Active HIGH) |
| Mode GPIO | 8 mode per pin | Input/Output/Disable |
| Fitur Khusus | Register BSRR/BRR atomic | Touch Sensor, RTC GPIO, GPIO Matrix |

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **LED Blink Dasar GPIO Output** | Dasar GPIO output — mengedipkan LED built-in |
| 02 | **Multi LED Running Pattern** | Pattern sequencing — LED bergantian menyala (running LED) |
| 03 | **LED Breathing Effect (LEDC PWM)** | Efek LED breathing menggunakan LEDC PWM peripheral |
| 04 | **Button Debounce State Machine** | Debouncing tombol dengan state machine — menghilangkan bouncing |
| 05 | **Long Press vs Short Press Detection** | Mendeteksi durasi tekan tombol (short press / long press) |
| 06 | **Toggle LED Latch Behavior** | Tombol toggle ON/OFF — latch behavior |
| 07 | **GPIO Drive Strength Configuration** | Mengatur kuat arus drive GPIO (5mA / 10mA / 20mA / 40mA) |
| 08 | **DIP Switch Reader** | Membaca multiple input DIP switch sekaligus |
| 09 | **LED Brightness Control via Serial** | Kontrol kecerahan LED lewat Serial Monitor |
| 10 | **GPIO Matrix** | Demonstrasi GPIO Matrix — bit manipulation |
| 11 | **Emergency Stop Logic** | Safety interlock — sistem berhenti darurat |
| 12 | **LED Test Pattern** | Pola diagnostik untuk testing semua LED/GPIO |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **LED Blink** | Dasar GPIO output — blink LED PC13 dengan register |
| 02 | **Multi-LED Running Pattern** | Pattern sequencing — LED berjalan bergantian |
| 03 | **LED Breathing Effect** | Software PWM breathing effect |
| 04 | **Button Debounce State Machine** | Debouncing tombol dengan state machine |
| 05 | **Long/Short Press** | Deteksi durasi tekan tombol |
| 06 | **Toggle Latch Behavior** | Tombol toggle ON/OFF |
| 07 | **GPIO Open-Drain Mode** | Konfigurasi mode Open-Drain (untuk I2C, level shifting) |
| 08 | **DIP Switch Reader** | Membaca multiple DIP switch input |
| 09 | **LED Brightness Control** | Kontrol kecerahan LED via Serial |
| 10 | **Bit Manipulation dengan BSRR Register** | Akses register BSRR/BRR untuk atomic GPIO control |
| 11 | **Emergency Stop Logic** | Safety interlock |
| 12 | **LED Test Pattern** | Pola diagnostik GPIO |

---

# ═══════════════════════════════════════════════════════════
# MODUL 02: Interrupt dan Timer
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **Interrupt** (mekanisme hardware agar CPU merespons event tanpa polling terus-menerus) dan **Timer/Counter** (peripheral untuk mengukur waktu, membangkitkan PWM, dan event periodik). Interrupt membuat sistem lebih **efisien, responsif, dan hemat daya** dibanding polling.

## 📖 Materi Utama
- **Polling vs Interrupt**: Interrupt jauh lebih efisien (CPU bebas saat menunggu event)
- **NVIC** (STM32): 16 level prioritas, nested interrupt, 12 clock cycle latency
- **EXTI** (STM32): 16 jalur external interrupt (Rising/Falling/Both edge)
- **Interrupt Matrix** (ESP32): 71 sumber interrupt, bisa diarahkan ke Core 0 atau Core 1
- **IRAM_ATTR** (ESP32): WAJIB agar ISR disimpan di Internal RAM (akses cepat, tanpa crash)
- **Timer STM32**: 4 timer 16-bit, rumus: `Period = (PSC+1) × (ARR+1) / APB_Clock`
- **Timer ESP32**: 4 timer 64-bit, prescaler 80 → 1µs per tick
- **volatile** keyword: WAJIB untuk variabel yang diakses ISR dan main loop
- **Critical Section**: `__disable_irq()` (STM32) / `portENTER_CRITICAL()` (ESP32)

## 🔧 Perbandingan

| Aspek | STM32F103 | ESP32 |
|-------|-----------|-------|
| Interrupt Controller | NVIC | Interrupt Matrix |
| Priority Levels | 16 (4-bit) | 7 levels |
| External Interrupt | 16 EXTI lines | Semua GPIO |
| Timer Count | 4 × 16-bit | 4 × 64-bit |
| ISR Attribute | Tidak perlu | IRAM_ATTR WAJIB |
| Critical Section | `__disable_irq()` | `portENTER_CRITICAL()` |

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **GPIO Interrupt (Basic)** | Interrupt dasar pada GPIO — deteksi tombol ditekan |
| 02 | **GPIO Interrupt (Advanced)** | Interrupt GPIO dengan debouncing di ISR |
| 03 | **Hardware Timer Interrupt** | Timer hardware periodik — toggle LED setiap interval |
| 04 | **One-Shot Timer** | Timer sekali jalan — delayed action |
| 05 | **LEDC PWM 50%** | PWM output 50% duty cycle menggunakan LEDC |
| 06 | **LEDC PWM Ramp** | PWM dengan duty cycle naik bertahap (ramp up) |
| 07 | **LEDC PWM Fade** | Hardware fade — LED fade in/out otomatis |
| 08 | **Pulse Counter using Interrupt** | Menghitung pulsa input menggunakan interrupt |
| 09 | **RMT Pulse Width Capture** | Mengukur lebar pulsa dengan RMT peripheral |
| 10 | **ESP Timer (millis)** | Timer berbasis `esp_timer` — pengganti millis() |
| 11 | **Multiple Timers** | Beberapa timer berjalan bersamaan |
| 12 | **ISR IRAM Optimization** | Optimasi ISR di IRAM untuk performa maksimal |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **EXTI Interrupt** | External interrupt dasar — deteksi tombol |
| 02 | **EXTI Debounce** | EXTI interrupt dengan debouncing software |
| 03 | **TIM2 Periodic** | Timer 2 periodik — interrupt setiap interval tetap |
| 04 | **One-Shot Timer** | Timer sekali jalan |
| 05 | **PWM 50%** | PWM output 50% duty cycle via TIM |
| 06 | **PWM Ramp** | PWM duty cycle naik bertahap |
| 07 | **PWM Breathing** | Efek LED breathing dengan PWM |
| 08 | **Output Compare Toggle** | Timer Output Compare — toggle pin otomatis |
| 09 | **Input Capture** | Mengukur frekuensi/periode sinyal input |
| 10 | **Encoder Simulation** | Simulasi pembacaan rotary encoder |
| 11 | **Multiple Timers** | Beberapa timer berjalan simultan |
| 12 | **NVIC Priority** | Demonstrasi prioritas interrupt NVIC (nested interrupt) |

---

# ═══════════════════════════════════════════════════════════
# MODUL 03: Serial UART Communication
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **UART (Universal Asynchronous Receiver-Transmitter)** — protokol komunikasi serial **asynchronous** paling umum. UART digunakan untuk komunikasi antar MCU, debugging via Serial Monitor, dan interfacing dengan modul GPS/Bluetooth/WiFi.

## 📖 Materi Utama
- **Frame UART**: Start Bit → Data Bits (5-9) → Parity (optional) → Stop Bit
- **Baud Rate**: Standar 9600, 115200. Throughput 8N1 = Baud/10 bytes/sec
- **STM32 USART**: 3 USART (USART1 APB2 72MHz, USART2/3 APB1 36MHz)
- **ESP32 UART**: 3 UART (UART0 USB, UART1/2 user). Pin bisa di-remap ke GPIO manapun
- **Komunikasi MCU-to-MCU**: TX→RX crossed, GND wajib dihubungkan, baud rate sama
- **Ring Buffer**: Circular buffer untuk non-blocking UART receive
- **DMA UART**: Transfer data tanpa CPU (STM32 DMA Channel 4/5 untuk USART1)
- **Error Handling**: Overrun, Framing, Noise, Parity error + Checksum (XOR/CRC)
- **Protocol Design**: Text-based `$CMD,PARAM*CHECKSUM\r\n` atau Binary packet

## 🔧 Perbandingan

| Aspek | STM32F103 | ESP32 |
|-------|-----------|-------|
| Jumlah UART | 3 (USART1-3) | 3 (UART0-2) |
| Max Baud Rate | 4.5 Mbps | 5 Mbps |
| Hardware FIFO | Tidak (single buffer) | 128 bytes |
| DMA Support | Ya | Ya |
| Pin Remapping | Terbatas (AFIO) | Full flexibility |

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **UART Echo** | Menerima data serial dan mengirim kembali (echo) |
| 02 | **UART Interrupt RX** | Menerima data UART via interrupt (non-blocking) |
| 03 | **Ring Buffer** | Implementasi circular buffer untuk data UART |
| 04 | **ESP Log Library** | Menggunakan ESP_LOGI/W/E untuk logging bertingkat |
| 05 | **Command Parser** | Parsing perintah teks dari Serial (LED ON/OFF, ADC read) |
| 06 | **Program 06** | Konfigurasi UART lanjutan |
| 07 | **Line Editor** | Editor baris interaktif dengan backspace/delete |
| 08 | **STX Protocol** | Protokol binary dengan header STX/ETX |
| 09 | **CRC Checksum** | Implementasi CRC/XOR checksum untuk validasi data |
| 10 | **Timeout Parser** | Parser dengan timeout — deteksi paket tidak lengkap |
| 11 | **UART Bridge** | Bridge/relay data antar dua UART |
| 12 | **Error Statistics** | Monitoring dan statistik error UART (overrun, frame, noise) |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **UART Echo** | Echo serial via USART1 register-level |
| 02 | **UART Interrupt** | UART receive via interrupt RXNE |
| 03 | **Ring Buffer** | Circular buffer untuk UART receive |
| 04 | **Program 04** | Konfigurasi UART format (baud, parity, stop) |
| 05 | **Command Parser** | Parser perintah teks via UART |
| 06 | **Program 06** | Komunikasi multi-format |
| 07 | **Line Editor** | Interactive line editing via terminal |
| 08 | **STX Protocol** | Binary protocol STX/ETX |
| 09 | **CRC Checksum** | Checksum XOR/CRC untuk error detection |
| 10 | **Timeout Parser** | Parser dengan mekanisme timeout |
| 11 | **UART Bridge** | Relay data antar USART |
| 12 | **Error Stats** | Statistik error UART (ORE, FE, NE, PE) |

---

# ═══════════════════════════════════════════════════════════
# MODUL 04: Analog-to-Digital Converter (ADC)
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **ADC** — komponen yang mengubah sinyal **analog** (suhu, cahaya, tekanan) menjadi nilai **digital** yang bisa diproses mikrokontroler. Rumus dasar: `ADC_Value = (Vin / Vref) × (2^n - 1)`.

## 📖 Materi Utama
- **Sampling & Kuantisasi**: Nyquist theorem `fs ≥ 2×fmax`
- **Resolusi**: 12-bit → 4096 level, step size = 3.3V/4095 ≈ 0.8mV
- **ADC STM32**: 2 unit ADC 12-bit (SAR), 16 ch eksternal + 2 internal, ~1µs @14MHz
- **ADC ESP32**: 2 unit ADC 12-bit, ADC1 (GPIO32-39) aman, ADC2 TIDAK BISA saat WiFi aktif
- **Attenuation ESP32**: 0dB(0-1.1V), 2.5dB(0-1.5V), 6dB(0-2.2V), 11dB(0-3.3V)
- **Kalibrasi**: Linear scaling, Steinhart-Hart untuk NTC thermistor
- **Tips Hardware**: Kapasitor decoupling 100nF, buffer Op-Amp, VREF stabil

## 🔧 Perbandingan

| Aspek | STM32F103 | ESP32 |
|-------|-----------|-------|
| Resolusi | 12-bit | 12-bit |
| Channel | 16 ext + 2 internal | ADC1: 8ch, ADC2: 10ch |
| Waktu Konversi | ~1µs | Lebih lambat |
| Mode | Single, Continuous, Scan, Injected | Oneshot, Continuous |
| Pin ADC | PA0-PA7, PB0-PB1 | GPIO32-39 (ADC1), GPIO0-27 (ADC2) |
| Masalah | — | Non-linear, ADC2 konflik WiFi |

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **ADC Basic** | Pembacaan ADC dasar — satu channel |
| 02 | **ADC Multi-Channel** | Membaca beberapa channel ADC sekaligus |
| 03 | **ADC Averaging** | Rata-rata beberapa sampel untuk mengurangi noise |
| 04 | **ADC Calibration** | Kalibrasi ADC ESP32 untuk akurasi lebih baik |
| 05 | **Analog Comparator** | Membandingkan tegangan analog dengan threshold |
| 06 | **NTC Temperature** | Membaca suhu dari NTC thermistor (Steinhart-Hart) |
| 07 | **LDR Light Sensor** | Membaca level cahaya dari sensor LDR |
| 08 | **ADC Single Channel** | Pembacaan ADC tunggal dengan konfigurasi detail |
| 09 | **ADC Attenuation Setting** | Konfigurasi attenuation (0/2.5/6/11 dB) |
| 10 | **ADC Averaging 64x** | Oversampling 64 kali untuk resolusi efektif lebih tinggi |
| 11 | **ADC Continuous I2S** | ADC mode continuous via I2S DMA — high speed |
| 12 | **Moving Average Filter** | Filter digital moving average untuk smoothing data |
| 13 | **ADC Oversampling 16x** | Oversampling 16x untuk tambahan resolusi |
| 14 | **ADC Threshold Monitor** | Monitor tegangan dengan alert saat melewati threshold |
| 15 | **Internal Temperature Sensor** | Membaca sensor suhu internal ESP32 |
| 16 | **Hall Sensor Reading** | Membaca sensor Hall internal ESP32 (deteksi magnet) |
| 17 | **Sampling Rate Test** | Benchmark kecepatan sampling ADC |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **ADC Basic** | Pembacaan ADC dasar — analogRead register |
| 02 | **ADC Multi-Channel** | Scan mode — baca beberapa channel berurutan |
| 03 | **ADC Averaging** | Rata-rata pembacaan untuk noise reduction |
| 04 | **ADC DMA** | ADC continuous + DMA transfer ke RAM tanpa CPU |
| 05 | **Comparator** | Analog comparator — perbandingan tegangan |
| 06 | **NTC Temperature** | Pembacaan suhu NTC thermistor |
| 07 | **LDR** | Pembacaan sensor cahaya LDR |

---

# ═══════════════════════════════════════════════════════════
# MODUL 05: DAC dan PWM Output
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari dua cara menghasilkan **output analog** dari mikrokontroler digital:
1. **DAC (Digital-to-Analog Converter)**: Konversi langsung nilai digital ke tegangan analog
2. **PWM (Pulse Width Modulation)**: Sinyal digital dengan duty cycle variabel → tegangan rata-rata

## 📖 Materi Utama
- **DAC**: `Vout = Vref × (Digital_Value / 2^n)`, arsitektur R-2R Ladder
- **DAC STM32**: 2 channel 12-bit (PA4, PA5), DMA support, trigger dari Timer
- **DAC ESP32**: 2 channel 8-bit (GPIO25, GPIO26), Cosine Wave Generator
- **PWM**: `Duty(%) = Ton/T × 100%`, `Vavg = Vmax × Duty`
- **PWM STM32**: Timer-based (TIM1-4), mode PWM1/PWM2, complementary output
- **PWM ESP32**: LEDC peripheral — 16 channel, 1-20 bit resolusi, hardware fade
- **Aplikasi**: LED dimming, motor DC, servo (50Hz, 0.5-2.5ms pulse), pseudo-DAC (PWM + RC filter)

## 🔧 Perbandingan

| Aspek | STM32F103 | ESP32 |
|-------|-----------|-------|
| DAC Channels | 2 × 12-bit | 2 × 8-bit |
| DAC Pins | PA4, PA5 | GPIO25, GPIO26 |
| Waveform Gen | Triangle/Noise | Cosine |
| PWM Peripheral | Timer (TIM1-4) | LEDC (16 channel) |
| PWM Resolution | 16-bit | 1-20 bit |
| Hardware Fade | Tidak | Ya |

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **DAC Output** | Output tegangan analog via DAC (0-3.3V, 8-bit) |
| 02 | **DAC Sine Wave** | Generate gelombang sinus via DAC |
| 03 | **PWM LED Control** | Kontrol kecerahan LED dengan LEDC PWM |
| 04 | **PWM Motor Control** | Kontrol kecepatan motor DC dengan PWM + H-Bridge |
| 05 | **Servo Control** | Kontrol posisi servo motor (0-180°, 50Hz) |
| 06 | **PWM Pseudo DAC** | PWM + RC low-pass filter → output analog |
| 07 | **DMA Circular Buffer I2S** | Audio output via I2S DMA circular buffer |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **DAC Output** | Output tegangan analog via DAC 12-bit |
| 02 | **DAC Sine** | Generate gelombang sinus via DAC + Timer trigger |
| 03 | **PWM LED** | Kontrol LED brightness dengan Timer PWM |
| 04 | **PWM Motor** | Kontrol motor DC dengan PWM |
| 05 | **Servo** | Kontrol servo motor (50Hz PWM) |

---

# ═══════════════════════════════════════════════════════════
# MODUL 06: I2C Bus dan Sensor Integration
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari protokol **I2C (Inter-Integrated Circuit)** — komunikasi serial synchronous 2 wire (SDA + SCL) yang bisa menghubungkan **banyak device** (sensor, OLED, RTC, EEPROM) dalam satu bus.

## 📖 Materi Utama
- **I2C**: 2 wire (SDA/SCL), open-drain + pull-up resistor (4.7kΩ), 7-bit addressing (127 device max)
- **Speed**: Standard 100kHz, Fast 400kHz, Fast Plus 1MHz
- **Frame**: START → Address(7bit) + R/W → ACK → Data(8bit) → ACK → STOP
- **STM32 I2C**: 2 peripheral (I2C1: PB6/7, I2C2: PB10/11), DMA support
- **ESP32 I2C**: 2 controller, flexible GPIO mapping, internal pull-up
- **Sensor**: BME280 (suhu/kelembaban/tekanan), SSD1306 OLED (128×64), DS3231 RTC, 24LC256 EEPROM
- **Bus Recovery**: 9 clock pulses untuk melepas SDA stuck LOW
- **Pull-up**: 4.7kΩ @100kHz/<100pF, 2.2kΩ @400kHz

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **I2C Bus Scan** | Scanning semua device I2C dan menampilkan alamatnya |
| 02 | **BME280 Sensor Reading** | Membaca suhu, kelembaban, tekanan dari BME280 |
| 03 | **OLED SSD1306 Display** | Menampilkan teks dan grafik di OLED 128×64 |
| 04 | **DS3231 RTC Reading** | Membaca waktu/tanggal dari RTC DS3231 |
| 05 | **DS3231 Alarm** | Setting alarm pada RTC DS3231 |
| 06 | **EEPROM 24LC Read/Write** | Baca/tulis byte ke EEPROM I2C |
| 07 | **EEPROM Page Write** | Tulis halaman (64 byte) ke EEPROM sekaligus |
| 08 | **I2C Bus Recovery** | Pemulihan bus I2C saat SDA stuck LOW |
| 09 | **I2C Multi Master** | Konfigurasi multi-master I2C |
| 10 | **Temperature Log OLED** | Log suhu BME280 dan tampilkan di OLED |
| 11 | **Time Display DayDate** | Tampilkan waktu + hari/tanggal di OLED |
| 12 | **EEPROM Wear Leveling** | Teknik wear leveling untuk memperpanjang umur EEPROM |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **I2C Bus Scan** | Scanning bus I2C dengan HAL_I2C_IsDeviceReady |
| 02 | **BME280 Sensor Reading** | Membaca BME280 via I2C HAL |
| 03 | **OLED SSD1306 Display** | Menampilkan teks di OLED via I2C |
| 04 | **DS3231 RTC Reading** | Membaca waktu dari DS3231 |
| 05 | **DS3231 Alarm** | Setting alarm RTC |
| 06 | **EEPROM 24LC Read/Write** | Baca/tulis EEPROM via HAL_I2C_Mem_Write/Read |
| 07 | **EEPROM Page Write** | Page write EEPROM |
| 08 | **I2C Bus Recovery** | Bus recovery algorithm |
| 09 | **I2C Multi Master** | Multi-master I2C |
| 10 | **Temperature Log OLED** | Logging suhu ke OLED |
| 11 | **Time Display DayDate** | Display waktu/tanggal |
| 12 | **EEPROM Wear Leveling** | Wear leveling EEPROM |

---

# ═══════════════════════════════════════════════════════════
# MODUL 07: SPI Bus dan Storage
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **SPI (Serial Peripheral Interface)** — protokol serial synchronous full-duplex berkecepatan tinggi (hingga 80+ MHz). Digunakan untuk komunikasi dengan **SD Card, Flash Memory (W25Q), sensor cepat, display**.

## 📖 Materi Utama
- **SPI**: 4 wire (MOSI, MISO, SCLK, CS), Master-Slave, full-duplex
- **Clock Mode**: 4 mode (CPOL×CPHA), Mode 0 paling umum (SD Card, Flash)
- **STM32 SPI**: SPI1 (APB2 max 36MHz), SPI2 (APB1 max 18MHz)
- **ESP32 SPI**: HSPI (GPIO14/12/13/15), VSPI (GPIO18/19/23/5), remap flexible
- **SD Card**: SPI mode, init sequence CMD0→CMD8→ACMD41→CMD58, FatFS file system
- **Flash W25Q**: Page 256B, Sector 4KB, Block 64KB. Harus erase sebelum write
- **File System**: FatFS (SD Card), LittleFS (recommended, power-safe), SPIFFS (deprecated)
- **DMA SPI**: Transfer besar tanpa CPU — CPU bebas saat transfer berlangsung

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **SPI Loopback** | Test SPI dengan menghubungkan MOSI→MISO |
| 02 | **SPI Clock Modes** | Demonstrasi 4 mode SPI (CPOL/CPHA) |
| 03 | **SPI DMA Transfer** | Transfer data SPI via DMA — CPU bebas |
| 04 | **SPI Sensor Read** | Membaca sensor via SPI |
| 05 | **SD Card Mount** | Mount/unmount SD Card via SPI |
| 06 | **SD Card Read** | Membaca file dari SD Card |
| 07 | **SD Card Write Log** | Menulis data log ke SD Card |
| 08 | **SD Card List Directory** | Listing isi direktori SD Card |
| 09 | **CSV Data Logger** | Data logging format CSV ke SD Card |
| 10 | **SPI Flash W25Q32** | Baca/tulis/erase flash memory W25Q32 |
| 11 | **SPIFFS/LittleFS Partition** | File system di flash internal (SPIFFS/LittleFS) |
| 12 | **NVS Key-Value Storage** | Non-Volatile Storage — penyimpanan key-value |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **SPI Loopback** | Test SPI loopback MOSI→MISO |
| 02 | **SPI Clock Modes** | Demonstrasi CPOL/CPHA modes |
| 03 | **SPI DMA Transfer** | SPI transfer via DMA |
| 04 | **SPI Sensor Read** | Membaca sensor SPI |
| 05 | **SD Card Mount** | Mount SD Card via FatFS |
| 06 | **SD Card Read** | Baca file dari SD Card |
| 07 | **SD Card Write Log** | Tulis log ke SD Card |
| 08 | **SD Card List Directory** | Listing isi direktori |
| 09 | **CSV Data Logger** | CSV data logger ke SD Card |
| 10 | **SPI Flash W25Q32** | Baca/tulis flash W25Q32 |
| 11 | **SPIFFS/LittleFS Partition** | Internal flash file system |
| 12 | **NVS Key-Value Storage** | Key-value persistent storage |

---

# ═══════════════════════════════════════════════════════════
# MODUL 08: Direct Memory Access (DMA)
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **DMA (Direct Memory Access)** — fitur hardware yang memungkinkan peripheral (ADC, UART, SPI) mentransfer data **langsung ke/dari RAM tanpa CPU**. CPU bebas melakukan kalkulasi lain sementara DMA bekerja di background.

## 📖 Materi Utama
- **DMA**: Transfer data tanpa CPU → efisiensi tinggi, kecepatan bus, hemat daya
- **Transfer Mode**: Normal (sekali jalan) vs Circular (berulang otomatis)
- **Data Width**: Byte (8-bit), Half-Word (16-bit), Word (32-bit)
- **STM32 DMA**: DMA1 7 channel, hardwired ke peripheral tertentu (Ch1=ADC1, Ch4=USART1_TX, Ch5=USART1_RX)
- **ESP32 DMA**: Terintegrasi per-peripheral (I2S DMA, SPI DMA), Linked List Descriptors
- **Double Buffering (Ping-Pong)**: Half Transfer + Transfer Complete interrupt → continuous processing
- **Interrupt**: Transfer Complete (TC), Half Transfer (HT), Transfer Error (TE)

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **DMA Memory-to-Memory Transfer** | Copy data array via DMA |
| 02 | **UART RX with DMA Circular Buffer** | Terima UART via DMA circular — non-blocking |
| 03 | **UART TX with DMA** | Kirim UART via DMA — CPU bebas |
| 04 | **ADC DMA Continuous Dual Buffer** | ADC continuous + DMA double buffer |
| 05 | **SPI RX/TX with DMA** | Transfer SPI via DMA |
| 06 | **DMA Double Buffer Ping-Pong** | Pattern ping-pong — proses sambil terima data |
| 07 | **DMA Stream Priority Levels** | Konfigurasi prioritas DMA stream |
| 08 | **DMA Interrupt with Callback** | DMA dengan callback interrupt TC/HT |
| 09 | **Throughput: DMA vs Polling** | Benchmark perbandingan kecepatan DMA vs polling |
| 10 | **ADC Timer Trigger + DMA** | ADC di-trigger timer, data via DMA |
| 11 | **Multi DMA Concurrent** | Beberapa DMA channel berjalan bersamaan |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **DMA Memory-to-Memory** | Copy data via DMA1 channel |
| 02 | **UART RX DMA Circular** | USART1 RX via DMA circular mode |
| 03 | **UART TX DMA** | USART1 TX via DMA |
| 04 | **ADC DMA Continuous Dual Buffer** | ADC1 continuous + DMA + double buffer |
| 05 | **SPI RX/TX DMA** | SPI1 DMA transfer |
| 06 | **DMA Double Buffer Ping-Pong** | Ping-pong buffering pattern |
| 07 | **DMA Stream Priority** | Priority levels DMA channels |
| 08 | **DMA Interrupt Callback** | Callback pada TC/HT/TE |
| 09 | **Throughput: DMA vs Polling** | Benchmark DMA vs polling UART |
| 10 | **ADC Timer Trigger DMA** | ADC triggered by TIM + DMA |
| 11 | **Multi DMA Concurrent** | Multiple DMA concurrent transfers |

---

# ═══════════════════════════════════════════════════════════
# MODUL 09: FreeRTOS Task Management
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **FreeRTOS** — Real-Time Operating System open-source untuk embedded. Fokus pada **Task Management**: membuat, mengelola, dan menjadwalkan banyak task (multi-tasking) agar berjalan "bersamaan" dengan prioritas dan timing yang terkontrol.

## 📖 Materi Utama
- **RTOS vs Bare-Metal**: RTOS = concurrent execution, preemptive scheduling, deterministic timing
- **Task**: Unit eksekusi independen dengan stack sendiri + TCB (Task Control Block)
- **Task States**: Ready → Running → Blocked/Suspended → Ready
- **Priority**: 0 (lowest/idle) sampai configMAX_PRIORITIES-1 (highest)
- **API**: `xTaskCreate()`, `vTaskDelay()`, `vTaskDelayUntil()`, `vTaskSuspend/Resume()`, `vTaskDelete()`
- **Stack Size**: STM32 dalam Words (×4 bytes), ESP32 dalam Bytes
- **Scheduler**: Priority-based preemptive + Round-robin (same priority)
- **Context Switch**: Save/restore CPU registers, ~12 cycles pada Cortex-M3
- **High Water Mark**: `uxTaskGetStackHighWaterMark()` untuk tuning stack

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **DMA Interrupt with Callback** | Task yang menangani DMA callback |
| 02 | **Throughput Comparison** | Benchmark task DMA vs polling |
| 03 | **ADC Timer Trigger + DMA** | Task pembaca ADC dengan timer + DMA |
| 04 | **Multi DMA Concurrent** | Multiple DMA tasks concurrent |
| 05 | **Cache Coherency & Data Consistency** | Menangani konsistensi data antar task/DMA |
| 06 | **Dynamic Task Injection** | Membuat dan menghapus task saat runtime |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01-07 | **Dynamic Task Injection** | Membuat/menghapus task secara dinamis saat runtime |
| 02-08 | **Rate Monotonic Scheduling** | Penjadwalan task berdasarkan periode (RMS) |
| 03-09 | **Absolute Timing Control** | `vTaskDelayUntil()` untuk timing presisi |
| 04-10 | **Idle Task Hook** | Hook function di Idle task (low-priority monitoring) |
| 05-11 | **Task Suspend/Resume** | Suspend dan resume task manual |
| 06-12 | **Struct Message Passing** | Mengirim struct data antar task |
| 13 | **Tickless Idle** | Mode hemat daya — skip tick saat idle |

---

# ═══════════════════════════════════════════════════════════
# MODUL 10: FreeRTOS Queue dan Semaphore
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **mekanisme komunikasi dan sinkronisasi antar task** dalam FreeRTOS:
- **Queue**: Transfer data antar task secara aman (FIFO)
- **Semaphore**: Sinkronisasi event (binary/counting)
- **Mutex**: Proteksi shared resource (mencegah race condition)

## 📖 Materi Utama
- **Masalah tanpa sinkronisasi**: Race condition, data corruption
- **Queue**: FIFO buffer thread-safe, `xQueueSend/Receive`, support ISR (`FromISR`)
- **Binary Semaphore**: Signal event (Give/Take), ISR-safe, untuk sinkronisasi
- **Counting Semaphore**: Menghitung resource tersedia (misal: slot parkir)
- **Mutex**: Proteksi shared resource, priority inheritance (mencegah priority inversion)
- **Queue dari ISR**: `xQueueSendFromISR()` + `portYIELD_FROM_ISR()`
- **Multiple Producer/Consumer**: Banyak task kirim ke satu queue

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **Rate Monotonic Scheduling** | Penjadwalan task periodik berbasis prioritas |
| 02 | **Absolute Timing Control** | Timing presisi dengan `vTaskDelayUntil()` |
| 03 | **Idle Task Hook** | Monitoring di Idle task |
| 04 | **Task Suspend/Resume** | Kontrol suspend/resume task |
| 05 | **Struct Message Passing** | Kirim struct via queue antar task |
| 06 | **Mailbox Pattern** | Pattern mailbox — overwrite data terbaru |
| 07 | **Flow Control with Backpressure** | Kontrol aliran data agar consumer tidak overwhelmed |
| 08 | **Queue Peek Operations** | Intip data queue tanpa menghapus |
| 09 | **Queue Sets** | Menunggu data dari beberapa queue sekaligus |
| 10 | **Gatekeeper Task Pattern** | Task gatekeeper untuk akses exclusive ke resource |
| 11 | **Recursive Mutex Locking** | Mutex yang bisa di-lock berulang oleh task sama |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **Mailbox Pattern** | Pattern mailbox — data terbaru saja |
| 02 | **Flow Control Backpressure** | Backpressure flow control |
| 03 | **Queue Peek Operations** | Peek queue tanpa consume |
| 04 | **Queue Sets** | Wait multiple queues |
| 05 | **Gatekeeper Task Pattern** | Exclusive resource access via gatekeeper |
| 06 | **Recursive Mutex Locking** | Recursive mutex |
| 07 | **Priority Inversion Fix** | Demonstrasi dan fix priority inversion |
| 08 | **Counting Semaphore Events** | Counting semaphore untuk event batching |
| 09 | **Barrier Synchronization** | Sinkronisasi barrier — semua task tunggu |
| 10-20 | **ISR Safe Queue** | Queue yang aman dipanggil dari ISR |

---

# ═══════════════════════════════════════════════════════════
# MODUL 11: FreeRTOS Software Timer dan Task Notification
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari dua fitur FreeRTOS yang sangat efisien:
- **Software Timer**: Menjalankan callback secara periodik/one-shot **tanpa task terpisah** (hemat memory)
- **Task Notification**: Komunikasi ringan point-to-point yang **45% lebih cepat** dari binary semaphore

## 📖 Materi Utama
- **Software Timer**: One-shot (sekali) atau Auto-reload (periodik), dikelola oleh Timer Daemon Task
- **Callback**: TIDAK BOLEH blocking/delay — harus singkat dan cepat
- **Timer API**: `xTimerCreate()`, `xTimerStart/Stop/Reset()`, `xTimerChangePeriod()`
- **Task Notification**: Built-in 32-bit value di setiap task, 0 bytes RAM tambahan
- **Notification Actions**: `eNoAction`, `eSetBits`, `eIncrement`, `eSetValueWithOverwrite`
- **API**: `xTaskNotifyGive()`, `ulTaskNotifyTake()`, `xTaskNotify()`, `xTaskNotifyWait()`
- **Keunggulan Notification**: 45% lebih cepat, 0 RAM extra, cocok untuk point-to-point

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **Direct Task Notification** | Notification dasar — wake up task |
| 02 | **Task Notification as Value** | Kirim nilai 32-bit via notification |
| 03 | **Task Notification Event Bits** | Gunakan notification sebagai event flags |
| 04 | **Pulse Notification** | Counting notification untuk pulse counting |
| 05 | **Software Watchdog Timer** | Watchdog implementasi software timer |
| 06 | **Auto-reload Timer** | Timer periodik auto-reload |
| 07 | **Timer with ID and Callback** | Satu callback untuk banyak timer (ID identifier) |
| 08 | **Debounce Timer** | Debouncing tombol dengan software timer reset |
| 09 | **Deferred ISR Processing** | ISR singkat → timer/notification → task proses |
| 10 | **Interrupt Nesting and Priority** | Demonstrasi nested interrupt dengan prioritas |
| 11 | **ISR Safe Queue** | Queue yang aman dari ISR |
| 12 | **Critical Section ISR** | Critical section dalam ISR |
| 13 | **Yield From ISR** | Context switch dari ISR (`portYIELD_FROM_ISR`) |
| 14 | **ISR Stream Buffer** | Stream buffer untuk kirim data dari ISR ke task |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01-14 | **Critical Section ISR** | Critical section di ISR STM32 |
| 02-15 | **Yield From ISR** | Yield dari ISR handler |
| 03-16 | **ISR Stream Buffer** | Stream buffer ISR→Task |
| 04-17 | **Direct Task Notification** | Wake task via notification |
| 05-18 | **Notification as Value** | Kirim value via notification |
| 06-19 | **Notification Event Bits** | Event bits notification |
| 07-20 | **Pulse Notification** | Pulse counting notification |
| 08-21 | **Software Watchdog** | Watchdog software timer |
| 09-22 | **Autoreload Timer** | Timer periodik |
| 10-23 | **Timer ID Callback** | Multi-timer dengan single callback |
| 11-24 | **Debounce Timer** | Debounce dengan timer |
| 12-25 | **Deferred ISR Processing** | Deferred interrupt processing |
| 13-26 | **Interrupt Nesting Priority** | Nested interrupt demo |
| 27 | **Watchdog Manager** | System watchdog manager |

---

# ═══════════════════════════════════════════════════════════
# MODUL 12: FreeRTOS Memory Management & Advanced Features
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **manajemen memori** FreeRTOS (heap, stack, fragmentasi) dan **fitur lanjutan** (Event Group, Stream Buffer, Message Buffer) untuk sistem embedded yang lebih kompleks.

## 📖 Materi Utama
- **Heap Schemes**: heap_1 (alloc only), heap_2 (deprecated), heap_3 (C stdlib), **heap_4 (recommended, coalescing)**, heap_5 (multi-region)
- **Stack Management**: Stack per task, STM32=Words (×4B), ESP32=Bytes
- **Stack Overflow Detection**: Method 1 (SP check) dan Method 2 (pattern 0xA5, recommended)
- **High Water Mark**: `uxTaskGetStackHighWaterMark()` — sisa stack minimum yang pernah terjadi
- **Static Allocation**: `xTaskCreateStatic()` — memori dari global variable, no heap fragmentation
- **Event Group**: 24-bit flags, broadcasting (satu event unblock banyak task), combination wait (AND/OR)
- **Stream Buffer**: Byte stream ISR→Task, tanpa metadata, sangat efisien
- **Message Buffer**: Discrete messages variable length, header 4 bytes panjang pesan

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **Heap Info** | Menampilkan informasi heap (free, minimum ever free) |
| 02 | **Stack Overflow** | Demonstrasi dan deteksi stack overflow |
| 03 | **PSRAM Usage** | Menggunakan PSRAM eksternal untuk alokasi besar |
| 04 | **EventGroup Advanced** | Event group — sinkronisasi multi-task |
| 05 | **Stream Buffer Basics/ISR** | Stream buffer dari ISR ke task |
| 06 | **MessageBuffer MultiCore** | Message buffer antar core (Core 0 ↔ Core 1) |
| 07 | **Static DualCore** | Static allocation pada dual core |
| 08 | **Light Sleep** | Integrasi FreeRTOS + light sleep |
| 09 | **Memory Leak Detection** | Deteksi kebocoran memori |
| 10 | **Heap Usage Monitoring** | Monitor penggunaan heap real-time |
| 11 | **Stack Overflow Detection** | Hook function stack overflow |
| 12 | **Static Memory Allocation** | `xTaskCreateStatic()` — tanpa heap |
| 13 | **Memory Pool Pattern** | Pattern memory pool — alokasi fixed-size |
| 14 | **Malloc Failed Hook** | Hook saat pvPortMalloc gagal |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **Heap Statistics** | Statistik heap (free, min ever free) |
| 02 | **Stack Overflow Detection** | Deteksi stack overflow Method 2 |
| 03 | **Memory Pool** | Pattern memory pool fixed-size |
| 04 | **EventGroup Init** | Event group untuk system initialization |
| 05 | **Stream Buffer** | Stream buffer basics |
| 06 | **Heap Usage Monitoring** | Monitor heap real-time |
| 07 | **Static Allocation** | Static memory allocation |
| 08 | **Low Power** | Integrasi FreeRTOS + low power mode |
| 09 | **Fragmentation Analysis** | Analisis fragmentasi heap |
| 10 | **Complete Memory System** | Sistem manajemen memori lengkap |
| 11 | **Cache Coherency** | Konsistensi data cache |
| 12-23 | **Extended Programs** | Variasi program lanjutan (ISR Stream Buffer, Message Buffer, dll) |

---

# ═══════════════════════════════════════════════════════════
# MODUL 13: Embedded Networking & IoT Protocols
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **komunikasi jaringan** pada embedded systems — menghubungkan MCU ke **internet/cloud** menggunakan WiFi, Ethernet, Bluetooth, dan protokol IoT (HTTP, MQTT, WebSocket, BLE).

## 📖 Materi Utama
- **TCP/IP Stack**: Link → Internet (IP) → Transport (TCP/UDP) → Application (HTTP/MQTT)
- **LwIP**: Lightweight IP stack untuk embedded (digunakan ESP-IDF dan STM32 Cube)
- **HTTP**: Client-Server, GET (ambil data), POST (kirim data), format JSON
- **MQTT**: Publish-Subscribe via Broker, Topic hierarkis, QoS 0/1/2
- **ESP32**: WiFi + Bluetooth built-in, LwIP terintegrasi
- **STM32 + W5500**: Ethernet SPI module, hardware TCP/IP stack (offloading)
- **BLE**: Bluetooth Low Energy — GATT Server/Client untuk IoT sensor

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **WiFi Station (DHCP)** | Konek ke WiFi Access Point, dapatkan IP via DHCP |
| 02 | **WiFi Access Point** | ESP32 sebagai WiFi AP — device lain konek ke ESP32 |
| 03 | **TCP Client / MQTT Publisher** | Koneksi TCP ke server / Publish data ke MQTT broker |
| 04 | **TCP Server / MQTT Subscriber** | Terima koneksi TCP / Subscribe topic MQTT |
| 05 | **HTTP Client / UDP Client** | Request HTTP GET/POST / Kirim data via UDP |
| 06 | **DNS Resolve / Web Server** | Resolve domain ke IP / ESP32 sebagai web server |
| 07 | **HTTP GET / WebSocket** | GET request ke API / WebSocket real-time bidirectional |
| 08 | **HTTP POST / Bluetooth Classic** | POST JSON ke server / Bluetooth SPP serial |
| 09 | **HTTPS TLS / BLE Server** | Koneksi HTTPS terenkripsi / BLE GATT server |
| 10 | **MQTT Publish / IoT Gateway** | Publish sensor data ke cloud / Gateway IoT |
| 11 | **MQTT Subscribe** | Subscribe dan terima pesan dari broker |
| 12 | **NTP Time Sync** | Sinkronisasi waktu dari server NTP |
| 13 | **BLE GATT Server** | BLE server dengan characteristic baca/tulis |
| 14 | **BLE Scan Connect** | Scan dan konek ke BLE device lain |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **W5500 DHCP Init / Serial Basic** | Init Ethernet W5500 + DHCP / UART dasar |
| 02 | **TCP Client / JSON Protocol** | Koneksi TCP client via W5500 / Komunikasi JSON |
| 03 | **TCP Server / Sensor Node** | TCP server via Ethernet / Node sensor serial |
| 04 | **UDP Client / Command Handler** | Kirim data UDP / Handler command serial |
| 05 | **DNS Resolve / Data Logger** | Resolve DNS / Logging data ke storage |
| 06 | **HTTP GET / Event System** | HTTP GET via W5500 / Sistem event |
| 07 | **HTTP POST / State Sync** | HTTP POST JSON / Sinkronisasi state |
| 08 | **NTP Time Sync / Multi Sensor** | NTP via Ethernet / Multi sensor serial |
| 09 | **MQTT Publish** | Publish data ke MQTT broker via W5500 |
| 10 | **MQTT Subscribe** | Subscribe dan terima pesan MQTT |
| 11 | **TLS Hash Local** | Hashing/enkripsi lokal (SHA256, AES) |
| 12 | **BLE UART HM10** | Komunikasi BLE via modul HM-10 |
| 13 | **NTP RTC Sync** | Sinkronisasi NTP → DS3231 RTC |
| 14 | **Connection Failover** | Failover otomatis saat koneksi gagal |

---

# ═══════════════════════════════════════════════════════════
# MODUL 14: Power Management & Low-Power Design
# ═══════════════════════════════════════════════════════════

## 📌 Tentang Apa?
Mempelajari **teknik hemat daya** untuk sistem embedded berbaterai. ESP32 aktif ~240mA, deep sleep ~10µA — **perbedaan 24.000x lipat**! Teknik low-power memungkinkan perangkat bertahan berminggu-minggu dengan baterai coin cell.

## 📖 Materi Utama
- **Rumus Daya**: $P = C \times V^2 \times f + I_{leak} \times V$
- **3 Strategi**: Turunkan frekuensi, turunkan tegangan, matikan modul tidak terpakai
- **STM32 Modes**: Run(~30mA) → Sleep(~10mA) → Stop(~20µA) → Standby(~2µA)
- **ESP32 Modes**: Active(~240mA) → Modem Sleep(~20mA) → Light Sleep(~0.8mA) → Deep Sleep(~10µA) → Hibernation(~5µA)
- **Wake-up STM32**: EXTI pin, RTC Alarm, RTC Wakeup Timer, IWDG, NRST
- **Wake-up ESP32**: Timer, Touch Pad, ext0/ext1 GPIO, ULP co-processor
- **Clock Gating**: Matikan clock peripheral tidak terpakai
- **Dynamic Frequency Scaling**: ESP32 native (10-240MHz), STM32 via clock reconfig
- **ULP Co-processor** (ESP32): Prosesor ultra-low-power aktif saat deep sleep (~150µA)
- **RTC Memory** (ESP32): 8KB `RTC_DATA_ATTR` bertahan saat deep sleep
- **Backup Register** (STM32): 20×16-bit register bertahan selama VBAT ada
- **Estimasi baterai**: $T = C_{battery} / I_{avg}$, duty cycling sangat efektif

---

### 🟦 ESP32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **Deep Sleep Timer** | Deep sleep + bangun setelah timer RTC (µs precision) |
| 02 | **Light Sleep GPIO** | Light sleep + bangun saat GPIO berubah |
| 03 | **Ext0/Ext1 Wakeup** | Deep sleep + bangun dari ext0 (1 GPIO) atau ext1 (multi GPIO) |
| 04 | **Touch Pad Wakeup** | Deep sleep + bangun dari sentuhan touch pad kapasitif |
| 05 | **Dynamic Frequency Scaling** | Otomatis turunkan frekuensi CPU saat idle (10-240MHz) |
| 06 | **RTC Memory Persistence** | Simpan data di RTC memory (`RTC_DATA_ATTR`) selama deep sleep |
| 07 | **Battery Monitor** | Monitoring tegangan baterai via ADC + voltage divider |
| 08 | **Hibernation Mode** | Mode paling hemat (~5µA) — RTC memory juga dimatikan |
| 09 | **Adaptive Duty Cycling** | Duty cycle adaptif — aktif singkat, tidur lama |
| 10 | **WiFi Power Save** | WiFi dengan mode hemat daya (modem sleep, DTIM) |
| 11 | **Solar Weather Station** | Proyek lengkap: stasiun cuaca solar-powered dengan deep sleep |

### 🟩 STM32 — Daftar Program & Fungsi

| No | Program | Fungsi |
|----|---------|--------|
| 01 | **Sleep Mode WFI** | Masuk Sleep mode dengan WFI (Wait For Interrupt) |
| 02 | **Stop Mode EXTI** | Stop mode + bangun dari EXTI pin (clock dikonfigurasi ulang) |
| 03 | **Standby RTC Wakeup** | Standby + bangun dari RTC alarm (SRAM hilang, seperti reset) |
| 04 | **Wakeup Pin Standby** | Standby + bangun dari WKUP pin |
| 05 | **Clock Frequency Scaling** | Turunkan clock dari PLL 72MHz ke HSI 8MHz untuk hemat daya |
| 06 | **Backup Register** | Simpan data di backup register (bertahan saat VBAT ada) |
| 07 | **Battery Monitor ADC** | Monitor baterai via ADC + VREFINT kalibrasi |
| 08 | **Peripheral Clock Gating** | Matikan clock peripheral tidak terpakai |
| 09 | **Adaptive Duty Cycle** | Pola duty cycle adaptif untuk hemat baterai |

---

# ═══════════════════════════════════════════════════════════
# 📊 RINGKASAN TOTAL
# ═══════════════════════════════════════════════════════════

## Total Program Praktikum

| Modul | Topik | ESP32 | STM32 |
|-------|-------|-------|-------|
| 01 | GPIO Digital I/O | 12 | 12 |
| 02 | Interrupt & Timer | 12 | 12 |
| 03 | Serial UART | 12 | 12 |
| 04 | ADC | 17 | 7 |
| 05 | DAC & PWM | 7 | 5 |
| 06 | I2C Sensor | 12 | 12 |
| 07 | SPI Storage | 12 | 12 |
| 08 | DMA | 11 | 11 |
| 09 | FreeRTOS Task | 6 | 13 |
| 10 | FreeRTOS Queue/Semaphore | 11 | 20 |
| 11 | FreeRTOS Timer/Notification | 14 | 27 |
| 12 | FreeRTOS Memory/Advanced | 14 | 23 |
| 13 | Network & IoT | 14 | 14 |
| 14 | Power Management | 11 | 9 |
| **TOTAL** | | **~165** | **~189** |

## Platform Summary

### STM32F103C8T6 (Blue Pill)
- **Arsitektur**: ARM Cortex-M3, 72MHz
- **Framework**: STM32Cube HAL (via PlatformIO)
- **Memori**: 64KB Flash, 20KB SRAM
- **Peripheral**: GPIO, USART×3, SPI×2, I2C×2, ADC×2 (12-bit), DAC×2 (12-bit), Timer×4, DMA
- **Networking**: Modul eksternal W5500 (Ethernet SPI) atau HM-10 (BLE)
- **Low Power**: Sleep / Stop (~20µA) / Standby (~2µA)

### ESP32 DevKitC
- **Arsitektur**: Xtensa LX6 Dual-Core, 240MHz
- **Framework**: ESP-IDF (via PlatformIO)
- **Memori**: 4MB Flash, 520KB SRAM, opsional 4MB PSRAM
- **Peripheral**: GPIO (34 pin), UART×3, SPI×2, I2C×2, ADC×2 (12-bit), DAC×2 (8-bit), LEDC (16ch PWM), Touch Sensor, Hall Sensor, ULP Co-processor
- **Networking**: WiFi 802.11 b/g/n + Bluetooth Classic + BLE built-in
- **Low Power**: Modem Sleep / Light Sleep (~0.8mA) / Deep Sleep (~10µA) / Hibernation (~5µA)

---

*Dokumen ini merangkum seluruh 14 modul Praktikum Sistem Embedded*
*Versi 1.0 — Februari 2026*
