# Modul 06: I2C Bus & Sensor Integration - Dokumen Komprehensif

---

## BAGIAN 1: TEORI & PRAKTIKUM (SLIDE 1-35)

### SLIDE 1: Pendahuluan I2C - Sejarah & Latar Belakang

I2C (Inter-Integrated Circuit) dikembangkan Philips Semiconductor tahun 1982 untuk komunikasi antar-chip dalam satu PCB dengan pin minimal. Hanya membutuhkan 2 jalur: SDA (Serial Data) dan SCL (Serial Clock), plus ground. Protokol ini bersifat half-duplex artinya data menjalani arah bergantian, bukan simultan. Open-drain output memerlukan pull-up resistor 4.7kΩ standar. Multi-master multi-slave memungkinkan hingga 127 device pada satu bus dengan addressing 7-bit unik setiap perangkat.

---

### SLIDE 2: Perbandingan I2C dengan SPI & UART

I2C menggunakan 2 wire (+ GND), SPI memerlukan minimal 4 (MOSI, MISO, CLK, CS), UART hanya 2 tetapi point-to-point saja. I2C max device 127, SPI unlimited (via chip select), UART hanya 1. Kecepatan I2C hingga 3.4 Mbps, SPI 10+ Mbps, UART ~1 Mbps. Kompleksitas I2C medium, SPI low, UART low. Saat butuh banyak sensor pada pin terbatas, I2C adalah pilihan. SPI lebih cepat tetapi butuh pin chip select lebih banyak.

---

### SLIDE 3: Arsitektur Bus I2C - Topologi & Koneksi

Bus I2C adalah sistem shared — semua device terhubung paralel pada 2 jalur sama (SDA dan SCL). Master (biasanya MCU) mengontrol SCL sebagai clock, semua slave menunggu. Pull-up resistor menarik SDA dan SCL ke HIGH saat idle — open-drain IC menghubungkan ke ground saja untuk tarik LOW. Multi-master dimungkinkan via arbitration — jika 2 master sama-sama ingin mengakses, yang menang adalah yang menarik SCL terendah. Setiap slave memiliki alamat unik 7-bit (0x00-0x7F).

---

### SLIDE 4: Pin I2C Mapping - ESP32 & STM32

ESP32: SDA default GPIO21, SCL GPIO22 (remap ke pin lain possible). STM32F103: I2C1 menggunakan PB7 (SDA), PB6 (SCL); I2C2 menggunakan PB11 (SDA), PB10 (SCL). Wajib konfigurasi GPIO mode "Open-Drain" (OD) di kedua platform. Pull-up resistor 4.7kΩ dari SDA dan SCL ke Vcc (3.3V di kedua MCU). Jika pakai modul breakout yang sudah ada on-board pull-up, perhatikan jangan double—pull-up terlalu banyak bisa cause low voltage levels.

---

### SLIDE 5: Sinyal I2C - SDA & SCL Behavior

SDA (Serial Data): jalur bidirectional, master dan slave sama-sama bisa tarik LOW. SCL (Serial Clock): jalur clock dari master, tapi slave bisa tarik LOW untuk clock stretching—memberitahu master "tunggu, saya belum siap." Data valid hanya saat SCL HIGH. Saat SCL LOW, SDA boleh berubah untuk persiapan bit selanjutnya. Idle state: SDA dan SCL keduanya HIGH karena pull-up. START: SDA turun saat SCL masih HIGH. STOP: SDA naik saat SCL HIGH.

---

### SLIDE 6: Kondisi START & STOP pada I2C

START condition: Master menarik SDA LOW terlebih dulu sementara SCL masih HIGH. Ini sinyal kepada semua slave "dengarkan, komunikasi dimulai." Byte address + R/W bit dikirim kemudian. STOP condition: Setelah data selesai, master melepas SDA (naik ke HIGH) saat SCL juga HIGH. Ini end-of-transmission signal. Repeated START (Sr): Master bisa kirim START ulang tanpa STOP dulu—digunakan untuk read setelah write (atomic operation) tanpa melepas bus.

---

### SLIDE 7: Frame Format I2C - Addressing & Bit R/W

