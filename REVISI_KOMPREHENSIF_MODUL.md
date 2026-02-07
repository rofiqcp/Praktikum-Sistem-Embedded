# 🎯 REVISI KOMPREHENSIF PRAKTIKUM SISTEM EMBEDDED
## ESP32 & STM32 - 14 Modul | 324+ Program Praktis

> **Edisi Lengkap dengan Panduan Praktis dari Kolban ESP32 & Mastering STM32 2nd Edition**

---

## 📋 DAFTAR ISI MODUL

### **MODUL 01: Dasar GPIO - Input/Output Digital & Kontrol Hardware**
### **MODUL 02: Interrupt & Timer - Respons Event & Penghitung Waktu**  
### **MODUL 03: Serial UART - Komunikasi Text-Based & Protokol**
### **MODUL 04: ADC - Akuisisi Signal Analog**
### **MODUL 05: DAC & PWM - Keluaran Analog & Kontrol Daya**
### **MODUL 06: I2C & Sensor - Bus 2-Wire & Perangkat Pintar**
### **MODUL 07: SPI & Storage - Bus Cepat & Media Penyimpanan**
### **MODUL 08: DMA - Transfer Data Zero-CPU-Overhead**
### **MODUL 09: FreeRTOS Task - Multi-Task & Scheduling**
### **MODUL 10: FreeRTOS Queue/Semaphore - Inter-Task Communication**
### **MODUL 11: FreeRTOS Timer & Notification - Waktu & Event Signaling**
### **MODUL 12: Memory Management & Advanced Patterns**
### **MODUL 13: Network & IoT - Wireless & Protokol Internet**
### **MODUL 14: Power Management - Efisiensi Energi & Sleep Modes**

---

# ═══════════════════════════════════════════════════════════════════════════════
# 🔌 MODUL 01: DASAR GPIO — INPUT/OUTPUT DIGITAL & KONTROL HARDWARE
# ═══════════════════════════════════════════════════════════════════════════════

## 📌 Ringkasan Modul
GPIO (General Purpose Input/Output) adalah **fondasi semua embedded system**. Setiap pin dapat dikonfigurasi sebagai:
- **Input**: Membaca state tombol, sensor digital, atau level logika
- **Output**: Mengendalikan LED, relay, buzzer, atau motor

Dalam modul ini Anda akan menguasai:
✓ Konfigurasi GPIO untuk input & output  
✓ Teknik debouncing (hardware & software)  
✓ Kontrol LED dengan efisiensi daya  
✓ Membaca sensor digital & button logic  
✓ State machine untuk aplikasi real-world  

---

## 🎓 Teori Singkat

### **STM32 GPIO (ARM Cortex-M3, 37 Pin)**
Dari buku "Mastering STM32 2nd Edition" (Chapter 6 - GPIO Management):
- **Port Structure**: 8 GPIO ports (PA - PH) × 16 pin/port = 128 GPIO maks
- **Mode Konfigurasi**: Input, Output (Push-Pull/Open-Drain), Alternate Function
- **Drive Strength**: Hingga 25mA/pin (total 180mA per port)
- **Register Control**: BSRR (Bit Set/Reset) untuk atomic operation tanpa read-modify-write

### **ESP32 GPIO (Xtensa LX6, 34 Pin)**
Dari buku "Kolban ESP32" (Pages 251-257):
- **Pin GPIO**: GPIO0-GPIO39 (beberapa khusus untuk strapping, boot, RTC)
- **Mode Konfigurasi**: Input, Output, Input+Output
- **Drive Current**: Hingga 40mA/pin (20mA recommended)
- **Special Features**: 
  - GPIO Matrix (bisa remap fungsi I/O)
  - RTC GPIO (tersedia dalam sleep mode)
  - Touch Sensor Pads
  - Pull-up/Pull-down otomatis

---

## 🛠️ Program & Deskripsi Praktis

### **Tingkat 1: DASAR - LED & Tombol Sederhana**

