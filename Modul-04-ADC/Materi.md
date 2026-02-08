# Modul 04: Menguak Dunia Analog — ADC (Analog-to-Digital Converter)

## Praktikum Sistem Embedded

---

## 1. Pendahuluan

Analog to Digital Converter (ADC) adalah komponen fundamental dalam sistem embedded yang mengkonversi sinyal analog kontinu menjadi representasi digital diskret. ADC memungkinkan mikrokontroler membaca besaran fisik seperti tegangan, suhu, cahaya, dan tekanan dari sensor-sensor analog.

### 1.1 Mengapa ADC Penting?

Dunia nyata bersifat analog — suhu berubah secara kontinu, cahaya memiliki intensitas yang bervariasi halus, dan tekanan udara berfluktuasi secara gradual. Namun mikrokontroler bekerja secara digital (0 dan 1). ADC menjadi jembatan antara dunia analog dan digital.

**Contoh aplikasi ADC:**
- Membaca posisi potensiometer (knob volume)
- Mengukur suhu dari sensor NTC/LM35
- Mendeteksi intensitas cahaya dari LDR
- Monitoring tegangan baterai
- Membaca sensor gas (MQ-series)
- Audio digitization

---

## 2. Teori Dasar ADC

### 2.1 Proses Konversi Analog ke Digital

Proses konversi ADC terdiri dari 3 tahap:

```
Sinyal Analog → [Sampling] → [Quantization] → [Encoding] → Data Digital
```

#### a) Sampling (Pencuplikan)
Mengambil nilai sesaat dari sinyal analog pada interval waktu tertentu. Frekuensi sampling (fs) menentukan seberapa sering sinyal dicuplik.

**Nyquist-Shannon Sampling Theorem:**

$$f_s \geq 2 \times f_{max}$$

Frekuensi sampling harus minimal 2 kali frekuensi tertinggi sinyal yang ingin didigitalkan. Jika tidak terpenuhi, terjadi **aliasing**.

#### b) Quantization (Kuantisasi)
Memetakan nilai analog kontinu ke level diskret terdekat. Jumlah level ditentukan oleh resolusi ADC.

$$L = 2^n$$

Dimana:
- $L$ = jumlah level kuantisasi
- $n$ = resolusi dalam bit

| Resolusi | Level | Langkah (3.3V Vref) |
|----------|-------|---------------------|
| 8-bit    | 256   | 12.89 mV            |
| 10-bit   | 1024  | 3.22 mV             |
| 12-bit   | 4096  | 0.806 mV            |
| 16-bit   | 65536 | 0.0503 mV           |

#### c) Encoding (Pengkodean)
Mengubah level kuantisasi menjadi kode biner.

### 2.2 Formula Konversi

**Raw ADC ke Tegangan:**

$$V_{analog} = \frac{raw \times V_{ref}}{2^n - 1}$$

**Tegangan ke Raw ADC:**

$$raw = \frac{V_{analog} \times (2^n - 1)}{V_{ref}}$$

Contoh untuk ADC 12-bit dengan Vref = 3.3V:

$$V = \frac{raw \times 3300}{4095} \text{ (dalam mV)}$$

### 2.3 Karakteristik ADC

#### Signal-to-Noise Ratio (SNR)

$$SNR_{dB} = 6.02n + 1.76 \text{ dB}$$

Dimana $n$ = resolusi bit. Untuk 12-bit ADC: SNR = 6.02 × 12 + 1.76 = 74 dB

#### Quantization Error

$$Q_e = \frac{V_{ref}}{2^{n+1}}$$

#### DNL dan INL
- **DNL (Differential Non-Linearity)**: Deviasi ukuran langkah aktual dari ideal
- **INL (Integral Non-Linearity)**: Deviasi kumulatif dari garis transfer ideal

---

## 3. ADC pada ESP32 (ESP-IDF)

### 3.1 Arsitektur ADC ESP32

ESP32 memiliki **2 modul ADC** dengan SAR (Successive Approximation Register):

| Fitur | ADC1 | ADC2 |
|-------|------|------|
| Jumlah Channel | 8 (CH0-CH7) | 10 (CH0-CH9) |
| GPIO | GPIO32-39 | GPIO0,2,4,12-15,25-27 |
| Resolusi | 9-12 bit | 9-12 bit |
| WiFi Conflict | ❌ Tidak | ⚠️ **YA!** |
| DMA Support | ✅ Ya | Terbatas |

### ⚠️ PERINGATAN PENTING: ADC2 dan WiFi