Frame I2C: START → 7-bit Address → R/W bit (1=read, 0=write) → ACK dari slave → Data bytes → ACK → ... → STOP. Setiap byte data diikuti 9 pulse SCL—8 untuk data, 1 ke-9 untuk ACK/NACK. ACK (Acknowledge): slave menarik SDA LOW pada pulse ke-9. NACK (Not Acknowledge): SDA tetap HIGH (punya pull-up). Master baca SDA di pulse ke-9 utk tahu slave siap atau tidak. Jika NACK, master kirim STOP dan komunikasi gagal.

---

### SLIDE 8: ACK/NACK Mechanism - Handshake Protocol

Setiap byte (address maupun data) wajib diverifikasi via ACK. Master mengirim 8 bit, kemudian lepas SDA. Slave yang "kenal" address, atau sudah siap menerima data, menarik SDA LOW saat SCL pulse ke-9. Ini ACK—berarti "terima kasih, lanjut." Jika slave tidak menarik SDA (tetap HIGH), itu NACK berarti "saya tidak siap" atau "alamat tidak cocok." Master akan stop dan send STOP condition. ACK phase sangat penting untuk error detection—jika tidak ada sensor, master akan deteksi via NACK.

---

### SLIDE 9: Clock Stretching - Slave Delays Master

Clock stretching: slave bisa menarik SCL LOW untuk memberi tahu master "tunggu, saya butuh waktu process data." Master menunggu SCL naik sebelum lanjut. Contoh: master kirim address 8 bit + tunggu 9 detik clock. Sebelum SCL naik, slave tarik SCL LOW, master cek SCL LOW, jadi tunggu. Setelah slave siap (misal selesai read sensor internal), slave lepas SCL, SCL naik (pull-up), master lanjut kirim bit berikutnya. Mekanisme ini prevent data corruption jika slave tidak siap.

---

### SLIDE 10: Pull-Up Resistor - Resistor Formula & Nilai

Pull-up wajib open-drain: Rpu = (Vcc - Vol) / Iol. Contoh: Vcc=3.3V, Vol=0.4V (max), Iol=3mA, maka Rpu_max = 2.9/3 ≈ 967Ω (ambil 4.7kΩ aman). Nilai standar: 4.7kΩ untuk 100kHz short bus (default). 2.2kΩ untuk 400kHz atau bus panjang (faster rise time). 10kΩ untuk 100kHz very short (2-3 device) atau untuk minimize power. Jangan asal pakai 1kΩ terus—bisa cause quiescent current tinggi di pull-up resistor (3.3V/1k=3.3mA wasted).

---

### SLIDE 11: I2C Speed Modes - 100kHz hingga 3.4 Mbps

Standard Mode: 100 kHz, toleran semua device, recommended untuk sensor. Fast Mode: 400 kHz, untuk transfer data lebih cepat, butuh rise time lebih cepat. Fast Mode Plus: 1 MHz, drive resistif rendah (ketat). High Speed: 3.4 MHz, very specialized, jarang dalam embedded. Mode yang dipilih tergantung sensor terlambat. Mayoritas sensor I2C (OLED, BMP280, DS3231) support 400kHz atau lebih. Saat eksperimen di praktikum, mulai 100kHz aman, naikkan 400kHz jika all sensor OK.

---

### SLIDE 12: Alamat I2C Default Device - Tabel Referensi

SSD1306 OLED: 0x3C/0x3D. BMP280: 0x76/0x77. MPU6050: 0x68/0x69 (AD0 pin). AT24C32: 0x50. DS3231 RTC: 0x68 (⚠️ conflict MPU6050). BH1750: 0x23/0x5C. PCF8574 LCD: 0x27/0x3F. PENTING: 2 device alamat sama = collision, komunikasi rusak. Solusi: ubah pin AD0/ADDR sensor salah satu.

---

### SLIDE 13: Konfigurasi I2C STM32F103 - Inisialisasi HAL

Urutannya: (1) Enable clock __HAL_RCC_I2C1_CLK_ENABLE() dan __HAL_RCC_GPIOB_CLK_ENABLE(). (2) Config GPIO PB6, PB7 ke mode GPIO_MODE_AF_OD (Alternate Function Open-Drain). (3) Setup I2C_HandleTypeDef: ClockSpeed=400000 (400kHz), DutyCycle=I2C_DUTYCYCLE_2, OwnAddress1=0 (master mode), AddressingMode=7BIT, DualAddressMode=DISABLE. (4) Panggil HAL_I2C_Init(&hi2c1). Hasil: STM32 siap jadi I2C master.

---

### SLIDE 14: I2C Scanning pada STM32 - Deteksi Semua Device

