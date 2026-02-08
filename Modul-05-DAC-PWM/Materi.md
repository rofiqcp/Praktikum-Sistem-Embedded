# Modul 05: Menghidupkan Sinyal — DAC & PWM Output

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami prinsip kerja DAC (Digital-to-Analog Converter) dan arsitekturnya
2. Memahami konsep PWM (Pulse Width Modulation) dan aplikasinya
3. Mengkonfigurasi dan memprogram DAC pada STM32 dan ESP32
4. Mengimplementasikan PWM untuk berbagai aplikasi (LED dimming, motor control, servo)
5. Membandingkan kelebihan dan kekurangan DAC vs PWM
6. Menerapkan teknik filtering untuk menghasilkan sinyal analog dari PWM
7. Melakukan debugging dan optimasi sistem DAC/PWM

---

## 📚 Materi Pembelajaran

### 1. Pendahuluan

Dalam sistem embedded, seringkali kita perlu menghasilkan sinyal analog dari mikrokontroler yang bekerja secara digital. Ada dua metode utama untuk menghasilkan output analog:

1. **DAC (Digital-to-Analog Converter)** - Konversi langsung nilai digital ke tegangan analog
2. **PWM (Pulse Width Modulation)** - Menggunakan sinyal digital dengan duty cycle variabel yang dapat difilter menjadi analog

Kedua metode memiliki karakteristik, kelebihan, dan aplikasi yang berbeda.

### 2. Teori Dasar DAC

#### 2.1 Prinsip Kerja DAC

DAC mengkonversi nilai digital (binary) menjadi tegangan analog proporsional.

```
Vout = Vref × (Digital_Value / 2^n)

Dimana:
- Vout = Tegangan output
- Vref = Tegangan referensi
- n = Resolusi bit DAC
- Digital_Value = Nilai digital input (0 sampai 2^n - 1)
```

**Contoh:** DAC 8-bit dengan Vref = 3.3V
- Input = 0 → Vout = 0V
- Input = 127 → Vout = 1.64V
- Input = 255 → Vout = 3.3V

#### 2.2 Arsitektur DAC

##### a) R-2R Ladder DAC
```
                    Vref
                     │
              ┌──────┼──────┐
              │      R      │
              │      │      │
         D3───┤     2R      │
              │      │      │
              │      R      │
              │      │      │
         D2───┤     2R      │
              │      │      │
              │      R      │
              │      │      │
         D1───┤     2R      │
              │      │      │
              │      R      │
              │      │      │
         D0───┤     2R      │
              │      │      │
              └──────┼──────┘
                     │
                   Vout
```

**Keuntungan R-2R:**
- Hanya memerlukan 2 nilai resistor
- Akurat dan linear
- Mudah dikaskade untuk resolusi lebih tinggi

##### b) Weighted Resistor DAC
```
        R
D3 ────/\/\/──┐
       2R     │
D2 ────/\/\/──┤
       4R     │    ┌────┐
D1 ────/\/\/──┼────│    │─── Vout
       8R     │    │Op  │
D0 ────/\/\/──┘    │Amp │
                   └────┘
```

##### c) Delta-Sigma DAC
- Menggunakan oversampling dan noise shaping
- Resolusi tinggi dengan komponen sederhana
- Bandwidth lebih rendah

#### 2.3 Spesifikasi Penting DAC

| Parameter | Deskripsi |
|-----------|-----------|
| **Resolution** | Jumlah bit (8, 10, 12-bit) |
| **INL (Integral Non-Linearity)** | Deviasi dari garis transfer ideal |
| **DNL (Differential Non-Linearity)** | Error step size antar kode |
| **Settling Time** | Waktu output mencapai nilai stabil |
| **Glitch Energy** | Transient saat perubahan kode |
| **SFDR (Spurious Free Dynamic Range)** | Rasio fundamental ke spurious terbesar |

### 3. DAC pada STM32F103

#### 3.1 Fitur DAC STM32F103

