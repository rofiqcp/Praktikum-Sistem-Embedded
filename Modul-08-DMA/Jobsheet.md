# JOBSHEET BAB 08: Direct Memory Access

## 📋 Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | Direct Memory Access (DMA) |
| **Platform** | STM32F103C8T6, ESP32 DevKit WROOM-32 |
| **Jumlah Program STM32** | 3 (Mem2Mem, ADC DMA, UART DMA) |
| **Jumlah Program ESP32** | 2 (SPI DMA, I2S DMA) |
| **Durasi** | 120 Menit |

---

## 🎯 Tujuan Praktikum

1. Mahasiswa mampu mengkonfigurasi DMA Controller untuk transfer Memory-to-Memory.
2. Mahasiswa mampu menggunakan DMA untuk akuisisi data ADC Multi-channel secara otomatis.
3. Mahasiswa mampu mengimplementasikan transfer data high-speed (UART/SPI) menggunakan DMA tanpa membebani CPU.

---

## 🛠️ Persiapan Alat dan Bahan

1. Board STM32F103C8T6 (Blue Pill) + ST-Link V2
2. Board ESP32 DOIT DevKit V1
3. Laptop dengan STM32CubeIDE & VS Code (PlatformIO)
4. Kabel Jumper & Breadboard
5. Potensiometer (10k Ohm) - 2 buah
6. Kabel USB Micro

---

## 📝 Langkah Kerja Praktikum

### A. Percobaan STM32: Memory to Memory Transfer

Tujuan: Memahami dasar konfigurasi DMA dan membandingkan beban CPU.

1. Buat Project baru di STM32CubeIDE.
2. Enable **DMA1 Channel 1** di tab `System Core > DMA`.
   - Direction: Memory To Memory
   - Mode: Normal
   - Data Width: Word (32-bit)
   - Increment Address: Src & Dst
3. Generate Code.
4. Tambahkan kode berikut di `main.c`:

```c
/* USER CODE BEGIN PV */
#define BUFFER_SIZE 32
uint32_t srcBuffer[BUFFER_SIZE];
uint32_t dstBuffer[BUFFER_SIZE];
/* USER CODE END PV */

/* USER CODE BEGIN 2 */
// Fill srcBuffer with data
for (int i = 0; i < BUFFER_SIZE; i++) {
    srcBuffer[i] = i * 111;
    dstBuffer[i] = 0; // Clear dest
}

// Start DMA Transfer
HAL_DMA_Start(&hdma_memtomem_dma1_channel1, (uint32_t)srcBuffer, (uint32_t)dstBuffer, BUFFER_SIZE);

// Wait for transfer complete
HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_channel1, HAL_DMA_FULL_TRANSFER, 100);

// Verify data
int error = 0;
for (int i = 0; i < BUFFER_SIZE; i++) {
    if (dstBuffer[i] != srcBuffer[i]) {
        error++;
    }
}
/* USER CODE END 2 */
```

5. Jalankan dalam Mode Debug. Periksa nilai `dstBuffer` di Live Watch. Pastikan isinya sama dengan `srcBuffer`.

### B. Percobaan STM32: ADC Multi-Channel Scan Mode + DMA

Tujuan: Membaca 2 potensiometer & Internal Temperature sensor secara simultan otomatis.

1. **Konfigurasi ADC1:**
   - Mode: Scan Conversion Mode (Enable)
   - Continuous Conversion: Enable
   - Number of Conversion: 3
   - Channel: IN0, IN1, TempSensor
2. **Konfigurasi DMA:**
   - Add Request: ADC1
   - Mode: Circular
   - Data Width: Half Word (16-bit)
   - Increment: Memory Only (Periph = Fixed)
3. Generate Code.