Fungsi HAL_I2C_IsDeviceReady() cek address 0x00 hingga 0x7F. Loop: for (uint8_t addr=1; addr<128; addr++) { if (HAL_I2C_IsDeviceReady(&hi2c1, addr<<1, 1, 10)==HAL_OK) printf("Device at 0x%02X\n", addr); }. Catatan: address di-shift kiri 1 bit (addr<<1) karena HAL expect 8-bit value (7-bit address + 1-bit R/W). Timeout 10ms cukup. Hasil: daftar semua alamat device yang terhubung di bus. Gunakan tool ini jika ada device tidak detected.

---

### SLIDE 15: I2C Write & Read pada STM32 - Memory Operation

Write ke register: HAL_I2C_Mem_Write(&hi2c1, dev_addr<<1, reg_addr, I2C_MEMADD_SIZE_8BIT, data_buffer, data_len, timeout). Contoh: write 1 byte ke BMP280 register 0xF4: HAL_I2C_Mem_Write(&hi2c1, 0x76<<1, 0xF4, I2C_MEMADD_SIZE_8BIT, &control_byte, 1, 100). Read dari register: HAL_I2C_Mem_Read(&hi2c1, 0x76<<1, 0xF7, I2C_MEMADD_SIZE_8BIT, data_buffer, 3, 100) baca 3 byte dari register 0xF7. Return HAL_OK atau HAL_TIMEOUT untuk error handling.

---

### SLIDE 16: Konfigurasi I2C ESP32 - Arduino & ESP-IDF Style

Arduino style (simpel): Wire.begin(); Wire.setClock(400000);—sudah jadi master pada SDA=21, SCL=22. Specify pin: Wire.begin(21, 22, 400000);. ESP-IDF style lebih kontrol: i2c_config_t conf = {.mode=I2C_MODE_MASTER, .sda_io_num=21, .scl_io_num=22, .sda_pullup_en=GPIO_PULLUP_ENABLE, .scl_pullup_en=GPIO_PULLUP_ENABLE, .master.clk_speed=400000}; i2c_param_config(I2C_NUM_0, &conf); i2c_driver_install(...). Arduino lebih mudah untuk cepat start, ESP-IDF lebih flexible Custom GPIO.

---

### SLIDE 17: I2C Scanner pada ESP32 - Code Snippet Arduino

void scanI2C() { Wire.begin(); for (int addr=1; addr<127; addr++) { Wire.beginTransmission(addr); int error = Wire.endTransmission(); if (error==0) Serial.printf("Device at 0x%02X\n", addr); } }. Pada ESP32 Arduino, scan lebih sederhana: beginTransmission(addr) + endTransmission() return 0 jika ACK, bukan-0 jika NACK/error. Loop semua address, daftar yang ACK. Sama hasilnya dengan STM32 scanner—list semua perangkat terhubung.

---

### SLIDE 18: I2C Write & Read pada ESP32 - Wire Library

Write: Wire.beginTransmission(dev_addr); Wire.write(reg_addr); Wire.write(data); Wire.endTransmission();. Contoh: set BMP280 control 0xF4=0x25: Wire.beginTransmission(0x76); Wire.write(0xF4); Wire.write(0x25); Wire.endTransmission();. Read: Wire.beginTransmission(dev_addr); Wire.write(reg_addr); Wire.endTransmission(); Wire.requestFrom(dev_addr, num_bytes); while (Wire.available()) data[i++]=Wire.read();. Memory-mapped READ: write register address, lalu request data. Kenyamanan Wire library—abstraction sempurna untuk basic RW.

---

### SLIDE 19: Program P01: I2C Scanner - Tujuan & Langkah

Tujuan: Scanning bus I2C untuk deteksi alamat semua device terhubung. Perangkat minimum: 2-3 modul I2C (OLED, BMP280, BH1750) + pull-up 4.7kΩ. Langkah: (1) Hubung sensor ke bus (SDA=GPIO21/PB7, SCL=GPIO22/PB6). (2) Pasang pull-up 4.7k ke 3.3V pada SDA & SCL. (3) Cek power & ground. (4) Upload kode scanner. (5) Open serial monitor 115200 baud. (6) Catat alamat perangkat ditemukan. (7) Lepas 1 device, scan ulang—verifikasi address hilang.

---

### SLIDE 20: Program P02: OLED SSD1306 - Display Teori

