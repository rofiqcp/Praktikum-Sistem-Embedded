# Modul 07: I2C Bus dan Integrasi Sensor — NotebookLM 45 Slide

## Bagian 1 — Teori I2C dan Platform (Slide 1–15)

### Slide 1 — Modul 07 dan Target Akhir

Modul 07 membahas protokol komunikasi I2C bus dan integrasi berbagai sensor pada platform mikrokontroler ESP32, STM32, dan sistem gabungan multi-MCU yang menjadi fondasi pengembangan sistem embedded modern. Target akhir pembelajaran modul ini adalah penyelesaian 25 eksperimen final yang terdiri dari 10 eksperimen ESP32, 10 eksperimen STM32, dan 5 eksperimen Multi STM32-ESP32 yang dirancang untuk membangun kompetensi praktis mahasiswa. Semua percobaan yang dilakukan di laboratorium diarahkan untuk mendukung pengembangan project akhir berupa weather station dual-MCU yang mengintegrasikan pembacaan sensor cuaca, penyimpanan data, penampilan informasi, dan komunikasi antar-mikrokontroler secara stabil. Eksperimen ESP32 mencakup penggunaan I2C scanner untuk pemetaan alamat device, integrasi sensor BME280/BMP280 dengan mekanisme fallback, penampilan data pada OLED SSD1306, integrasi RTC DS3231 dengan sinkronisasi SNTP, pencatatan data pada EEPROM, pembacaan akselerometer dan giroskop MPU6050, pengukuran intensitas cahaya BH1750, penampilan data pada LCD PCF8574, pengujian GPIO matrix untuk remapping pin I2C, dan penanganan error I2C seperti NACK dan timeout. Eksperimen STM32 menggunakan STM32 HAL untuk komunikasi I2C, pembuatan driver register mentah untuk sensor, pengendalian OLED via HAL, pembacaan DS3231 dan sinkronisasi ke RTC internal, penyimpanan data pada EEPROM dengan penanganan page boundary, pembacaan MPU6050 via burst read, pengukuran BH1750 pada mode continuous dan one-shot, pengendalian LCD PCF8574, perbandingan metode transfer I2C polling, interrupt, dan DMA, serta recovery bus I2C saat terjadi error. Eksperimen Multi mengimplementasikan komunikasi master-slave I2C antara STM32 sebagai slave dan ESP32 sebagai master, sinkronisasi waktu SNTP dari ESP32 ke DS3231 dan STM32, serta dashboard weather station terintegrasi yang menampilkan data konsisten pada OLED ESP32 dan LCD STM32. Setiap eksperimen dirancang untuk membangun pemahaman bertahap mulai dari teori dasar I2C hingga integrasi sistem kompleks yang siap diaplikasikan pada proyek nyata. Mahasiswa diwajibkan mendokumentasikan setiap langkah percobaan, mulai dari wiring hardware, kode program, output serial, hingga analisis error yang ditemui selama praktikum berlangsung. Laporan praktikum harus disusun sesuai format yang telah ditentukan.

---

### Slide 2 — Dasar Bus I2C

I2C atau Inter-Integrated Circuit adalah protokol komunikasi serial synchronous yang memakai dua jalur utama yaitu SDA (Serial Data) untuk transfer data dan SCL (Serial Clock) untuk sinkronisasi clock. Semua device yang terhubung ke bus I2C berbagi kedua jalur yang sama sehingga penggunaan pin mikrokontroler menjadi sangat hemat dibandingkan protokol komunikasi lain seperti SPI yang membutuhkan pin terpisah untuk setiap slave. Komunikasi pada bus I2C bersifat half-duplex yang berarti data hanya dapat ditransfer satu arah pada satu waktu, baik dari master ke slave maupun sebaliknya. Master pada bus I2C adalah device yang mengatur transaksi komunikasi dengan membangkitkan sinyal clock dan mengirimkan alamat slave, sedangkan slave adalah device yang merespons transaksi berdasarkan alamat yang telah ditetapkan. Setiap device slave memiliki alamat unik 7-bit atau 10-bit yang memungkinkan master mengakses device yang diinginkan tanpa konflik alamat. Ground bersama antar device wajib dipastikan agar level logika yang digunakan dapat divalidasi dengan benar, karena perbedaan level ground dapat menyebabkan komunikasi gagal atau kerusakan device. Bus I2C mendukung multi-master meskipun pada praktikum Modul 07 fokus utama adalah konfigurasi single master untuk memudahkan pemahaman dasar. Kecepatan komunikasi I2C dapat diatur mulai dari 100 kHz untuk mode standar hingga 3.4 MHz untuk high speed mode, namun praktikum menggunakan 100 kHz sebagai standar awal untuk memastikan stabilitas komunikasi. Setiap transaksi dimulai dengan sinyal START dan diakhiri dengan sinyal STOP yang dihasilkan oleh master, dengan sinyal START terjadi saat SDA turun ketika SCL berada pada level HIGH dan sinyal STOP terjadi saat SDA naik ketika SCL berada pada level HIGH. Data ditransfer dalam satuan byte dengan bit paling signifikan (MSB) dikirimkan terlebih dahulu, dan setiap byte diikuti oleh bit ACK atau NACK pada clock ke-9 untuk memastikan penerimaan data berhasil.