```c
/* USER CODE BEGIN PV */
uint16_t adcValues[3]; // Buffer untuk CH0, CH1, Temp
/* USER CODE END PV */

/* USER CODE BEGIN 2 */
// Kalibrasi ADC (Optional)
HAL_ADCEx_Calibration_Start(&hadc1);

// Start ADC dengan DMA
HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcValues, 3);
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
while (1) {
    // Data otomatis terupdate di background
    // Bisa dibaca kapan saja
    uint16_t pot1 = adcValues[0];
    uint16_t pot2 = adcValues[1];
    uint16_t tempRaw = adcValues[2];
    
    HAL_Delay(100);
}
/* USER CODE END 3 */
```

4. Sambungkan Potensiometer ke PA0 dan PA1.
5. Debug dan lihat array `adcValues`. Putar potensiometer dan lihat nilainya berubah real-time tanpa ada kode `HAL_ADC_GetValue` di `while(1)`.

---

### C. Percobaan ESP32: SPI Master DMA Loopback

Tujuan: Mengirim buffer data besar lewat SPI menggunakan DMA.

1. Siapkan Project PlatformIO Arduino ESP32.
2. Hubungkan pin **MOSI (23)** ke **MISO (19)** (Loopback hardware).
3. Kode Program:

```cpp
#include <SPI.h>

#define BUFFER_SIZE 128

SPIClass *vspi = NULL;
uint8_t tx_buf[BUFFER_SIZE];
uint8_t rx_buf[BUFFER_SIZE];

void setup() {
  Serial.begin(115200);
  
  // Init Buffer
  for(int i=0; i<BUFFER_SIZE; i++) {
    tx_buf[i] = i; 
    rx_buf[i] = 0;
  }

  // Init SPI Virtual
  vspi = new SPIClass(VSPI);
  vspi->begin(); // Default: SCK:18, MISO:19, MOSI:23, SS:5

  Serial.println("Starting SPI DMA Transfer...");

  // Transaksi SPI
  vspi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  
  // TransferBytes otomatis menggunakan DMA jika buffer cukup besar (>64 bytes pada beberapa core)
  // Atau secara eksplisit jika menggunakan driver lower level, tapi di Arduino layer ini abstracted
  vspi->transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
  
  vspi->endTransaction();

  // Verifikasi
  int err = 0;
  for(int i=0; i<BUFFER_SIZE; i++) {
    if(tx_buf[i] != rx_buf[i]) err++;
  }

  if(err == 0) Serial.println("Success: Data Match!");
  else Serial.printf("Failed: %d errors\n", err);
}

void loop() {
  delay(1000);
}
```

---

## 💡 Tugas Latihan / Challenge

**Judul: "High Speed Data Logger Simulator"**

1. Gabungkan materi Timer (Bab 2) dan DMA (Bab 8).
2. Buat program STM32 yang:
   - Menggunakan **Timer Trigger** untuk sampling ADC setiap 1ms (1 kHz).
   - Menggunakan **DMA Circular Mode** untuk menyimpan 1000 sampel ke buffer.
   - Saat Buffer Setengah Penuh (Half Transfer) -> Kirim 500 data pertama ke UART (bisa pakai DMA UART atau Blocking).
   - Saat Buffer Penuh (Transfer Complete) -> Kirim 500 data kedua ke UART.
   - **Tujuan:** Data ADC tidak boleh putus/hilang meskipun CPU sedang sibuk mengirim Serial.

**Deliverables:**
- Flowchart sistem.
- Source code STM32.
- Bukti tangkapan Serial Plotter (Sinusoid/Potensio wave).

---

## 📊 Rubrik Penilaian

| Kriteria | Bobot | Deskripsi |
|----------|-------|-----------|
| **Setup DMA** | 30% | Konfigurasi Channel, Direction, & Data Width benar. |
| **Logic** | 30% | Logika buffer handling (Circular/Normal) benar. |
| **Integrasi** | 20% | Integrasi DMA Interrupt (HTC/TC) berfungsi. |
| **Analisis** | 20% | Pemahaman tentang beban CPU (DMA vs Polling). |


