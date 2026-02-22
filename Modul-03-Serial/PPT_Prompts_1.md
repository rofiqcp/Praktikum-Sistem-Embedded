# Prompt untuk Pembuatan PPT - Bagian 1: Teori Serial UART
## Modul 03: Komunikasi Serial UART

---

## 📋 Informasi Presentasi

| Item | Keterangan |
|------|------------|
| **Topik** | Teori Komunikasi Serial UART |
| **Jumlah Slide** | 28 Slide |
| **Durasi** | 45-60 menit |
| **Target Audiens** | Mahasiswa Teknik Elektro/Informatika |
| **Software** | PowerPoint / Google Slides / Canva |

---

## 🎨 Panduan Desain

### Tema Visual
- **Color Palette:** Deep Blue (#1e3a5f), Tech Cyan (#00d4ff), White (#ffffff), Gray (#e0e0e0)
- **Font Heading:** Montserrat Bold atau Poppins Bold
- **Font Body:** Open Sans atau Roboto Regular
- **Style:** Technical, Clean, Professional dengan diagram timing

### Elemen Visual
- Timing diagram untuk sinyal serial
- Frame format illustrations
- Flowchart komunikasi
- Oscilloscope captures (ilustrasi)
- Code snippets dengan syntax highlighting

---

## 📊 Struktur Slide Detail

### Slide 1: Cover
**Prompt:**
```
Buat slide cover presentasi dengan judul "Komunikasi Serial UART" dan subtitle 
"Universal Asynchronous Receiver/Transmitter untuk Embedded Systems".

Elemen desain:
- Background gradient dari deep blue ke tech cyan
- Ilustrasi sinyal digital dengan timing diagram UART
- Logo institusi di pojok
- Nama dosen dan mata kuliah: Praktikum Sistem Embedded
- Tampilkan visual TX/RX lines dengan data streaming

Visual style: Technical, professional dengan nuansa elektronik dan komunikasi data
```

---

### Slide 2: Agenda Pembelajaran
**Prompt:**
```
Buat slide agenda dengan layout modern menggunakan numbered cards.

Topik yang akan dibahas:
1. Konsep Komunikasi Serial
2. Protokol UART: Frame & Timing
3. Parameter Konfigurasi (Baud Rate, Data Bits, Parity)
4. UART pada STM32F103C8T6
5. UART pada ESP32
6. Komunikasi Antar-MCU
7. Troubleshooting & Best Practices

Design: Gunakan icon yang relevan untuk setiap topik
- Icon gelombang untuk konsep serial
- Icon frame untuk protokol
- Icon gear untuk konfigurasi
- Icon chip untuk MCU
- Icon connection untuk antar-MCU
```

---

### Slide 3: Pengantar Komunikasi Serial
**Prompt:**
```
Buat slide pengantar komunikasi serial dengan perbandingan visual.

Konten:
"Komunikasi Serial vs Parallel"

PARALLEL:
- Banyak kabel, banyak data sekaligus
- Cepat untuk jarak pendek
- Mahal dan rentan crosstalk
- Contoh: Bus internal komputer

SERIAL:
- Sedikit kabel, data berurutan
- Reliable untuk jarak jauh
- Murah dan simple
- Contoh: USB, UART, SPI, I2C

Ilustrasi:
- Diagram parallel: 8 garis data + control lines
- Diagram serial: 2 garis saja (TX, RX)
- Animasi/visual data mengalir bit per bit

Quote: "Serial communication: Less wires, more reliable"
```

---

### Slide 4: Jenis-Jenis Komunikasi Serial
**Prompt:**
```
Buat slide klasifikasi komunikasi serial dalam bentuk hierarchy diagram.

SERIAL COMMUNICATION
├── SYNCHRONOUS (dengan clock)
│   ├── SPI (Serial Peripheral Interface)
│   ├── I2C (Inter-Integrated Circuit)
│   └── USART mode synchronous
│
└── ASYNCHRONOUS (tanpa clock)
    └── UART (Universal Asynchronous Receiver/Transmitter)

Tabel perbandingan:
| Fitur      | UART  | SPI   | I2C   |
|------------|-------|-------|-------|
| Clock      | No    | Yes   | Yes   |
| Wires      | 2     | 4     | 2     |
| Speed      | ~1Mb  | ~20Mb | ~3.4Mb|
| Devices    | 1:1   | Many  | Many  |

Highlight UART sebagai fokus modul ini dengan border atau glow effect
```

---

### Slide 5: Apa itu UART?
**Prompt:**
```
Buat slide definisi UART yang komprehensif.

Judul: "UART: Universal Asynchronous Receiver/Transmitter"

Definisi dengan highlight:
"Hardware peripheral yang mengkonversi data PARALLEL (dari CPU) 
menjadi data SERIAL (untuk transmisi) dan sebaliknya, 
TANPA memerlukan sinyal clock bersama."

Ilustrasi blok diagram:
┌─────────┐         ┌───────────────┐         ┌─────────┐
│   CPU   │◄──8b───►│ TX │    │ RX │◄───1b───►│   Line  │
│  (MCU)  │ parallel│    │UART│    │  serial  │ (Wire)  │
└─────────┘         └───────────────┘         └─────────┘

Key Characteristics (dengan icon):
⚡ Asynchronous: Tidak perlu shared clock
🔄 Full-duplex: TX dan RX bersamaan
📊 Buffered: FIFO untuk data handling
⚙️ Configurable: Baud rate, parity, stop bits
```

---

### Slide 6: UART Frame Structure
**Prompt:**
```
Buat slide detail struktur frame UART dengan timing diagram profesional.

Judul: "Anatomi UART Frame"

Visual timing diagram yang detail:

     Idle  Start                Data Bits              Parity Stop Idle
      │     │    │─────────────────────────────│        │    │    │
      │     │    │                             │        │    │    │
HIGH ─┴─────┐    D0   D1   D2   D3   D4   D5   D6   D7  P   ┌────┴───
            │    │    │    │    │    │    │    │    │   │   │
LOW         └────┴────┴────┴────┴────┴────┴────┴────┴───┴───┘

Komponen Frame (dengan penjelasan):
1. IDLE State: HIGH level (Mark state)
2. START Bit: Transisi HIGH→LOW (Space)
3. DATA Bits: 5-9 bits, LSB first
4. PARITY Bit: Optional error detection
5. STOP Bit(s): 1, 1.5, atau 2 bits HIGH

Total bits per character: 10 (8N1 config)
```

---

### Slide 7: Start Bit & Stop Bit
**Prompt:**
```
Buat slide yang menjelaskan fungsi start dan stop bit.

START BIT:
- Selalu LOW (logic 0)
- Memberikan "wake-up" ke receiver
- Memulai proses sinkronisasi
- Receiver mulai sampling pada falling edge

Timing diagram detail:
IDLE (HIGH) → FALLING EDGE → START BIT (LOW) → Data mulai

STOP BIT:
- Selalu HIGH (logic 1)
- Mengakhiri frame
- Memberikan waktu untuk processing
- Return to idle state

Ilustrasi:
- Zoom pada transisi start bit
- Zoom pada stop bit
- Interval antar frame (idle time)

Tips: "Stop bit memastikan ada jeda untuk receiver reset 
sebelum menerima data berikutnya"
```

---

### Slide 8: Baud Rate Explained
**Prompt:**
```
Buat slide penjelasan Baud Rate yang intuitif.

Judul: "Baud Rate: Detak Jantung UART"

Definisi:
"Baud Rate = jumlah simbol/state changes per detik"
"Untuk binary UART: 1 baud = 1 bit per second (bps)"

Standard Baud Rates dengan visual speedometer:
🐢 9600    - Legacy devices, reliable
🚶 19200   - Common embedded
🏃 38400   - Faster but compatible
🚴 57600   - Enhanced speed
🏎️ 115200  - High speed standard
🚀 921600  - Maximum common
⚡ Custom  - Use with caution

Perhitungan penting:
"Bit Time = 1 / Baud Rate"
- @ 9600 baud:   Bit Time = 104.17 µs
- @ 115200 baud: Bit Time = 8.68 µs

Throughput calculation (8N1):
Actual data rate = Baud Rate × (8/10) = 80% efficiency
```

---

### Slide 9: Bit Time dan Sampling
**Prompt:**
```
Buat slide tentang timing dan sampling point.

Judul: "Bit Timing & Sampling Point"

Diagram timing:
        ◄─────── Bit Time ───────►
        │                         │
TX Data ─────┬───────────────────┬─────
             │                   │
             │   ← Sampling Point (center)
             │
        Start of bit

Proses sampling:
1. Deteksi falling edge (start bit)
2. Wait 1.5 bit times to center of first data bit
3. Sample setiap 1 bit time setelahnya

Oversampling (16x):
- MCU samples 16x per bit time
- Uses middle samples for decision
- Provides noise immunity

Rumus:
USART_BRR = SystemClock / (16 × BaudRate)
```

---

### Slide 10: Data Format Configuration
**Prompt:**
```
Buat slide konfigurasi format data dengan visual options.

Judul: "Konfigurasi Format Data"

DATA BITS (visual dengan bit patterns):
• 7-bit: Legacy ASCII (0x00 - 0x7F)
• 8-bit: Standard, full byte (0x00 - 0xFF) ← Most common
• 9-bit: Special uses, address mode

PARITY (visual dengan checksum example):
• None (N): No error checking
• Even (E): Total 1s harus genap
• Odd (O): Total 1s harus ganjil

Example dengan data 0x5A (01011010):
- Even parity: P=0 (4 ones, already even)
- Odd parity: P=1 (need 5 ones, odd)

STOP BITS (visual timing):
• 1 bit: Standard
• 1.5 bits: Old devices
• 2 bits: Slower but safer

Common notation: "8N1" = 8 data, No parity, 1 stop
```

---

### Slide 11: Frame Format Examples
**Prompt:**
```
Buat slide dengan contoh frame format real.

Judul: "Contoh Frame UART dalam Praktik"

Example 1: Mengirim huruf 'A' (0x41 = 01000001) dengan 8N1
┌─────┬────────────────────────────┬─────┐
│Start│ 1 0 0 0 0 0 1 0            │Stop │
│  0  │ (LSB first = 10000010)     │  1  │
└─────┴────────────────────────────┴─────┘

Example 2: Mengirim byte 0x55 (01010101) dengan 8E1
┌─────┬────────────────────────────┬────┬─────┐
│Start│ 1 0 1 0 1 0 1 0            │Par │Stop │
│  0  │ (LSB first = 10101010)     │ 0  │  1  │
└─────┴────────────────────────────┴────┴─────┘
Parity = 0 (4 ones = even)

Visual oscilloscope-style waveform untuk kedua contoh
```

---

### Slide 12: STM32F103 USART Overview
**Prompt:**
```
Buat slide overview USART pada STM32F103.

Judul: "USART pada STM32F103C8T6"

Fitur USART STM32:
• 3 USART peripherals: USART1, USART2, USART3
• Full-duplex asynchronous & synchronous
• Hardware flow control (CTS/RTS)
• DMA support
• Up to 4.5 Mbps
• Multi-processor communication
• LIN Master/Slave capability

Pin Mapping Table:
┌─────────┬──────┬──────┬──────┬───────────────┐
│ USART   │  TX  │  RX  │ CK   │ Notes         │
├─────────┼──────┼──────┼──────┼───────────────┤
│ USART1  │ PA9  │ PA10 │ PA8  │ APB2 (72MHz)  │
│ USART2  │ PA2  │ PA3  │ PA4  │ APB1 (36MHz)  │
│ USART3  │ PB10 │ PB11 │ PB12 │ APB1 (36MHz)  │
└─────────┴──────┴──────┴──────┴───────────────┘

Block diagram USART peripheral dengan TX/RX shift registers
```

---

### Slide 13: STM32 USART Registers
**Prompt:**
```
Buat slide register penting USART STM32.

Judul: "Register USART STM32"

Key Registers dengan visual bit fields:

USART_SR (Status Register):
[TXE|TC|RXNE|IDLE|ORE|NE|FE|PE]
- TXE: Transmit empty (ready)
- RXNE: Read data not empty (data available)
- TC: Transmission complete

USART_DR (Data Register):
[──────────DR[8:0]──────────]
- Write: Send data
- Read: Receive data

USART_BRR (Baud Rate Register):
[DIV_Mantissa[11:0]|DIV_Fraction[3:0]]
- Determines baud rate

USART_CR1 (Control Register 1):
[UE|M|WAKE|PCE|PS|PEIE|TXEIE|TCIE|RXNEIE|IDLEIE|TE|RE|...]
- UE: USART Enable
- TE: Transmitter Enable
- RE: Receiver Enable
```

---

### Slide 14: STM32 Baud Rate Calculation
**Prompt:**
```
Buat slide perhitungan baud rate STM32.

Judul: "Perhitungan Baud Rate STM32"

Formula:
┌─────────────────────────────────────────────────┐
│                     f_PCLK                       │
│  Baud Rate = ─────────────────────────────       │
│              16 × USARTDIV                      │
│                                                 │
│  USARTDIV = f_PCLK / (16 × Desired_BaudRate)   │
└─────────────────────────────────────────────────┘

Example untuk 115200 baud:
- USART1: f_PCLK = 72 MHz (APB2)
  USARTDIV = 72000000 / (16 × 115200) = 39.0625
  Mantissa = 39, Fraction = 0.0625 × 16 = 1
  BRR = (39 << 4) | 1 = 0x271

- USART2/3: f_PCLK = 36 MHz (APB1)
  USARTDIV = 36000000 / (16 × 115200) = 19.53125
  Mantissa = 19, Fraction = 0.53125 × 16 ≈ 8.5 ≈ 9
  BRR = (19 << 4) | 9 = 0x139

Table common baud rates dengan BRR values
```

---

### Slide 15: STM32 UART dengan STM32Cube HAL
**Prompt:**
```
Buat slide implementasi UART STM32 dengan STM32Cube HAL framework.

Judul: "UART STM32 dengan STM32Cube HAL"

Inisialisasi (di main.c setelah MX_USARTx_UART_Init):
┌────────────────────────────────────────────────────┐
│ // USART2 untuk komunikasi (PA2-TX, PA3-RX)        │
│ extern UART_HandleTypeDef huart2;                   │
│                                                    │
│ // Transmit blocking                               │
│ char msg[] = "Hello STM32\r\n";                     │
│ HAL_UART_Transmit(&huart2, (uint8_t*)msg,           │
│                   strlen(msg), HAL_MAX_DELAY);      │
│                                                    │
│ // Receive interrupt                                │
│ uint8_t rxByte;                                     │
│ HAL_UART_Receive_IT(&huart2, &rxByte, 1);           │
└────────────────────────────────────────────────────┘

Fungsi HAL UART:
• HAL_UART_Transmit()      - Kirim data (blocking)
• HAL_UART_Receive()       - Terima data (blocking)
• HAL_UART_Transmit_IT()   - Kirim via interrupt
• HAL_UART_Receive_IT()    - Terima via interrupt
• HAL_UART_RxCpltCallback() - Callback saat RX selesai

Notes: STM32CubeMX generates UART init code, HAL menyediakan API high-level
```

---

### Slide 16: ESP32 UART Overview
**Prompt:**
```
Buat slide overview UART pada ESP32.

Judul: "UART pada ESP32"

Fitur UART ESP32:
• 3 UART controllers: UART0, UART1, UART2
• Flexible pin mapping (GPIO matrix)
• Hardware flow control
• DMA support
• RS485 support
• Interrupt driven
• Up to 5 Mbps

Default Pin Mapping:
┌────────┬──────┬──────┬────────────────────┐
│ UART   │  TX  │  RX  │ Notes              │
├────────┼──────┼──────┼────────────────────┤
│ UART0  │ GPIO1│ GPIO3│ USB Serial (debug) │
│ UART1  │ GPIO10│GPIO9│ Flash pins!        │
│ UART2  │ GPIO17│GPIO16│ Safe to use       │
└────────┴──────┴──────┴────────────────────┘

⚠️ Warning: UART1 default pins connected to flash!
💡 Recommendation: Remap UART1 atau gunakan UART2

Blok diagram ESP32 UART dengan GPIO matrix
```

---

### Slide 17: ESP32 UART dengan ESP-IDF
**Prompt:**
```
Buat slide implementasi UART ESP32 dengan ESP-IDF framework.

Judul: "UART ESP32 dengan ESP-IDF Framework"

Inisialisasi:
┌────────────────────────────────────────────────────┐
│ #include "driver/uart.h"                            │
│                                                    │
│ const uart_config_t uart_cfg = {                    │
│     .baud_rate = 115200,                            │
│     .data_bits = UART_DATA_8_BITS,                  │
│     .parity    = UART_PARITY_DISABLE,               │
│     .stop_bits = UART_STOP_BITS_1,                  │
│     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,          │
│ };                                                  │
│                                                    │
│ uart_driver_install(UART_NUM_2, 256, 256, 0,        │
│                     NULL, 0);                       │
│ uart_param_config(UART_NUM_2, &uart_cfg);           │
│ uart_set_pin(UART_NUM_2, 17, 16, -1, -1);           │
└────────────────────────────────────────────────────┘

Fungsi utama ESP-IDF UART:
• uart_driver_install()     - Install UART driver
• uart_param_config()       - Set baud, parity, etc.
• uart_set_pin()            - Map TX/RX ke GPIO
• uart_write_bytes()        - Kirim data
• uart_read_bytes()         - Baca data (blocking + timeout)

 Pin remapping: uart_set_pin() allows flexible GPIO assignment!
```

---

### Slide 18: Komunikasi MCU-to-MCU
**Prompt:**
```
Buat slide komunikasi antara STM32 dan ESP32.

Judul: "Komunikasi STM32 ↔ ESP32 via UART"

Wiring Diagram:
┌───────────────┐                 ┌───────────────┐
│   STM32       │                 │    ESP32      │
│               │                 │               │
│  PA2 (TX) ●───┼────────────────►│● GPIO16 (RX) │
│               │                 │               │
│  PA3 (RX) ●◄──┼────────────────●│ GPIO17 (TX)  │
│               │                 │               │
│  GND      ●───┼────────────────●│ GND          │
└───────────────┘                 └───────────────┘

Poin penting:
✓ TX ke RX (cross connection)
✓ GND harus tersambung
✓ Logic levels: STM32 = 3.3V, ESP32 = 3.3V ✓ Compatible!

⚠️ JANGAN hubungkan ke 5V device tanpa level shifter!
```

---

### Slide 19: Data Exchange Protocols
**Prompt:**
```
Buat slide tentang protokol pertukaran data.

Judul: "Protocol Design untuk MCU Communication"

BASIC: Plain text
STM32 → "TEMP:25.5\n"
ESP32 ← Parse string

STRUCTURED: CSV format  
STM32 → "25.5,65,1024\n"
ESP32 ← Split by comma

JSON: Self-describing
STM32 → '{"temp":25.5,"hum":65}'
ESP32 ← Parse JSON object

BINARY: Efficient
┌────┬────────┬────────┬──────┐
│ ID │ Length │ Data   │ CRC  │
└────┴────────┴────────┴──────┘
→ Compact, faster, requires parsing

Trade-offs diagram:
Simple ◄─────────────────────► Complex
Fast   ◄─────────────────────► Flexible
Binary ◄─────────────────────► Human-readable
```

---

### Slide 20: Buffer dan Flow Control
**Prompt:**
```
Buat slide tentang buffer management dan flow control.

Judul: "Buffer Management & Flow Control"

UART Buffers:
┌─────────────────────────────────────────────────────┐
│  TX Buffer (FIFO)          RX Buffer (FIFO)         │
│  ┌───┬───┬───┬───┐         ┌───┬───┬───┬───┐       │
│  │ A │ B │ C │   │ →TX→    │ X │ Y │ Z │   │ ←RX←  │
│  └───┴───┴───┴───┘         └───┴───┴───┴───┘       │
│       Write →  Read →            ← Write  ← Read   │
└─────────────────────────────────────────────────────┘

Hardware Flow Control:
• RTS (Request To Send): "I'm ready to receive"
• CTS (Clear To Send): "You can send now"

Diagram:
Device A                    Device B
  RTS ─────────────────────► CTS
  CTS ◄───────────────────── RTS

Software Flow Control:
• XON (0x11): Resume transmission
• XOFF (0x13): Pause transmission
```

---

### Slide 21: Error Detection & Handling
**Prompt:**
```
Buat slide tentang error detection pada UART.

Judul: "Deteksi dan Penanganan Error UART"

Jenis Error:

1. FRAMING ERROR
   └─ Stop bit tidak terdeteksi
   └─ Penyebab: Baud rate mismatch

2. PARITY ERROR
   └─ Checksum tidak cocok
   └─ Penyebab: Noise, data corruption

3. OVERRUN ERROR
   └─ Data baru datang sebelum yang lama dibaca
   └─ Penyebab: Processing terlalu lambat

4. NOISE ERROR
   └─ Sampling di middle bit tidak konsisten
   └─ Penyebab: Signal integrity issues

Solutions visual:
• Verify baud rate match ✓
• Use parity for critical data ✓
• Read data promptly (interrupt) ✓
• Shield cables for noise ✓
• Use checksums (CRC) ✓
```

---

### Slide 22: CRC dan Checksum
**Prompt:**
```
Buat slide tentang CRC dan checksum.

Judul: "Error Detection: Checksum & CRC"

Simple Checksum:
Sum semua bytes, ambil LSB
Data: [0x01, 0x02, 0x03]
Sum = 0x01 + 0x02 + 0x03 = 0x06
Send: [0x01, 0x02, 0x03, 0x06]

XOR Checksum:
XOR semua bytes
Data: [0x12, 0x34, 0x56]
XOR = 0x12 ^ 0x34 ^ 0x56 = 0x50
Send: [0x12, 0x34, 0x56, 0x50]

CRC-16 (lebih robust):
┌─────────────────────────────────────────┐
│ uint16_t crc16(uint8_t *data, size_t len) {│
│     uint16_t crc = 0xFFFF;               │
│     while (len--) {                      │
│         crc ^= *data++;                  │
│         for (int i = 0; i < 8; i++)      │
│             crc = (crc >> 1) ^           │
│                 ((crc & 1) ? 0xA001 : 0);│
│     }                                    │
│     return crc;                          │
│ }                                        │
└─────────────────────────────────────────┘

Detection capability:
Simple checksum < XOR < CRC-16 < CRC-32
```

---

### Slide 23: Interrupt-driven UART
**Prompt:**
```
Buat slide tentang UART dengan interrupt.

Judul: "Interrupt-Driven UART Communication"

Polling vs Interrupt:

POLLING (blocking):
while (!(USART1->SR & USART_SR_RXNE)) {
    // Menunggu, CPU sibuk
}
data = USART1->DR;
→ Membuang CPU cycles

INTERRUPT (non-blocking):
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // Dipanggil otomatis saat byte diterima
    buffer[index++] = rxByte;
    HAL_UART_Receive_IT(huart, &rxByte, 1);  // Re-arm
}
→ CPU bebas untuk tugas lain

Diagram timeline:
POLLING:
CPU: [Wait][Wait][Wait][Read][Process]

INTERRUPT:
CPU: [Work][Work][Work][ISR][Work][Process]
         ↑ Interrupt triggered

Best practice: Gunakan interrupt + ring buffer
```

---

### Slide 24: Ring Buffer Implementation
**Prompt:**
```
Buat slide implementasi ring buffer.

Judul: "Ring Buffer untuk UART Data"

Struktur Ring Buffer:
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │
└───┴───┴───┴───┴───┴───┴───┴───┘
      ▲               ▲
     TAIL           HEAD
  (read here)    (write here)

Implementasi:
┌─────────────────────────────────────────────────────┐
│ #define BUFFER_SIZE 64                              │
│ uint8_t buffer[BUFFER_SIZE];                        │
│ volatile uint8_t head = 0, tail = 0;                │
│                                                     │
│ void push(uint8_t data) {                           │
│     uint8_t next = (head + 1) % BUFFER_SIZE;        │
│     if (next != tail) {  // Not full                │
│         buffer[head] = data;                        │
│         head = next;                                │
│     }                                               │
│ }                                                   │
│                                                     │
│ uint8_t pop() {                                     │
│     if (head != tail) {  // Not empty               │
│         uint8_t data = buffer[tail];                │
│         tail = (tail + 1) % BUFFER_SIZE;            │
│         return data;                                │
│     }                                               │
│ }                                                   │
└─────────────────────────────────────────────────────┘
```

---

### Slide 25: Debugging UART
**Prompt:**
```
Buat slide tips debugging komunikasi UART.

Judul: "Debugging UART Communication"

Tools untuk Debugging:

1. Serial Monitor (PlatformIO / ESP-IDF Monitor)
   - View incoming/outgoing data
   - Set baud rate
   - Line ending options

2. Logic Analyzer
   - Capture raw signals
   - Decode protocol
   - Measure timing

3. USB-to-Serial Adapter
   - Tap into communication line
   - Monitor traffic

Common Issues & Solutions:
┌────────────────────┬─────────────────────────────────┐
│ Problem            │ Solution                        │
├────────────────────┼─────────────────────────────────┤
│ Garbage data       │ Check baud rate match           │
│ No data received   │ Verify TX↔RX connections        │
│ Data corruption    │ Add GND connection              │
│ Intermittent data  │ Check cable/connections         │
│ Overflow errors    │ Increase buffer/speed up reads  │
└────────────────────┴─────────────────────────────────┘
```

---

### Slide 26: Best Practices
**Prompt:**
```
Buat slide best practices komunikasi UART.

Judul: "Best Practices UART Communication"

DO ✓
• Selalu hubungkan GND antar device
• Gunakan baud rate standard
• Implement timeout untuk receive
• Buffer incoming data
• Use start/end markers untuk framing
• Add checksum untuk reliability
• Test dengan loopback terlebih dahulu

DON'T ✗
• Jangan blocking wait untuk data
• Jangan mix 5V dan 3.3V tanpa level shifter
• Jangan assume data selalu valid
• Jangan ignore buffer overflow
• Jangan hardcode string parsing
• Jangan lupa handle partial messages

Protocol Design Tips:
💡 Use clear delimiters: STX/ETX atau newline
💡 Include length field untuk binary
💡 Always validate before processing
```

---

### Slide 27: Aplikasi UART
**Prompt:**
```
Buat slide aplikasi real-world UART.

Judul: "Aplikasi UART di Dunia Nyata"

EMBEDDED SYSTEMS:
• GPS Modules (NMEA protocol)
• Bluetooth Modules (AT commands)
• GSM/LTE Modems
• RFID Readers
• Barcode Scanners

INDUSTRIAL:
• PLC Communication
• Sensor Networks
• Motor Controllers
• Industrial Robots

CONSUMER:
• 3D Printers (G-code)
• Arduino shields
• Raspberry Pi serial
• Smart home devices

RS-232/RS-485 Standards:
• RS-232: Point-to-point, ±15V, <15m
• RS-485: Multi-drop, ±5V, <1200m

Visual: Photos atau icons dari masing-masing aplikasi
```

---

### Slide 28: Kesimpulan
**Prompt:**
```
Buat slide kesimpulan yang merangkum modul.

Judul: "Kesimpulan Modul Serial UART"

Key Takeaways:

📡 UART adalah komunikasi serial asynchronous paling sederhana

🔧 Parameter Konfigurasi:
   - Baud rate (kecepatan)
   - Data bits (5-9)
   - Parity (none/even/odd)
   - Stop bits (1/2)

⚡ STM32F103 memiliki 3 USART dengan fitur lengkap

📱 ESP32 memiliki 3 UART dengan flexible pin mapping

🔗 MCU-to-MCU communication: Cross TX-RX, common GND

💡 Best Practice:
   - Use protocol dengan framing
   - Implement error detection
   - Buffer management penting

Next: Praktikum implementasi UART pada STM32 dan ESP32!
```

---

## 📝 Notes untuk Presenter

1. **Demonstrasi Live:**
   - Tunjukkan Serial Monitor dengan data real
   - Demo loopback test (TX ke RX sendiri)
   - Logic analyzer capture jika tersedia

2. **Engagement:**
   - Quiz singkat tentang format 8N1
   - Hitung baud rate bersama
   - Identifikasi error dari waveform

3. **Waktu:**
   - Jangan terlalu lama di teori register
   - Fokus pada konsep dan praktik
   - Sisakan waktu untuk Q&A