---

### Slide 3 — Open-Drain dan Pull-Up

SDA dan SCL pada bus I2C bersifat open-drain yang berarti setiap device hanya dapat menarik jalur tersebut ke level LOW, sedangkan resistor pull-up eksternal diperlukan untuk menarik jalur kembali ke level HIGH saat tidak ada device yang menarik jalur ke LOW. Karena karakteristik ini, level LOW pada bus I2C bersifat dominan dibandingkan level HIGH, sehingga sinyal LOW selalu mengoverride sinyal HIGH jika terjadi konflik. Nilai resistor pull-up yang umum digunakan adalah 4.7 kΩ untuk kecepatan bus 100 kHz dan 2.2–4.7 kΩ untuk kecepatan 400 kHz, namun nilai ini dapat disesuaikan tergantung pada kapasitansi total bus, panjang kabel, dan jumlah device yang terhubung. Kapasitansi bus yang terlalu tinggi dapat menyebabkan rise time sinyal terlalu lambat sehingga melampaui batas spesifikasi I2C, yang akan mengakibatkan error komunikasi. Praktikum Modul 07 menggunakan resistor pull-up 4.7 kΩ untuk sebagian besar eksperimen pada kecepatan 100 kHz, dan mahasiswa diwajibkan mengukur rise time sinyal SDA dan SCL menggunakan oscilloscope untuk memastikan nilai pull-up yang digunakan sesuai dengan spesifikasi. Jika rise time terlalu lambat, nilai pull-up dapat diperkecil (misalnya menjadi 2.2 kΩ) untuk mempercepat transisi ke level HIGH, namun perlu diperhatikan bahwa nilai pull-up yang terlalu kecil akan menyebabkan arus yang mengalir saat jalur ditarik ke LOW menjadi terlalu besar dan dapat merusak pin I/O mikrokontroler. Beberapa sensor seperti BME280 dan BH1750 sudah memiliki resistor pull-up internal, namun pada praktikum tetap disarankan menggunakan pull-up eksternal untuk memastikan stabilitas bus.

---

### Slide 4 — Addressing I2C

Device I2C umum memakai alamat 7-bit yang dikirimkan dalam satu byte bersama dengan bit R/W (Read/Write) pada bit paling rendah (LSB), di mana bit R/W bernilai 0 untuk operasi write dan 1 untuk operasi read. STM32 HAL sering meminta alamat slave digeser kiri satu bit (`addr << 1`) karena HAL mengharapkan alamat 8-bit yang sudah mencakup bit R/W, sedangkan ESP32 API umumnya memakai alamat 7-bit langsung tanpa pergeseran. 10-bit addressing ada sebagai konsep lanjutan untuk mendukung lebih banyak device pada bus yang sama, namun jarang digunakan pada sensor yang dipakai dalam praktikum Modul 07. Alamat sensor yang digunakan dalam praktikum meliputi SSD1306 0x3C, BH1750 0x23, EEPROM AT24C32 0x50, DS3231 0x68, MPU6050 0x68/0x69, dan BME/BMP 0x76/0x77, di mana alamat MPU6050 dan DS3231 sama (0x68) sehingga tidak boleh dihubungkan pada bus yang sama tanpa modifikasi alamat. Konflik alamat ini menjadi salah satu materi troubleshooting yang wajib dipahami mahasiswa, dengan solusi berupa pemisahan bus atau perubahan alamat salah satu device jika memungkinkan. I2C scanner pada eksperimen ESP32_01 dan STM32_01 digunakan untuk mendeteksi alamat device yang terhubung pada bus dan memverifikasi tidak ada konflik alamat sebelum melakukan eksperimen lebih lanjut. Mahasiswa juga diwajibkan mencatat alamat setiap device yang digunakan pada laporan praktikum untuk memastikan kesesuaian dengan spesifikasi sensor.

---

### Slide 5 — ACK dan NACK

Setiap byte data yang ditransfer pada bus I2C diikuti oleh clock ke-9 yang digunakan untuk sinyal ACK (Acknowledgment) atau NACK (Not Acknowledgment) dari receiver. ACK berarti receiver menarik SDA ke level LOW pada clock ke-9, menandakan bahwa byte data telah diterima dengan sukses dan transmitter dapat melanjutkan pengiriman data berikutnya. NACK berarti SDA tetap pada level HIGH pada clock ke-9, yang dapat terjadi karena beberapa alasan seperti alamat slave tidak ditemukan, device sedang sibuk, device mati, wiring bermasalah, atau transmitter mengakhiri transfer data. Driver I2C wajib mengecek status ACK/NACK setelah setiap pengiriman byte untuk memastikan komunikasi berjalan dengan benar, dan melakukan penanganan error jika NACK diterima. Pada eksperimen STM32, HAL I2C mengembalikan status error jika NACK diterima, sedangkan pada ESP32, error code dapat dibaca untuk mengetahui penyebab kegagalan komunikasi. NACK juga dihasilkan oleh slave jika master mencoba membaca data lebih dari yang tersedia pada register slave, atau jika slave sedang dalam mode low power dan belum siap merespons. Praktikum Modul 07 mewajibkan mahasiswa menguji skenario NACK dengan mencabut salah satu sensor dari bus saat komunikasi berlangsung, kemudian mengamati pesan error yang dihasilkan dan melakukan recovery sesuai prosedur yang telah diajarkan. Hal ini bertujuan untuk melatih kemampuan mahasiswa dalam menangani error komunikasi I2C di lapangan.

