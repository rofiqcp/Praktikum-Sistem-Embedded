# Modul 08: SPI Bus dan Komunikasi STM32-ESP32 — NotebookLM 45 Slide

## Bagian 1 — Teori SPI dan Platform (Slide 1–15)

### Slide 1 — Modul 08 dan Target Akhir

Modul 08 membahas protokol komunikasi SPI bus dan integrasi berbagai device pada platform mikrokontroler ESP32, STM32, dan sistem gabungan multi-MCU yang menjadi fondasi pengembangan sistem embedded modern. Target akhir pembelajaran modul ini adalah penyelesaian 25 eksperimen final yang terdiri dari 7 eksperimen STM32 SPI, 3 eksperimen STM32 SPI RTOS, 7 eksperimen ESP32 SPI, 3 eksperimen ESP32 SPI RTOS, 3 eksperimen Multi STM32-ESP32 SPI non-RTOS, dan 2 eksperimen Multi STM32-ESP32 SPI RTOS yang dirancang untuk membangun kompetensi praktis mahasiswa. Semua percobaan yang dilakukan di laboratorium diarahkan untuk mendukung pengembangan project akhir berupa Data Acquisition System dual-MCU yang mengintegrasikan akuisisi data sensor, penyimpanan data, penampilan informasi, generasi sinyal, dan komunikasi antar-mikrokontroler secara stabil. Eksperimen STM32 SPI mencakup penggunaan SPI Bus Scanner, W25Q64 Flash Read/Write, Micro SD Card File System, SSD1306 OLED SPI Graphics, MCP3008 ADC Multi-Channel, MCP4921 DAC Signal Generation, dan ADXL345 Accelerometer SPI. Eksperimen STM32 SPI RTOS mencakup SPI RTOS Multi Device, SPI RTOS DMA Transfer, dan SPI RTOS Data Logger. Eksperimen ESP32 SPI mencakup SPI Bus Master Configuration, W25Q64 Flash ID dan Memory Map, Micro SD Card SPI Mode, SSD1306 OLED SPI Display, MCP3008 ADC Read Potentiometer, MCP4921 DAC Waveform Output, dan ADXL345 SPI Acceleration. Eksperimen ESP32 SPI RTOS mencakup SPI RTOS Multi Task, SPI RTOS DMA Benchmark, dan SPI RTOS Interrupt Driven. Eksperimen Multi mencakup SPI Master-Slave Basic, SPI Multi Slave CS Management, SPI Shared Bus Multi Device, SPI RTOS Gateway, dan SPI Data Acquisition System. Setiap eksperimen dirancang untuk membangun pemahaman bertahap mulai dari teori dasar hingga integrasi sistem lengkap.

---

### Slide 2 — Dasar Bus SPI

SPI atau Serial Peripheral Interface adalah protokol komunikasi serial synchronous yang memakai empat jalur utama yaitu MOSI (Master Out Slave In) untuk data dari master ke slave, MISO (Master In Slave Out) untuk data dari slave ke master, SCLK (Serial Clock) untuk sinkronisasi clock dari master, dan CS/SS (Chip Select/Slave Select) untuk memilih slave yang aktif. Semua device slave yang terhubung ke bus SPI berbagi jalur MOSI, MISO, dan SCLK yang sama sehingga penghematan pin lebih baik dibandingkan parallel bus, namun membutuhkan pin CS tambahan untuk setiap slave. Komunikasi pada bus SPI bersifat full-duplex yang berarti data dapat ditransfer dua arah secara bersamaan dalam satu siklus clock, berbeda dengan I2C yang half-duplex. Master pada bus SPI adalah device yang mengatur transaksi komunikasi dengan membangkitkan sinyal clock dan memilih slave melalui pin CS, sedangkan slave adalah device yang merespons transaksi berdasarkan seleksi CS. Setiap device slave memiliki pin CS tersendiri yang aktif LOW, sehingga hanya slave dengan CS=LOW yang akan merespons transaksi SPI. Ground bersama antar device wajib dipastikan agar level logika yang digunakan dapat divalidasi dengan benar. Kecepatan komunikasi SPI dapat diatur mulai dari beberapa kHz hingga 50+ MHz tergantung kemampuan master dan slave, namun praktikum menggunakan 1-10 MHz sebagai standar untuk stabilitas. Setiap transaksi dimulai dengan master menurunkan CS slave ke LOW, mengirimkan dan menerima data secara full-duplex, lalu menaikkan CS kembali ke HIGH setelah transaksi selesai.

---

### Slide 3 — Clock Polarity dan Clock Phase (CPOL/CPHA)

SPI menggunakan dua bit konfigurasi untuk menentukan timing data: Clock Polarity (CPOL) dan Clock Phase (CPHA). CPOL menentukan level SCLK saat idle: CPOL=0 berarti SCLK idle LOW, CPOL=1 berarti SCLK idle HIGH. CPHA menentukan kapan data disample: CPHA=0 berarti data disample pada rising edge (transisi LOW→HIGH), CPHA=1 berarti data disample pada falling edge (transisi HIGH→LOW). Kombinasi CPOL dan CPHA menghasilkan 4 mode SPI: Mode 0 (CPOL=0, CPHA=0), Mode 1 (CPOL=0, CPHA=1), Mode 2 (CPOL=1, CPHA=0), Mode 3 (CPOL=1, CPHA=1). Mode 0 dan 3 adalah yang paling umum digunakan pada sensor dan modul SPI komersial. W25Q64 Flash, Micro SD Card, dan SSD1306 OLED menggunakan Mode 0 (CPOL=0, CPHA=0). ADXL345 Accelerometer menggunakan Mode 3 (CPOL=1, CPHA=1). MCP3008 ADC dan MCP4921 DAC umumnya menggunakan Mode 0. Pastikan master dan slave menggunakan mode SPI yang sama, jika berbeda data akan salah atau tidak terbaca. Pada STM32, konfigurasi CPOL dan CPHA dilakukan melalui register SPI_CR1 (bits CPOL dan CPHA) atau melalui HAL: SPI_POLARITY_LOW/HIGH dan SPI_PHASE_1EDGE/2EDGE. Pada ESP32, konfigurasi mode dilakukan saat menambahkan device: .mode = 0/1/2/3 dalam struktur spi_device_interface_config_t.