OLED SSD1306 128×64 pixel monochrome display, I2C address 0x3C atau 0x3D, power 3.3V (atau toleran 5V). Library Adafruit_SSD1306: display.begin(SSD1306_SWITCHCAPVCC, 0x3C) inisialisasi. display.clearDisplay() kosongkan buffer RAM sampai hitam. display.setTextSize(1), setCursor(x,y), println("text") tulis ke buffer. display.display() kirim buffer ke OLED lalu tampil. Refresh tiap 100ms aman. OLED sendiri tidak consume banyak power vs LCD—organic emissive, tinggi kontras, cocok battery powered.

---

### SLIDE 21: Program P03: BMP280 Suhu & Tekanan - Percobaan Lapangan

BMP280 sensor suhu (-40~+85℃ ±1℃), tekanan (300-1100hPa ±1hPa), I2C address 0x76/0x77, pin CSB ke 3.3V (mode I2C, bukan SPI). Setup: BMP280 harus di-init via library Adafruit_BME280 (note: library BME280 untuk BME280 yg ada humidity, sementara BMP280 tidak). Baca: temp_C = bme.readTemperature(), press_hPa = bme.readPressure()/100. Ketinggian: h = 44330 × (1-(P/P0)^0.1903). Eksperimen: tiup sensor (napas hangat)—suhu naik karena panas napas. Bandingkan tekanan measured vs weather forecast lokal.

---

### SLIDE 22: Program P04: MPU6050 Akselerometer & Giroscope - 6-axis

MPU6050: 3-axis accel (±2/4/8/16g) + 3-axis gyro (±250/500/1000/2000°/s), addr 0x68/0x69 (AD0). Setup: register 0x6B bit7=0 wake up. Data: Ax,Ay,Az (g), Gx,Gy,Gz (°/s). Eksperimen: flat→Z≈1g, miring→X,Y berubah. Roll=atan2(Ay,√(Ax²+Az²)), Pitch=atan2(-Ax,√(Ay²+Az²)).

---

### SLIDE 23: Program P05: EEPROM AT24C32 - Non-volatile Storage

AT24C32: 4KB (32×128byte), addr 0x50-0x57, 2-byte ptr. Write: addr MSB/LSB + data, tunggu 5ms. Read: addr then request. Page boundary: 32-byte page—wrap jika tulis >32byte. Praktik: tulis "Hello EEPROM", power off 1m, power on—data tetap (non-volatile magic).

---

### SLIDE 24: Program P06: RTC DS3231 - Real-Time Clock Presisi

DS3231: akurat ±2ppm (±60 detik/tahun), battery backup CR2032, suhu internal 0.25℃ resolution, I2C address 0x68 (fixed). Data BCD format (Binary-Coded Decimal): 2 nibble per byte, contoh suhu 23℃ = 0x23. Setup RTC: set time sekali di code (DateTime(2024,3,15,14,30,0)). Baca: now = rtc.now(), akses .year(), .month(), .day(), .hour(), .minute(), .second(). Internal temperature: rtc.getTemperature() baca suhu internal DS3231 akurat ±3℃. Loss power test: cabut power 5 menit, power on—waktu lanjut sesuai battery, tdk reset.

---

### SLIDE 25: Program P07: BH1750 Light Sensor - Lux Measurement

BH1750: sensor cahaya ambient yang output lux (brightness intensity), I2C address 0x23/0x5C (ADDR pin control), sensitivity hingga 0.5 lux. Mode: Continuous High (1 lux resolution, ~180ms), Continuous High2 (0.5 lux, ~180ms), Continuous Low (4 lux, ~24ms). Setup: BH1750.begin(), setMode(). Baca: lux = bh1750.readLightLevel(). Eksperimen: gelap=0-10 lux, ruangan 100-1000, outdoor 10000-100000 lux. Aplikasi: auto brightness SCL, day/night detection, farming crop monitoring.

---

### SLIDE 26: Program P08: LCD PCF8574 - 16x2 Character Display

LCD 16×2 dengan I2C backpack PCF8574 (address 0x27/0x3F), butuh 5V power tapi I2C level 3.3V OK. Library LiquidCrystal_I2C: lcd.init(), lcd.backlight() nyalakan, lcd.setCursor(col, row), lcd.print("text") tulis. Karakteristik: 16 kolom × 2 baris, 8 custom karakter slot tersedia. Backlight: LED putih/biru, bisa on/off via software. Kontras trimpot potentiometer 10kΩ di belakang modul. Eksperimen: tampil counter yang increment, scroll teks, custom karakter (panah, derajat Celcius).