> **ADC2 pada ESP32 TIDAK DAPAT digunakan bersamaan dengan WiFi!**
> Ketika WiFi aktif, ADC2 dikuasai oleh driver WiFi.
> Pembacaan ADC2 akan menghasilkan nilai acak atau error.
>
> **Solusi:** Selalu gunakan ADC1 (GPIO32-39) untuk pembacaan analog yang membutuhkan WiFi.

### 3.2 Pin Mapping ADC1 ESP32

| Channel | GPIO | Catatan |
|---------|------|---------|
| ADC1_CH0 | GPIO36 | Input only, no pull-up (SVP) |
| ADC1_CH1 | GPIO37 | Input only |
| ADC1_CH2 | GPIO38 | Input only |
| ADC1_CH3 | GPIO39 | Input only (SVN) |
| ADC1_CH4 | GPIO32 | Bisa juga output |
| ADC1_CH5 | GPIO33 | Bisa juga output |
| ADC1_CH6 | GPIO34 | Input only |
| ADC1_CH7 | GPIO35 | Input only |

### 3.3 Atenuasi (Attenuation)

| Atenuasi | Konstanta | Rentang Tegangan | Penggunaan |
|----------|-----------|------------------|------------|
| 0 dB | ADC_ATTEN_DB_0 | ~0 - 1.1V | Sensor tegangan rendah |
| 2.5 dB | ADC_ATTEN_DB_2_5 | ~0 - 1.5V | |
| 6 dB | ADC_ATTEN_DB_6 | ~0 - 2.2V | |
| 11 dB | ADC_ATTEN_DB_11 | ~0 - 3.3V | Potentiometer, sensor umum |

### 3.4 Non-Linearitas dan Kalibrasi ADC ESP32

ADC ESP32 memiliki **non-linearitas** signifikan terutama pada tegangan mendekati 0V dan 3.3V.

**Metode Kalibrasi:**
1. **eFuse Vref** — Tegangan referensi dikalibrasi di pabrik
2. **eFuse Two Point** — Dua titik kalibrasi (lebih akurat)
3. **Default Vref** — Nilai default 1100 mV jika tidak ada data kalibrasi

### 3.5 API ESP-IDF Legacy

```c
#include "driver/adc.h"
#include "esp_adc_cal.h"

// Konfigurasi
adc1_config_width(ADC_WIDTH_BIT_12);
adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);

// Pembacaan
int raw = adc1_get_raw(ADC1_CHANNEL_6);

// Kalibrasi
esp_adc_cal_characteristics_t chars;
esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11,
                          ADC_WIDTH_BIT_12, 1100, &chars);
uint32_t voltage_mv;
esp_adc_cal_raw_to_voltage(raw, &chars, &voltage_mv);
```

### 3.6 API ESP-IDF New (ADC Oneshot)

```c
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

adc_oneshot_unit_handle_t adc_handle;
adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
adc_oneshot_new_unit(&unit_cfg, &adc_handle);

adc_oneshot_chan_cfg_t chan_cfg = {
    .bitwidth = ADC_BITWIDTH_12,
    .atten = ADC_ATTEN_DB_11,
};
adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &chan_cfg);

int raw;
adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &raw);
```

### 3.7 Mode Continuous (DMA)

```c
#include "esp_adc/adc_continuous.h"

adc_continuous_handle_t adc_handle;
adc_continuous_handle_cfg_t handle_cfg = {
    .max_store_buf_size = 1024,
    .conv_frame_size = 256,
};
adc_continuous_new_handle(&handle_cfg, &adc_handle);

adc_continuous_config_t config = {
    .sample_freq_hz = 20000,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
};
adc_continuous_config(adc_handle, &config);
adc_continuous_start(adc_handle);
```

### 3.8 Pin Mapping ESP32-S2 dan ESP32-S3

| Platform | ADC1 GPIO | ADC2 GPIO | Catatan |
|----------|-----------|-----------|---------|
| ESP32 | 32-39 | 0,2,4,12-15,25-27 | ADC2 konflik WiFi |
| ESP32-S2 | 1-10 | 11-20 | Lebih banyak channel |
| ESP32-S3 | 1-10 | 11-20 | Mirip S2 |

---

## 4. ADC pada STM32 (HAL)

### 4.1 Arsitektur ADC STM32