---

### Slide 4 — SPI Modes (0, 1, 2, 3)

Empat mode SPI ditentukan oleh kombinasi CPOL dan CPHA yang harus sesuai antara master dan slave agar komunikasi berjalan benar. Mode 0 (CPOL=0, CPHA=0): SCLK idle LOW, data ditransfer pada rising edge (sample) dan berubah pada falling edge (setup). Mode 1 (CPOL=0, CPHA=1): SCLK idle LOW, data ditransfer pada falling edge dan berubah pada rising edge. Mode 2 (CPOL=1, CPHA=0): SCLK idle HIGH, data ditransfer pada falling edge dan berubah pada rising edge. Mode 3 (CPOL=1, CPHA=1): SCLK idle HIGH, data ditransfer pada rising edge dan berubah pada falling edge. W25Q64, SD Card, SSD1306, MCP3008, MCP4921 umumnya Mode 0. ADXL345 menggunakan Mode 3. Beberapa device dapat dikonfigurasi ke mode berbeda melalui register internal. Jika mode salah, data yang dibaca akan salah (bit shift atau totally wrong). Praktikum Modul 08 mewajibkan mahasiswa menguji device dengan mode yang benar dan mencatat mode pada laporan. Jika device tidak respons, hal pertama yang dicek adalah mode SPI (CPOL/CPHA) sesuai datasheet. STM32 HAL: hspi.Init.CLKPolarity dan hspi.Init.CLKPhase. ESP-IDF: .mode = 0/1/2/3.

---

### Slide 5 — Full-Duplex, Half-Duplex, dan Simplex

SPI umumnya beroperasi dalam mode full-duplex, di mana setiap transaksi melibatkan pertukaran byte: master mengirim byte melalui MOSI sambil menerima byte dari slave melalui MISO secara simultan. Dalam full-duplex, saat master mengirim 1 byte, slave juga mengirim 1 byte (data register slave digeser keluar). Half-duplex SPI hanya menggunakan satu jalur data (MOSI atau MISO) pada satu waktu, bisa dikonfigurasi untuk bidirectional communication pada satu pin (jarang dipakai). Simplex berarti satu arah tetap: hanya transmit (MOSI saja) atau hanya receive (MISO saja). Pada praktikum SPI, full-duplex digunakan untuk komunikasi normal, misalnya: master kirim command byte, slave respon dengan status/data byte dalam transaksi yang sama. Contoh: W25Q64 Read JEDEC ID: master kirim 0x9F (MOSI), sambil menerima 3 byte ID (MISO). Pada STM32, full-duplex menggunakan HAL_SPI_TransmitReceive(). Half-duplex bisa menggunakan HAL_SPI_Transmit() atau HAL_SPI_Receive() terpisah. Keuntungan full-duplex: efisiensi waktu, 1 transaksi = kirim + terima. Keuntungan half-duplex/simplex: hemat pin, bisa gunakan 3-pin SPI (tanpa MISO atau tanpa MOSI) untuk device tertentu.

---

### Slide 6 — Chip Select (CS/SS) dan Multi-Slave

Setiap slave SPI membutuhkan pin Chip Select (CS) atau Slave Select (SS) tersendiri dari master. CS aktif LOW: slave dipilih saat CS=LOW, dinonaktifkan saat CS=HIGH. Saat CS=HIGH, slave mengabaikan SCLK dan mematikan output MISO (high-impedance) agar tidak mengganggu slave lain pada bus. Multi-slave pada satu bus SPI berbagi MOSI, MISO, SCLK, namun memiliki CS masing-masing. Untuk akses Slave 0: CS0=LOW, CS1=HIGH, CS2=HIGH. Untuk akses Slave 1: CS0=HIGH, CS1=LOW, CS2=HIGH. Untuk akses Slave 2: CS0=HIGH, CS1=HIGH, CS2=LOW. Hanya satu slave yang aktif pada satu waktu (kecuali daisy-chain). Jika dua slave aktif bersamaan (CS LOW bersamaan) dan kedua mengeluarkan data di MISO, akan terjadi bus contention (short circuit). STM32: CS dapat dikelola sebagai GPIO output (software CS) atau hardware NSS (jarang dipakai untuk multi-slave). ESP32: CS dikelola sebagai gpio_num dalam konfigurasi device (.spics_io_num). Praktikum Modul 08 menggunakan software CS untuk fleksibilitas. Jumlah slave dibatasi oleh jumlah GPIO yang tersedia untuk CS pins.

---

### Slide 7 — Daisy-Chain SPI

Daisy-chain SPI menghubungkan multiple slave secara seri: MOSI master → Slave 0 MOSI → Slave 0 MISO → Slave 1 MOSI → Slave 1 MISO → Slave 2 MOSI → Slave 2 MISO → MISO master. Semua slave menggunakan CS yang sama. Data ditransfer bergeser melalui chain: byte pertama untuk Slave 2, byte kedua untuk Slave 1, byte ketiga untuk Slave 0 (tergantung implementasi). Keuntungan daisy-chain: hemat pin CS (hanya butuh 1 CS untuk semua slave). Kekurangan: latensi lebih tinggi (harus mengirim data untuk semua slave dalam satu transaksi), kompleksitas software meningkat, tidak semua device mendukung daisy-chain. W25Q64 dan kebanyakan sensor tidak mendukung daisy-chain; mereka menggunakan independent CS. Daisy-chain umum pada device seperti shift register (74HC595) atau ADC/DAC multi-channel yang dirancang untuk chaining. Pada praktikum Modul 08, mahasiswa menggunakan independent CS (bukan daisy-chain) untuk kemudahan pengembangan. Konsep daisy-chain dijelaskan sebagai pengetahuan tambahan untuk industri.

---

### Slide 8 — SPI Speed dan Signal Integrity

