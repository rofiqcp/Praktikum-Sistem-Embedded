# KONTEN TAMBAHAN: BAB 4-14

Ini adalah konten lengkap untuk Bab 4 sampai Bab 14 yang akan digabungkan dengan file utama.

---

## BAB 4: UART/SERIAL COMMUNICATION

> **Tujuan Pembelajaran:**
> - Komunikasi serial asynchronous
> - Polling vs Interrupt vs DMA mode
> - Printf redirection untuk debugging
> - Binary data protocol
> - Error detection dan recovery

### 📖 PENJELASAN MATERI

**UART (Universal Asynchronous Receiver/Transmitter):**
Protokol serial komunikasi yang paling simple dan widely used untuk:
- Debugging (printf ke komputer)
- Komunikasi antar MCU
- GPS, Bluetooth modules
- Sensor serial (CO2, PM2.5, etc)

**Frame Format:**
```
START BIT (0) → DATA BITS (5-9) → PARITY (optional) → STOP BIT (1)
     ↓              ↓                   ↓                  ↓
   1 bit       LSB first         Error check          1-2 bits
```

**Baud Rate Common:**
- 9600:   Standard (reliable)
- 115200: High-speed debug
- 460800: Fast data transfer
- 921600: Very fast (check cable quality!)

**TX/RX Connection:**
```
Device A          Device B
  TX ─────────────→ RX
  RX ←───────────── TX
  GND ────────────── GND
```
⚠️ **CROSSOVER** wiring! TX to RX, bukan TX to TX!

---

### 🔗 COMMON GROUND: Fitur UART yang Sama

#### 1. **Basic Serial Print**
**Deskripsi:** Printf untuk debugging

**Contoh Program STM32:**
- **"UART Printf Retarget"** - Redirect printf ke UART1 (115200 baud)
- **"Float Printf"** - Enable float support di printf (tambahkan -u _printf_float di linker)
- **"Multi-UART Print"** - Print ke multiple UART sekaligus

**Contoh Program ESP32:**
- **"Serial Monitor Print"** - Printf built-in ESP32 (UART0)
- **"Dual UART Debug"** - UART0 untuk console, UART2 untuk data
- **"Colored Log Output"** - ESP_LOGI dengan warna (ESP-IDF)

#### 2. **Receiving Data (Polling)**
**Deskripsi:** Baca data blocking

**Contoh Program STM32:**
- **"UART Echo"** - Echo karakter kembali
- **"Command Parser"** - Parse command line ("LED ON", "LED OFF")
- **"Timeout Handling"** - Handle UART timeout gracefully

**Contoh Program ESP32:**
- **"UART Driver Read"** - uart_read_bytes() dengan timeout
- **"Ring Buffer Reception"** - Continuous receive dengan buffer
- **"AT Command Handler"** - Parse AT commands seperti modem

#### 3. **Interrupt-Driven RX**
**Deskripsi:** Non-blocking receive dengan callback

**Contoh Program STM32:**
- **"HAL_UART_Receive_IT"** - RX interrupt mode
- **"Circular Buffer ISR"** - ISR isi buffer, main loop parse
- **"Line-Based Reception"** - Detect '\\n' untuk complete command

**Contoh Program ESP32:**
- **"UART Event Queue"** - event-driven dengan FreeRTOS queue
- **"Pattern Detection"** - Hardware detect '\\n' atau '\\r'
- **"Error Event Handling"** - Handle FIFO overflow, parity error

#### 4. **Binary Protocol**
**Deskripsi:** Kirim struct/data binary (bukan text)

**Contoh Program STM32:**
- **"Struct Transfer"** - Kirim typedef struct via UART
- **"CRC16 Checksum"** - Binary protocol dengan error detection
- **"Packet Framing"** - Header (0xAA), Length, Data, CRC

**Contoh Program ESP32:**
- **"JSON over UART"** - Serialize JSON, kirim via UART
- **"Protobuf Serial"** - Google Protocol Buffers untuk efficiency
- **"Binary Sensor Data"** - Pack multiple sensors dalam 1 frame

#### 5. **Multi-UART Communication**
**Deskripsi:** Gunakan multiple UART ports

**Contoh Program STM32:**
- **"UART Router"** - Forward data UART1 ↔ UART2
- **"Multi-Slave Polling"** - Master poll 3 slaves via different UARTs
- **"Debug + Data Separation"** - UART1 debug, UART2 production data

**Contoh Program ESP32:**
- **"Triple UART Setup"** - UART0 (debug), UART1 (internal), UART2 (external)
- **"GPS + Debug UART"** - GPS di UART2, debug di UART0
- **"Modbus RTU Master"** - Modbus via UART dengan RS485 transceiver

---

### ⭐ STM32 UART: Keunggulan Unik

#### 1. **DMA Mode (Zero CPU Overhead)**
**Deskripsi:** Transfer tanpa CPU load

**Contoh Program:**
- **"DMA TX Large Buffer"** - Kirim 10KB data tanpa CPU blocking
- **"DMA RX Circular"** - Continuous RX, process saat idle
- **"Double Buffer DMA"** - Ping-pong buffer untuk real-time data

#### 2. **Hardware Flow Control (RTS/CTS)**
**Deskripsi:** Auto flow control untuk prevent data loss

**Contoh Program:**
- **"RTS/CTS Configuration"** - Enable hardware flow control
- **"High-Speed Reliable Transfer"** - 921600 baud dengan flow control
- **"Buffer Management"** - Monitor CTS untuk detect receiver busy

#### 3. **USART Synchronous Mode**
**Deskripsi:** Clock signal untuk precise timing

**Contoh Program:**
- **"USART + CLK Pin"** - Synchronous communication
- **"SPI-like USART"** - Use USART sebagai pseudo-SPI
- **"LCD 8-bit Sync"** - Drive parallel LCD via USART

#### 4. **LIN Mode (Automotive Bus)**
**Deskripsi:** Local Interconnect Network protocol

**Contoh Program:**
- **"LIN Master Send"** - LIN frame transmission
- **"LIN Slave Response"** - Listen dan respond ke master
- **"Automotive Sensor Bus"** - Multiple sensors on LIN