#### **Program 01: LED Blink Dasar — "Hello Hardware"**
```
Tujuan: Mengedipkan LED dengan periode tetap
Konsep: 
  - GPIO output mode
  - Timing dengan delay
  - Non-blocking approach dengan millis()
Platform: 
  - ESP32: GPIO2 → LED built-in
  - STM32: PC13 → LED built-in (catatan: active LOW pada BluePill)
Pembelajaran: Interaksi pertama dengan hardware, timing yang akurat
```

#### **Program 02: Multi LED Running Pattern — "Chase Effect"**
```
Tujuan: Menampilkan efek visual LED bergantian (seperti lampu rem mobil)
Konsep:
  - Array GPIO pins
  - Multiplexing waktu
  - Pattern state machine
Platform: 
  - ESP32: GPIO4, GPIO5, GPIO18, GPIO19 (atau sesuai board)
  - STM32: PC13, PB12, PB13, PB14 (sesuai board)
Pembelajaran: Sequencing, array handling, visual feedback
```

#### **Program 03: LED Breathing Effect — "Pulse Effect dengan PWM"**
```
Tujuan: LED naik-turun terang perlahan (seperti napas device)
Konsep:
  - PWM (Pulse Width Modulation) dasar
  - Kurva breathing (sine/exponential)
  - Frequency 1kHz, duty cycle 0-100%
Platform:
  - ESP32: LEDC peripheral (Built-in PWM engine)
  - STM32: Timer PWM (TIM1, TIM2, etc)
Aplikasi: Status indicator, ambient lighting
Pembelajaran: Continuous signal control, PWM principles
```

---

### **Tingkat 2: INTERMEDIATE - Button & Logic**

#### **Program 04: Button Debounce State Machine — "Solid Input Reading"**
```
Tujuan: Membaca button tanpa false trigger (bouncing)
Konsep:
  - Hardware debounce: 10-100µF capacitor + resistor
  - Software debounce: State machine (3-state)
    States: IDLE → PRESSED → RELEASED
  - Sampling jangka pendek (20ms per sample)
Platform:
  - ESP32: GPIO9 input dengan internal pull-up
  - STM32: GPIO14 input dengan internal pull-up
Pembelajaran: Practical input handling, state patterns
Referensi: Mastering STM32 hal 143-155 (EXTI configuration)
```

#### **Program 05: Long Press vs Short Press Detection — "Smart Button"**
```
Tujuan: Membedakan short click (0.5s) vs long press (1-2s)
Konsep:
  - Time measurement saat press terjadi
  - Action trigger tergantung durasi
  - State machine: IDLE → PRESSED → DETERMINE → ACTION
Platform:
  - ESP32: GPIO9 dengan timer measurement
  - STM32: EXTI + TIM untuk timing
Aplikasi: 
  - Double-tap untuk off
  - Long press untuk reset
  - Short press untuk toggle
Pembelajaran: Time-based state machine, user interface patterns
```

#### **Program 06: Toggle LED Latch Behavior — "Smart Switch"**
```
Tujuan: Tombol menghidupkan/mematikan LED (state tetap saat lepas)
Konsep:
  - Latch/toggle logic
  - Boolean state variable
  - Edge detection (rising edge trigger only)
Platform:
  - ESP32: 1 button → 1 LED toggle
  - STM32: 1 button → 1 LED toggle
Aplikasi: On/Off switch, relay control
Pembelajaran: State persistence, edge vs level triggering
```

---

### **Tingkat 3: ADVANCED - Protokol & Pattern**

#### **Program 07: GPIO Drive Strength Configuration — "Current Optimization"**
```
Tujuan: Mengatur kekuatan arus output untuk efisiensi daya
Konsep:
  - ESP32: drive_strength 0-3 (5mA, 10mA, 20mA, 40mA)
  - STM32: speed setting dalam GPIO config (low/medium/high)
  - Trade-off: Power vs EMI vs switching speed
Platform:
  - ESP32: gpio_set_drive_capability()
  - STM32: HAL_GPIO_Init() dengan GPIO_SPEED
Aplikasi: 
  - LED ringan → 5mA (hemat daya)
  - Motor/relay → 20-40mA
  - High-speed digital → high speed setting
Pembelajaran: Power optimization, EMI awareness
Referensi: Kolban p.251-257, Mastering STM32 hal 135-139
```