Kecepatan SPI dapat mencapai 10+ MHz hingga 50+ MHz pada kondisi ideal, jauh lebih cepat dari I2C (max 3.4 MHz). Faktor pembatas kecepatan SPI: kemampuan slave (cek datasheet), panjang kabel, kapasitansi bus, noise, dan kualitas ground. SD Card: init wajib 400 kHz, lalu dapat dinaikkan ke 10-25 MHz setelah init sukses. W25Q64: dapat hingga 133 MHz (W25Q64JV), namun praktikum gunakan 10 MHz untuk stabilitas. MCP3008 ADC: maksimal 3.6 MHz clock (Tsettling). MCP4921 DAC: maksimal 20 MHz. ADXL345: maksimal 5 MHz. SSD1306 OLED: 10 MHz cukup untuk refresh lancar. Jika komunikasi error/gangguan pada speed tinggi: turunkan clock, periksa kabel lebih pendek, tambahkan resistor 10-100 ohm pada MOSI/MISO dekat master, gunakan ground yang baik. STM32: baud rate prescaler (SPI_BAUDRATEPRESCALER_x) menentukan clock: PCLK/2, /4, /8, /16, dst. ESP32: clock_speed_hz dalam spi_device_interface_config_t. Mulai dari 1 MHz untuk bring-up, naik bertahap setelah stabil.

---

### Slide 9 — Device Modul 08

Device utama yang digunakan dalam Modul 08 meliputi W25Q64 SPI Flash, Micro SD Card Module, SSD1306 OLED SPI, MCP3008 ADC, MCP4921 DAC, dan ADXL345 Accelerometer. W25Q64 adalah SPI Flash memory 8 MB (64 Mbit) dengan page size 256 byte, sector 4 KB, block 32/64 KB, dan operasi Read/Write/Erase melalui command SPI. Micro SD Card Module menggunakan SPI mode untuk akses penyimpanan massal dengan kapasitas mulai 4 GB hingga 32+ GB menggunakan FAT32 file system. SSD1306 OLED SPI 128x64 piksel membutuhkan pin tambahan selain SPI: CS, DC (Data/Command), dan RES (Reset) untuk kontrol display. MCP3008 adalah 10-bit 8-channel ADC yang membaca tegangan 0-3.3V/Vref dan menghasilkan nilai 0-1023 melalui interface SPI mode 0. MCP4921 adalah 12-bit single DAC yang menerima data 0-4095 melalui SPI mode 0 dan menghasilkan tegangan analog 0-Vref×Gain. ADXL345 adalah 3-axis accelerometer yang dapat diakses via SPI mode 3 (CPOL=1, CPHA=1) atau I2C, dengan register DEVID tetap 0xE5. Semua device ini beroperasi pada level logika 3.3V dan dapat digabung dalam satu bus SPI dengan CS berbeda. Jumlah maksimal device dibatasi GPIO CS yang tersedia dan kapasitansi bus.

---

### Slide 10 — W25Q64 SPI Flash

W25Q64 adalah SPI Flash memory 8 MB (64 Mbit) dengan organisasi: 32768 pages × 256 bytes = 8,388,608 bytes. Device mendukung standard SPI, Dual SPI, dan Quad SPI (pada praktikum gunakan standard SPI). Command penting: Read JEDEC ID (0x9F) menghasilkan 3 byte: Manufacturer ID (0xEF untuk Winbond), Memory Type (0x40), Capacity (0x17 untuk 64Mbit). Write Enable (0x06) harus dikirim sebelum setiap operasi Write/Erase. Read Status Register 1 (0x05) untuk cek BUSY bit (bit 0) dan WEL bit (bit 1). Page Program (0x02) menulis hingga 256 byte pada alamat tertentu (3-byte address). Sector Erase (0x20) menghapus 4 KB (16 pages). Block Erase 32KB (0x52) atau 64KB (0xD8). Chip Erase (0xC7) menghapus seluruh chip. Read Data (0x03) membaca data dari alamat. Operasi tulis/erase membutuhkan waktu: Page Program ~0.5-3 ms, Sector Erase ~45-400 ms, Block Erase ~120-1600 ms. Polling status register BUSY=0 sebelum operasi baru. STM32: HAL_SPI_Transmit/Receive. ESP32: spi_device_transmit dengan tx_buffer/rx_buffer.

---

### Slide 11 — Micro SD Card SPI Mode

Micro SD Card dapat diakses dengan SPI mode (alternatif dari SD Bus 4-bit). Langkah inisialisasi SPI: 1) Clock 400 kHz wajib untuk init (STM32/ESP32 set baud rate rendah). 2) CMD0 (GO_IDLE_STATE) dengan CS=LOW untuk reset SD card ke SPI mode. 3) CMD8 (SEND_IF_COND) dengan argumen 0x1AA untuk cek voltage range (2.7-3.6V). 4) ACMD41 (SD_SEND_OP_COND) dengan polling hingga bit 31 (POWER_UP) = 1 (card ready). 5) CMD58 (READ_OCR) untuk baca Operating Conditions Register. 6) CMD16 (SET_BLOCKLEN) set block length = 512 byte. Setelah init sukses, naikkan clock ke 10-25 MHz. Command umum: CMD17 (READ_SINGLE_BLOCK) baca 512 byte dari alamat block. CMD24 (WRITE_SINGLE_BLOCK) tulis 512 byte. CMD55 (APP_CMD) prefix untuk ACMD. ACMD41 inisialisasi. Gunakan FatFS library untuk file system FAT16/FAT32 agar dapat mengelola file (datalog.csv, log.txt, dll). SD Card capacity: cek CSD register (Card Specific Data). 4 GB = ~4,000,000,000 bytes = ~7,800,000 blocks (512 byte each). SDHC (High Capacity) menggunakan alamat block bukan byte-address.

---

### Slide 12 — SSD1306 OLED SPI

SSD1306 OLED 128×64 dengan interface SPI butuh 3 pin kontrol: CS (Chip Select), DC (Data/Command), dan RES (Reset) selain jalur SPI MOSI dan SCK. DC=LOW berarti command, DC=HIGH berarti data (display RAM). Reset: RES LOW → delay 10ms → RES HIGH → delay 10ms. Sequence inisialisasi: kirim command: 0xAE (display off), 0xD5 (display clock), 0xA8 (multiplex ratio), 0xD3 (display offset), 0x40 (start line), 0x8D (charge pump), 0xA1/A0 (segment remap), 0xC8/C0 (COM output scan), 0xDA (COM pins), 0x81 (contrast), 0xD9 (pre-charge), 0xDB (VCOMH), 0xA4 (display all on resume), 0xA6 (normal display), 0xAF (display on). Framebuffer: 128×64/8 = 1024 byte. Setiap bit = 1 pixel. CS aktif LOW saat kirim command/data. SPI mode 0 (CPOL=0, CPHA=0). Speed: 8-10 MHz untuk refresh lancar. STM32: HAL_SPI_Transmit untuk kirim command/data. ESP32: spi_device_transmit. Perbedaan I2C vs SPI: I2C hanya SDA/SCL, address 0x3C/0x3D, speed max 400 kHz-1 MHz; SPI butuh lebih banyak pin tetapi jauh lebih cepat refresh.