#### 5. **SmartCard Mode (ISO 7816)**
**Deskripsi:** Smart card reader interface

**Contoh Program:**
- **"SmartCard ATR Read"** - Read Answer To Reset
- **"EMV Card Reader"** - Payment card communication
- **"SIM Card Interface"** - GSM SIM card reader

#### 6. **Multi-Processor Mode**
**Deskripsi:** Addressed multi-drop network

**Contoh Program:**
- **"Multi-Drop Master"** - 1 master, 8 slaves dengan address
- **"Slave Address Detection"** - Mute mode, wake on own address
- **"RS485 Multi-Point"** - RS485 bus dengan addressing

---

### ⚡ ESP32 UART: Keunggulan Unik

#### 1. **Flexible Pin Mapping**
**Deskripsi:** UART ke pin manapun!

**Contoh Program:**
- **"Custom UART Pins"** - TX=GPIO25, RX=GPIO26 (bukan default)
- **"Conflict Resolution"** - Move UART jika pin bentrok
- **"Triple UART Custom Pins"** - Semua 3 UART di pins pilihan

#### 2. **Large Hardware FIFO (128 bytes)**
**Deskripsi:** Buffer hardware besar untuk reduce interrupt

**Contoh Program:**
- **"FIFO Threshold Interrupt"** - Interrupt saat FIFO 80% full
- **"Batch Processing"** - Read 100 bytes sekaligus
- **"Low Interrupt Frequency"** - Efficient CPU usage

#### 3. **Pattern Detection (Hardware)**
**Deskripsi:** Detect sequence di data stream

**Contoh Program:**
- **"Newline Detection"** - Hardware detect '\\n' untuk line parsing
- **"AT Command Parser"** - Detect "\\r\\n" untuk AT response
- **"Frame Delimiter"** - Detect 0xAA 0x55 header

#### 4. **RS485 Half-Duplex Mode**
**Deskripsi:** Built-in RS485 support

**Contoh Program:**
- **"RS485 Auto Direction"** - RTS pin auto-control transceiver
- **"Modbus RTU"** - Modbus over RS485
- **"Industrial Sensor Bus"** - RS485 multi-drop network

#### 5. **Inverse Signal Support**
**Deskripsi:** Invert TX/RX logic level

**Contoh Program:**
- **"Inverted UART"** - Interface inverted RS232
- **"IR Remote TX"** - Infrared transmission via inverted UART
- **"Custom Protocol"** - Inverted signal untuk compatibility

#### 6. **Wake from Light Sleep**
**Deskripsi:** UART activity wake MCU

**Contoh Program:**
- **"Sleep on Idle"** - Sleep saat tidak ada data, wake on RX
- **"Remote Wakeup"** - Kirim karakter untuk bangunkan ESP32
- **"Low-Power UART Logger"** - Sleep between log entries

---

### 📊 UART COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Instances** | 2-8 (series dependent) | 3 (UART0/1/2) |
| **Max Baud Rate** | Up to fPCLK/16 | 5 Mbps |
| **FIFO Size** | 1-32 bytes (series) | 128 bytes RX+TX |
| **DMA Support** | ✅ Full hardware DMA | ❌ No DMA |
| **HW Flow Control** | ✅ RTS/CTS | ✅ RTS/CTS |
| **Pin Flexibility** | ⚠️ Fixed AF | ✅ Any GPIO |
| **Pattern Detection** | ❌ No | ✅ Hardware |
| **RS485 Mode** | ⚠️ Manual RTS | ✅ Auto built-in |
| **Synchronous Mode** | ✅ USART CLK | ❌ No |
| **LIN/SmartCard** | ✅ Supported | ❌ No |
| **Signal Inversion** | ❌ Need hardware | ✅ Software |
| **Sleep Wakeup** | ✅ Via EXTI | ✅ Direct UART |

---

### 💼 MINI PROJECT: Wireless Sensor Network

**Deskripsi:** Master-slave sensor network

**STM32 Master:**
- Program "Multi-Slave Poller" - Poll 4 slaves setiap 5 detik
- Program "Data Aggregator" - Collect + average sensor data
- Program "SD Card Logger" - Log data ke SD card

**ESP32 Slave:**
- Program "Sensor Node" - Respond to master requests
- Program "Deep Sleep Between Polls" - Sleep untuk save power
- Program "Multi-Sensor Read" - Temperature, humidity, light

**Protocol:**
```
Master → Slave: [0xAA][SlaveID][CMD]
Slave → Master: [0xAA][SlaveID][Temp_H][Temp_L][Humid_H][Humid_L][CRC]
```

---

## BAB 5: ADC - ANALOG INPUT & SENSOR READING

> **Tujuan Pembelajaran:**
> - Membaca sensor analog (potensiometer, temperature, light)
> - ADC resolution & sampling time
> - Calibration & noise filtering
> - Multi-channel scanning
> - DMA continuous mode

### 📖 PENJELASAN MATERI

**ADC (Analog-to-Digital Converter):**
Mengubah voltage analog (0-3.3V) menjadi nilai digital:

```
12-bit ADC: 0-4095 steps
  3.3V → 4095
  1.65V → 2048
  0V   → 0
  
Resolution: 3.3V / 4095 = 0.8 mV per step
```

**Sampling Time:**
- Longer = More accurate (high impedance sources)
- Shorter = Faster conversion (but less accurate)

**Typical Sensors:**
- Potensiometer: 0-3.3V linear
- LDR (Light): Voltage divider dengan resistor
- LM35 Temperature: 10mV/°C
- TMP36: 10mV/°C (500mV offset di 0°C)

---

### 🔗 COMMON GROUND: Fitur ADC yang Sama

#### 1. **Single Channel Read (Potentiometer)**
**Deskripsi:** Baca potensiometer

**Contoh Program STM32:**
- **"HAL_ADC_Start + Poll"** - Baca ADC blocking
- **"Voltage Conversion"** - Convert ADC value ke voltage
- **"Percentage Mapping"** - Map 0-4095 ke 0-100%