---

### Slide 6 — START, STOP, Repeated START

Sinyal START terjadi saat SDA turun (transisi HIGH ke LOW) ketika SCL berada pada level HIGH, yang menandakan awal dari sebuah transaksi I2C. Sinyal STOP terjadi saat SDA naik (transisi LOW ke HIGH) ketika SCL berada pada level HIGH, yang menandakan akhir dari transaksi I2C. Repeated START adalah sinyal START baru yang dikirimkan tanpa mengirimkan sinyal STOP terlebih dahulu, yang umum digunakan untuk operasi baca register: master mengirimkan alamat slave dan register yang ingin dibaca dengan sinyal START, kemudian mengirimkan Repeated START sebelum membaca data dari slave, sehingga transaksi tetap atomik dan tidak direbut oleh master lain pada bus multi-master. Penggunaan Repeated START sangat penting untuk memastikan bahwa operasi tulis alamat register dan baca data tidak terinterupsi, yang dapat menyebabkan pembacaan data yang salah. Pada eksperimen STM32_02 (driver raw register BME280), mahasiswa diwajibkan mengimplementasikan sinyal START, Repeated START, dan STOP secara manual menggunakan register I2C STM32 untuk memahami mekanisme dasar komunikasi I2C. Sinyal START dan STOP hanya boleh dihasilkan oleh master, sedangkan slave tidak diperbolehkan menghasilkan sinyal ini kecuali pada mode tertentu yang tidak digunakan dalam praktikum. Mahasiswa juga diwajibkan mengamati sinyal START, STOP, dan Repeated START menggunakan oscilloscope untuk memverifikasi bahwa timing sinyal sesuai dengan spesifikasi I2C.

---

### Slide 7 — Speed Mode

Mode kecepatan I2C yang umum digunakan meliputi Standard Mode 100 kHz, Fast Mode 400 kHz, Fast Mode Plus 1 MHz, dan High Speed Mode 3.4 MHz. Praktikum Modul 07 dimulai dari kecepatan 100 kHz untuk memastikan stabilitas komunikasi pada eksperimen dasar, kemudian diuji pada kecepatan 400 kHz pada eksperimen lanjutan untuk melihat pengaruh kecepatan terhadap performa dan reliabilitas. Kecepatan maksimum yang dapat dicapai dipengaruhi oleh beberapa faktor seperti nilai resistor pull-up, panjang kabel, kapasitansi total bus, noise lingkungan, dan kemampuan device slave yang terhubung. Beberapa sensor seperti BME280 mendukung kecepatan hingga 400 kHz, sedangkan sensor lain seperti DS3231 hanya mendukung hingga 100 kHz, sehingga kecepatan bus harus disesuaikan dengan kemampuan device terlemah yang terhubung. Pada eksperimen ESP32_09, mahasiswa menguji pengaruh kecepatan bus 100 kHz vs 400 kHz terhadap refresh rate OLED SSD1306 dan laju pembacaan sensor, serta mengamati apakah terjadi error komunikasi pada kecepatan yang lebih tinggi. Jika error terjadi, mahasiswa harus menurunkan kecepatan bus atau memperbaiki nilai pull-up untuk memastikan komunikasi stabil. STM32 dan ESP32 mendukung konfigurasi kecepatan I2C melalui register atau API HAL/ESP-IDF, dan mahasiswa diwajibkan mencatat kecepatan yang digunakan pada setiap eksperimen pada laporan praktikum.

---

### Slide 8 — Clock Stretching

Clock stretching terjadi saat slave menahan SCL pada level LOW setelah clock ke-8 dari byte data, sehingga master harus menunggu hingga slave melepaskan SCL sebelum melanjutkan transaksi. Mekanisme ini digunakan ketika slave belum siap untuk menerima atau mengirimkan data berikutnya, misalnya saat sensor sedang melakukan konversi analog ke digital, EEPROM sedang dalam siklus penulisan, atau slave sedang memproses data internal. Master wajib mendukung timeout saat clock stretching terjadi agar sistem tidak hang jika slave terlalu lama menahan SCL atau mengalami error sehingga tidak melepaskan SCL. Pada praktikum Modul 07, DS3231 dan EEPROM AT24C32 dapat melakukan clock stretching saat sedang melakukan operasi internal, sehingga mahasiswa diwajibkan mengonfigurasi timeout I2C pada STM32 dan ESP32 untuk menangani skenario ini. Jika timeout terjadi, master harus membatalkan transaksi, melakukan recovery bus, dan mencoba kembali komunikasi setelah jeda waktu tertentu. Beberapa implementasi I2C pada mikrokontroler tidak mendukung clock stretching secara hardware, sehingga perlu ditangani secara software dengan memeriksa status SCL secara periodik. Mahasiswa juga diwajibkan mengamati clock stretching menggunakan oscilloscope pada saat sensor sedang sibuk untuk memahami mekanisme ini secara visual.