---

### Slide 13 — MCP3008 ADC dan MCP4921 DAC

MCP3008 adalah 10-bit 8-channel ADC dengan interface SPI mode 0. Pembacaan channel: 1) CS LOW. 2) Kirim 5 bit konfigurasi: start bit (1), single-ended/differential (S/D=1 untuk single), channel select (D2,D1,D0 untuk channel 0-7). 3) Baca 10-bit hasil (MSB first) dalam 2 byte. Vref biasanya 3.3V atau 5V. Nilai ADC: 0-1023 → Tegangan = (ADC/1023) × Vref. Channel 0-7 masing-masing input analog (0-3.3V). MCP4921 adalah 12-bit single DAC dengan interface SPI mode 0. Format data 16-bit: bit 15-12 = konfigurasi (Write/Buffer/Gain/SHDN), bit 11-0 = 12-bit DAC data. Vout = (DAC_data / 4096) × Vref × Gain. Gain=1x jika bit 13=1, Gain=2x jika bit 13=0. Vref=4.096V direkomendasikan untuk resolusi 1 mV/bit. Output dapat diukur dengan multimeter atau dibaca balik dengan MCP3008 ADC untuk verifikasi. Penggunaan: MCP3008 baca potentiometer/sensor, MCP4921 hasilkan sinyal ramp/triangle/sine untuk control/actuator. Kedua device menggunakan SPI mode 0, 10-bit ADC dan 12-bit DAC memberikan resolusi cukup untuk instrumentasi embedded.

---

### Slide 14 — ADXL345 Accelerometer SPI

ADXL345 adalah 3-axis accelerometer dengan interface SPI/I2C (pilih mode dengan pin CS: CS=LOW → SPI mode; CS=HIGH → I2C mode). SPI Mode: 3 (CPOL=1, CPHA=1). Register penting: DEVID (0x00) = 0xE5 (fixed device ID untuk verifikasi). POWER_CTL (0x2D): bit 3 = 1 (measure mode) untuk aktifkan pengukuran. DATA_FORMAT (0x31): bit 1-0 untuk range (0=±2g, 1=±4g, 2=±8g, 3=±16g), bit 6 = 1 (full resolution 13-bit). DATAX0, DATAX1 (0x32-0x33): X-axis data 16-bit two's complement. DATAY0, DATAY1 (0x34-0x35): Y-axis. DATAZ0, DATAZ1 (0x36-0x37): Z-axis. Baca 6 byte sekaligus untuk mendapatkan X,Y,Z. Konversi ke nilai g: (raw_data / 256) × range (misal ±2g → range=2). Output berubah saat sensor digerakkan atau dimiringkan. Aplikasi: deteksi orientasi (portrait/landscape), deteksi guncangan/tamper, pengukuran kemiringan (pitch/roll). SPI mode 3: SCLK idle HIGH, data sample pada rising edge. Perhatikan CPOL=1, CPHA=1 pada konfigurasi master. STM32: SPI_POLARITY_HIGH, SPI_PHASE_2EDGE. ESP32: .mode = 3.

---

### Slide 15 — STM32 SPI HAL, Register, dan DMA

STM32 memiliki beberapa peripheral SPI (SPI1, SPI2, SPI3) tergantung seri. SPI1 umum di PA5/PA6/PA7 (SCK/MISO/MOSI) untuk STM32F103 Blue Pill. Konfigurasi HAL: hspi.Instance = SPI1; hspi.Init.Mode = SPI_MODE_MASTER; hspi.Init.Direction = SPI_DIRECTION_2LINES (full-duplex); hspi.Init.DataSize = SPI_DATASIZE_8BIT; hspi.Init.CLKPolarity = SPI_POLARITY_LOW (mode 0); hspi.Init.CLKPhase = SPI_PHASE_1EDGE (mode 0); hspi.Init.NSS = SPI_NSS_SOFT (software CS); HAL_SPI_Init(&hspi). Operasi: HAL_SPI_Transmit(&hspi, tx, size, timeout); HAL_SPI_Receive(&hspi, rx, size, timeout); HAL_SPI_TransmitReceive(&hspi, tx, rx, size, timeout). DMA: HAL_SPI_Transmit_DMA(&hspi, tx, size); callback HAL_SPI_TxCpltCallback(). Register: SPIx_CR1 (config), SPIx_SR (status: TXE, RXNE, BSY), SPIx_DR (data). DMA membebaskan CPU saat transfer blok besar (Flash page 256 byte, OLED framebuffer 1024 byte). STM32F4/F7 memiliki DMA stream/channel untuk SPI. FreeRTOS: gunakan mutex untuk akses SPI shared bus antar task. Error handling: cek HAL status, timeout, bus recovery (deinit/init). STM32 sebagai SPI slave: konfigurasi SPI_MODE_SLAVE, NSS hardware atau software, prepare transmit buffer.

---

## Bagian 2 — Praktikum 25 Eksperimen (Slide 16–30)

### Slide 16 — Struktur Final Praktikum