**Contoh Program ESP32:**
- **"analogRead() Simple"** - Arduino style ADC read
- **"adc1_get_raw()"** - ESP-IDF native read
- **"Resolution Configuration"** - Set 9/10/11/12-bit

#### 2. **LDR (Light Sensor)**
**Deskripsi:** Detect light intensity

**Contoh Program STM32:**
- **"Light-Activated Switch"** - LED ON saat gelap
- **"Ambient Light Meter"** - Display lux value
- **"Auto-Brightness Control"** - Adjust LED brightness

**Contoh Program ESP32:**
- **"Smart Light"** - WiFi-enabled light automation
- **"Day/Night Detector"** - Schedule berdasarkan light
- **"Threshold Notification"** - MQTT alert saat gelap

#### 3. **Temperature Sensor (LM35)**
**Deskripsi:** Analog temperature measurement

**Contoh Program STM32:**
- **"LM35 Reader"** - °C calculation
- **"Temperature Logger"** - Log every 1 minute
- **"Over-Temperature Alarm"** - Buzzer saat > 40°C

**Contoh Program ESP32:**
- **"IoT Temperature Monitor"** - Upload ke cloud
- **"Temperature + WiFi Dashboard"** - Web interface
- **"Email Alert"** - Kirim email saat overheat

#### 4. **Multi-Channel Scanning**
**Deskripsi:** Read multiple sensors

**Contoh Program STM32:**
- **"ADC Scan Mode"** - 4 channels sequential
- **"Multi-Sensor Dashboard"** - LCD display all sensors
- **"Sensor Array"** - 8-channel data acquisition

**Contoh Program ESP32:**
- **"Multi-ADC Sequential"** - Read ADC1_CH4-7
- **"Sensor Fusion"** - Combine multiple analog inputs
- **"IoT Multi-Sensor Node"** - All data to cloud

#### 5. **Averaging Filter**
**Deskripsi:** Noise reduction

**Contoh Program STM32:**
- **"64-Sample Average"** - Stable readings
- **"Moving Average Filter"** - 10-sample buffer
- **"Median Filter"** - Reject outliers

**Contoh Program ESP32:**
- **"Multisampling + Calibration"** - esp_adc_cal
- **"Kalman Filter"** - Advanced filtering
- **"Exponential Moving Average"** - Smooth curve

---

### ⭐ STM32 ADC: Keunggulan Unik

#### 1. **DMA Continuous Mode**
**Deskripsi:** Zero-CPU high-speed sampling

**Contoh Program:**
- **"DMA Circular Buffer"** - 1000 samples continuous
- **"Audio Sampling"** - 44.1 kHz audio input
- **"Signal Analyzer"** - FFT analysis dari DMA data

#### 2. **Timer-Triggered ADC**
**Deskripsi:** Precise sampling timing

**Contoh Program:**
- **"1 kHz Sampling"** - TIM2 trigger ADC setiap 1 ms
- **"Data Acquisition System"** - Multi-channel + timer
- **"Oscilloscope Mode"** - Capture waveform

#### 3. **Internal Temperature Sensor**
**Deskripsi:** MCU die temperature

**Contoh Program:**
- **"Chip Temperature Monitor"** - Read internal sensor
- **"Thermal Management"** - Throttle saat overheat
- **"Temperature Compensation"** - Calibrate dengan temp

#### 4. **VREFINT Calibration**
**Deskripsi:** Measure actual VDD

**Contoh Program:**
- **"VDD Measurement"** - Calculate true VDD voltage
- **"Self-Calibration"** - Accurate ADC tanpa external ref
- **"Battery Monitor"** - Measure battery via VDD

#### 5. **Analog Watchdog**
**Deskripsi:** Hardware threshold monitoring

**Contoh Program:**
- **"Over-Voltage Detect"** - Interrupt saat > threshold
- **"Window Comparator"** - Alert jika outside range
- **"Safety Monitor"** - Critical parameter monitoring

#### 6. **16-bit ADC (STM32H7)**
**Deskripsi:** Ultra-high resolution

**Contoh Program:**
- **"Precision Measurement"** - 0.05 mV per step
- **"Strain Gauge"** - High-accuracy load cell
- **"High-End Instrumentation"** - Lab equipment

---

### ⚡ ESP32 ADC: Keunggulan Unik

#### 1. **Built-in Calibration (eFuse)**
**Deskripsi:** Factory-calibrated accuracy

**Contoh Program:**
- **"ESP ADC Calibration"** - esp_adc_cal_characterize()
- **"Two-Point Calibration"** - Use eFuse calibration data
- **"Linearization"** - Compensate non-linearity

#### 2. **Attenuation Control**
**Deskripsi:** Adjustable voltage range

**Contoh Program:**
- **"0-800mV Range"** - ADC_ATTEN_DB_0 (most linear)
- **"0-1.1V Range"** - ADC_ATTEN_DB_2_5
- **"0-3.3V Range"** - ADC_ATTEN_DB_11

#### 3. **I2S DMA Mode (Fast Sampling)**
**Deskripsi:** Up to 150 kHz continuous

**Contoh Program:**
- **"Audio Input Processing"** - Microphone sampling
- **"Signal Capture"** - Fast waveform digitization
- **"FFT Analysis"** - Frequency domain analysis

#### 4. **Hall Sensor (Built-in)**
**Deskripsi:** Magnetic field detection

**Contoh Program:**
- **"Magnetic Switch"** - hall_sensor_read()
- **"RPM Counter"** - Count magnet pulses
- **"Door Sensor"** - Detect open/close

#### 5. **ADC2 WiFi Sharing**
**Deskripsi:** ADC2 unusable dengan WiFi

**Contoh Program:**
- **"ADC1 Only with WiFi"** - Safe GPIO selection
- **"WiFi Stop Workaround"** - Temporarily disable WiFi
- **"Pin Planning"** - Route sensors ke ADC1 pins

---