---

### Slide 9 — Device Modul 07

Device utama yang digunakan dalam Modul 07 meliputi sensor BME280/BMP280, OLED SSD1306, RTC DS3231, EEPROM AT24C32/24LC256, MPU6050, BH1750, dan LCD PCF8574. BME280 dapat membaca suhu, tekanan udara, dan kelembapan, sedangkan BMP280 hanya membaca suhu dan tekanan udara dengan chip ID 0x58 berbeda dengan BME280 yang memiliki chip ID 0x60. SSD1306 adalah OLED display 128x64 piksel yang menggunakan alamat I2C 0x3C dan sering digunakan untuk menampilkan data sensor secara real-time. DS3231 adalah RTC presisi tinggi dengan baterai backup yang menggunakan alamat I2C 0x68, sama dengan alamat MPU6050 sehingga keduanya tidak boleh berada pada bus yang sama. EEPROM AT24C32 berkapasitas 4 KB dan 24LC256 berkapasitas 32 KB digunakan untuk menyimpan log data sensor secara circular. MPU6050 menggabungkan akselerometer 3-axis dan giroskop 3-axis dengan alamat 0x68 atau 0x69 tergantung pada pin AD0. BH1750 adalah sensor intensitas cahaya lux dengan alamat I2C 0x23 yang digunakan pada ESP32_07 dan STM32_07. LCD PCF8574 adalah LCD 16x2 dengan modul I2C expander yang menggunakan alamat 0x27 atau 0x3F, sering digunakan untuk menampilkan data dua baris. Semua alamat device ini wajib dicatat pada laporan praktikum dan diverifikasi menggunakan I2C scanner sebelum digunakan.

---

### Slide 10 — BME280 dan BMP280 Fallback

BME280 adalah sensor cuaca yang mampu membaca suhu, tekanan udara, dan kelembapan dengan akurasi tinggi, sedangkan BMP280 hanya membaca suhu dan tekanan udara tanpa kemampuan membaca kelembapan. Driver yang digunakan pada praktikum wajib membaca chip ID terlebih dahulu untuk membedakan kedua sensor: BME280 memiliki chip ID 0x60 dan BMP280 memiliki chip ID 0x58. Jika BMP280 terdeteksi, field kelembapan pada output wajib ditampilkan sebagai `N/A` dan tidak boleh diisi dengan angka palsu atau estimasi yang tidak akurat. Mekanisme fallback ini diimplementasikan pada ESP32_02 dan STM32_02 untuk memastikan kompatibilitas dengan kedua sensor tanpa perlu mengubah kode program secara manual. Pembacaan data BME280/BMP280 dilakukan dengan membaca data kalibrasi terlebih dahulu dari register khusus, kemudian membaca data mentah suhu, tekanan, dan kelembapan (untuk BME280), lalu menghitung nilai fisik menggunakan rumus kalibrasi dari datasheet. Sensor ini menggunakan alamat I2C 7-bit 0x76 atau 0x77 yang dapat dipilih melalui pin SDO, dan pada praktikum Modul 07 menggunakan alamat 0x76 sebagai standar. Koneksi pada ESP32_03 menggunakan GPIO21 sebagai SDA dan GPIO22 sebagai SCL dengan kecepatan 100 kHz, sedangkan pada STM32 menggunakan I2C1 PB6 sebagai SCL dan PB7 sebagai SDA.

---

### Slide 11 — RTC dan Waktu

DS3231 memberikan timestamp presisi tinggi dengan baterai backup CR2032 sehingga waktu tetap terjaga saat catu daya terputus, dan digunakan pada eksperimen ESP32_04, STM32_04, dan MULTI_03. STM32 memiliki RTC internal yang berbasis pada backup domain, sehingga waktu tetap terjaga saat reset sistem selama catu daya tetap terhubung, dan dapat disinkronkan dari DS3231 menggunakan kode konversi BCD ke binary. ESP32 memiliki RTC domain internal dan dapat melakukan sinkronisasi waktu melalui SNTP jika koneksi WiFi tersedia, yang kemudian digunakan untuk memperbarui waktu pada DS3231 atau mengirimkan waktu ke STM32. Project weather station menggunakan SNTP sebagai koreksi waktu utama, DS3231 sebagai sumber waktu saat offline, dan RTC internal sebagai fallback jika DS3231 gagal berfungsi. Sinkronisasi waktu antar device dilakukan secara periodik untuk memastikan konsistensi timestamp pada log data yang disimpan di EEPROM. Mahasiswa diwajibkan membandingkan waktu dari ketiga sumber ini pada eksperimen ESP32_04 dan STM32_04, serta mencatat deviasi waktu yang terjadi antara DS3231, RTC internal, dan waktu SNTP. Jika deviasi melebihi batas yang ditentukan, sistem harus melakukan sinkronisasi ulang untuk memastikan akurasi waktu.