---

### SLIDE 27: Program P09: Multi-Sensor Read - Efisiensi Siklus I2C

Baca multiple sensor (BMP280, BH1750, MPU6050) dalam 1 siklus loop tanpa delay berlebihan. Pseudocode: loop { temp = bmp280.readTemp(); pressure = bmp280.readPressure(); lux = bh1750.readLux(); accel = mpu6050.readAccel(); } delay(100); walaupun 3 sensor, total I2C transaction masih ~5-10ms. Optimasi: batch register read jika ada—BMP280 bisa baca 6 register data sekaligus datanya keluar berurutan di memory. Measure timing: gunakan millis() sebelum & sesudah, catat total waktu per siklus.

---

### SLIDE 28: Program P10: Raw I2C Read/Write STM32 - Tanpa Library

Akses register langsung via HAL tanpa Adafruit/RTCLib. Contoh BMP280: (1) read chip ID dari 0xD0, expect 0x60. (2) set control register 0xF4 = 0x25 (oversampling x16, normal mode). (3) read pressure data dari 0xF7:0xF9 (3 byte), combine: pressure_raw = (MSB<<12) | (LSB<<4) | (XLSB>>4). (4) compensate pressure via calibration data di register 0x88-0xA1. Tujuan: skill menulis driver sensor dari scratch, bukan plug&play library, pahami protocol mendalam.

---

### SLIDE 29: Program P11: Clock Speed Optimization Test - 100kHz vs 400kHz

Compare performance 100kHz vs 400kHz untuk setiap sensor. Loop: baca sensor 100x, catat waktu elapsed, hitung timing per read. Print hasil dengan dua speed. 100kHz: ~30-50ms per full multi-sensor cycle. 400kHz: ~8-15ms per cycle (3-4x lebih cepat). Trade-off: 400kHz memerlukan pull-up resistor tighter (2.2k better dari 4.7k), risiko noise di bus panjang. Rekomendasi: produksi pakai 400kHz (aman semua sensor modern support), lab pakai 100kHz jika ada connection issue.

---

### SLIDE 30: Program P12: Error Recovery & Reconnection - Robustness

Sensor sometimes disconnect (loose cable, power glitch). Error handling: jika read failed (NACK/timeout), mark sensor offline flag, tidak reset seluruh sistem. Re-init ulang: jika sensor ditemukan offline, per interval 5 detik coba HAL_I2C_IsDeviceReady() lagi. Rescan full bus jika > 1 sensor offline sekaligus (indication restart atau tampering). Log error count per device. Pengalaman praktikum: lebih sering error adalah kabel loose atau pull-up lemah, bukan chip rusak.

---

### SLIDE 31: Troubleshooting I2C - Common Issues & Solutions

(1) Device tidak terdeteksi: cek power supply, ground. (2) Hangs/timeout: alamat salah, device belum terhubung, low battery pada RTC backup. (3) Data corrupted: pull-up resistor lemah (rise time lambat), bus panjang noisy, clock stretching tidak handled. (4) I2C locked (SCL/SDA stuck LOW): use GPIO bitbang reset—pulse SCL 9x, output STOP condition, or hardware reset. (5) Conflict alamat: gunakan I2C scanner, identifikasi double address, ubah ADDR pin. (6) Temperature sensor inaccurate: ±2℃ normal, bukan design flaw.

---

### SLIDE 32: Pull-up Resistor Tuning - Praktik Lapangan

Default: 4.7kΩ. Unstable 400kHz→2.2kΩ. Ultra-low power→10kΩ (short bus <50cm). Check: SDA/SCL rise time <300ns (fast mode). Warning: double pull-up (on-board+manual)→LOW voltage problem (0.8V vs 3.3V).

---

### SLIDE 33: Sensor Selection & Datasheet Reading

Datasheet adalah "alkitab" sensor. Min info: (1) I2C address (dan cara ubah jika ADDR pin ada). (2) Register map (control, data, status). (3) Timing constraints (read/write delay). (4) Calibration data (offset, scale factor). (5) Supply voltage & current rating. (6) Temperature operating range & accuracy spec. Contoh BMP280 datasheet: calibration data embedded di EEPROM internal (0x88-0xA1), harus di-read saat init, lalu gunakan pada every measurement utk kompensasi non-linear.