Modul 08 memiliki tepat 25 eksperimen yang terdiri dari STM32_01 sampai STM32_07 (7 eksperimen STM32 SPI), STM32_08 sampai STM32_10 (3 eksperimen STM32 SPI RTOS), ESP32_01 sampai ESP32_07 (7 eksperimen ESP32 SPI), ESP32_08 sampai ESP32_10 (3 eksperimen ESP32 SPI RTOS), MULTI_01 sampai MULTI_03 (3 eksperimen Multi STM32-ESP32 SPI non-RTOS), dan MULTI_04 sampai MULTI_05 (2 eksperimen Multi STM32-ESP32 SPI RTOS). Struktur ini memastikan mahasiswa memahami SPI dari sisi STM32, ESP32, dan integrasi dua MCU secara bertahap dari dasar hingga lanjut. Setiap eksperimen memiliki tujuan pembelajaran yang spesifik, mulai dari pengenalan SPI, pembacaan device, penyimpanan data, penampilan informasi, generasi sinyal, hingga integrasi sistem. Eksperimen STM32 difokuskan pada penggunaan STM32 HAL dan register mentah, ESP32 pada ESP-IDF dan GPIO matrix, serta Multi pada komunikasi antar-MCU. Mahasiswa diwajibkan menyelesaikan semua 25 eksperimen untuk mendapatkan nilai lengkap, dengan setiap eksperimen harus menunjukkan output yang valid dan dokumentasi yang lengkap. Struktur ini juga memastikan bahwa semua topik SPI yang diajarkan pada teori tercakup pada praktikum, sehingga mahasiswa memiliki kompetensi penuh dalam komunikasi SPI setelah menyelesaikan modul ini.

---

### Slide 17 — STM32_01 sampai STM32_03

STM32_01 adalah SPI Bus Scanner yang mendeteksi device SPI dengan membaca ID register atau status. Output: serial menampilkan device terdeteksi beserta ID/nilai pembacaan (W25Q64 JEDEC ID, OLED status, ADC value, DAC output, ADXL DEVID). STM32_02 membaca dan menulis data ke W25Q64 Flash: baca JEDEC ID, baca status register, Write Enable, Sector Erase, Page Program, Read Data, verifikasi tulis dan baca sama. Clock 10 MHz, mode 0. STM32_03 mengakses Micro SD Card dengan SPI mode: reset dengan CMD0, cek voltage CMD8, inisialisasi ACMD41, baca CSD untuk kapasitas, baca/tulis blok 512 byte, implementasikan FatFS sederhana. Init clock 400 kHz, lalu naik ke 10 MHz. Output: serial menampilkan tipe SD, kapasitas, berhasil baca/tulis blok.

---

### Slide 18 — STM32_04 sampai STM32_07

STM32_04 menampilkan grafis pada SSD1306 OLED SPI: init SPI mode 0, reset via RES pin, kirim sequence inisialisasi, buat framebuffer 1024 byte, gambar teks/grafik, kirim ke OLED, update periodik dengan animasi. STM32_05 membaca 8 channel MCP3008 ADC: kirim konfigurasi channel, baca 10-bit hasil, konversi ke tegangan, tampilkan di serial. STM32_06 menghasilkan sinyal analog MCP4921 DAC: konfigurasi 16-bit data, generate ramp/triangle/sine, update DAC periodik, ukur output dengan ADC untuk verifikasi. STM32_07 membaca akselerasi ADXL345 SPI mode 3: baca DEVID, set measure mode, baca 6 byte X,Y,Z, konversi ke g, hitung orientasi. Output: serial menampilkan akselerasi berubah saat sensor digerakkan.

---

### Slide 19 — STM32_08 sampai STM32_10 (RTOS)

STM32_08 mengakses multi device SPI dengan FreeRTOS: buat task baca ADC, task update OLED, task log ke Flash, gunakan mutex untuk akses SPI shared, gunakan queue untuk antar task, monitor CPU usage. STM32_09 SPI RTOS DMA Transfer: init SPI dengan DMA, buat task trigger DMA, pakai semaphore dari callback, bandingkan dengan polling, ukur CPU usage. STM32_10 SPI RTOS Data Logger: task baca ADC periodik, simpan ke buffer circular, task logger tulis ke W25Q64 dengan page program, gunakan semaphore untuk sync, cek integritas CRC. Output: multi-task berjalan stabil tanpa konflik akses SPI.

---

### Slide 20 — ESP32_01 sampai ESP32_03

ESP32_01 mengonfigurasi ESP32 sebagai SPI master dengan ESP-IDF: install ESP-IDF, setup project, konfigurasi SPI bus dengan spi_bus_initialize, tambah device dengan spi_bus_add_device (mode 0, clock 10 MHz), siapkan transaksi spi_transaction_t, kirim command read JEDEC ID ke W25Q64, baca response. ESP32_02 membaca W25Q64 Flash ID dan Memory Map: baca JEDEC ID (0xEF4017), baca Unique ID, baca status register, tulis/verify data test, tampilkan memory map 8 MB = 128 blocks = 2048 sectors = 32768 pages. ESP32_03 mengakses Micro SD Card SPI mode: init SPI 400 kHz, reset CMD0, cek CMD8, init ACMD41, baca CSD, baca/tulis blok 512 byte, gunakan FatFS. Output: serial menampilkan konfigurasi SPI sukses dan ID terbaca.

---

### Slide 21 — ESP32_04 sampai ESP32_07

ESP32_04 menampilkan SSD1306 OLED SPI: init SPI bus mode 0, reset OLED via RES, inisialisasi register, buat framebuffer, gambar teks/grafik, update display. ESP32_05 membaca MCP3008 ADC: init SPI mode 0, baca channel 0-7, konversi ke tegangan, tampilkan di serial/OLED, uji dengan potentiometer. ESP32_06 menghasilkan MCP4921 DAC waveform: init SPI mode 0, tulis register DAC, generate ramp/triangle/sine, ukur output dengan multimeter/oscilloscope atau baca balik ADC. ESP32_07 membaca ADXL345 SPI mode 3: baca DEVID=0xE5, set measure mode, baca X,Y,Z, hitung g dan orientasi, tampilkan di serial/OLED. Output: nilai ADC/DAC/Accel berubah sesuai input/gerakan.

---

### Slide 22 — ESP32_08 sampai ESP32_10 (RTOS)

ESP32_08 SPI RTOS Multi Task: buat task baca ADC, task update OLED, task log ke Flash, gunakan mutex untuk SPI shared, gunakan queue untuk komunikasi antar task. ESP32_09 SPI RTOS DMA Benchmark: lakukan tulis/baca Flash dengan polling, lakukan dengan DMA, ukur waktu transfer untuk berbagai ukuran data, bandingkan CPU usage, tampilkan hasil benchmark. ESP32_10 SPI RTOS Interrupt Driven: konfigurasi SPI dengan interrupt callback, buat task menunggu semaphore dari ISR, lakukan transfer non-blocking, bandingkan dengan polling, ukur response time. Output: multi-task SPI berjalan bersama tanpa konflik, DMA lebih efisien.