#### **Program 08: DIP Switch Reader — "Multi-Input Logic"**
```
Tujuan: Membaca 8 switch sekaligus (e.g., address selector, configuration)
Konsep:
  - Parallel input reading (1 byte)
  - Address decoding (0-255 kemungkinan)
  - Bit masking & shifting
Platform:
  - ESP32: GPIO9-GPIO16 (8 pin) → read as byte
  - STM32: PA8-PA15 → read as uint8_t
Aplikasi: Device addressing, configuration selector
Pembelajaran: Bit operations, address decoding
```

#### **Program 09: LED Brightness Control via Serial — "Interactive Dimmer"**
```
Tujuan: Kontrol terang-redup LED dari serial command (e.g., "SET 128")
Konsep:
  - Serial input parsing
  - PWM duty cycle mapping (0-255 → 0-100%)
  - Real-time adjustment
Platform:
  - ESP32: Serial + GPIO19 (PWM LED)
  - STM32: UART + PB0 (PWM LED)
Aplikasi: User dimmer interface, lab testing
Pembelajaran: Serial protocol integration, PWM control
```

---

## 📊 Tabel Perbandingan Platform

| Aspek | ESP32 | STM32 |
|-------|-------|-------|
| **GPIO Count** | 34 pin | 37 pin |
| **Tegangan I/O** | 3.3V | 3.3V (5V tolerant pada pins tertentu) |
| **Max Drive Current** | 40mA (20mA rec) | 25mA |
| **Push-Pull/Open-Drain** | Ya / Ya | Ya / Ya |
| **Pull-Up/Pull-Down** | Internal software-controlled | Internal hardware |
| **PWM Native** | LEDC peripheral | Timer (TIM1-4, dst) |
| **Special Features** | GPIO Matrix, Touch Sensor, RTC GPIO | Atomic BSRR register |
| **Boot Pins** | GPIO0, GPIO2, GPIO4 (strapping) | BOOT0 (strap to GND) |
| **LED Built-in** | GPIO2 (active HIGH) | PC13 (active LOW) |

---

## 🎯 Takeaway Poin Penting

1. **GPIO Output**: Untuk kontrol hardware (LED, relay, solenoid)
2. **GPIO Input**: Untuk membaca sensor digital (button, switch, Hall effect)
3. **Debouncing**: Hardware RC filter + software state machine adalah kombinasi terbaik
4. **PWM**: Untuk kontrol daya dan dimming (LED, motor speed)
5. **Atomic Operation**: Gunakan BSRR pada STM32 untuk set/reset yang aman dari interrupt
6. **Power Awareness**: Adjust drive strength sesuai kebutuhan
7. **Edge vs Level**: Edge trigger untuk button, level untuk sensor digital

---

# ═══════════════════════════════════════════════════════════════════════════════
# ⏰ MODUL 02: INTERRUPT & TIMER — RESPONS EVENT & PENGHITUNG WAKTU
# ═══════════════════════════════════════════════════════════════════════════════

## 📌 Ringkasan Modul
Interrupt adalah mekanisme hardware yang memungkinkan CPU **merespons event dengan cepat** tanpa harus polling terus-menerus.
Timer adalah peripheral untuk **mengukur waktu** dan membangkitkan event periodik (PWM, clock, alarm).

Dalam modul ini Anda akan menguasai:
✓ Interrupt (External, Timer-driven)  
✓ Interrupt priority & nesting  
✓ Timer modes (periodic, one-shot, PWM, input capture)  
✓ PWM generation untuk kontrol daya  
✓ ISR (Interrupt Service Routine) yang aman & efisien  

---

## 🎓 Teori Singkat

### **STM32 Interrupt (NVIC - Nested Vectored Interrupt Controller)**
Dari "Mastering STM32" (Chapter 7 - Interrupt Management):
- **16 Priority Levels**: 0 (highest) to 15 (lowest)
- **Nested Interrupt**: ISR dengan prioritas lebih tinggi bisa interrupt ISR sebelumnya
- **EXTI Lines**: 16 external interrupt lines (GPIO0-15)
- **Vector Table**: Tabel pointer ISR handler di memory
- **Latency**: Typical 12 clock cycle dari interrupt trigger → ISR start

