# BAB 04: Analog-to-Digital Converter

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami konsep dasar Analog-to-Digital Converter
2. Mengimplementasikan program Analog-to-Digital Converter pada STM32 dan ESP32
3. Melakukan debugging dan troubleshooting
4. Menerapkan best practices dalam pengembangan embedded systems

---

## 📚 Materi Pembelajaran

### Pendahuluan

# Modul 04: Analog to Digital Converter (ADC)

## 1. Konsep Dasar Sinyal Analog dan Digital

Di dunia nyata, hampir semua besaran fisik bersifat **analog** (kontinyu), seperti suhu, cahaya, suara, dan tekanan. Namun, mikrokontroler adalah perangkat **digital** yang hanya mengerti logika 0 dan 1 (diskrit). Untuk dapat memproses sinyal analog tersebut, diperlukan antarmuka yang disebut **Analog to Digital Converter (ADC)**.

### 1.1 Sampling dan Kuantisasi

Proses konversi sinyal analog ke digital melibatkan dua tahap utama:

1.  **Sampling (Pencuplikan):** Mengambil nilai sinyal analog pada interval waktu tertentu (Perioda Sampling, $T_s$). Frekuensi sampling ($f_s$) harus memenuhi **Teorema Nyquist**, yaitu minimal 2x frekuensi maksimum sinyal input ($f_s \ge 2f_{max}$) untuk menghindari *aliasing*.
2.  **Quantization (Kuantisasi):** Memetakan nilai amplitudo yang dicuplik ke level diskrit terdekat berdasarkan resolusi ADC.

### 1.2 Resolusi ADC

Resolusi menentukan seberapa presisi ADC dapat membedakan perubahan tegangan input. Resolusi dinyatakan dalam **bit ($n$)**. Jumlah level diskrit adalah $2^n$.

Rumus dasar pembacaan ADC:

$$ ADC\_Value = \frac{V_{in}}{V_{ref}} \times (2^n - 1) $$

Atau untuk mencari tegangan dari nilai ADC:

$$ V_{in} = \frac{ADC\_Value}{2^n - 1} \times V_{ref} $$

**Contoh:**
ADC 12-bit ($2^{12} = 4096$ level, range 0-4095) dengan $V_{ref} = 3.3V$.
Jika $V_{in} = 1.65V$, maka:
$$ ADC = \frac{1.65}{3.3} \times 4095 \approx 2047 $$
Step size (Voltage per bit) = $3.3V / 4095 \approx 0.8 mV$.

---

## 2. ADC pada STM32F103C8T6 (Blue Pill)

STM32F103 memiliki dua unit ADC 12-bit (ADC1 dan ADC2) yang canggih dengan arsitektur **Successive Approximation Register (SAR)**.

### 2.1 Spesifikasi ADC STM32
-   **Resolusi:** 12-bit (Konfigurasi: 12, 10, 8, atau 6 bit).
-   **Channel:** Hingga 16 channel eksternal + 2 internal (Temperature sensor, Vrefint).
-   **Range tegangan:** $0V$ sampai $V_{DDA}$ ($3.3V$).
-   **Waktu Konversi:** ~1 $\mu s$ pada clock ADC 14 MHz.
-   **Mode Konversi:**
    -   *Single Mode:* Konversi satu kali lalu stop.
    -   *Continuous Mode:* Konversi terus menerus.
    -   *Scan Mode:* Mengkonversi grup channel secara berurutan.
    -   *Injected Mode:* Konversi prioritas tinggi (seperti interrupt untuk ADC).

### 2.2 Alignment Data
Hasil konversi 12-bit disimpan dalam register 16-bit, bisa **Right Aligned** (standar) atau **Left Aligned**.

### 2.3 Metode Pembacaan di Arduino Framework
Library STM32duino memudahkan pembacaan ADC.

```cpp
// Konfigurasi resolusi (default Arduino biasanya 10-bit, STM32 bisa 12-bit)
analogReadResolution(12); // Set output 0-4095

int val = analogRead(PA0);
float voltage = val * (3.3 / 4095.0);
```

Pin ADC STM32F103C8T6: PA0-PA7 (ADC12_IN0 - ADC12_IN7), PB0-PB1 (ADC12_IN8 - ADC12_IN9).

---

## 3. ADC pada ESP32

ESP32 memiliki dua unit SAR ADC 12-bit: **ADC1** dan **ADC2**. Namun, ADC ESP32 memiliki karakteristik non-linear dan batasan tertentu yang perlu diperhatikan.