---

### Slide 23 — MULTI_01 dan MULTI_02

MULTI_01: STM32 sebagai SPI slave, ESP32 sebagai master. STM32 konfigurasi SPI slave (hardware NSS atau software), siapkan data buffer, ESP32 master kirim command, STM32 slave respon dengan data register virtual, ESP32 baca data dari STM32 slave. Wiring: ESP32 MOSI ↔ STM32 MISO, ESP32 MISO ↔ STM32 MOSI, ESP32 SCLK ↔ STM32 SCK, ESP32 CS → STM32 NSS. MULTI_02: SPI Multi Slave CS Management. Master (ESP32 atau STM32) akses W25Q64, SSD1306, MCP3008 pada satu bus dengan CS berbeda. Implementasikan fungsi select slave (aktifkan CS slave tertentu, nonaktifkan CS lain). Demostrasikan multitasking akses slave berbeda tanpa konflik. Output: semua slave dapat diakses tanpa konflik CS.

---

### Slide 24 — MULTI_03, MULTI_04, dan MULTI_05

MULTI_03: SPI Shared Bus Multi Device. ESP32 dan STM32 terhubung ke bus SPI yang sama dengan CS masing-masing. ESP32 sebagai master utama, STM32 akses device via protokol tertentu. Pastikan CS tidak aktif bersamaan untuk slave yang sama. MULTI_04: SPI RTOS Gateway. STM32 baca sensor (ADC, Accel) tiap 1 s dengan FreeRTOS task, ESP32 request data dari STM32 via SPI, ESP32 tampilkan di OLED dan kirim via UART ke PC, implementasikan queue/semaphore RTOS. MULTI_05: SPI Data Acquisition System Final Integration. Integrasi: scanner, W25Q64, SD Card, OLED, ADC, DAC, ADXL345, ESP32-STM32 communication, error recovery, RTOS multi-task. Output: Data Acquisition System dual-MCU RTOS berjalan stabil dengan semua fitur berfungsi.

---

### Slide 25 — Data Pengamatan Praktikum

Setiap eksperimen catat kode, platform, wiring, device, mode (CPOL/CPHA), clock speed, CS pin, output serial/display, error, solusi. Tabel 25 eksperimen wajib lengkap. Foto wiring dan screenshot serial/OLED jadi bukti laporan. Data disimpan dalam folder terstruktur per eksperimen. Format pengamatan: Kode eksperimen, Platform (STM32/ESP32/Multi), Device SPI, Mode SPI, Clock speed, CS Pin, Output serial/display, Error yang muncul, Solusi. Analisis error: NACK tidak ada di SPI (SPI tidak pakai ACK/NACK), yang ada adalah timeout atau data korup. Recovery: deinit SPI, re-init, reset device jika ada pin RES, cek CS active, cek mode benar, turunkan clock speed. Troubleshooting: cek GND bersama, MOSI/MISO/SCLK tidak tertukar, CS tidak floating, mode sesuai datasheet, clock sesuai kemampuan device.

---

## Bagian 3 — Project, Video, dan Evaluasi (Slide 31–45)

### Slide 31 — Project Data Acquisition System

Project akhir Modul 08 adalah Data Acquisition System dual-MCU. ESP32 sebagai gateway, display OLED, konfigurasi, agregasi data, error recovery, SD Card logging. STM32 sebagai sensor/logger node dengan SPI register-level, DMA, real-time sampling, ADC/Accel reading. Keduanya tukar data via SPI master-slave atau UART. Device: W25Q64 Flash untuk log circular, Micro SD Card untuk file system besar, SSD1306 OLED untuk tampilan lokal, MCP3008 ADC untuk pembacaan sensor analog, MCP4921 DAC untuk generasi sinyal, ADXL345 untuk deteksi orientasi. Sistem menggunakan shared SPI bus atau split bus (Bus A: ESP32 master, Bus B: STM32 master) dengan komunikasi antar-MCU via SPI master-slave (MULTI_01) atau UART. Data dikirim ke PC untuk monitoring via serial/UART WiFi. Error recovery otomatis: timeout, retry, device offline mode, bus reinit.

---

### Slide 32 — Arsitektur Project

Sensor dan device: W25Q64 Flash, Micro SD Card, SSD1306 OLED, MCP3008 ADC, MCP4921 DAC, ADXL345 Accel. ESP32 kelola SPI master, OLED dashboard, SD Card logging, koordinator sistem. STM32 kelola sensor ADC/Accel, Flash logging, DAC output, real-time sampling. Komunikasi antar-MCU: SPI master-slave (ESP32 master, STM32 slave) atau UART 115200 baud. Data flow: ADC/Accel → STM32 → SPI/UART → ESP32 → OLED Dashboard + SD Card + Serial ke PC. Log data: W25Q64 circular (400000 record 18 byte) + SD Card file (datalog.csv). Error handling: timeout, retry, device offline, bus recovery. System architecture diagram menunjukkan koneksi MOSI/MISO/SCLK/CS pins antar device dan MCU. Split bus atau shared bus dengan CS management yang tepat untuk mencegah konflik.

---

### Slide 33 — Record Log Project

Record log 18 byte: timestamp 4 byte (millis()), ADC Channel 0 2 byte (0-1023), ADC Channel 1 2 byte, Accel X 2 byte (mg), Accel Y 2 byte, Accel Z 2 byte, DAC Output 2 byte (0-4095), Status flags 1 byte (bitfield device/error), Checksum 1 byte (XOR/CRC8). Metadata W25Q64: magic 2 byte (0x4D08), version 1 byte, record_size 1 byte, write_index 4 byte, record_count 2 byte, reserved 8 byte. Total metadata 18 byte. W25Q64 8 MB = 8388608 byte, dikurangi metadata: 8388590 byte / 18 byte = 466033 record mentah, batas aman project: maksimal 400000 record. SD Card: simpan dalam file CSV (datalog.csv), setiap baris: timestamp,adc0,adc1,ax,ay,az,dac,status. Kapasitas SD Card (4 GB+) hampir tidak terbatas untuk praktikum. Log data ditulis secara periodik (100ms-1000ms) atau berdasarkan event (accel threshold, adc change).