### **ESP32 Interrupt (Interrupt Matrix)**
Dari "Kolban ESP32" (Pages 199-248):
- **71 Interrupt Sources**: WiFi, Bluetooth, Timer, UART, DMA, GPIO, etc
- **7 Priority Levels**: 1-7 (dapat dikonfigurasi per ISR)
- **Dual Core**: ISR bisa diarahkan ke Core 0 atau Core 1
- **IRAM_ATTR**: WAJIB untuk ISR agar kode disimpan di Internal RAM (fast access)
- **Critical Section**: portENTER_CRITICAL() untuk atomic operation

---

## 🛠️ Program & Deskripsi Praktis

### **Tingkat 1: DASAR - External Interrupt & Basic Timer**

#### **Program 01: GPIO Interrupt — "Button Event Handler"**
```
Tujuan: Memicu aksi saat button ditekan (edge-triggered)
Konsep:
  - External interrupt (EXTI pada STM32, GPIO interrupt pada ESP32)
  - Rising/falling/both edge detection
  - ISR callback
Platform:
  - STM32: EXTI0 (PA0) → GPIO interrupt
  - ESP32: GPIO9 → interrupt
Pembelajaran: Event-driven programming, ISR basics
Referensi: Mastering STM32 hal 148-158 (EXTI configuration)
```

#### **Program 02: GPIO Interrupt with Debounce — "Clean Button Reading"**
```
Tujuan: GPIO interrupt + software debouncing untuk input clean
Konsep:
  - ISR trigger → set flag
  - Main loop: cek flag, delay, cek lagi untuk debounce
  - Anti-jitter logic
Platform:
  - STM32: EXTI rising edge → ISR set flag → main debounce
  - ESP32: gpio_isr_register() → same logic
Pembelajaran: ISR + main loop coordination
```

#### **Program 03: Hardware Timer Interrupt — "Periodic Event"**
```
Tujuan: Timer trigger ISR setiap X milidetik (untuk LED blink, sensor polling)
Konsep:
  - Timer configuration: prescaler + period
  - STM32: PSC=(16-1), ARR=(1000-1) → 1ms interrupt @ 16MHz
  - ESP32: prescaler=80 → 1µs per tick → 1000 for 1ms
  - ISR handler: toggle LED atau update state
Platform:
  - STM32: TIM2 CCR1 interrupt every 1ms
  - ESP32: timer_create() + callback
Pembelajaran: Periodic interrupts, accurate timing
Referensi: Mastering STM32 hal 263-337 (Timers chapter)
```

#### **Program 04: One-Shot Timer — "Delayed Execution"**
```
Tujuan: Trigger action sekali setelah delay tertentu (e.g., relay off after 5 sec)
Konsep:
  - Timer mode: non-periodic
  - Load timer dengan count value
  - Start → ISR fire once → stop automatically
  - Reuse untuk next trigger
Platform:
  - STM32: HAL_TIM_OnePulse_Start_IT()
  - ESP32: timer_set_alarm_value() one time
Aplikasi: Auto-shutoff, timeout handler
Pembelajaran: Timer modes
```

---

### **Tingkat 2: INTERMEDIATE - PWM & Output Compare**

#### **Program 05: PWM 50Hz Basic — "Servo Signal"**
```
Tujuan: Generate 50Hz PWM (standard servo frequency)
Konsep:
  - Frequency 50Hz = period 20ms
  - Duty cycle: 1ms (0°), 1.5ms (90°), 2ms (180°)
  - STM32: TIM1 CCR1 mode
  - ESP32: LEDC channel
Platform:
  - STM32: TIM1 50Hz, CCR1 variable duty
  - ESP32: LEDC 50Hz, duty variable
Aplikasi: Servo motor control, PWM signal generation
Pembelajaran: PWM frequency/duty relationship
```

