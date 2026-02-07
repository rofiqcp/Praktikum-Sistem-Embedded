# JOBSHEET BAB 07: SPI Bus dan Storage

## 📋 Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | SPI Bus, SD Card, dan Flash Memory |
| **Platform** | STM32F103C8T6, ESP32 DevKit V1 |
| **Jumlah Program STM32** | 6 Program |
| **Jumlah Program ESP32** | 6 Program |
| **Durasi** | 2 x 50 menit |

---

## 🎯 Tujuan Praktikum

1. Mahasiswa mampu mengkonfigurasi periferal SPI pada STM32 dan ESP32.
2. Mahasiswa mampu mengakses chip Flash Memory eksternal (W25Q series).
3. Mahasiswa mampu menggunakan SD Card untuk penyimpanan data (FatFS/SD Library).
4. Mahasiswa memahami perbedaan file system (FatFS, SPIFFS, LittleFS).
5. Mahasiswa mampu membuat sistem data logger sederhana.

---

## 🛠️ Alat dan Bahan

1. Development Board STM32F103C8T6 (Blue Pill)
2. Development Board ESP32 DOIT DevKit V1
3. Modul Micro SD Card SPI Adapter
4. Micro SD Card (Format FAT32)
5. Modul Flash Memory W25Q64/W25Q128 (Optional, jika tidak ada di board adapter)
6. Logic Analyzer (Optional)
7. Kabel Jumper dan Breadboard
8. Laptop dengan VS Code + PlatformIO

---

## 📝 Tugas Praktikum A: STM32F103 (HAL Framework)

### Program 1: SPI Loopback Test
**Tujuan:** Memverifikasi komunikasi SPI dasar dengan menghubungkan MOSI ke MISO.

1. Hubungkan Pin PA7 (MOSI) ke Pin PA6 (MISO).
2. Konfigurasi SPI1 Master, Full Duplex, 8-bit.
3. Kirim data dan verifikasi data yang diterima sama.

```c
/* Code Snippet - main.c */
uint8_t txData[] = "Hello SPI";
uint8_t rxData[10];

while (1) {
    HAL_SPI_TransmitReceive(&hspi1, txData, rxData, sizeof(txData), 100);
    // Debug: Check if rxData matches txData
    HAL_Delay(1000);
}
```

### Program 2: Read Chip ID W25Qxx Flash
**Tujuan:** Membaca Manufacturer ID dan Device ID dari chip flash W25Qxx via SPI.

1. Hubungkan W25Qxx ke SPI1 (CS ke PA4).
2. Kirim command `0x90` (Read Manufacturer/Device ID).
3. Baca response 2 byte.

```c
/* Code Snippet */
#define W25Q_CMD_ID 0x90

void W25Q_ReadID(void) {
    uint8_t cmd[] = {W25Q_CMD_ID, 0, 0, 0};
    uint8_t id[2];
    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // CS Low
    HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    HAL_SPI_Receive(&hspi1, id, 2, 100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);   // CS High
    
    // Print logic here: valid ID for W25Q64 is EF 16
}
```

### Program 3: Read/Write/Erase W25Qxx
**Tujuan:** Menulis data ke flash memory dan membacanya kembali.

**Alur:**
1. Enable Write (Cmd `0x06`).
2. Erase Sector 4KB (Cmd `0x20`).
3. Tunggu Busy Flag clear.
4. Program Page (Cmd `0x02`) dengan data string.
5. Tunggu Busy Flag clear.
6. Read Data (Cmd `0x03`) dan validasi.

### Program 4: SD Card Mount Check (FatFS)
**Tujuan:** Menginisialisasi SD Card menggunakan library FatFS middleware STM32.

1. Aktifkan FATFS via STM32CubeMX (User Defined/SD SPI mode).
2. Generate code.
3. Panggil `f_mount()`.

```c
/* Code Snippet */
FATFS fs;
FRESULT res;

res = f_mount(&fs, "", 1);
if (res == FR_OK) {
    // Mount success
}
```

### Program 5: Write & Read File SD Card
**Tujuan:** Membuat file teks, menulis data, dan membacanya kembali.