---

### Slide 34 — Kapasitas Penyimpanan Project

W25Q64 8 MB (8388608 byte): metadata 18 byte, record 18 byte, 400000 record aman (7,200,000 byte + 18 = 7,200,018 byte). Lebih dari ini masih dalam kapasitas fisik tetapi berisiko fragmentation. SD Card 4 GB: FAT32 file system, file CSV bisa mencapai GB. Format CSV: timestamp,adc0,adc1,ax,ay,az,dac,status. Contoh: 1000000,512,256,100,-50,980,2048,0x00. Data logging: W25Q64 untuk ring buffer (data terbaru), SD Card untuk penyimpanan permanen (archive). Log rotation: jika W25Q64 penuh, overwrite record tertua (circular). Jika SD Card penuh, buat file baru (datalog1.csv, datalog2.csv). Penulisan perhatikan W25Q64 page boundary 256 byte: jangan menulis melewati batas page (alamat 0xXX00). Untuk multi-byte write, cek alamat % 256 == 0, jika ya dan data > sisa page, pecah menjadi dua write operasi.

---

### Slide 35 — Alur Startup Project

Startup: ESP32 dan STM32 boot, jalankan SPI scanner (baca JEDEC ID Flash, detect SD Card, test OLED, baca DEVID ADXL345), init W25Q64 (baca status, cek metadata), init SD Card (baca CSD, mount file system), init OLED (reset, sequence inisialisasi), init ADC dan DAC, init ADXL345 (measure mode, range), tampilkan device terdeteksi di serial/OLED. Jika device tidak ditemukan, status offline tapi sistem tetap jalan dengan device lain. Sinkronisasi waktu jika ESP32 punya WiFi: gunakan SNTP untuk timestamp (opsional). Jika device error saat init: retry 3x, jika tetap error tandai offline. Output startup: serial menampilkan semua device terdeteksi beserta ID dan status. OLED menampilkan splash screen "MODUL 08 SPI" lalu dashboard.

---

### Slide 36 — Alur Normal Project

Mode normal: baca ADC periodik (100-1000 ms), baca ADXL345 untuk akselerasi, validasi range data, update OLED SSD1306 dengan dashboard, buat record 18 byte, simpan ke W25Q64 Flash circular (page program), simpan ke SD Card CSV (setiap 10 record atau per menit), generate DAC output (misal: ramp mengikuti ADC channel 0), kirim ringkasan antar MCU via SPI slave/master, catat error count. Loop non-blocking: gunakan interrupt/DMA untuk efisiensi, jangan blocking lama di loop utama. Data dikirim ke serial monitor setiap 1 detik. Refresh OLED 10-30 fps (setiap 33-100 ms). DAC update sesuai frekuensi yang diinginkan (misal 1 kHz untuk sine wave). STM32 dan ESP32 sinkronisasi: ESP32 request data dari STM32 setiap 1 detik via SPI, STM32 respon dengan data ADC/Accel terbaru.

---

### Slide 37 — Error Mode Project

Jika device timeout/tidak respons: tandai offline, pakai device lain yang masih online. Recovery berkala: timeout detection, retry 3x, jika gagal tandai offline, jika berhasil kembalikan online. Jika SPI bus error: deinit SPI peripheral, reinit dengan konfigurasi benar, reset device jika ada pin RES (OLED, ADXL), lakukan dummy read/write untuk sync. Jika W25Q64 BUSY lama: polling status register, tunggu BUSY=0, jika timeout >5 detik, reset Flash. Error count dicatat di metadata Flash atau RAM. System tetap berjalan meskipun beberapa device offline (graceful degradation). Watchdog timer (jika ada) reset system jika hang. STM32 FreeRTOS: task watchdog monitor error_count, jika melebihi threshold lakukan SPI bus recovery atau system reset. ESP32: gunakan timer atau task RTOS untuk monitor health system.

---

### Slide 38 — Troubleshooting Wajib

Troubleshooting SPI: cek GND bersama (multimeter: 0 ohm), MOSI/MISO/SCLK tidak tertukar, CS pin tidak floating (pull-up jika perlu), mode SPI (CPOL/CPHA) sesuai datasheet, clock speed tidak melebihi kemampuan slave, SD Card init wajib 400 kHz lalu naik setelah ready, W25Q64 Write Enable (0x06) sebelum setiap write/erase, OLED pin DC dan RES terhubung benar, ADC MCP3008 Vref sesuai tegangan ukur, DAC MCP4921 Vref dan gain sesuai, ADXL345 CS=LOW untuk SPI mode, scanner sebagai langkah debug pertama. Gunakan oscilloscope/logic analyzer untuk validasi waveform SCLK, MOSI, MISO, CS. Cek voltage SDA/SCL... (maksudnya MOSI/MISO/SCLK) dengan multimeter: idle SCLK sesuai CPOL, CS=HIGH saat idle. Jika data korup: turunkan clock speed, periksa kabel lebih pendek, tambahkan resistor 10-100 ohm pada MOSI/MISO dekat master.

---

### Slide 39 — Deliverable Laporan

Laporan: teori SPI (MOSI/MISO/SCLK/CS, CPOL/CPHA, modes, full-duplex, multi-slave), tabel 25 eksperimen (kode, platform, device, mode, clock, CS, output, error, solusi), foto hardware wiring, screenshot output serial/OLED, analisis error dan troubleshooting, source code utama atau link repository, integrasi project Data Acquisition System, kesimpulan. Bahasa Indonesia, penamaan konsisten Modul 08, format laporan mengikuti template yang diberikan. Lampiran: foto wiring tiap kelompok eksperimen, screenshot serial monitor, OLED display, oscilloscope waveform (jika ada). Struktur laporan: Cover, Identitas, Daftar Isi, Teori, Metodologi (25 eksperimen), Hasil dan Pembahasan, Project, Kesimpulan, Daftar Pustaka, Lampiran. Sistematika sesuai panduan dosen pengampu.

---

### Slide 40 — Tugas Video Modul 08