#### STM32F103 (Blue Pill)
| Fitur | Spesifikasi |
|-------|-------------|
| Jumlah ADC | 2 unit (ADC1, ADC2) |
| Resolusi | 12-bit (0-4095) |
| Kecepatan Max | 1 MSPS (14 MHz ADC clock) |
| Channel Eksternal | 10 (PA0-PA7, PB0-PB1) |
| Channel Internal | Temperature, Vrefint |
| Vref | 3.3V (VDDA) |

#### STM32F401/F411 (Black Pill)
| Fitur | Spesifikasi |
|-------|-------------|
| Jumlah ADC | 1 unit (ADC1) |
| Resolusi | 6/8/10/12-bit selectable |
| Kecepatan Max | 2.4 MSPS |
| Channel Eksternal | 16 |
| Channel Internal | Temperature, Vrefint, Vbat |

### 4.2 Mode Operasi ADC STM32

#### Single Conversion
```c
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
uint32_t value = HAL_ADC_GetValue(&hadc1);
HAL_ADC_Stop(&hadc1);
```

#### Continuous Mode
- ADC terus-menerus konversi tanpa henti
- Set `ContinuousConvMode = ENABLE`

#### Scan Mode
- Memindai beberapa channel berurutan
- Digunakan dengan DMA untuk efisiensi

#### Injected Mode
- Interupsi konversi regular untuk prioritas tinggi
- Hingga 4 injected channel

### 4.3 Sampling Time STM32F103

| Setting | Siklus | Waktu @14MHz |
|---------|--------|--------------|
| 1CYCLE_5 | 1.5 | 0.107 µs |
| 7CYCLES_5 | 7.5 | 0.536 µs |
| 13CYCLES_5 | 13.5 | 0.964 µs |
| 28CYCLES_5 | 28.5 | 2.036 µs |
| 41CYCLES_5 | 41.5 | 2.964 µs |
| 55CYCLES_5 | 55.5 | 3.964 µs |
| 71CYCLES_5 | 71.5 | 5.107 µs |
| 239CYCLES_5 | 239.5 | 17.107 µs |

Total waktu konversi = sampling time + 12.5 cycles.

### 4.4 Analog Watchdog

```c
ADC_AnalogWDGConfTypeDef wdg = {0};
wdg.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
wdg.HighThreshold = 3000;
wdg.LowThreshold = 1000;
wdg.Channel = ADC_CHANNEL_0;
wdg.ITMode = ENABLE;
HAL_ADC_AnalogWDGConfig(&hadc1, &wdg);
```

### 4.5 ADC dengan DMA

```c
uint16_t adc_buffer[256];
HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 256);
```

DMA memungkinkan transfer data ADC ke memori tanpa intervensi CPU.

### 4.6 Channel Internal STM32

| Channel | Deskripsi | Rumus |
|---------|-----------|-------|
| TEMPSENSOR | Sensor suhu internal | T = (V_sense - V_25) / Avg_Slope + 25 |
| VREFINT | Tegangan referensi 1.2V | VDDA = 3300 × CAL / raw |
| VBAT | Monitor baterai | V_bat = raw × 3300 / 4095 × 2 |

---

## 5. Perbandingan ESP32 vs STM32 ADC

| Fitur | ESP32 | STM32F103 | STM32F4 |
|-------|-------|-----------|---------|
| Resolusi | 9-12 bit | 12 bit | 6-12 bit |
| ADC Unit | 2 | 2 | 1 |
| Channel Ext. | 18 | 10 | 16 |
| Max SPS | ~100k | 1M | 2.4M |
| DMA | ✅ | ✅ | ✅ |
| Kalibrasi | eFuse | Self-cal | Factory |
| Watchdog | Software | Hardware | Hardware |
| Linearitas | ⚠️ Kurang | ✅ Baik | ✅ Baik |
| WiFi Issue | ⚠️ ADC2 | N/A | N/A |
| Int. Temp | Dedicated | ADC ch | ADC ch |

---

## 6. Teknik Filtering ADC

### 6.1 Moving Average Filter

$$y[n] = \frac{1}{N} \sum_{i=0}^{N-1} x[n-i]$$

```c
#define N 16
int buf[N]; int idx = 0; long sum = 0;
int moving_avg(int val) {
    sum -= buf[idx]; buf[idx] = val; sum += val;
    idx = (idx + 1) % N;
    return sum / N;
}
```

### 6.2 Exponential Moving Average (EMA)

$$y[n] = \alpha \cdot x[n] + (1 - \alpha) \cdot y[n-1]$$

```c
float ema = 0; float alpha = 0.1;
float ema_filter(int val) {
    ema = alpha * val + (1.0 - alpha) * ema;
    return ema;
}
```

### 6.3 Median Filter