**STM32F103C8T6** memiliki:
- 2 channel DAC (DAC1 dan DAC2)
- Resolusi 12-bit
- Output pada PA4 (DAC_OUT1) dan PA5 (DAC_OUT2)
- Output buffer terintegrasi
- DMA support
- Trigger dari Timer atau Software

```
┌─────────────────────────────────────────────────┐
│                   STM32F103                      │
│                                                  │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐   │
│  │  DHR12R1 │───►│  DAC     │───►│  Buffer  │──►│──PA4
│  │(Data Reg)│    │ Converter│    │ (Op-Amp) │   │
│  └──────────┘    └──────────┘    └──────────┘   │
│       ▲                │                        │
│       │                │                        │
│  ┌────┴────┐     ┌─────▼─────┐                  │
│  │ DMA/CPU │     │ Trigger   │                  │
│  └─────────┘     │ (TIM/EXTI)│                  │
│                  └───────────┘                  │
└─────────────────────────────────────────────────┘
```

#### 3.2 Register DAC STM32

| Register | Fungsi |
|----------|--------|
| DAC_CR | Control Register - enable, trigger, wave gen |
| DAC_SWTRIGR | Software Trigger Register |
| DAC_DHR12R1 | Data Holding Register 12-bit right-aligned Ch1 |
| DAC_DHR12L1 | Data Holding Register 12-bit left-aligned Ch1 |
| DAC_DHR8R1 | Data Holding Register 8-bit right-aligned Ch1 |
| DAC_DOR1 | Data Output Register Channel 1 |

#### 3.3 Mode Trigger DAC

```c
// Sumber Trigger DAC STM32
typedef enum {
    DAC_TRIGGER_NONE     = 0x00,  // Tanpa trigger, langsung convert
    DAC_TRIGGER_T6_TRGO  = 0x00,  // Timer 6 TRGO
    DAC_TRIGGER_T8_TRGO  = 0x01,  // Timer 8 TRGO
    DAC_TRIGGER_T7_TRGO  = 0x02,  // Timer 7 TRGO
    DAC_TRIGGER_T5_TRGO  = 0x03,  // Timer 5 TRGO
    DAC_TRIGGER_T2_TRGO  = 0x04,  // Timer 2 TRGO
    DAC_TRIGGER_T4_TRGO  = 0x05,  // Timer 4 TRGO
    DAC_TRIGGER_EXTI9    = 0x06,  // External interrupt line 9
    DAC_TRIGGER_SOFTWARE = 0x07   // Software trigger
} DAC_Trigger_TypeDef;
```

#### 3.4 Konfigurasi DAC STM32 dengan HAL

```c
// Struktur konfigurasi DAC
DAC_HandleTypeDef hdac;
DAC_ChannelConfTypeDef sConfig;

void DAC_Init(void) {
    // Enable clock
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA4 sebagai Analog
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Konfigurasi DAC
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    // Konfigurasi Channel 1
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
    
    // Start DAC
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

// Set nilai DAC (0-4095 untuk 12-bit)
void DAC_SetValue(uint16_t value) {
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
}
```

### 4. DAC pada ESP32

#### 4.1 Fitur DAC ESP32

ESP32 memiliki:
- 2 channel DAC 8-bit
- DAC1 pada GPIO25
- DAC2 pada GPIO26
- Cosine Wave Generator terintegrasi
- DMA support untuk audio

```
┌─────────────────────────────────────────────────┐
│                     ESP32                        │
│                                                  │
│  ┌──────────────┐     ┌──────────┐              │
│  │ Digital Value│────►│ 8-bit    │────►GPIO25    │
│  │   (0-255)    │     │   DAC1   │              │
│  └──────────────┘     └──────────┘              │
│                                                  │
│  ┌──────────────┐     ┌──────────┐              │
│  │ Digital Value│────►│ 8-bit    │────►GPIO26    │
│  │   (0-255)    │     │   DAC2   │              │
│  └──────────────┘     └──────────┘              │
│                                                  │
│  ┌──────────────┐                                │
│  │Cosine Wave   │───► DAC1 atau DAC2            │
│  │ Generator    │                                │
│  └──────────────┘                                │
└─────────────────────────────────────────────────┘
```