### 📊 ADC COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Resolution** | 12-bit (16-bit H7) | 12-bit (13-bit S2/S3) |
| **Channels** | 16-24 | ADC1: 8, ADC2: 10 |
| **Max Speed** | 5 MSPS | 150 kSPS (I2S) |
| **DMA Support** | ✅ Hardware | ⚠️ Via I2S only |
| **Timer Trigger** | ✅ Hardware | ❌ Software |
| **Calibration** | ⚠️ Manual | ✅ Factory eFuse |
| **Analog Watchdog** | ✅ Hardware | ❌ Software |
| **Internal Temp** | ✅ Built-in | ❌ No |
| **VREF Measure** | ✅ VREFINT | ❌ No |
| **Voltage Range** | Fixed 0-VDDA | ✅ 4 attenuation levels |
| **Hall Sensor** | ❌ External | ✅ Built-in |
| **WiFi Conflict** | ✅ No issue | ⚠️ ADC2 blocked |

---

### 💼 MINI PROJECT: Weather Station

**Deskripsi:** Multi-sensor weather monitoring

**STM32 Implementation:**
- Program "4-Channel Logger" - Temp, Humidity, Light, Pressure
- Program "DMA Continuous Sampling" - 1 Hz data logging
- Program "SD Card Storage" - Save to FAT32 filesystem

**ESP32 Implementation:**
- Program "IoT Weather Station" - WiFi + MQTT
- Program "ThingSpeak Upload" - Cloud data logging
- Program "Web Dashboard" - Real-time chart display

**Sensors:**
- Temperature: LM35 / TMP36
- Humidity: Resistive sensor (voltage divider)
- Light: LDR / Phototransistor
- Pressure: MPX5700 (analog pressure sensor)

---

## BAB 6: TIMER, PWM & OUTPUT CONTROL

> **Tujuan Pembelajaran:**
> - Timer basics (counter, prescaler, period)
> - Generate PWM untuk motor/LED
> - Input capture untuk frequency measurement
> - Output compare untuk precise timing
> - Encoder interface

### 📖 PENJELASAN MATERI

**Timer = Counter + Clock:**
```
Timer Frequency = Clock / (Prescaler + 1)
Period = Timer Frequency / (Period + 1)

Example:
Clock = 84 MHz
Prescaler = 83 → Timer clock = 84MHz / 84 = 1 MHz
Period = 999 → PWM freq = 1MHz / 1000 = 1 kHz
```

**PWM (Pulse Width Modulation):**
```
Duty Cycle = (CCR / ARR) * 100%

     ┌──┐    ┌──┐    ┌──┐
ON   │  │    │  │    │  │
OFF──┘  └────┘  └────┘  └───
     ←→ ←─────→
     50%   25%
```

**Applications:**
- LED brightness control
- DC motor speed control
- Servo motor angle control
- Audio tone generation
- Heater power control

---

### 🔗 COMMON GROUND: Fitur Timer yang Sama

#### 1. **Basic PWM Output (LED Dimming)**
**Deskripsi:** Control LED brightness

**Contoh Program STM32:**
- **"TIM2 PWM PA0"** - 1 kHz PWM di pin PA0
- **"Breathing LED"** - Fade in/fade out effect
- **"Multi-Channel PWM"** - RGB LED control

**Contoh Program ESP32:**
- **"LEDC PWM"** - LED Control peripheral untuk PWM
- **"Smooth Fade"** - Hardware fade dengan ledc_set_fade()
- **"16-Channel PWM"** - Control 16 LED independent

#### 2. **DC Motor Speed Control**
**Deskripsi:** Variable speed control

**Contoh Program STM32:**
- **"H-Bridge Motor Driver"** - L298N control
- **"Speed Ramp"** - Gradual acceleration
- **"Direction Control"** - Forward/reverse/brake

**Contoh Program ESP32:**
- **"MCPWM Motor Control"** - Motor Control PWM peripheral
- **"Dual Motor Control"** - Left/right motors
- **"PID Speed Control"** - Closed-loop dengan encoder

#### 3. **Servo Motor Control**
**Deskripsi:** Angle positioning (RC servo)

**Contoh Program STM32:**
- **"SG90 Servo"** - 0-180° control
- **"Multi-Servo Controller"** - 4 servos simultaneous
- **"Servo Sweep"** - Scan 0-180° continuously

**Contoh Program ESP32:**
- **"ESP32Servo Library"** - Arduino-style servo
- **"MCPWM Servo"** - Native ESP-IDF servo
- **"WiFi-Controlled Servo"** - Remote control via web

#### 4. **Input Capture (Frequency Measurement)**
**Deskripsi:** Measure signal frequency

**Contoh Program STM32:**
- **"Frequency Counter"** - Input Capture mode
- **"RPM Measurement"** - Motor speed sensing
- **"Pulse Width Measurement"** - Duration timing

**Contoh Program ESP32:**
- **"RMT Peripheral"** - Remote Control RX
- **"PCNT Pulse Counter"** - Count pulses
- **"Frequency from GPIO"** - Software timing

#### 5. **Timer Interrupt (Periodic Tasks)**
**Deskripsi:** Execute function setiap interval

**Contoh Program STM32:**
- **"1 ms Tick Timer"** - TIM6 untuk system tick
- **"LED Blink Timer"** - 1 Hz timer interrupt
- **"Multiple Timers"** - TIM2 (1s), TIM3 (500ms)

**Contoh Program ESP32:**
- **"ESP Timer"** - High-resolution periodic timer
- **"FreeRTOS Timer"** - Software timer
- **"Hardware Timer"** - hw_timer_t untuk us precision

---

### ⭐ STM32 TIMER: Keunggulan Unik

#### 1. **32-bit Advanced Timers (TIM2/TIM5)**
**Deskripsi:** Long period tanpa overflow

**Contoh Program:**
- **"Long Duration Timer"** - 4.2 billion counts
- **"High-Resolution PWM"** - 32-bit duty cycle resolution
- **"Microsecond Delay"** - Precise timing functions

#### 2. **Complementary PWM (TIM1/TIM8)**
**Deskripsi:** PWM + inverted PWM (motor control)