### 3.1 Peta Pin ADC ESP32
-   **ADC1:** 8 Channel (GPIO 32-39). **Aman digunakan kapan saja.**
-   **ADC2:** 10 Channel (GPIO 0, 2, 4, 12-15, 25-27). **Tidak bisa digunakan saat WiFi aktif.** Driver WiFi memonopoli ADC2.

### 3.2 Attenuation (Peredaman)
Tegangan input ESP32 ADC secara default memiliki range terbatas (~0 - 1.1V). Untuk membaca tegangan hingga 3.3V, kita harus mengatur **attenuation**.

| Attenuation | Input Range (Approx) | Fungsi `analogSetAttenuation()` |
|:---:|:---:|:---:|
| 0dB | 0 - 1.1V | `ADC_0db` |
| 2.5dB | 0 - 1.5V | `ADC_2_5db` |
| 6dB | 0 - 2.2V | `ADC_6db` |
| 11dB | 0 - 3.3V | `ADC_11db` (Default di Arduino) |

### 3.3 Isu Non-Linearity
ADC ESP32 terkenal tidak linear sempurna, terutama di batas bawah (dekat 0V) dan batas atas (dekat 3.3V).
-   Input < 0.1V sering terbaca 0.
-   Input > 3.2V sering terbaca 4095.
Solusi: Gunakan kalibrasi (polynomial correction) atau gunakan range kerja aman (0.1V - 3.1V).

```cpp
// Setup (biasanya default sudah 11db/12-bit di core terbaru)
analogReadResolution(12);
analogSetAttenuation(ADC_11db);

int val = analogRead(34);
```

---

## 4. Sampling Rate & DMA (Direct Memory Access)

Untuk aplikasi yang membutuhkan kecepatan tinggi (seperti sampling audio atau sinyal AC), metode `analogRead()` (polling) terlalu lambat karena CPU harus menunggu proses konversi selesai.

### 4.1 DMA (Direct Memory Access)
DMA memungkinkan peripheral (ADC) menulis data langsung ke memori (RAM) tanpa intervensi CPU.
-   **Keuntungan:** Beban CPU minimal, sampling rate tinggi dan stabil.
-   **Aplikasi:** FFT (Fast Fourier Transform), pembacaan multi-channel simultan.

*Catatan: Topik DMA secara mendalam akan dibahas pada Modul 08, namun konsep dasarnya dikenalkan di sini hubungannya dengan ADC continuous scan.*

---

## 5. Kalibrasi dan Konversi Unit Fisik

Nilai mentah (RAW) dari ADC tidak memiliki makna fisis sebelum dikonversi.

### 5.1 Simple Scaling (Linear)
Untuk sensor linear (misal LM35, Potensiometer):
$$ Nilai\_Fisik = \frac{ADC}{ADC_{max}} \times (Max\_Fisik - Min\_Fisik) + Min\_Fisik $$

### 5.2 Sensor Non-Linear (NTC Thermistor, LDR)
Beberapa sensor memiliki respons logaritmik atau eksponensial.
Contoh NTC Thermistor memerlukan persamaan **Steinhart-Hart**:

$$ \frac{1}{T} = A + B \ln(R) + C (\ln(R))^3 $$

Di mana $R$ dihitung dari rangkaian pembagi tegangan (Voltage Divider) yang dibaca ADC.

---

## 6. Tips Hardware ADC
1.  **Gunakan Kapasitor Decoupling:** Pasang 100nF dekat pin ADC untuk memfilter noise frekuensi tinggi.
2.  **Impedansi Input:** ADC memiliki impedansi input tertentu. Jika sumber sinyal memiliki impedansi output tinggi (>10kΩ), tegangan akan drop saat sampling. Gunakan **Op-Amp Buffer (Voltage Follower)**.
3.  **VREF Stabil:** Ketelitian ADC sangat bergantung pada stabilitas tegangan referensi (VCC 3.3V). Gunakan regulator LDO yang bagus.

---

## 7. Referensi Lanjutan
1.  AN2834 Application Note: *How to get the best ADC accuracy in STM32F10xxx devices*.
2.  Espressif Docs: *Analog to Digital Converter (ADC)*.


### Teori Dasar

(Isi teori dasar di sini)

### Implementasi

(Isi implementasi di sini)

---

## 📖 Referensi

1. STM32 Reference Manual
2. ESP32 Technical Reference Manual
3. FreeRTOS Documentation