#### **Program 06: PWM Ramp — "Smooth Brightness"**
```
Tujuan: Gradually increase PWM duty 0→100→0 (e.g., LED dim-bright-dim)
Konsep:
  - Loop duty cycle dalam ISR atau timer
  - Linear ramp: duty += step each tick
  - Smooth effect untuk user perception
Platform:
  - STM32: Timer ISR modify CCR value
  - ESP32: LEDC ISR modify duty
Aplikasi: Soft-start motor, breathing effect
Pembelajaran: Dynamic PWM control
```

#### **Program 07: PWM Breathing Effect — "Organic Dim"**
```
Tujuan: LED breathing dengan curve sine/exponential (lebih smooth dari linear)
Konsep:
  - Sine/exp curve lookup table untuk natural feel
  - Frequency: ~1Hz (1 cycle per second)
  - Phase: 0-360°
Platform:
  - STM32: TIM2 trigger + sine LUT
  - ESP32: LEDC + sine calculation
Aplikasi: Status LED, ambient lighting, design aesthetic
Pembelajaran: Signal generation, curve fitting
```

#### **Program 08: Output Compare - Toggle Mode — "Frequency Generator"**
```
Tujuan: Generate square wave pada output pin (untuk testing/clock)
Konsep:
  - Output Compare: toggle pin setiap compare match
  - Frequency = clock / (2 × ARR × prescaler)
  - No ISR needed (hardware-driven toggle)
Platform:
  - STM32: TIM1 OC mode toggle → PA8 frequency output
  - ESP32: Similar using timer output pin
Aplikasi: Clock signal for external IC, frequency testing
Pembelajaran: Hardware vs software control trade-off
Referensi: Mastering STM32 hal 308-313 (Output Compare)
```

---

### **Tingkat 3: ADVANCED - Input Capture & Complex Patterns**

#### **Program 09: Input Capture — "Measure Pulse Width"**
```
Tujuan: Measure pulse width dari input signal (e.g., echo time dari ultrasonic)
Konsep:
  - Timer capture rising & falling edges
  - Calculate time difference → distance
  - STM32: TIM3 IC mode
  - ESP32: pulse_counter peripheral
Platform:
  - STM32: TIM3 dual channel IC
  - ESP32: pulse_counter_create() + watch()
Aplikasi: Ultrasonic distance, frequency measurement
Pembelajaran: Timing measurement, edge detection
Referensi: Mastering STM32 hal 300-307 (Input Capture)
```

#### **Program 10: Encoder Simulation — "Rotary Input"**
```
Tujuan: Read encoder signals (quadrature A/B) untuk posisi tracking
Konsep:
  - 2 signals 90° apart → 4 states per step
  - STM32: Encoder mode (automatic)
  - ESP32: Pulse counter channel tracking
Platform:
  - STM32: TIM2 encoder mode → reads PHASEA & PHASEB
  - ESP32: Pulse counter with 2 channels
Aplikasi: Rotary knob, motor shaft position
Pembelajaran: Quadrature signals, position feedback
```

---

## 📊 Tabel Timer Configuration

| Parameter | STM32F103 | ESP32 |
|-----------|-----------|-------|
| **Clock Frequency** | 16MHz APB | 80MHz |
| **Timer Count** | 4 × 16-bit | 4 × 64-bit |
| **Prescaler Range** | 0-65535 | 1-1023 |
| **Period (ARR/MAX)** | 0-65535 | Variable |
| **1µs Resolution** | 16MHz /16 = 16 ticks | 80MHz /80 = 1 tick |
| **50Hz Period Ticks** | 20000 ticks @ 1MHz | 1,600,000 ticks @ 1MHz |

---

## 🎯 Takeaway Poin Penting

1. **Interrupt vs Polling**: Interrupt jauh lebih efisien & responsif
2. **ISR Harus Singkat**: Jangan blocking operation dalam ISR
3. **volatile Keyword**: WAJIB untuk variabel shared ISR-main
4. **IRAM_ATTR (ESP32)**: Sangat penting agar ISR di RAM (cepat)
5. **Timer Formula**:
   - Interrupt Period = (prescaler+1) × (period+1) / clock_freq
6. **PWM Duty Cycle**: Persentase waktu signal HIGH dalam period
7. **Edge Triggering**: Untuk button, gunakan rising/falling edge
8. **Priority Management**: Assign priority sesuai urgency

---