---

### SLIDE 34: I2C di Praktik Nyata: Sistem Embedded Dunia

Aplikasi: (1) Smartphone: PMIC, charger, light, accel. (2) Car: suhu mesin, tekanan ban, dashboard. (3) Smart grid: meter, sensor arus. (4) Smart home: temp/humidity sensors. (5) Medical: pulse ox, glucose, BP cuff. Kunci: simple, reliable, power-efficient—standar 40+ tahun.

---

### SLIDE 35: Summary Bagian 1 - Teori & Praktikum I2C Sensor

Teori: Protocol I2C 2-wire, half-duplex, multi-slave, 7-bit addressing, open-drain output, pull-up mandatory, ACK/NACK handshake. Kecepatan 100/400kHz/1Mbps. Praktikum STM32 & ESP32: scan device, OLED SSD1306, BMP280 temp, MPU6050 accel, EEPROM write/read, RTC DS3231, BH1750 lux, LCD PCF8574, multi-sensor, raw I2C, clock speed test, error recovery. Semua 12 program foundation untuk memahami komunikasi antar-chip modern. Skill ini dipakai di AI edge device, drone, robot, sensor IoT.

---

## BAGIAN 2: PROJECT (SLIDE 36-40)

### SLIDE 36: Project Overview - Weather Station Multi-Sensor I2C

Skenario: Koperasi petani "Tani Maju" Malang butuh stasiun cuaca otomatis untuk 5 lahan pertanian. Pengukuran: suhu, tekanan udara, kelembaban, intensitas cahaya, orientasi arah angin (via akselerometer). Display real-time LCD + OLED, data logging EEPROM dengan timestamp RTC akurat: Tidak hilang saat power off. ESP32 hub utama—baca BMP280, BH1750, MPU6050, tampil di OLED + LCD. STM32 logger cadangan—akses sensor via raw I2C (register level), sinkronisasi UART ke ESP32.

---

### SLIDE 37: Project Architecture & Device Configuration

Bus I2C shared: ESP32 master utama, STM32 master alternatif (time-multiplex, tdk bersamaan). Device: BMP280 (0x76 suhu/tekanan), BH1750 (0x23 cahaya), MPU6050 (0x69, AD0=HIGH arah angin), AT24C32 EEPROM (0x50 circular data log), DS3231 RTC (0x68 timestamp), SSD1306 OLED (0x3C display), PCF8574 LCD (0x27 display cadangan). Modul: ADC_I2C.c (baca semua 6 sensor), Storage_I2C.c (EEPROM circular buffer), Display_I2C.c (OLED + LCD graphics), RTC_I2C.c (timestamp), ErrorRecovery.c (sensor reconnect).

---

### SLIDE 38: Project Flow & Operational Modes

**Mode Startup** (0-5 detik): I2C scanner detect semua device, map address, init OLED tampil "Initializing...". **Mode Normal Active** (5 detik+): loop baca 6 sensor per 10 detik, update OLED graphics (temperature graph 24h, ikon cuaca, alert if anomaly), log ke EEPROM circular (max 1000 record, auto wrap). **Error Handling**: jika 1 sensor dropout, flag offline, skip, continue read 5 sensor lain. Setiap 30 detik retry reconnect sensor. **Power Loss Recovery**: RTC backup battery maintain time, EEPROM non-volatile retain log, saat power up lanjut log tanpa hilang history.

---

### SLIDE 39: Implementasi Detail Project - Coding Guidelines

STM32 raw I2C: akses BMP280 register 0xF7-0xF9 via HAL_I2C_Mem_Read(), multiply 3 byte: pressure_raw=(data[0]<<12)|(data[1]<<4)|(data[2]>>4). ESP32 pakai library Adafruit saja efficiency. Circular buffer EEPROM: pointer_write (address K), pointer_read (address K+1), setiap new record: ptr_write = (ptr_write + 1) % 1000; write record ptr_write. Display OLED: refresh 1 Hz (100ms interval), draw bar graph suhu min/max/avg, BH1750 lux bar, MPU6050 vector magnitude (isometric plot). Modularization sangat importance—ADC, Storage, Display terpisah file/function reusable.

---

### SLIDE 40: Project Deliverables & Rubrik Assessment