Video 20–35 menit: pembukaan (nama, NIM, Modul 08, daftar hardware), ringkasan teori SPI (MOSI/MISO/SCLK/CS, CPOL/CPHA, 4 modes, full-duplex, CS management, daisy-chain vs independent CS), demo 7 STM32 SPI, demo 3 STM32 SPI RTOS, demo 7 ESP32 SPI, demo 3 ESP32 SPI RTOS, demo 3 Multi SPI non-RTOS, demo 2 Multi SPI RTOS, project final Data Acquisition System, troubleshooting, kesimpulan. Rekaman hardware dan screen recording wajib. Audio jelas, tampilan kode dan output terbaca. Demo wajib menunjukkan: SPI scanner, W25Q64 Flash read/write, SD Card SPI mode, OLED display, ADC read, DAC waveform, ADXL345 accel, RTOS multi-task, ESP32-STM32 SPI communication, error recovery. Bahasa Indonesia. Upload ke YouTube Unlisted atau link e-learning sesuai instruksi dosen.

---

### Slide 41 — Checklist Demo STM32 SPI

Demo STM32 SPI: SPI scanner (device terdeteksi, ID Flash, DEVID ADXL), W25Q64 Flash (read/write/verify), SD Card (baca/tulis blok 512 byte, FAT FS), SSD1306 OLED (teks/grafik tampil), MCP3008 ADC (8 channel, potentiometer berubah), MCP4921 DAC (waveform terukur multimeter/oscilloscope), ADXL345 (accel X,Y,Z berubah saat digerakkan). Output serial log dan OLED terlihat. RTOS: multi-task SPI (semaphore/mutex berfungsi), DMA transfer (lebih cepat dari polling), data logger (log ke Flash circular). Error recovery: timeout, retry, device offline mode, bus reinit. Hardware ESP32 dan STM32 terlihat, wiring MOSI/MISO/SCLK/CS dijelaskan. CPOL/CPHA sesuai mode device. Clock speed disebutkan (1 MHz, 10 MHz).

---

### Slide 42 — Checklist Demo ESP32 SPI

Demo ESP32 SPI: SPI master configuration (ESP-IDF, spi_bus_initialize, spi_bus_add_device), W25Q64 Flash ID (0xEF4017) dan memory map 8 MB, SD Card SPI mode (init 400 kHz, baca kapasitas, tulis baca blok), SSD1306 OLED SPI (dashboard real-time), MCP3008 ADC (potentiometer berubah), MCP4921 DAC (waveform ramp/triangle/sine), ADXL345 SPI (mode 3, accel berubah). Output serial log, OLED display, DAC multimeter reading terlihat. RTOS: multi-task SPI, DMA benchmark (speed comparison), interrupt-driven SPI. ESP32 GPIO matrix: MOSI/MISO/SCLK dapat dipindah ke GPIO lain (demo opsional). ESP32 sebagai SPI master, STM32 sebagai slave (MULTI_01): data transfer valid. Error recovery dan debugging tools (logic analyzer waveform) ditunjukkan.

---

### Slide 43 — Checklist Demo Multi SPI

Demo Multi SPI: MULTI_01 SPI Master-Slave Basic (STM32 slave, ESP32 master, data transfer valid), MULTI_02 SPI Multi Slave CS Management (W25Q64, OLED, ADC pada satu bus, CS management benar), MULTI_03 SPI Shared Bus Multi Device (ESP32 dan STM32 akses bus bersama, tanpa konflik), MULTI_04 SPI RTOS Gateway (STM32 baca sensor, ESP32 request data, display dashboard, RTOS queue/semaphore), MULTI_05 SPI Data Acquisition System (integrasi akhir: semua device berfungsi, logging, display, error recovery). Data konsisten di ESP32, STM32, OLED, serial. Komunikasi SPI antar-MCU stabil. Project final menunjukkan: startup scanner, sensor reading, OLED dashboard, Flash/SD logging, DAC output, error recovery, RTOS multi-task. Hardware wiring rapi, common ground, CS pins jelas.

---

### Slide 44 — Rubrik Penilaian

Penilaian: teori SPI lengkap (CPOL/CPHA, modes, full-duplex, CS management) 15%, demo 7 STM32 SPI 10%, demo 3 STM32 SPI RTOS 5%, demo 7 ESP32 SPI 10%, demo 3 ESP32 SPI RTOS 5%, demo 3 Multi SPI non-RTOS 10%, demo 2 Multi SPI RTOS 10%, project Data Acquisition System dual-MCU 20%, hardware demo dan wiring explanation 10%, video quality dan struktur presentasi 5%. Penalti: penamaan tidak konsisten (salah sebut Modul 07) -5%, demo tidak lengkap 25 eksperimen (proporsional), tidak jelaskan mode SPI -5%, tidak jelaskan CS management -5%, tanpa hardware demo -25%, audio tidak jelas -10%, video terlalu pendek <15 menit -10%, terlambat sesuai kebijakan kelas. Pastikan semua 25 eksperimen didemo atau minimal ditunjukkan screenshot/outputnya.

---

### Slide 45 — Kesimpulan Modul 08

Modul 08 bangun keterampilan SPI lengkap: teori bus (MOSI/MISO/SCLK/CS, CPOL/CPHA, 4 modes, full-duplex, multi-slave), device SPI (W25Q64 Flash, SD Card, SSD1306 OLED, MCP3008 ADC, MCP4921 DAC, ADXL345 Accel), STM32 HAL/register/DMA, ESP32 ESP-IDF/GPIO matrix, SPI RTOS multi-task, master-slave communication, error recovery, bus management, integrasi project Data Acquisition System dual-MCU. Hasil akhir: 25 eksperimen terdokumentasi, project final berjalan stabil dengan fitur akuisisi data, penyimpanan, penampilan, generasi sinyal, dan komunikasi antar-MCU. Perbandingan dengan Modul 07 I2C: SPI lebih cepat, butuh lebih banyak pin CS, full-duplex, tidak ada ACK/NACK, tidak perlu pull-up (push-pull). Mahasiswa siap menerapkan SPI pada sistem embedded industri, IoT, data acquisition, dan instrumentasi. SPI menjadi fondasi untuk komunikasi high-speed antar chip dalam sistem modern.