**Contoh Program:**
- **"BLDC Motor Control"** - 6-step commutation
- **"Dead-Time Insertion"** - Prevent shoot-through
- **"Break Input"** - Emergency stop input

#### 3. **Quadrature Encoder Interface**
**Deskripsi:** Hardware encoder counting

**Contoh Program:**
- **"Rotary Encoder"** - TIM3 encoder mode
- **"Motor Position Control"** - Closed-loop position
- **"Velocity Calculation"** - Speed dari encoder pulses

#### 4. **One-Pulse Mode**
**Deskripsi:** Single precise pulse output

**Contoh Program:**
- **"Stepper Motor Step"** - Generate step pulses
- **"Camera Trigger"** - Precise timing trigger
- **"Ultrasonic Sensor"** - Generate 10us pulse

#### 5. **Timer Synchronization (Master/Slave)**
**Deskripsi:** Multiple timers synchronized

**Contoh Program:**
- **"Cascaded Timers"** - TIM2 triggers TIM3
- **"ADC + Timer Sync"** - Timer trigger ADC sampling
- **"PWM Synchronized Channels"** - Phase-locked PWM

#### 6. **DMA Burst Mode**
**Deskripsi:** Update timer registers via DMA

**Contoh Program:**
- **"Waveform Generation"** - Complex PWM patterns
- **"DMA-Based Frequency Change"** - Smooth transitions
- **"Multi-Step Sequence"** - Automated timing sequence

---

### ⚡ ESP32 TIMER: Keunggulan Unik

#### 1. **LEDC PWM (16 Channels)**
**Deskripsi:** LED Control dengan 16 independent channels

**Contoh Program:**
- **"16 LED Independent Control"** - Semua fade berbeda
- **"Hardware Fade"** - ledc_set_fade_with_time()
- **"RGB Strip Control"** - Multiple RGB LEDs

#### 2. **MCPWM (Motor Control PWM)**
**Deskripsi:** Dedicated motor control peripheral

**Contoh Program:**
- **"BLDC 3-Phase Control"** - Brushless DC motor
- **"H-Bridge Driver"** - Built-in dead-time
- **"Fault Detection"** - Hardware fault pin

#### 3. **RMT (Remote Control)**
**Deskripsi:** Flexible TX/RX untuk IR/NeoPixel

**Contoh Program:**
- **"WS2812B LED Strip"** - NeoPixel control
- **"IR Remote TX"** - Send NEC/RC5 codes
- **"IR Remote RX"** - Decode remote signals

#### 4. **PCNT (Pulse Counter)**
**Deskripsi:** Hardware pulse counting

**Contoh Program:**
- **"Frequency Counter"** - Count pulses up to 40 MHz
- **"Quadrature Decoder"** - Rotary encoder
- **"Flow Meter"** - Water/gas flow measurement

#### 5. **ESP Timer (High-Resolution)**
**Deskripsi:** 64-bit microsecond timer

**Contoh Program:**
- **"Microsecond Periodic Callback"** - esp_timer_create()
- **"One-Shot Timer"** - Single execution
- **"Timer Statistics"** - Track timer overhead

#### 6. **Pin Flexibility**
**Deskripsi:** PWM/Timer ke pin manapun

**Contoh Program:**
- **"Custom PWM Pins"** - Output ke GPIO pilihan
- **"Avoid Pin Conflicts"** - Route PWM freely
- **"Dynamic Pin Allocation"** - Runtime pin change

---

### 📊 TIMER/PWM COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Timer Instances** | 2-17 (series) | 4 timers + LEDC + MCPWM |
| **PWM Channels** | 4 per timer | LEDC: 16, MCPWM: 6 |
| **Resolution** | 16/32-bit | 16-bit (up to 20-bit LEDC) |
| **Max Frequency** | Clock dependent | 40 MHz (LEDC), 160 MHz (MCPWM) |
| **Complementary PWM** | ✅ TIM1/TIM8 | ✅ MCPWM |
| **Dead-Time Insert** | ✅ Hardware | ✅ MCPWM |
| **Encoder Interface** | ✅ Hardware | ✅ PCNT |
| **DMA Support** | ✅ Full DMA | ❌ No timer DMA |
| **Hardware Fade** | ❌ No | ✅ LEDC fade |
| **IR/NeoPixel** | ⚠️ Via software | ✅ RMT hardware |
| **Pin Flexibility** | ⚠️ AF fixed | ✅ Any GPIO |
| **Timer Sync** | ✅ Master/Slave | ❌ Limited |

---

### 💼 MINI PROJECT: Smart Fan Controller

**Deskripsi:** Temperature-controlled variable speed fan

**STM32 Implementation:**
- Program "PWM Fan Control" - 25kHz PWM untuk fan
- Program "Temperature-Based Speed" - ADC temperature → PWM duty
- Program "PID Controller" - Maintain setpoint temperature

**ESP32 Implementation:**
- Program "IoT Smart Fan" - WiFi control via mobile app
- Program "Web Dashboard" - Real-time temp + speed graph
- Program "Schedule Timer" - Auto ON/OFF based on time

**Features:**
- Temperature sensor (LM35)
- DC fan (12V, PWM control via MOSFET)
- OLED display (temp + speed percentage)
- Buttons untuk manual override
- WiFi remote control (ESP32 only)

---

*Lanjutan Bab 7-14 akan saya tambahkan di reply berikutnya karena panjang konten. Silakan konfirmasi jika format dan struktur ini sudah sesuai!*

## BAB 7: I²C PROTOCOL & DEVICE COMMUNICATION

> **Tujuan Pembelajaran:**
> - I²C protocol basics (master/slave, addressing)
> - Scan I²C bus untuk detect devices
> - Read/write sensors (OLED, RTC, EEPROM)
> - Multi-master arbitration
> - Clock stretching & error handling

### 📖 PENJELASAN MATERI

**I²C (Inter-Integrated Circuit):**
Bus komunikasi serial 2-wire:
- **SDA**: Serial Data (bidirectional)
- **SCL**: Serial Clock (dari master)
- **Pull-up resistors**: Required (4.7kΩ typical)