Efektif menghilangkan spike noise — ambil nilai median dari N sampel terakhir.

### 6.4 Oversampling

4x sampel = +1 bit resolusi efektif. 16 sampel → 14-bit dari 12-bit ADC.

---

## 7. Aplikasi Praktis

### 7.1 Voltage Divider untuk Battery Monitor

```
V_bat ──[R1=10kΩ]──┬──[R2=10kΩ]── GND
                    │
                  GPIO_ADC
```

V_adc = V_bat × R2/(R1+R2) = V_bat × 0.5; V_bat = V_adc × 2

### 7.2 LDR (Light Dependent Resistor)

```
3.3V ──[R=10kΩ]──┬──[LDR]── GND
                  │
                GPIO_ADC
```

### 7.3 NTC Thermistor

Menggunakan persamaan Beta:

$$T = \frac{1}{\frac{1}{T_0} + \frac{1}{\beta} \ln\left(\frac{R}{R_0}\right)}$$

---

## 8. Daftar Program Praktikum

### ESP32 (12 Program)

| No | Program | Hardware | Konsep |
|----|---------|----------|--------|
| 01 | ADC_Single_Read | Pot 10kΩ | Single-shot dasar |
| 02 | ADC_Voltage_Display | Pot | Raw → volt, kalibrasi |
| 03 | ADC_Moving_Average | Pot | Filter N=16/32 |
| 04 | ADC_Multi_Channel | 2× Pot | Scan 2 channel |
| 05 | ADC_Calibration | Pot + multimeter | eFuse calibration |
| 06 | ADC_Continuous_DMA | Pot | DMA continuous |
| 07 | ADC_Threshold_Alert | Pot + LED + Buzzer | Threshold trigger |
| 08 | ADC_Battery_Monitor | Voltage divider | Monitor baterai |
| 09 | ADC_Temperature_Internal | — | Sensor suhu internal |
| 10 | ADC_Sampling_Rate | Pot | Benchmark SPS |
| 11 | ADC_Light_Sensor | LDR + 10kΩ | Sensor cahaya |
| 12 | ADC_Statistical_Analysis | Pot | Min/max/avg/stdev |

### STM32 (12 Program)

| No | Program | Hardware | Konsep |
|----|---------|----------|--------|
| 01 | ADC_Single_Read | Pot 10kΩ | HAL polling |
| 02 | ADC_Voltage_Display | Pot | Raw → mV |
| 03 | ADC_Moving_Average | Pot | Filter |
| 04 | ADC_Multi_Channel | 2× Pot | Scan mode |
| 05 | ADC_Calibration | Pot + multimeter | HAL calibration |
| 06 | ADC_Continuous_DMA | Pot | DMA circular |
| 07 | ADC_Threshold_Alert | Pot + LED | Analog Watchdog |
| 08 | ADC_Battery_Monitor | Voltage divider | Monitor baterai |
| 09 | ADC_Temperature_Internal | — | Internal temp |
| 10 | ADC_Sampling_Rate | Pot | Benchmark SPS |
| 11 | ADC_Light_Sensor | LDR + 10kΩ | Sensor cahaya |
| 12 | ADC_Statistical_Analysis | Pot | Statistik noise |

---

## 9. Tips & Best Practices

1. **Selalu gunakan ADC1** pada ESP32 jika membutuhkan WiFi
2. **Kalibrasi** ADC ESP32 untuk akurasi yang lebih baik
3. **Gunakan filter** (moving average minimal) untuk pembacaan stabil
4. **Perhatikan impedansi sumber** — impedansi tinggi butuh sampling time lebih lama
5. **Decoupling capacitor** pada VDDA/VREF untuk mengurangi noise
6. **Hindari switching noise** — jangan sampling saat motor/relay switching
7. **Multisampling** untuk meningkatkan resolusi efektif
8. **Gunakan DMA** untuk sampling rate tinggi
9. **Ground plane** yang baik mengurangi noise
10. **Dokumentasikan** pin mapping dan konfigurasi atenuasi

---

## 10. Referensi

1. ESP-IDF ADC Documentation — docs.espressif.com
2. Kolban's Book on ESP32 — Pages 308-310
3. Mastering STM32 2nd Edition — Chapter 12: ADC
4. STM32F1xx Reference Manual (RM0008) — Chapter 11
5. STM32F4xx Reference Manual (RM0383) — Chapter 13
6. AN2834: How to get the best ADC accuracy in STM32

---

*Modul disusun untuk Praktikum Sistem Embedded — Teknik Otomasi*