#### 4.2 Kode DAC ESP32 (Arduino)

```cpp
#include <Arduino.h>
#include <driver/dac.h>

#define DAC_CHANNEL DAC_CHANNEL_1  // GPIO25

void setup() {
    Serial.begin(115200);
    dac_output_enable(DAC_CHANNEL);
}

void loop() {
    // Ramp up (0V to 3.3V)
    for (int i = 0; i < 256; i++) {
        dac_output_voltage(DAC_CHANNEL, i);
        delay(10);
    }
    
    // Ramp down (3.3V to 0V)
    for (int i = 255; i >= 0; i--) {
        dac_output_voltage(DAC_CHANNEL, i);
        delay(10);
    }
}
```

#### 4.3 Cosine Wave Generator ESP32

```cpp
#include <driver/dac.h>

void generateCosineWave(uint8_t channel, uint32_t frequency) {
    dac_cw_config_t cw_config = {
        .en_ch = (dac_channel_t)channel,
        .scale = DAC_CW_SCALE_1,     // Amplitude: full scale
        .phase = DAC_CW_PHASE_0,     // Phase: 0 degrees
        .freq = frequency,            // Frequency in Hz
        .offset = 0                   // DC offset
    };
    
    dac_cw_generator_config(&cw_config);
    dac_cw_generator_enable();
    dac_output_enable((dac_channel_t)channel);
}
```

### 5. Teori Dasar PWM

#### 5.1 Konsep PWM

PWM adalah teknik modulasi dimana lebar pulsa (duty cycle) diatur untuk mengontrol daya rata-rata yang dikirim ke beban.

```
Duty Cycle 25%:
    ┌──┐      ┌──┐      ┌──┐      ┌──┐
    │  │      │  │      │  │      │  │
────┘  └──────┘  └──────┘  └──────┘  └──────

Duty Cycle 50%:
    ┌────┐    ┌────┐    ┌────┐    ┌────┐
    │    │    │    │    │    │    │    │
────┘    └────┘    └────┘    └────┘    └────

Duty Cycle 75%:
    ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐
    │      │  │      │  │      │  │      │
────┘      └──┘      └──┘      └──┘      └──
```

**Formula:**
```
Duty Cycle (%) = (Ton / T) × 100%
Vavg = Vmax × Duty Cycle

Dimana:
- Ton = Waktu HIGH
- T = Periode (Ton + Toff)
- Vavg = Tegangan rata-rata
```

#### 5.2 Frekuensi PWM

Pemilihan frekuensi PWM sangat penting:

| Aplikasi | Frekuensi | Alasan |
|----------|-----------|--------|
| LED Dimming | 1-10 kHz | Tidak terlihat flicker (>100Hz) |
| Motor DC | 10-20 kHz | Mengurangi noise audio |
| Audio | 44.1 kHz+ | Sampling rate audio standard |
| Servo | 50 Hz | Standard hobby servo |
| Power Supply | 100 kHz+ | Komponen filter lebih kecil |

#### 5.3 Resolusi PWM

```
Resolusi = log2(Max_Count + 1)

Contoh:
- 8-bit: 0-255 (256 level)
- 10-bit: 0-1023 (1024 level)
- 16-bit: 0-65535 (65536 level)

Trade-off: Resolusi tinggi vs Frekuensi tinggi
Max_Freq = Clock / (2^Resolusi)
```

### 6. PWM pada STM32

#### 6.1 Timer untuk PWM

STM32 menggunakan Timer untuk generate PWM:

```
┌───────────────────────────────────────────────────────┐
│                    Timer Block                         │
│                                                        │
│  ┌─────────┐    ┌─────────┐    ┌──────────────────┐   │
│  │  Clock  │───►│ Prescaler│───►│   Counter (CNT)  │   │
│  │  (APB)  │    │  (PSC)   │    │                  │   │
│  └─────────┘    └─────────┘    └────────┬─────────┘   │
│                                         │              │
│                     ┌───────────────────┼───────────┐ │
│                     ▼                   ▼           │ │
│              ┌──────────┐        ┌──────────┐       │ │
│              │   ARR    │        │   CCR1   │       │ │
│              │(Period)  │        │(Compare) │       │ │
│              └────┬─────┘        └────┬─────┘       │ │
│                   │                   │             │ │
│              ┌────▼───────────────────▼────┐        │ │
│              │      Output Compare          │        │ │
│              │      Mode: PWM1/PWM2         │────────┼─┼─► PWM Output
│              └──────────────────────────────┘        │ │
└───────────────────────────────────────────────────────┘
```

#### 6.2 Konfigurasi PWM STM32 dengan HAL

```c
TIM_HandleTypeDef htim3;

void PWM_Init(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    // Enable clocks
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA6 (TIM3_CH1) sebagai Alternate Function
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Timer Configuration
    // PWM Frequency = Timer_Clock / ((PSC + 1) * (ARR + 1))
    // Example: 72MHz / (72 * 1000) = 1 kHz
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;          // 72MHz / 72 = 1MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;           // 1MHz / 1000 = 1kHz
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);
    
    // PWM Channel Configuration
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;                   // 50% duty cycle
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    
    // Start PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

// Set duty cycle (0-1000 untuk period 1000)
void PWM_SetDutyCycle(uint16_t duty) {
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}
```

#### 6.3 Mode PWM

```c
// PWM Mode 1: Active saat CNT < CCR
// PWM Mode 2: Active saat CNT >= CCR

/*
PWM Mode 1 (TIM_OCMODE_PWM1):
    ┌────────┐
    │        │
    │   CCR  │
────┘        └────────────────
    |<--Ton->|<----Toff----->|
    |<-------Period--------->|

PWM Mode 2 (TIM_OCMODE_PWM2):
             ┌────────────────
             │
    CCR      │
────────────┘
    |<-Toff->|<----Ton------>|
*/
```

### 7. PWM pada ESP32

#### 7.1 LEDC (LED Control) Peripheral

ESP32 memiliki LEDC peripheral yang didesain untuk LED control tetapi dapat digunakan untuk PWM umum:

```
┌───────────────────────────────────────────────────┐
│                 ESP32 LEDC                         │
│                                                    │
│  High Speed Channels (0-7):                        │
│  ┌────────┐    ┌────────┐    ┌────────┐           │
│  │Timer 0 │───►│ Ch 0-1 │───►│GPIO Out│           │
│  └────────┘    └────────┘    └────────┘           │
│  ┌────────┐    ┌────────┐    ┌────────┐           │
│  │Timer 1 │───►│ Ch 2-3 │───►│GPIO Out│           │
│  └────────┘    └────────┘    └────────┘           │
│  ...                                               │
│                                                    │
│  Low Speed Channels (8-15):                        │
│  ┌────────┐    ┌────────┐    ┌────────┐           │
│  │Timer 0 │───►│ Ch 8-9 │───►│GPIO Out│           │
│  └────────┘    └────────┘    └────────┘           │
│  ...                                               │
└───────────────────────────────────────────────────┘
```

**Fitur LEDC ESP32:**
- 16 channel independen
- 4 timer (0-3) per speed mode
- Resolusi 1-20 bit
- Hardware fade support
- Frekuensi hingga 40 MHz

#### 7.2 Kode PWM ESP32 (Arduino)

```cpp
#include <Arduino.h>

#define PWM_PIN      25
#define PWM_CHANNEL  0
#define PWM_FREQ     5000
#define PWM_RESOLUTION 8  // 0-255

void setup() {
    // Configure PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
}

void loop() {
    // Fade in
    for (int duty = 0; duty <= 255; duty++) {
        ledcWrite(PWM_CHANNEL, duty);
        delay(10);
    }
    
    // Fade out
    for (int duty = 255; duty >= 0; duty--) {
        ledcWrite(PWM_CHANNEL, duty);
        delay(10);
    }
}
```