**Kecepatan:**
- Standard Mode: 100 kHz
- Fast Mode: 400 kHz  
- Fast Mode Plus: 1 MHz
- High Speed: 3.4 MHz (rare)

**Addressing:**
```
7-bit address: 0x00 - 0x7F (128 devices)
10-bit address: 0x000 - 0x3FF (extended)

Example devices:
  OLED SSD1306:   0x3C
  RTC DS3231:     0x68
  EEPROM 24C32:   0x50
  BMP280:         0x76 atau 0x77
```

**Transaction Format:**
```
START → [Address + R/W] → ACK → [Data byte] → ACK → ... → STOP
  ↓            ↓           ↓          ↓          ↓
Master    7-bit addr    Slave    Transfer    Master
         + 1 read bit   respond   data       release
```

---

### 🔗 COMMON GROUND: Fitur I²C yang Sama

#### 1. **I²C Scanner (Device Detection)**
**Deskripsi:** Scan semua address untuk find devices

**Contoh Program STM32:**
- **"HAL I2C Scanner"** - Scan 0x00-0x7F, print found addresses
- **"Device Identifier"** - Read device ID register
- **"Bus Diagnostic Tool"** - Check SDA/SCL levels

**Contoh Program ESP32:**
- **"Wire.begin() Scanner"** - Arduino I2C scanner
- **"I2C Driver Scanner"** - ESP-IDF i2c_master_probe()
- **"OLED Auto-Detect"** - Detect 0x3C or 0x3D

#### 2. **OLED Display (SSD1306)**
**Deskripsi:** 128x64 monochrome display

**Contoh Program STM32:**
- **"SSD1306 HAL Library"** - Display text + graphics
- **"Real-Time Sensor Display"** - Show ADC values
- **"Menu System"** - Button navigation UI

**Contoh Program ESP32:**
- **"Adafruit SSD1306"** - Arduino library
- **"u8g2 Graphics"** - Advanced graphics library
- **"WiFi Status Display"** - Show IP, signal strength

#### 3. **RTC (Real-Time Clock DS3231)**
**Deskripsi:** Accurate timekeeping

**Contoh Program STM32:**
- **"Set/Get Time"** - Write/read time registers
- **"Alarm Function"** - Configure alarm interrupts
- **"Temperature Compensated"** - Read internal temp

**Contoh Program ESP32:**
- **"RTC + NTP Sync"** - Sync dengan internet time
- **"Data Logger Timestamp"** - Add time to log entries
- **"Scheduler"** - Execute tasks at specific time

#### 4. **EEPROM (24C32/24C256)**
**Deskripsi:** Non-volatile data storage

**Contoh Program STM32:**
- **"EEPROM Write/Read"** - Store configuration
- **"Wear Leveling"** - Distribute writes evenly
- **"Factory Reset"** - Restore default settings

**Contoh Program ESP32:**
- **"Preferences vs EEPROM"** - NVS better alternative
- **"Config Storage"** - WiFi credentials storage
- **"User Settings"** - Persistent user data

#### 5. **Multi-Sensor Reading**
**Deskripsi:** Multiple I²C sensors on same bus

**Contoh Program STM32:**
- **"BME280 + OLED"** - Temp/humidity/pressure display
- **"Sensor Fusion"** - MPU6050 + BMP280
- **"Multi-Device Poll"** - Read 5 sensors sequentially

**Contoh Program ESP32:**
- **"I2C Sensor Array"** - 4 temperature sensors (different addresses)
- **"Weather Station"** - BME280 + BH1750 (light) + CCS811 (air quality)
- **"IoT Sensor Node"** - All data to MQTT

---

### ⭐ STM32 I²C: Keunggulan Unik

#### 1. **Hardware DMA Support**
**Deskripsi:** I²C transfer tanpa CPU blocking

**Contoh Program:**
- **"DMA I2C Transfer"** - HAL_I2C_Master_Transmit_DMA()
- **"Large Data Transfer"** - EEPROM 1KB write dengan DMA
- **"Continuous Sensor Read"** - DMA circular buffer

#### 2. **Dual Address Mode (Slave)**
**Deskripsi:** Respond to 2 addresses

**Contoh Program:**
- **"I2C Slave Dual Address"** - 0x50 dan 0x51
- **"Multi-Function Slave"** - Different functions per address
- **"I2C Bridge"** - Forward commands to UART

#### 3. **SMBus / PMBus Support**
**Deskripsi:** SMBus protocol (I²C variant)

**Contoh Program:**
- **"SMBus Battery Monitor"** - Read smart battery
- **"PEC (Packet Error Check)"** - CRC untuk reliability
- **"Alert Response"** - SMBus alert handling

#### 4. **Fast Mode Plus (1 MHz)**
**Deskripsi:** High-speed I²C

**Contoh Program:**
- **"1 MHz I2C Configuration"** - Fast communication
- **"High-Speed Data Acquisition"** - Rapid sensor reading
- **"Performance Benchmark"** - Compare speeds

#### 5. **Hardware Timeout**
**Deskripsi:** Automatic bus hang recovery

**Contoh Program:**
- **"Bus Timeout Config"** - Prevent infinite wait
- **"Error Recovery"** - Auto-reset on hang
- **"Reliable Communication"** - Handle slave failures

---

### ⚡ ESP32 I²C: Keunggulan Unik

#### 1. **Flexible Pin Mapping**
**Deskripsi:** I²C ke pin manapun

**Contoh Program:**
- **"Custom I2C Pins"** - SDA=GPIO21, SCL=GPIO22 (or anywhere!)
- **"Dual I2C Bus"** - I2C0 dan I2C1 di pins berbeda
- **"Avoid Pin Conflicts"** - Route around used pins

#### 2. **Two Independent I²C**
**Deskripsi:** I2C0 dan I2C1 simultan

**Contoh Program:**
- **"Dual Bus Operation"** - 2 separate I2C networks
- **"Master + Slave Mode"** - I2C0 master, I2C1 slave
- **"Isolated Sensors"** - Prevent address conflicts