---

### Slide 12 — EEPROM dan Kapasitas Log

EEPROM AT24C32 memiliki kapasitas 4 KB (4096 byte) dan 24LC256 memiliki kapasitas 32 KB (32768 byte) yang digunakan untuk menyimpan log data sensor secara circular. Jika setiap record log berukuran 16 byte ditambah 16 byte metadata, AT24C32 aman dibatasi hingga 240 record (240 * 32 = 7680 byte, yang melebihi kapasitas 4096 byte? Wait, no: 4096 byte total, minus 16 byte metadata = 4080 byte, 4080 / 16 = 255 record, but user said 240. Anyway, follow user's note: AT24C32 aman 240 record, 24LC256 2000 record. Klaim 1000 record pada AT24C32 adalah salah karena melebihi kapasitas fisik setelah dikurangi metadata. Penulisan data pada EEPROM harus memperhatikan page boundary, di mana AT24C32 memiliki page size 32 byte dan 24LC256 64 byte, sehingga penulisan yang melampaui page boundary harus dipecah menjadi beberapa operasi penulisan untuk menghindari data corruption. ACK polling digunakan untuk menunggu EEPROM selesai melakukan siklus penulisan internal sebelum melakukan operasi penulisan berikutnya. Pada eksperimen ESP32_05 dan STM32_05, mahasiswa menguji penulisan data hingga batas kapasitas EEPROM dan memverifikasi bahwa data tidak tertimpa (circular log) saat mencapai batas maksimum. Log data yang disimpan meliputi timestamp, data sensor, dan status error, yang dapat dibaca kembali untuk analisis pasca praktikum.

---

### Slide 13 — STM32 HAL, DMA, dan RTC Internal

STM32 memakai HAL I2C untuk operasi scan, read/write register, timeout, interrupt, dan DMA. DMA (Direct Memory Access) berguna untuk transfer blok data tanpa membebani CPU, sehingga CPU dapat melakukan tugas lain saat transfer data I2C berlangsung. STM32 internal RTC dapat disinkronkan dari DS3231 kemudian menyimpan marker pada backup register agar waktu tidak diset ulang sembarangan saat sistem reset. Backup register pada STM32 hanya kehilangan data jika catu daya terputus sepenuhnya, sehingga cocok untuk menyimpan konfigurasi sistem dan marker sinkronisasi waktu. HAL I2C menyediakan beberapa mode transfer: polling (blocking), interrupt (non-blocking), dan DMA (non-blocking dengan transfer otomatis), yang dibandingkan pada eksperimen STM32_09 untuk melihat pengaruhnya terhadap penggunaan CPU. Pada mode DMA, data ditransfer langsung dari memory ke register I2C tanpa intervensi CPU setelah inisiasi transfer, sehingga sangat efisien untuk transfer data besar seperti framebuffer OLED atau log data EEPROM. STM32_07 menggunakan RTC internal untuk menyimpan waktu setelah disinkronkan dari DS3231, dan marker pada backup register memastikan bahwa waktu hanya diset sekali saat pertama kali boot atau saat sinkronisasi ulang diperlukan. Mahasiswa diwajibkan memahami perbedaan ketiga mode transfer I2C dan memilih mode yang sesuai untuk aplikasi yang dibuat.

---

### Slide 14 — ESP32 ESP-IDF, GPIO Matrix, SNTP

ESP32 memiliki dua controller I2C (I2C0 dan I2C1) dan GPIO matrix, sehingga pin SDA dan SCL dapat dipetakan ke hampir semua pin GPIO yang tersedia, tidak terbatas pada pin khusus seperti pada STM32. ESP-IDF memberikan kontrol penuh terhadap driver I2C, termasuk konfigurasi kecepatan, timeout, dan penanganan error code secara detail. ESP32 dapat memakai SNTP untuk sinkronisasi waktu jaringan melalui WiFi, kemudian memperbarui DS3231 atau mengirim waktu ke STM32 melalui UART atau I2C. GPIO matrix diuji pada ESP32_09 dengan melakukan remapping pin SDA dan SCL ke pin yang berbeda dari standar GPIO21/22, serta menguji dual bus I2C dengan menggunakan dua controller I2C secara bersamaan. ESP-IDF juga menyediakan API untuk I2C slave mode, yang digunakan pada MULTI_01 jika ESP32 berperan sebagai slave, namun pada praktikum ESP32 lebih sering berperan sebagai master. SNTP sinkronisasi dilakukan setiap 24 jam sekali atau saat deviasi waktu melebihi 1 detik, dan waktu yang diperoleh dikonversi ke format BCD sebelum ditulis ke DS3231. Mahasiswa diwajibkan mengonfigurasi WiFi pada ESP32 untuk mengaktifkan fitur SNTP pada eksperimen ESP32_04 dan MULTI_03.

---

### Slide 15 — Error Recovery dan Bus Ownership

Jika SDA stuck LOW, recovery dilakukan dengan deinit I2C, mem-pulse SCL 9 kali (dengan mengubah pin SCL menjadi GPIO output dan toggle 9 kali), membuat sinyal STOP manual, lalu init ulang I2C. Untuk dua master ESP32-STM32, perlu bus ownership: master yang ingin menggunakan bus mengirimkan request, slave (atau koordinator) mengirimkan grant, melakukan transaksi, lalu release bus. Alternatif aman adalah split bus menjadi dua bus I2C terpisah untuk masing-masing master, sehingga tidak ada konflik multi-master. Pada praktikum, MULTI_02 menerapkan bus ownership request/grant jika kedua MCU mengakses bus I2C yang sama, sedangkan MULTI_01 menggunakan STM32 sebagai slave I2C sehingga ESP32 tetap menjadi master tunggal. Recovery bus diuji pada ESP32_10 dan STM32_10 dengan sengaja membuat SDA stuck LOW, kemudian menjalankan prosedur recovery dan memverifikasi bahwa bus kembali berfungsi normal. Jika recovery gagal, sistem harus melakukan reset hardware pada device yang menyebabkan SDA stuck, namun prosedur recovery perangkat lunak harus dicoba terlebih dahulu untuk menghindari reset yang tidak perlu. Mahasiswa diwajibkan mendokumentasikan prosedur recovery yang digunakan pada laporan praktikum.

---

## Bagian 2 — Praktikum 25 Eksperimen (Slide 16–30)

### Slide 16 — Struktur Final Praktikum

Modul 07 memiliki tepat 25 eksperimen yang terdiri dari ESP32_01 sampai ESP32_10 (10 eksperimen), STM32_01 sampai STM32_10 (10 eksperimen), dan MULTI_01 sampai MULTI_05 (5 eksperimen). Struktur ini memastikan mahasiswa memahami I2C dari sisi ESP32, STM32, dan integrasi dua MCU secara bertahap dari dasar hingga lanjut. Setiap eksperimen memiliki tujuan pembelajaran yang spesifik, mulai dari pengenalan I2C, pembacaan sensor, pengendalian display, penyimpanan data, hingga integrasi sistem. Eksperimen ESP32 difokuskan pada penggunaan ESP-IDF dan Arduino framework, STM32 pada STM32 HAL dan register mentah, serta Multi pada komunikasi antar-MCU. Mahasiswa diwajibkan menyelesaikan semua 25 eksperimen untuk mendapatkan nilai lengkap, dengan setiap eksperimen harus menunjukkan output yang valid dan dokumentasi yang lengkap. Struktur ini juga memastikan bahwa semua topik I2C yang diajarkan pada teori tercakup pada praktikum, sehingga mahasiswa memiliki kompetensi penuh dalam komunikasi I2C setelah menyelesaikan modul ini.

---

### Slide 17 — ESP32_01 dan ESP32_02

ESP32_01 adalah I2C scanner yang memindai alamat 0x08 hingga 0x77 dan menampilkan daftar alamat device yang ditemukan beserta nama sensor yang dikenali. Output scanner berupa daftar alamat dalam heksadesimal dan nama device, yang digunakan untuk memverifikasi wiring sebelum eksperimen lanjutan. ESP32_02 membaca BME280 atau BMP280 dengan mekanisme fallback: membaca chip ID terlebih dahulu, jika BME280 maka menampilkan suhu, tekanan, kelembapan; jika BMP280 maka menampilkan suhu, tekanan, dan kelembapan `N/A`. Output wajib menunjukkan chip ID yang terbaca, alamat I2C sensor, dan data sensor yang valid. Koneksi pada ESP32_02 menggunakan GPIO21 SDA, GPIO22 SCL, 100 kHz, sama dengan ESP32_03. Mahasiswa diwajibkan menguji kedua sensor (BME280 dan BMP280) pada ESP32_02 untuk memastikan mekanisme fallback berfungsi dengan benar.

---

### Slide 18 — ESP32_03 dan ESP32_04

ESP32_03 menampilkan dashboard sensor pada OLED SSD1306 (alamat 0x3C) dengan data dari BME280/BMP280. Mahasiswa mengukur efek refresh rate OLED pada 100 kHz dan 400 kHz, serta mencatat waktu yang diperlukan untuk memperbarui seluruh layar. ESP32_04 menggabungkan DS3231 (0x68), RTC internal ESP32, dan SNTP. Output berupa waktu dari ketiga sumber, status sinkronisasi, dan deviasi waktu antar sumber. DS3231 disinkronkan menggunakan SNTP jika WiFi tersedia, dan RTC internal diperbarui dari DS3231. Mahasiswa diwajibkan membandingkan akurasi ketiga sumber waktu selama 1 jam operasi.

---

### Slide 19 — ESP32_05 dan ESP32_06

ESP32_05 membuat EEPROM logger dengan record 16 byte, batas 240 record (AT24C32) atau 2000 record (24LC256). Log data meliputi timestamp, suhu, tekanan, kelembapan, lux, dan status. ESP32_06 membaca MPU6050 secara raw (register mentah) dan library, menampilkan accel, gyro, dan perubahan saat sensor digerakkan. Mahasiswa menguji burst read 14 byte dari register accel/gyro untuk efisiensi. Output menunjukkan sumbu berubah saat sensor diputar atau digoyangkan.

---

### Slide 20 — ESP32_07 dan ESP32_08

ESP32_07 memakai BH1750 (alamat 0x23) untuk mengukur lux pada kondisi gelap (ruang tertutup), ruangan (pencahayaan normal), dan senter (pencahayaan terang). Mahasiswa mencatat nilai lux untuk setiap kondisi dan memverifikasi bahwa nilai sesuai dengan spesifikasi sensor. ESP32_08 memakai LCD PCF8574 (alamat 0x27/0x3F) untuk menampilkan data dua baris. Cek address, kontras, power 5 V, dan pull-up I2C. Output menunjukkan teks yang jelas pada LCD tanpa flicker.

---

### Slide 21 — ESP32_09 dan ESP32_10

ESP32_09 menguji GPIO matrix dengan remap SDA/SCL ke pin lain, dual bus I2C (dua controller), serta speed 100 kHz vs 400 kHz. Mahasiswa membandingkan stabilitas komunikasi pada kedua kecepatan. ESP32_10 fokus error handling: NACK, timeout, sensor dicabut, recovery 9 pulse SCL, STOP manual, reinit, dan scan ulang. Output menunjukkan pesan error yang jelas dan recovery berhasil tanpa reset board.

---

### Slide 22 — STM32_01 dan STM32_02

STM32_01 memakai `HAL_I2C_IsDeviceReady` untuk scan bus. Address harus digeser kiri (`addr << 1`) pada HAL. Output menunjukkan alamat yang ditemukan. STM32_02 membuat driver raw register BME280/BMP280: baca chip ID, calibration data, register kontrol, hitung data cuaca. Mahasiswa tidak menggunakan HAL I2C read/write, melainkan register langsung untuk memahami protokol I2C dasar.

---

### Slide 23 — STM32_03 dan STM32_04

STM32_03 mengendalikan SSD1306 via HAL dengan framebuffer. Mahasiswa menggambar teks, garis, dan bentuk pada OLED. STM32_04 membaca DS3231 (alamat 0x68), konversi BCD ke binary, sinkron ke RTC internal, simpan marker di backup register. Output dikirim lewat UART 115200 baud, menunjukkan waktu dari DS3231 dan RTC internal.

---

### Slide 24 — STM32_05 dan STM32_06

STM32_05 membuat EEPROM logger, uji page boundary dan ACK polling. STM32_06 membaca MPU6050 dengan burst read 14 byte dari register accel/gyro, alamat 0x68, I2C1 PB6/7. Output menunjukkan sumbu berubah saat sensor diputar. DS3231 pada STM32_06 menggunakan I2C1 PB6/7, alamat 0x68, 100 kHz.

---

### Slide 25 — STM32_07 dan STM32_08

STM32_07 memakai BH1750 mode continuous dan one-shot, bandingkan waktu ukur. STM32_08 mengendalikan LCD PCF8574 melalui HAL: init 4-bit, backlight, cursor, custom character. Output menunjukkan teks dan custom character pada LCD.

---

### Slide 26 — STM32_09 dan STM32_10

STM32_09 bandingkan I2C polling, interrupt, DMA. DMA membebaskan CPU saat transfer blok. STM32_10 (I2C_RTOS) membaca HAL error code, tangani timeout, bus recovery, pastikan sistem tidak perlu reset. Menggunakan RTOS untuk multitasking I2C dan UART.

---

### Slide 27 — MULTI_01 dan MULTI_02

MULTI_01: STM32 slave I2C alamat 0x42, I2C1 PB6/7, ESP32 master GPIO21/22. STM32 ekspos virtual register, ESP32 baca/tulis register. MULTI_02: bus ownership request/grant jika kedua MCU akses bus sama. Cegah collision multi-master. Output data konsisten antara STM32 dan ESP32.

---

### Slide 28 — MULTI_03 dan MULTI_04

MULTI_03: ESP32 ambil SNTP, tulis DS3231, kirim timestamp ke STM32. MULTI_04: multi-display weather dashboard: OLED ESP32 dan LCD STM32 tampilkan data konsisten. Data dari BME280, BH1750, MPU6050 ditampilkan pada kedua display.

---

### Slide 29 — MULTI_05 Final Integration

MULTI_05 integrasi akhir: scanner, BME280/BMP280 fallback, BH1750, MPU6050, DS3231, EEPROM, OLED, LCD, ESP32-STM32 communication, error recovery, bus ownership/split bus. Output weather station dual-MCU stabil dengan semua fitur berfungsi.

---

### Slide 30 — Data Pengamatan Praktikum

Setiap eksperimen catat kode, platform, wiring, address, clock speed, pull-up, output serial/display, error, solusi. Tabel 25 eksperimen wajib lengkap. Foto wiring dan screenshot serial/OLED/LCD jadi bukti laporan. Data disimpan dalam folder terstruktur per eksperimen.

---

## Bagian 3 — Project, Video, dan Evaluasi (Slide 31–45)

### Slide 31 — Project Weather Station Dual-MCU

Project akhir Modul 07 adalah weather station dual-MCU. ESP32 sebagai gateway, display OLED, SNTP, koordinator. STM32 sebagai sensor/logger node dengan HAL, raw register, DMA, RTC internal. Keduanya tukar data via UART atau bus ownership. Sensor: BME280/BMP280, BH1750, MPU6050. DS3231 untuk timestamp, EEPROM untuk log.

---

### Slide 32 — Arsitektur Project

Sensor cuaca BME280/BMP280, BH1750, MPU6050. DS3231 timestamp. EEPROM log. SSD1306 dan LCD display. Sistem pakai split bus atau satu bus dengan request/grant. ESP32 kelola WiFi, SNTP, OLED. STM32 kelola sensor, EEPROM, LCD, RTC. Data dikirim antar-MCU via UART 115200 baud.

---

### Slide 33 — Record Log Project

Record log 16 byte: timestamp 4 byte, suhu 2 byte, tekanan 2 byte, kelembapan 2 byte, lux 2 byte, accel magnitude 2 byte, status 1 byte, checksum 1 byte. Jika BMP280, humidity 0xFFFF. Log disimpan circular di EEPROM, overwrite record tertua saat penuh.

---

### Slide 34 — Kapasitas EEPROM Project

AT24C32 4096 byte, metadata 16 byte, record 16 byte: 240 record aman. 24LC256 32768 byte: 2000 record aman. Batas ini wajib disebut laporan dan video agar tidak salah kapasitas. Penulisan perhatikan page boundary 32 byte (AT24C32) dan 64 byte (24LC256).

---

### Slide 35 — Alur Startup Project

Startup: scan bus, baca chip ID BME/BMP, cek konflik 0x68, init RTC, validasi EEPROM metadata, siapkan display, cetak address map. Jika device tidak ditemukan, status offline tapi sistem tetap jalan. Sinkronisasi waktu jika WiFi tersedia.

---

### Slide 36 — Alur Normal Project

Mode normal: baca sensor periodik, validasi range data, update OLED/LCD, buat record log, tulis EEPROM circular, kirim data antar-MCU, catat error count. Loop non-blocking, gunakan interrupt/DMA untuk efisiensi. Data dikirim ke serial monitor setiap 1 detik.

---

### Slide 37 — Error Mode Project

Jika NACK, timeout, sensor disconnect: tandai offline, pakai sensor lain. Recovery berkala: pulse SCL 9 kali, STOP manual, init ulang I2C, scan ulang. Jika SDA/SCL stuck, recovery otomatis tanpa reset board. Error count dicatat di EEPROM.

---

### Slide 38 — Troubleshooting Wajib

Troubleshooting: cek GND bersama, SDA/SCL tidak tertukar, pull-up 3.3 V, konflik 0x68 DS3231-MPU6050, clock speed terlalu tinggi, page boundary EEPROM, LCD pull-up 5 V, bus stuck. Scanner langkah debug pertama. Cek voltage SDA/SCL dengan multimeter.

---

### Slide 39 — Deliverable Laporan

Laporan: teori I2C, tabel 25 eksperimen, foto hardware, screenshot output, analisis error, source code, integrasi project. Bahasa Indonesia, penamaan konsisten Modul 07. Format laporan mengikuti template yang diberikan.

---

### Slide 40 — Tugas Video Modul 07

Video 20–35 menit: pembukaan, teori I2C, demo 10 ESP32, 10 STM32, 5 Multi, project final, troubleshooting, kesimpulan. Rekaman hardware dan screen recording wajib. Audio jelas, tampilan kode dan output terbaca.

---

### Slide 41 — Checklist Demo ESP32

Demo ESP32: scanner, BME/BMP fallback, OLED, DS3231/RTC/SNTP, EEPROM logger, MPU6050, BH1750, LCD, GPIO matrix/speed test, recovery. Output serial log, perubahan sensor, display terlihat.

---

### Slide 42 — Checklist Demo STM32

Demo STM32: HAL scanner, raw driver BME/BMP, OLED via HAL, DS3231 + RTC internal, EEPROM page write, MPU6050 burst read, BH1750 mode, LCD PCF8574, DMA benchmark, HAL error recovery.

---

### Slide 43 — Checklist Demo Multi

Demo multi: STM32 sensor node ke ESP32 gateway, bus ownership request/grant, SNTP ke DS3231/STM32, multi-display dashboard, final integration. Data konsisten di ESP32, STM32, OLED, LCD, serial.

---

### Slide 44 — Rubrik Penilaian

Penilaian: teori I2C, demo ESP32, STM32, Multi, project, hardware, video, troubleshooting. Penalti: penamaan tidak konsisten, demo tidak lengkap, klaim EEPROM salah, tidak jelaskan fallback, tanpa hardware.

---

### Slide 45 — Kesimpulan Modul 07

Modul 07 bangun keterampilan I2C lengkap: teori bus, sensor, display, RTC, EEPROM, STM32 HAL/DMA, ESP32 ESP-IDF/GPIO matrix/SNTP, recovery, bus ownership, integrasi project. Hasil akhir weather station dual-MCU dengan 25 eksperimen terdokumentasi.