Code moduler: ADC_I2C.c, Storage_I2C.c, Display_I2C.c, RTC_I2C.c, ErrorRecovery.c. Multi-sensor (BMP280+BH1750+MPU6050): 20%. Display OLED+LCD real-time: 15%. EEPROM data log + RTC timestamp: 20%. Scanner + error recovery: 15%. STM32 raw I2C: 10%. Clock speed optimization: 5%. Documentation & code quality: 10%. Final demo video: 5%. Working system expected saat akhir 2 minggu project.

---

## BAGIAN 3: TUGAS VIDEO (SLIDE 41-45)

### SLIDE 41: Tugas Video Requirements - Format & Platform

Video laporan praktikum Modul 06: durasi 15-25 menit, upload YouTube Unlisted (private but shareable), submit link di e-learning. Konten: webcam + screen recording, hardware physical pada meja (tidak animasi/CGI). Kualitas: 720p minimum, audio jelas (noise floor <-40dB), subtitle Indonesia (optional tapi recommended agar mudah dipahami kalau audio amburadul). Equipment: smartphone/webcam untuk PiP (Picture-in-Picture), 1 layar display kode, 1 camera untuk sensor hardware close-up.

---

### SLIDE 42: Struktur Video - Durasi & Konten Breakdown

**Sec 1 (1-2m)**: Pembukaan, nama, NIM, topik. **Sec 2 (2-3m)**: Teori—I2C 2-wire, SDA/SCL, 7-bit addr, pull-up, clock stretch, 100/400kHz, vs SPI/UART. **Sec 3 (8-14m)**: Demo P01-P12: scanner, OLED, BMP280, MPU6050, EEPROM, RTC, BH1750, LCD, multi-sensor, raw I2C, speed, recovery.

---

### SLIDE 43: Demonstrasi Hardware & Project Highlight

Hardware tracking: (1) Setup board ESP32 + STM32 + breadboard sensor tertata rapi. (2) Tunjuk koneksi kabel SDA SCL pull-up resistor pada camera close-up (90 detik footage). (3) Power on, serial monitor window tampil di screen, device addresses muncul. (4) Demo project weather station: OLED menampilkan grafik suhu 24h, LCD display cuaca, EEPROM data log terisi periodik. (5) Error demo: cabut 1 sensor connector, layar tampil "BH1750 offline", nunggu beberapa detik dia auto-reconnect "Device reconnected". (6) Final: tunjuk code structure modular, dokumentasi.

---

### SLIDE 44: Video Quality & Submission Checklist

Aspek kualitas: (1) Lighting cukup di wajah presentor (webcam). (2) Suara jernih, gunakan mic extern jika audio laptop jelek. (3) Screen sharing resolution cukup baca kode (14pt font). (4) Hardware camera video stabil (guna tripod, jangan handheld berguncang). (5) Editing minimal—boleh cut silence, paste bagian sensor demo, jangan "effect-happy." (6) Subtitle harfiah 95% akurat (gunakan tools autocaption YouTube, review & edit manual). Submission: link YouTube Unlisted, buka access untuk user dengan link boleh, private OK.

---

### SLIDE 45: Penalti & Tips Lolos - Praktik Final Presentasi

Penalti: Webcam-20%, no hardware-20%, durasi-10%, terlambat-10%. Tips: (1) Latihan 3x script. (2) OBS record 1080p. (3) Draft test audio/video. (4) Script detail jelas. (5) Malam upload (CPU rileks). (6) Simulasi live sebelum rekam.

---

## KESIMPULAN & REKOMENDASI LANJUT

Modul 06 I2C mengajarkan komunikasi multi-device fundamental modern embedded. Masukkan lanjut: wireless (WiFi 6LoWPAN, zigbee pakai chip I2C), IoT data logging cloud, sensor fusion (kalman filter accel+gyro+compass), embedded Linux (Raspberry Pi pakai I2C HAT modul). Praktikum ini fondasi system desktop-less—dari smart home, agriculture 4.0, industrial IoT semuanya berpacu pada bus I2C/SPI tiap device talk satu sama lain. Skill ini adalah asset karir engineer embedded jangka panjang.

---

*Dokumen ini disusun agar mudah di-proses NotebookLM atau sistem AI—struktur linear, bahasa ringkas, analogi praktik, perintah eksplisit setiap slide. Total 45 slide, 250-280 karakter per slide, detail komprehensif I2C protocol + 12 praktikum + project + video requirement.*