#### 3. **Large TX/RX Buffer**
**Deskripsi:** Hardware buffer untuk burst transfer

**Contoh Program:**
- **"256 Byte Transfer"** - Write large data at once
- **"EEPROM Fast Write"** - Page write optimization
- **"Buffered Read"** - Reduce interrupt frequency

#### 4. **Built-in Slave Mode**
**Deskripsi:** Act as I²C slave device

**Contoh Program:**
- **"I2C Slave Peripheral"** - ESP32 sebagai sensor
- **"Remote Control Slave"** - Accept commands dari master
- **"I2C to UART Bridge"** - Protocol converter

#### 5. **Clock Stretching Support**
**Deskripsi:** Slave can hold SCL low

**Contoh Program:**
- **"Slow Slave Handling"** - Support old devices
- **"Clock Stretch Config"** - i2c_set_timeout()
- **"Robust Communication"** - Handle slow responses

---

### 📊 I²C COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Instances** | 1-4 (series) | 2 (I2C0, I2C1) |
| **Max Speed** | 1 MHz (Fm+) | 1 MHz (configurable) |
| **DMA Support** | ✅ Hardware DMA | ❌ No DMA |
| **Pin Flexibility** | ⚠️ AF mapping | ✅ Any GPIO |
| **Master Mode** | ✅ Full support | ✅ Full support |
| **Slave Mode** | ✅ Supported | ✅ Supported |
| **Dual Address** | ✅ Slave mode | ❌ No |
| **SMBus/PMBus** | ✅ Supported | ❌ No |
| **Buffer Size** | Small (6 bytes) | 256 bytes (configurable) |
| **Clock Stretching** | ✅ Supported | ✅ Supported |
| **10-bit Address** | ✅ Supported | ✅ Supported |
| **Multi-Master** | ✅ Arbitration | ✅ Arbitration |

---

### 💼 MINI PROJECT: Environmental Monitoring Station

**Deskripsi:** Multi-sensor I²C dashboard

**STM32 Implementation:**
- Program "Sensor Hub" - BME280 (temp/humidity) + BH1750 (light) + OLED display
- Program "Data Logger" - Save to EEPROM every 5 minutes
- Program "RTC Timestamp" - DS3231 real-time clock

**ESP32 Implementation:**
- Program "IoT Weather Station" - Same sensors + WiFi
- Program "Web Dashboard" - Chart.js visualization
- Program "MQTT Publisher" - Send to cloud every minute

**I²C Devices:**
- BME280 (0x76): Temperature, humidity, pressure
- BH1750 (0x23): Light intensity
- SSD1306 (0x3C): OLED display 128x64
- DS3231 (0x68): RTC dengan alarm
- 24C256 (0x50): 32KB EEPROM (optional)

---

## BAB 8: SPI PROTOCOL & HIGH-SPEED TRANSFER

> **Tujuan Pembelajaran:**
> - SPI full-duplex communication
> - Chip Select management (multiple slaves)
> - Read/write SD card (FAT filesystem)
> - NRF24L01 wireless module
> - High-speed data transfer (DMA)

### 📖 PENJELASAN MATERI

**SPI (Serial Peripheral Interface):**
Bus 4-wire full-duplex:
```
MOSI: Master Out, Slave In (TX dari master)
MISO: Master In, Slave Out (RX ke master)
SCK:  Serial Clock (dari master)
CS:   Chip Select (aktif LOW)
```

**Clock Polarity & Phase:**
```
Mode 0: CPOL=0, CPHA=0 (idle LOW, sample on rising)
Mode 1: CPOL=0, CPHA=1 (idle LOW, sample on falling)
Mode 2: CPOL=1, CPHA=0 (idle HIGH, sample on falling)
Mode 3: CPOL=1, CPHA=1 (idle HIGH, sample on rising)
```

**Kecepatan:**
- Typical: 1-10 MHz
- Fast: 10-50 MHz
- Ultra-fast: 50-100 MHz (short wires!)

**Multiple Slaves:**
```
Master                Slave 1
  MOSI ─────┬────────→ MOSI
  MISO ←────┼────────── MISO
  SCK  ─────┼────────→ SCK
  CS1  ─────┴────────→ CS

            └────────→ Slave 2
                       MOSI
              ←─────── MISO
            ─────────→ SCK
  CS2  ─────────────→ CS
```

---

### 🔗 COMMON GROUND: Fitur SPI yang Sama

#### 1. **Basic SPI Transfer**
**Deskripsi:** Send/receive data byte

**Contoh Program STM32:**
- **"HAL_SPI_Transmit"** - Send data blocking
- **"HAL_SPI_TransmitReceive"** - Full-duplex exchange
- **"SPI Speed Test"** - Benchmark transfer rate

**Contoh Program ESP32:**
- **"spi_device_transmit"** - ESP-IDF SPI transfer
- **"SPIClass::transfer()"** - Arduino SPI
- **"Loopback Test"** - Connect MOSI to MISO

#### 2. **SD Card (FAT Filesystem)**
**Deskripsi:** Read/write files di SD card

**Contoh Program STM32:**
- **"FatFs Mount SD"** - f_mount(), f_open(), f_read()
- **"Data Logger CSV"** - Append sensor data to file
- **"File Browser"** - List directory contents

**Contoh Program ESP32:**
- **"SD_MMC vs SD SPI"** - SD card via SPI or SDIO
- **"SPIFFS vs SD"** - Compare flash vs SD storage
- **"Audio WAV Player"** - Play WAV dari SD card

#### 3. **NRF24L01 (Wireless Module)**
**Deskripsi:** 2.4 GHz RF communication

**Contoh Program STM32:**
- **"NRF24 Library"** - nRF24L01+ transmit/receive
- **"Wireless Sensor Node"** - Send data via RF
- **"Multi-Node Network"** - 1 master, 5 slaves

**Contoh Program ESP32:**
- **"NRF24 + WiFi Gateway"** - RF to WiFi bridge
- **"Low-Power Remote"** - NRF24 untuk control
- **"Mesh Network"** - ESP-NOW alternative