1. `f_open(&fil, "test.txt", FA_WRITE | FA_CREATE_ALWAYS)`
2. `f_write()`
3. `f_close()`
4. `f_open(&fil, "test.txt", FA_READ)`
5. `f_read()`

### Program 6: Data Logger with Timestamp
**Tujuan:** Mencatat data dummy sensor + counter ke file CSV di SD Card setiap detik.

Format: `timestamp_ms, counter, value`

```c
void log_data(uint32_t count) {
    // Open "log.csv" function with FA_OPEN_APPEND
    f_printf(&fil, "%lu,%lu,%d\n", HAL_GetTick(), count, rand()%100);
    // Always close or sync to save data
}
```

---

## 📝 Tugas Praktikum B: ESP32 (Arduino Framework)

### Program 7: SD Card Info (SPI Mode)
**Tujuan:** Membaca informasi kartu SD (Type, Size) menggunakan library `SD.h`.

**Wiring (Default VSPI):**
- CS -> GPIO 5
- MOSI -> GPIO 23
- MISO -> GPIO 19
- SCLK -> GPIO 18

```cpp
#include <SPI.h>
#include <SD.h>

void setup() {
    Serial.begin(115200);
    if(!SD.begin(5)) {
        Serial.println("Card Mount Failed");
        return;
    }
    
    uint8_t cardType = SD.cardType();
    Serial.print("SD Card Type: ");
    // Print type...
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}
```

### Program 8: List Files and Directory
**Tujuan:** Menampilkan struktur file dan folder dalam SD Card.

Gunakan fungsi rekursif untuk membaca direktori root `/`.

### Program 9: Read/Write Text File
**Tujuan:** Operasi dasar file I/O pada SD Card.

1. `writeFile(SD, "/hello.txt", "Hello world!")`
2. `appendFile(SD, "/hello.txt", "Appending data...")`
3. `readFile(SD, "/hello.txt")`

### Program 10: ESP32 Internal Flash (LittleFS)
**Tujuan:** Menggunakan memori flash internal ESP32 untuk penyimpanan file (pengganti EEPROM/SD untuk data kecil).

1. Install plugin "ESP32 LittleFS Data Upload" (optional) atau format via code.
2. `#include <LittleFS.h>`
3. Mount LittleFS: `LittleFS.begin(true)` (true = format if fail).
4. Write/Read file operation seperti SD Card.

### Program 11: High Speed SPI (Change Clock)
**Tujuan:** Menguji performa tulis/baca dengan mengubah SPI Clock frequency.

Ubah `SD.begin(5, SPI, 4000000)` vs `SD.begin(5, SPI, 20000000)`.
Ukur waktu yang dibutuhkan untuk menulis 100KB data.

### Program 12: Data Logger Server
**Tujuan:** Mencatat data ke SD Card dan menampilkannya saat tombol ditekan via Serial.

Simulasikan "Black Box" recorder yang mencatat data terus menerus, dan bisa di-dump datanya.

---

## 📋 Evaluasi dan Pertanyaan

1. Jelaskan perbedaan SPI Mode 0, 1, 2, dan 3! Mode mana yang paling umum digunakan untuk SD Card?
2. Mengapa SD Card perlu diinisialisasi dengan clock rendah (400kHz) terlebih dahulu?
3. Apa perbedaan fungsi `f_sync()` dan `f_close()` pada FatFS? Kapan harus menggunakan `f_sync()`?
4. Mengapa pada ESP32 disarankan menggunakan LittleFS daripada SPIFFS untuk flash internal?
5. Hitung throughput teoretis SPI pada clock 18 MHz! Berapa throughput nyata yang biasa didapat saat menulis ke SD Card? Jelaskan bottleneck-nya.

---

## 📝 Laporan Praktikum

Buat laporan dalam format PDF yang berisi:
1. Kode program lengkap dengan komentar yang menjelaskan setiap bagian penting.
2. Foto rangkaian hardware.
3. Screenshot output Serial Monitor atau Logic Analyzer.
4. Jawaban pertanyaan evaluasi.
5. Analisis permasalahan yang dihadapi (jika ada).

Format nama file: `Laporan_Modul07_NIM_Nama.pdf`
Dikumpulkan di: [Link Pengumpulan]
Deadline: 1 minggu setelah praktikum.