#### 7.3 Hardware Fade ESP32

```cpp
#include <driver/ledc.h>

void setup() {
    // Configure timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    // Configure channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = 25,
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    
    // Install fade function
    ledc_fade_func_install(0);
}

void fadeToTarget(uint32_t target_duty, int fade_time_ms) {
    ledc_set_fade_time_and_start(
        LEDC_HIGH_SPEED_MODE,
        LEDC_CHANNEL_0,
        target_duty,
        fade_time_ms,
        LEDC_FADE_WAIT_DONE
    );
}
```

### 8. Aplikasi PWM

#### 8.1 Kontrol Motor DC

```cpp
// H-Bridge motor control dengan PWM
#define MOTOR_PWM_PIN  25
#define MOTOR_DIR_A    26
#define MOTOR_DIR_B    27

void motorSetSpeed(int speed) {
    // speed: -255 to 255
    if (speed >= 0) {
        // Forward
        digitalWrite(MOTOR_DIR_A, HIGH);
        digitalWrite(MOTOR_DIR_B, LOW);
        ledcWrite(0, speed);
    } else {
        // Reverse
        digitalWrite(MOTOR_DIR_A, LOW);
        digitalWrite(MOTOR_DIR_B, HIGH);
        ledcWrite(0, -speed);
    }
}
```

#### 8.2 Kontrol Servo Motor

```
Servo Signal Timing:
┌────┐                              ┌────┐
│    │                              │    │
│0.5ms│ = 0° (min)                  │    │
└────┴──────────────────────────────┘    └───

┌────────┐                          ┌────────┐
│        │                          │        │
│ 1.5ms  │ = 90° (center)           │        │
└────────┴──────────────────────────┘        └─

┌────────────┐                      ┌───────────
│            │                      │
│   2.5ms    │ = 180° (max)         │
└────────────┴──────────────────────┘

Period = 20ms (50Hz)
```

```cpp
// ESP32 Servo Control
#include <ESP32Servo.h>

Servo myServo;

void setup() {
    myServo.attach(25);  // Servo pada GPIO25
}

void loop() {
    // Sweep 0 to 180 degrees
    for (int angle = 0; angle <= 180; angle++) {
        myServo.write(angle);
        delay(15);
    }
    
    // Sweep 180 to 0 degrees
    for (int angle = 180; angle >= 0; angle--) {
        myServo.write(angle);
        delay(15);
    }
}
```

#### 8.3 PWM sebagai Pseudo-DAC

```cpp
// PWM + Low-pass filter = Pseudo DAC
// RC Filter: R = 10kΩ, C = 100nF
// Cutoff frequency: fc = 1/(2πRC) = 159 Hz

/*
PWM Out ─────/\/\/\──────┬────── Analog Out
              R          │
                         C
                         │
                        GND
*/

// Untuk audio quality yang baik:
// - Gunakan PWM frequency >> audio frequency
// - Gunakan multiple RC stages untuk filtering lebih baik
// - Atau gunakan active low-pass filter
```

### 9. Perbandingan DAC vs PWM

| Aspek | DAC | PWM |
|-------|-----|-----|
| **Resolusi** | Biasanya 8-12 bit | Dapat >16 bit |
| **Bandwidth** | DC hingga MHz | Terbatas oleh filter |
| **Ripple** | Sangat rendah | Ada ripple (perlu filter) |
| **Pin Requirement** | Dedicated DAC pin | Any GPIO |
| **Power Efficiency** | Moderate | Tinggi (switching) |
| **Cost** | Perlu DAC peripheral | Hanya timer |
| **Audio Quality** | Lebih baik | Perlu filtering |

### 10. Best Practices

#### 10.1 DAC Best Practices