#### 4. **TFT Display (ILI9341)**
**Deskripsi:** Color touchscreen LCD

**Contoh Program STM32:**
- **"TFT Graphics"** - Draw shapes, text, images
- **"Touch Input"** - Read touch coordinates
- **"GUI Dashboard"** - Sensor data visualization

**Contoh Program ESP32:**
- **"TFT_eSPI Library"** - High-performance graphics
- **"LVGL GUI"** - Professional UI framework
- **"Video Streaming"** - Display camera feed

#### 5. **External Flash (W25Q32)**
**Deskripsi:** SPI NOR flash memory

**Contoh Program STM32:**
- **"Flash Read/Write"** - Store large data
- **"Bootloader Update"** - Firmware storage
- **"Data Buffer"** - Circular buffer di flash

**Contoh Program ESP32:**
- **"External Flash Partition"** - esp_partition API
- **"OTA from External Flash"** - Firmware update
- **"Large Data Storage"** - Audio/image files

---

### ⭐ STM32 SPI: Keunggulan Unik

#### 1. **DMA Circular Mode**
**Deskripsi:** Continuous high-speed transfer

**Contoh Program:**
- **"DMA SPI Streaming"** - Transfer large buffers
- **"Display DMA Update"** - Fast screen refresh
- **"Audio DAC Output"** - Stream audio via SPI

#### 2. **Hardware NSS Management**
**Deskripsi:** Auto CS control

**Contoh Program:**
- **"Hardware CS Pin"** - Auto-toggle NSS
- **"Multi-Slave Auto"** - Hardware multiplexing
- **"Simplified Code"** - No manual GPIO toggle

#### 3. **SPI Master/Slave Mode**
**Deskripsi:** Flexible master atau slave

**Contoh Program:**
- **"SPI Slave Peripheral"** - Act as SPI slave
- **"SPI to SPI Bridge"** - Master ↔ Slave forward
- **"Data Acquisition Slave"** - External ADC control

#### 4. **TI Mode & Motorola Mode**
**Deskripsi:** Protocol variants

**Contoh Program:**
- **"TI Synchronous Serial"** - TI protocol mode
- **"Motorola SPI"** - Standard SPI mode
- **"Protocol Compatibility"** - Support old devices

#### 5. **CRC Hardware Calculation**
**Deskripsi:** Built-in error detection

**Contoh Program:**
- **"SPI CRC Check"** - Enable hardware CRC
- **"Reliable Transfer"** - Detect transmission errors
- **"Critical Data"** - Verify integrity

---

### ⚡ ESP32 SPI: Keunggulan Unik

#### 1. **Flexible GPIO Mapping**
**Deskripsi:** SPI pins anywhere

**Contoh Program:**
- **"Custom SPI Pins"** - MOSI=GPIO23, MISO=GPIO19, etc
- **"VSPI + HSPI"** - 2 independent SPI buses
- **"Pin Conflict Resolution"** - Route around used pins

#### 2. **HSPI & VSPI (Dual SPI)**
**Deskripsi:** 2 independent SPI peripherals

**Contoh Program:**
- **"VSPI for Display"** - TFT on VSPI
- **"HSPI for SD Card"** - Separate bus untuk SD
- **"Dual-Bus Performance"** - Parallel transfers

#### 3. **DMA-Like Transfer**
**Deskripsi:** Large buffer transfer

**Contoh Program:**
- **"Transaction Queue"** - Queue multiple SPI transactions
- **"Large Display Update"** - 76800 bytes (320x240 TFT)
- **"Background Transfer"** - Non-blocking DMA-style

#### 4. **Integrated SD/SDIO Controller**
**Deskripsi:** Dedicated SD card interface

**Contoh Program:**
- **"SDIO 4-bit Mode"** - High-speed SD (50 MHz)
- **"SD_MMC vs SPI"** - Compare performance
- **"Fast File Access"** - 10+ MB/s throughput

#### 5. **Slave HD Mode (Half-Duplex)**
**Deskripsi:** SPI slave command/data mode

**Contoh Program:**
- **"SPI Slave Device"** - ESP32 as slave peripheral
- **"Command Response"** - Handle master commands
- **"Large Data Transfer"** - Slave DMA buffer

---

### 📊 SPI COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Instances** | 1-6 (series) | 4 (SPI0/1/2/3, usable: 2/3) |
| **Max Speed** | 50 MHz (F4/F7: 90 MHz) | 80 MHz (40 MHz practical) |
| **DMA Support** | ✅ Full DMA | ⚠️ Transaction queue |
| **Pin Flexibility** | ⚠️ AF mapping | ✅ Any GPIO |
| **Master Mode** | ✅ Supported | ✅ Supported |
| **Slave Mode** | ✅ Supported | ✅ HD/FD mode |
| **Hardware CS** | ✅ NSS auto | ⚠️ Manual GPIO |
| **Dual SPI** | ⚠️ Series dependent | ✅ VSPI + HSPI |
| **CRC Hardware** | ✅ Built-in | ❌ No |
| **TI/Motorola Mode** | ✅ Supported | ✅ Motorola only |
| **SDIO Interface** | ⚠️ Separate | ✅ Integrated |
| **Transaction Queue** | ❌ No | ✅ 7-transaction FIFO |

---

### 💼 MINI PROJECT: Data Logger with Display

**Deskripsi:** Multi-sensor logger + TFT display

**STM32 Implementation:**
- Program "SPI TFT Display" - ILI9341 real-time graph
- Program "SD Card Logger" - FatFs write sensor CSV
- Program "Touch UI" - Button navigation menu

**ESP32 Implementation:**
- Program "IoT Logger" - TFT display + WiFi upload
- Program "SPIFFS + SD Dual Storage" - Local + removable
- Program "Web Config" - Configure via browser

**Hardware:**
- TFT 2.4" ILI9341 (SPI)
- MicroSD card module (SPI atau SDIO di ESP32)
- Multiple sensors (I²C)
- Touch screen (XPT2046 SPI)

---

*Bab 9-14 akan dilanjutkan...*