```c
// 1. Gunakan buffer untuk menghindari loading effect
HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

// 2. Gunakan DMA untuk waveform generation
HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)sine_wave, 
                  SINE_SAMPLES, DAC_ALIGN_12B_R);

// 3. Synchronize dengan timer untuk timing akurat
sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;

// 4. Pertimbangkan settling time
// STM32F103: ~3µs settling time
```

#### 10.2 PWM Best Practices

```c
// 1. Pilih frekuensi sesuai aplikasi
// LED: 1-10 kHz, Motor: 10-20 kHz, Audio: >40 kHz

// 2. Gunakan dead-time untuk H-bridge
// Mencegah shoot-through current
TIM_BDTRInitStruct.DeadTime = 100;  // Dead time cycles

// 3. Soft-start untuk motor
void motorSoftStart(int targetSpeed) {
    for (int i = 0; i <= targetSpeed; i += 5) {
        PWM_SetDutyCycle(i);
        delay(10);
    }
}

// 4. Gunakan complementary output untuk full-bridge
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
```

### 11. Troubleshooting

| Problem | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| DAC output tidak stabil | Loading effect | Enable output buffer |
| PWM glitch saat update | Non-atomic update | Gunakan shadow register |
| Motor noise | PWM freq dalam audio range | Tingkatkan ke >20kHz |
| Servo jitter | Interrupt mengganggu timing | Gunakan hardware PWM |
| DAC stepping | Resolusi kurang | Gunakan dithering |

### 12. Rangkuman

1. **DAC** mengkonversi nilai digital langsung ke tegangan analog dengan kualitas tinggi
2. **PWM** menggunakan duty cycle untuk mengontrol daya rata-rata
3. **STM32F103** memiliki 2-channel 12-bit DAC dan multiple timer untuk PWM
4. **ESP32** memiliki 2-channel 8-bit DAC dan 16-channel LEDC untuk PWM
5. Pilihan antara DAC dan PWM tergantung pada aplikasi:
   - DAC: Audio, precision analog, fast response
   - PWM: LED dimming, motor control, power efficient
6. PWM dapat difilter menjadi pseudo-DAC dengan RC filter

---

## 📊 Diagram Perbandingan Platform

```
┌────────────────────────────────────────────────────────────────┐
│                    DAC & PWM Comparison                         │
├──────────────────────┬─────────────────────┬───────────────────┤
│      Feature         │     STM32F103       │      ESP32        │
├──────────────────────┼─────────────────────┼───────────────────┤
│ DAC Channels         │         2           │        2          │
│ DAC Resolution       │      12-bit         │      8-bit        │
│ DAC Pins             │    PA4, PA5         │   GPIO25, 26      │
│ DAC DMA              │        Yes          │       Yes         │
│ Waveform Generator   │    Triangle/Noise   │   Cosine          │
├──────────────────────┼─────────────────────┼───────────────────┤
│ PWM Timers           │    TIM1-4,5,8       │    LEDC           │
│ PWM Channels         │    Up to 32         │       16          │
│ PWM Resolution       │     16-bit          │    1-20 bit       │
│ Hardware Fade        │        No           │       Yes         │
│ Complementary PWM    │   TIM1, TIM8        │       No          │
└──────────────────────┴─────────────────────┴───────────────────┘
```

---

## 📖 Referensi

1. STM32F103 Reference Manual (RM0008)
2. ESP32 Technical Reference Manual
3. "Mastering STM32" by Carmine Noviello - Chapter 11: DAC
4. "PWM Techniques: A Pure Sine Wave Inverter" - Application Note
5. ESP-IDF Programming Guide - LEDC PWM Controller
6. AN3126: Audio and waveform generation using DAC in STM32

---

## 🔗 Link Terkait

- [Modul 04: ADC](../Modul-04-ADC/Materi.md) - Input Analog
- [Modul 06: I2C](../Modul-06-I2C-Sensor/Materi.md) - Komunikasi dengan sensor
- [Modul 08: DMA](../Modul-08-DMA/Materi.md) - Transfer data efisien untuk DAC

