# Modul 09: FreeRTOS Middle - GPIO, Interrupt, Encoder, Serial, DAC, ADC, I2C, SPI — NotebookLM 45 Slide

## Bagian 1 — Teori FreeRTOS (Slide 1–15)

### Slide 1 — Modul 09 dan Target Akhir

Modul 09 membahas FreeRTOS Middle untuk pengembangan sistem embedded dengan RTOS. Target akhir pembelajaran modul ini adalah penyelesaian 25 eksperimen final yang terdiri dari 10 eksperimen STM32, 10 eksperimen ESP32, dan 5 eksperimen Multi STM32-ESP32 yang dirancang untuk membangun kompetensi praktis mahasiswa dalam pengembangan sistem real-time. Semua percobaan di laboratorium diarahkan untuk mendukung pengembangan project akhir berupa RTOS Sensor Hub System yang mengintegrasikan GPIO tasks, external interrupt, encoder 2-pin, serial communication, DAC, ADC, I2C, dan SPI dalam lingkungan FreeRTOS. Eksperimen STM32 menggunakan FreeRTOS via CMSIS-RTOS atau STM32Cube dengan implementasi task, semaphore, mutex, queue, dan event group. Eksperimen ESP32 menggunakan FreeRTOS native ESP32 dengan pengelolaan multitasking yang efisien. Eksperimen Multi mengimplementasikan komunikasi antar-MCU dengan RTOS primitives untuk pembagian data dan sinkronisasi task. Setiap eksperimen dirancang untuk membangun pemahaman bertahap mulai dari teori dasar RTOS hingga integrasi sistem kompleks dengan multiple task dan interrupt handling.

---

### Slide 2 — Dasar FreeRTOS

FreeRTOS adalah real-time operating system kernel untuk mikrokontroler yang menyediakan multitasking preemptive. Konsep utama FreeRTOS meliputi task sebagai unit eksekusi, scheduler yang mengatur eksekusi berdasarkan priority, dan context switch untuk menyimpan dan memulihkan state task. Task didefinisikan sebagai fungsi yang menerima parameter void pointer dan berisi loop tak terbatas dengan delay non-blocking. Scheduler FreeRTOS menggunakan prioritas untuk menentukan task mana yang akan dieksekusi: semakin tinggi angka prioritas, semakin tinggi prioritas task tersebut. Context switch terjadi saat scheduler berpindah dari satu task ke task lainnya, dengan menyimpan register dan stack task yang sedang berjalan dan memulihkan state task yang akan dieksekusi. FreeRTOS mendukung berbagai RTOS primitives: semaphore (binary, counting), mutex, queue, event group, dan software timer. Semua primitives ini memungkinkan komunikasi antar task, sinkronisasi, dan proteksi shared resource tanpa blocking yang berlebihan. Pada praktikum Modul 09, mahasiswa diwajibkan membuat minimal 3 task dan menggunakan minimal 2 RTOS primitives untuk setiap eksperimen.

---

### Slide 3 — Task Creation dan Scheduler

Task dibuat dengan `xTaskCreate()` pada ESP32 atau `osThreadNew()` pada STM32 CMSIS-RTOS. Parameter task creation meliputi task function, task name, stack depth (dalam words), parameter, priority, dan task handle. Stack depth harus cukup untuk menampung variabel lokal, function call, dan context save; jika kurang akan menyebabkan stack overflow. Priority task berkisar dari 0 (idle task) hingga configMAX_PRIORITIES-1, dengan semakin tinggi angka semakin tinggi prioritasnya. Scheduler pada ESP32 berjalan otomatis setelah `app_main()` selesai, sedangkan pada STM32Cube harus memanggil `osKernelStart()` atau `vTaskStartScheduler()`. Task delay menggunakan `vTaskDelay()` (tick-based, waktu sejak delay dipanggil) atau `vTaskDelayUntil()` (fixed frequency, waktu sejak task terakhir bangun). Penggunaan `vTaskDelay()` atau `osDelay()` penting untuk melepas CPU agar task lain dapat berjalan, berbeda dengan `HAL_Delay()` yang bersifat blocking dan harus dihindari dalam RTOS.

---

### Slide 4 — ISR dan RTOS

Interrupt Service Routine (ISR) pada RTOS harus sangat cepat dan non-blocking karena dijalankan di context interrupt dengan stack terpisah. FreeRTOS menyediakan ISR-safe API yang berakhiran `FromISR()` untuk digunakan di dalam ISR: `xSemaphoreGiveFromISR()`, `xQueueSendFromISR()`, `xEventGroupSetBitsFromISR()`. Aturan penting dalam ISR: jangan gunakan `vTaskDelay()` karena akan menyebabkan error, jangan gunakan `xQueueSend()` (tanpa FromISR) karena bukan ISR-safe, dan gunakan `portYIELD_FROM_ISR()` atau `xHigherPriorityTaskWoken` untuk meminta context switch setelah ISR selesai jika ada task dengan prioritas lebih tinggi yang menjadi unblocked. Contoh: EXTI interrupt untuk button, ISR memberikan semaphore dengan `xSemaphoreGiveFromISR()`, kemudian task yang menunggu semaphore akan bangun dan menangani event button tersebut. Pendekatan ini memastikan ISR tetap cepat dan pemrosesan logika dilakukan di task level, bukan di ISR.

---

### Slide 5 — Semaphore dan Mutex

Semaphore dan mutex adalah RTOS primitives untuk synchronization dan mutual exclusion. Binary semaphore digunakan untuk signaling satu-ke-satu, misalnya dari ISR ke task. Counting semaphore digunakan untuk resource pool dengan jumlah terbatas, misalnya pool buffer. Mutex digunakan untuk proteksi shared resource (mutual exclusion) dan memiliki fitur priority inheritance untuk mencegah priority inversion. Perbedaan utama: semaphore tidak dimiliki oleh task tertentu, sedangkan mutex dimiliki oleh task yang mengambilnya dan harus dikembalikan oleh task yang sama. Contoh penggunaan semaphore: ISR button memberikan semaphore, task menunggu semaphore dengan `xSemaphoreTake()`. Contoh penggunaan mutex: task A dan task B mengakses UART shared, keduanya harus mengambil mutex sebelum menulis ke UART dan melepasnya setelah selesai. Priority inheritance pada mutex: jika task prioritas rendah memegang mutex dan task prioritas tinggi menunggu mutex, prioritas task rendah akan dinaikan sementara ke prioritas tinggi agar segera melepas mutex, mencegah priority inversion.

---

### Slide 6 — Queue

Queue menyediakan mekanisme inter-task communication atau ISR-to-task data transfer. Queue dapat berisi data dengan tipe bebas (struct, int, char, dll) dengan ukuran dan jumlah elemen yang ditentukan saat creation. Queue bersifat FIFO (First-In-First-Out) secara default, tetapi dapat digunakan sebagai binary semaphore alternatif dengan ukuran 1. Pengiriman data ke queue: `xQueueSend()` (task) atau `xQueueSendFromISR()` (ISR). Penerimaan data dari queue: `xQueueReceive()` (task). Queue dapat dicek apakah penuh atau kosong, dan dapat digunakan dengan timeout (tick-based) untuk menunggu data tersedia. Contoh: ADC task membaca ADC dan mengirim hasil ke queue, display task menerima data dari queue dan menampilkannya. ISR UART RX menerima karakter dan memasukkannya ke queue, task UART processor membaca queue dan memproses command. Queue ini thread-safe dan dapat diakses dari multiple task dan ISR tanpa race condition.

---

### Slide 7 — Event Group

Event group memungkinkan sinkronisasi task berdasarkan bitmask dengan multiple events. Event group terdiri dari 8 atau 24 bit (tergantung konfigurasi) yang dapat diset atau diclear secara individual. Task dapat menunggu satu atau beberapa bit menjadi set (any bit atau all bits) dengan `xEventGroupWaitBits()`. ISR dapat mengset bit dengan `xEventGroupSetBitsFromISR()`. Event group sangat berguna untuk sinkronisasi multiple task yang menunggu multiple events, misalnya: task display menunggu event data ADC ready ATAU data sensor I2C ready ATAU command UART diterima. Dengan event group, task dapat menunggu satu dari beberapa event dan mengetahui event mana yang terjadi. Event group juga dapat digunakan untuk sinkronisasi start: beberapa task menunggu bit START diset oleh task inisiasi, setelah semua task siap, task inisiasi set bit START dan semua task mulai berjalan secara bersamaan.

---

### Slide 8 — GPIO Tasks

GPIO dengan RTOS menggunakan task yang mengendalikan pin secara periodik dengan delay non-blocking. Pada ESP32, GPIO dikonfigurasi dengan `gpio_pad_select_gpio()`, `gpio_set_direction()`, dan `gpio_set_level()`. Task blink LED menggunakan `vTaskDelay(pdMS_TO_TICKS(500))` untuk delay 500 ms. Pada STM32, GPIO dikonfigurasi dengan HAL `HAL_GPIO_Init()`, `HAL_GPIO_TogglePin()`, dan task menggunakan `osDelay(500)`. Penting: jangan gunakan `HAL_Delay()` di dalam task RTOS karena akan memblokir scheduler dan mencegah task lain berjalan. Gunakan `vTaskDelay()` atau `osDelay()` yang akan menyarankan scheduler untuk beralih ke task lain selama delay. Beberapa task GPIO dapat berjalan bersamaan, masing-masing dengan priority dan delay sendiri-sendiri, semua dikelola oleh scheduler RTOS secara preemptive.

---

### Slide 9 — External Interrupt dengan RTOS

External interrupt pada RTOS ditangani dengan semaphore dari ISR ke task. Pada STM32, EXTI dikonfigurasi melalui HAL `HAL_GPIO_EXTI_IRQHandler()` dan menulis handler `EXTIx_IRQHandler()` yang memanggil `xSemaphoreGiveFromISR()` untuk memberikan semaphore. Pada ESP32, interrupt ditambahkan dengan `gpio_install_isr_service()`, `gpio_isr_handler_add()`, dan ISR handler memanggil `xSemaphoreGiveFromISR()`. Task yang menangani button menunggu semaphore dengan `xSemaphoreTake()` dan melakukan aksi (toggle LED, increment counter, dll). Debounce tidak boleh dilakukan di ISR karena akan memblokir; debounce harus dilakukan di task level dengan delay atau filter software. Penggunaan semaphore memastikan ISR tetap cepat (hanya memberikan semaphore) dan pemrosesan logika (termasuk debounce) dilakukan di task dengan delay yang diperbolehkan.

---

### Slide 10 — Encoder 2-Pin Interrupt RTOS

Rotary encoder memiliki 2 pin output (A dan B) dengan phase 90° (quadrature). Pembacaan dengan interrupt pada kedua pin: interrupt pada rising/falling pin A, kemudian baca pin B untuk menentukan arah rotasi. Jika B = HIGH saat A falling → ClockWise (CW); jika B = LOW → Counter ClockWise (CCW). Event rotasi dikirim ke queue atau semaphore dari ISR. Queue lebih disarankan karena dapat mengirim data arah (1 untuk CW, -1 untuk CCW) atau struct dengan informasi lebih lengkap. Task encoder memproses queue, menghitung counter, dan mengupdate display. Debounce dilakukan di task level dengan ignore event dalam jendela waktu singkat (misal 5 ms) atau dengan filter state machine. Encoder 2-pin interrupt ini merupakan aplikasi nyata dari kombinasi ISR, queue, dan task RTOS untuk membaca input mekanis dengan akurat tanpa membebani CPU dengan polling.

---

### Slide 11 — Serial Communication RTOS

UART dengan RTOS menggunakan queue untuk RX dan TX data. Pada ESP32, UART driver diinstall dengan `uart_driver_install()` dengan RX buffer, TX buffer, dan queue event. ISR UART RX memasukkan karakter ke queue atau memicu event di queue event UART. Task RX menerima event dari queue event, membaca data dengan `uart_read_bytes()`, dan memproses command. Task TX mengirim data dengan `uart_write_bytes()`. Pada STM32, UART RX interrupt diaktifkan dengan `HAL_UART_Receive_IT()`, dan callback `HAL_UART_RxCpltCallback()` memasukkan karakter ke queue dengan `xQueueSendFromISR()`. Task UART processor membaca queue dan memproses command. Protokol komunikasi sederhana dapat diimplementasikan: command `LED ON`, `LED OFF`, `ADC START`, `ADC STOP`, `GET STATUS`, dengan respon dari task lain. Queue memastikan data UART tidak hilang meskipun task processor sedang sibuk.

---

### Slide 12 — DAC dengan RTOS

DAC menghasilkan sinyal analog dari nilai digital. Task RTOS menulis nilai DAC secara periodik dengan delay untuk mengontrol frekuensi output. Pada ESP32, DAC dienable dengan `dac_output_enable()` dan ditulis dengan `dac_output_voltage()` (8-bit, 0-255 untuk 0-3.3V). Lookup table digunakan untuk generate bentuk gelombang: sin, triangle, sawtooth. Delay task mengontrol frekuensi: `vTaskDelay(pdMS_TO_TICKS(1))` untuk 1 ms delay → 1 kHz jika lookup table 100 sampel per cycle. Pada STM32, DAC diinit dengan HAL `HAL_DAC_Init()`, ditulis dengan `HAL_DAC_SetValue()` dengan alignment 8-bit atau 12-bit. Task DAC dapat menggenerate sin wave dengan menghitung nilai sin secara real-time atau menggunakan lookup table. Osiloskop digunakan untuk memverifikasi bentuk gelombang dan frekuensi output. DAC RTOS task ini menunjukkan kemampuan RTOS untuk menggenerate sinyal analog secara deterministik tanpa blocking task lain.

---

### Slide 13 — ADC dengan RTOS

ADC membaca tegangan analog dan task RTOS membaca secara periodik, mengirim hasil ke queue untuk diproses. Pada ESP32, ADC1 dikonfigurasi dengan `adc1_config_width()` (12-bit) dan `adc1_config_channel_atten()` (attenuasi 0-11dB untuk range 0-3.3V). Task ADC memanggil `adc1_get_raw()` dan mengirim nilai ke queue. Konversi ke tegangan: `voltage = adc_val * 3.3 / 4095`. Pada STM32, ADC diinit dengan HAL `HAL_ADC_Init()`, dijalankan dengan `HAL_ADC_Start()`, dan dibaca dengan `HAL_ADC_GetValue()`. Task ADC dapat menggunakan polling, interrupt, atau DMA untuk pembacaan. Queue digunakan untuk mengirim hasil ADC ke display task atau UART task. Pembacaan periodik dengan `vTaskDelay(pdMS_TO_TICKS(100))` akan membaca ADC setiap 100 ms. Filter sederhana (moving average) dapat diimplementasikan di task untuk menghaluskan fluktuasi ADC.

---

### Slide 14 — I2C dengan RTOS

I2C bus shared harus dilindungi dengan mutex agar tidak ada race condition saat multiple task mengakses bus yang sama. Pada ESP32, I2C diinit dengan `i2c_param_config()` dan `i2c_driver_install()`. Mutex dibuat dengan `xSemaphoreCreateMutex()`. Task yang mengakses I2C memanggil `xSemaphoreTake()` sebelum akses, melakukan transfer dengan `i2c_master_write_read_device()`, dan memanggil `xSemaphoreGive()` setelah selesai. Pada STM32, I2C diinit dengan HAL `HAL_I2C_Init()`. Task mengambil mutex dengan `xSemaphoreTake()`, melakukan transfer dengan `HAL_I2C_Mem_Read()` atau `HAL_I2C_Mem_Write()`, dan melepas mutex. Mutex memastikan hanya satu task yang mengakses bus I2C pada satu waktu, mencegah konflik data saat multiple task (sensor read, display update, logger) mengakses sensor I2C yang sama.

---

### Slide 15 — SPI dengan RTOS

SPI bus juga harus dilindungi dengan mutex jika diakses oleh multiple task. Pada ESP32, SPI bus diinit dengan `spi_bus_initialize()`, device ditambahkan dengan `spi_bus_add_device()`. Transfer dilakukan dengan `spi_device_transmit()` dengan struct `spi_transaction_t`. Mutex digunakan untuk proteksi akses SPI. Pada STM32, SPI diinit dengan HAL `HAL_SPI_Init()`. Transfer dengan `HAL_SPI_TransmitReceive()` atau DMA. Task SPI mengambil mutex sebelum transfer, melakukan komunikasi, dan melepas mutex setelah selesai. SPI dapat digunakan untuk komunikasi dengan SD card (penyimpanan log), sensor (ADXL345 accelerometer), display, atau komunikasi antar-MCU. RTOS memungkinkan SPI transfer dilakukan secara non-blocking background, sementara task lain tetap berjalan, dan menggunakan callback atau semaphore untuk notifikasi transfer selesai jika menggunakan interrupt/DMA.

---

## Bagian 2 — Praktikum 25 Eksperimen (Slide 16–30)

### Slide 16 — Struktur Final Praktikum

Modul 09 memiliki tepat 25 eksperimen yang terdiri dari STM32_01 sampai STM32_10 (10 eksperimen), ESP32_01 sampai ESP32_10 (10 eksperimen), dan MULTI_01 sampai MULTI_05 (5 eksperimen). Struktur ini memastikan mahasiswa memahami FreeRTOS dari sisi STM32, ESP32, dan integrasi dua MCU secara bertahap dari dasar hingga lanjut. Setiap eksperimen memiliki tujuan pembelajaran yang spesifik, mulai dari pembuatan task, penanganan interrupt, penggunaan RTOS primitives, hingga komunikasi antar-MCU. Eksperimen STM32 difokuskan pada FreeRTOS via CMSIS-RTOS/STM32Cube, ESP32 pada FreeRTOS native dan ESP-IDF, serta Multi pada komunikasi antar-MCU dengan RTOS. Mahasiswa diwajibkan menyelesaikan semua 25 eksperimen untuk mendapatkan nilai lengkap, dengan setiap eksperimen harus menunjukkan output yang valid dan dokumentasi yang lengkap. Struktur ini juga memastikan bahwa semua topik FreeRTOS yang diajarkan pada teori tercakup pada praktikum.

---

### Slide 17 — STM32_01 dan STM32_02

STM32_01 adalah GPIO Task Blink RTOS yang membuat task `vLEDTask` dengan delay `osDelay(500)` untuk blink LED 1 Hz. Output berupa blink LED dan status task di serial monitor. STM32_02 adalah External Interrupt RTOS dengan EXTI0 pada PA0. ISR memberikan binary semaphore `xSemaphoreGiveFromISR()`, task menunggu semaphore dan toggle LED saat tombol ditekan. Output berupa LED toggle setiap tombol ditekan tanpa debounce blocking di ISR. Kedua eksperimen ini mengenalkan konsep dasar task creation dan interrupt handling dengan RTOS. Mahasiswa diwajibkan memastikan penggunaan `osDelay()` bukan `HAL_Delay()` di dalam task, dan penggunaan `xSemaphoreGiveFromISR()` di dalam ISR bukan `xSemaphoreGive()`.

---

### Slide 18 — STM32_03 dan STM32_04

STM32_03 adalah Encoder 2-Pin Interrupt RTOS dengan rotary encoder di PB0 (A) dan PB1 (B). EXTI interrupt pada kedua pin mengirim event ke queue, task memproses arah CW/CCW dan mengupdate counter. Output berupa arah dan nilai counter di serial monitor. STM32_04 adalah Serial Communication RTOS dengan UART RX interrupt memasukkan karakter ke queue, task receiver memproses command, task transmitter mengirim respon. Output berupa echo karakter dan respon command dari STM32. Kedua eksperimen ini mengajarkan penggunaan queue untuk data transfer dari ISR ke task dan antar task. Mahasiswa diwajibkan mengimplementasikan debounce di task level untuk encoder, bukan di ISR.

---

### Slide 19 — STM32_05 dan STM32_06

STM32_05 adalah DAC dengan RTOS yang menghasilkan sinyal analog menggunakan lookup table untuk sin wave atau triangle wave. Task DAC menulis nilai DAC secara periodik dengan delay task untuk mengontrol frekuensi. Output diamati dengan osiloskop berupa bentuk gelombang periodik. STM32_06 adalah ADC dengan RTOS yang membaca potensiometer di PA0. Task ADC membaca nilai tiap 100 ms, mengirim ke queue, task display menerima dan menampilkan tegangan. Output berupa nilai ADC dan tegangan real-time di serial. Kedua eksperimen ini menunjukkan penggunaan RTOS untuk generate dan read sinyal analog secara deterministik tanpa memblokir task lain.

---

### Slide 20 — STM32_07 dan STM32_08

STM32_07 adalah I2C dengan RTOS yang membaca sensor I2C (BME280) dengan proteksi mutex. Task sensor mengambil mutex, membaca data I2C, melepas mutex, dan task display menampilkan data. Output berupa data sensor I2C terbaca dan ditampilkan. STM32_08 adalah SPI dengan RTOS yang berkomunikasi dengan modul SPI (SD Card/ADXL345). Task SPI mengambil mutex sebelum transfer, melakukan komunikasi, dan melepas mutex setelah selesai. Output berupa data SPI berhasil dibaca/ditulis. Kedua eksperimen ini mengajarkan penggunaan mutex untuk proteksi shared bus I2C dan SPI agar tidak ada race condition saat multiple task mengakses bus yang sama.

---

### Slide 21 — STM32_09 dan STM32_10

STM32_09 adalah Multi Task GPIO-ADC-UART yang mengintegrasikan multiple task RTOS: GPIO blink, ADC reader dengan queue ke UART task, dan UART command parser. Event group digunakan untuk sinkronisasi task. Output berupa sistem multi-task berjalan stabil dengan command UART untuk kontrol LED dan ADC. STM32_10 adalah FreeRTOS Semaphore Mutex yang mendemonstrasikan penggunaan binary semaphore untuk task synchronization dan mutex untuk shared resource (UART). Counting semaphore untuk buffer pool juga dapat diimplementasikan. Output berupa akses resource teratur tanpa race condition. Kedua eksperimen ini menyatukan berbagai konsep RTOS untuk membangun sistem multi-task yang kompleks namun stabil.

---

### Slide 22 — ESP32_01 dan ESP32_02

ESP32_01 adalah GPIO Task Blink RTOS dengan task `vLEDTask` menggunakan `vTaskDelay(pdMS_TO_TICKS(500))`. Output berupa LED blink 1 Hz dan status task di serial. ESP32_02 adalah External Interrupt RTOS dengan GPIO interrupt pada GPIO0. ISR memberikan binary semaphore, task menunggu semaphore dan toggle LED. Output berupa LED toggle saat tombol ditekan. Kedua eksperimen ini mengenalkan FreeRTOS native ESP32 dengan task creation dan interrupt handling. Mahasiswa diwajibkan memahami perbedaan FreeRTOS native ESP32 dengan CMSIS-RTOS pada STM32, terutama dalam pembuatan task dan penggunaan delay.

---

### Slide 23 — ESP32_03 dan ESP32_04

ESP32_03 adalah Encoder 2-Pin Interrupt RTOS dengan encoder di GPIO4 (A) dan GPIO5 (B). GPIO interrupt mengirim event ke queue, task menghitung arah CW/CCW. Output berupa arah dan counter encoder di serial. ESP32_04 adalah Serial Communication RTOS dengan UART driver `uart_driver_install()`. Queue event UART digunakan, task RX memproses data, task TX mengirim respon. Output berupa echo dan respon command. Kedua eksperimen ini mengajarkan queue untuk data transfer dan UART driver ESP-IDF dengan RTOS. Mahasiswa diwajibkan mengimplementasikan command parser sederhana untuk mengontrol LED atau membaca status sistem.

---

### Slide 24 — ESP32_05 dan ESP32_06

ESP32_05 adalah DAC dengan RTOS dengan DAC channel 1 (GPIO25). Task generate sin wave dengan lookup table dan delay task. Output diamati dengan osiloskop. ESP32_06 adalah ADC dengan RTOS dengan ADC1 channel 6 (GPIO34). Task ADC baca tiap 100 ms, kirim ke queue, task display tampilkan tegangan. Output berupa nilai ADC real-time di serial. Kedua eksperimen ini menunjukkan penggunaan DAC dan ADC ESP32 dalam lingkungan RTOS. Mahasiswa diwajibkan mengkalibrasi ADC jika tersedia dan menampilkan tegangan yang akurat sesuai potensiometer.

---

### Slide 25 — ESP32_07 dan ESP32_08

ESP32_07 adalah I2C dengan RTOS dengan I2C master init `i2c_param_config()`. Mutex digunakan untuk proteksi bus I2C saat akses sensor. Output berupa data sensor I2C tampil di serial. ESP32_08 adalah SPI dengan RTOS dengan SPI bus init `spi_bus_initialize()`. Mutex untuk proteksi SPI bus. Output berupa data SPI transfer berhasil. Kedua eksperimen ini mengajarkan penggunaan mutex pada ESP32 untuk I2C dan SPI shared bus. Mahasiswa diwajibkan membaca register device atau menulis data ke SPI device dan memverifikasi data dengan pembacaan ulang.

---

### Slide 26 — ESP32_09 dan ESP32_10

ESP32_09 adalah Multi Task GPIO-ADC-UART yang mengintegrasikan multiple task: LED blink, ADC reader, UART command parser dengan event group sinkronisasi. Output berupa sistem multi-task stabil. ESP32_10 adalah FreeRTOS Semaphore Mutex dengan demonstrasi binary semaphore, mutex untuk UART shared, dan counting semaphore. Output berupa akses resource tanpa race condition. Kedua eksperimen ini menyatukan konsep RTOS ESP32 untuk sistem kompleks. Mahasiswa diwajibkan mengimplementasikan kontrol LED dan ADC melalui command UART yang diproses oleh multiple task yang disinkronisasi dengan event group.

---

### Slide 27 — MULTI_01 dan MULTI_02

MULTI_01 adalah RTOS GPIO Task Sync antara STM32 dan ESP32 via UART. STM32 task blink LED dengan delay tertentu, ESP32 task blink dengan sinkronisasi waktu. Komunikasi UART antar-MCU untuk sync timing dengan protokol START, SYNC, ACK. Output berupa LED kedua MCU blink secara sinkron. MULTI_02 adalah RTOS ADC-DAC Communication: STM32 baca ADC potensiometer, kirim ke ESP32 via UART, ESP32 set DAC output sesuai nilai ADC. Queue digunakan di kedua sisi. Output berupa DAC ESP32 mengikuti potensiometer STM32. Kedua eksperimen ini mengimplementasikan komunikasi RTOS antar-MCU dengan protokol sederhana dan pembagian tugas yang sinkron.

---

### Slide 28 — MULTI_03 dan MULTI_04

MULTI_03 adalah RTOS I2C Sensor Sharing: STM32 baca sensor I2C dengan RTOS, ESP32 request data via UART, STM32 kirim data sensor ke ESP32, ESP32 tampilkan data. Mutex/semaphore untuk koordinasi. Output berupa data sensor dapat diakses kedua MCU. MULTI_04 adalah RTOS SPI Data Exchange: STM32 baca data SPI (ADXL345), kirim ke ESP32 via UART, ESP32 proses data dan kirim command balik. Protokol data exchange diimplementasikan. Output berupa data SPI tersedia di ESP32. Kedua eksperimen ini mendemonstrasikan berbagi data sensor dan komunikasi dua arah antar-MCU dengan RTOS primitives.

---

### Slide 29 — MULTI_05 Final Integration

MULTI_05 adalah integrasi akhir RTOS Sensor Hub System: STM32 baca ADC dan sensor I2C dengan RTOS, kirim data teraggregasi ke ESP32 via UART. ESP32 tampilkan data hub dan kirim command. Error handling dan recovery diimplementasikan. Output berupa sistem sensor hub RTOS stabil dengan komunikasi MCU. Eksperimen ini menggabungkan semua konsep: GPIO tasks, interrupt, encoder, serial, DAC, ADC, I2C, SPI, dan komunikasi antar-MCU dalam satu sistem terintegrasi yang robust dan modular.

---

### Slide 30 — Data Pengamatan Praktikum

Setiap eksperimen catat kode, platform, task yang dibuat, FreeRTOS primitive yang digunakan, interrupt yang digunakan, output serial/display, error yang muncul, dan solusi. Tabel 25 eksperimen wajib lengkap. Foto wiring dan screenshot serial/OLED/LCD jadi bukti laporan. Data disimpan dalam folder terstruktur per eksperimen. Mahasiswa diwajibkan mendokumentasikan stack high water mark untuk setiap task dengan `uxTaskGetStackHighWaterMark()` untuk memverifikasi stack usage dan menghindari stack overflow. Laporan juga harus mencantumkan konfigurasi FreeRTOS yang digunakan: configTOTAL_HEAP_SIZE, configMAX_PRIORITIES, configMINIMAL_STACK_SIZE, dan configCHECK_FOR_STACK_OVERFLOW jika diaktifkan.

---

## Bagian 3 — Project, Video, dan Evaluasi (Slide 31–45)

### Slide 31 — Project RTOS Sensor Hub System

Project akhir Modul 09 adalah RTOS Sensor Hub System. ESP32 sebagai gateway, mengelola komunikasi ke PC, koordinator sistem RTOS. STM32 sebagai sensor node dengan FreeRTOS, membaca ADC, encoder, sensor I2C, mengirim data teraggregasi ke ESP32. Keduanya tukar data via UART dengan protokol RTOS. Sensor: potensiometer (ADC), encoder 2-pin, sensor I2C (BME280), sensor SPI (ADXL345). DAC untuk output analog. Semua task berjalan konkuren dengan RTOS primitives untuk sinkronisasi dan proteksi resource. Project ini mengintegrasikan seluruh materi Modul 09 menjadi sistem praktis yang menunjukkan kekuatan RTOS dalam mengelola multiple peripheral dan task secara bersamaan.

---

### Slide 32 — Arsitektur Project

Sensor ADC (potensiometer), encoder 2-pin, sensor I2C (BME280), sensor SPI (ADXL345). DAC output analog. STM32 sebagai sensor node: task ADC, task encoder, task I2C, task SPI, task UART TX ke ESP32. ESP32 sebagai gateway: task UART RX dari STM32, task display, task DAC, task command TX ke STM32. Komunikasi antar-MCU via UART 115200 baud dengan protokol sederhana: START, DATA, CHECKSUM, ACK/NACK. Sistem menggunakan mutex untuk shared resource (UART, I2C, SPI) dan queue untuk data transfer antar task. Event group untuk sinkronisasi multi-event. Arsitektur ini menunjukkan pembagian tugas yang jelas antara STM32 (sensor akuisisi) dan ESP32 (gateway dan kontrol).

---

### Slide 33 — Task dan RTOS Primitives Project

STM32 Tasks: vLEDTask, vEncoderTask, vADCTask, vI2CTask, vSPITask, vUARTTxTask, vUARTRxTask. ESP32 Tasks: vLEDTask, vDACTask, vUARTRxTask, vUARTTxTask, vMonitorTask. RTOS Primitives: Binary semaphore untuk encoder interrupt, mutex untuk I2C/SPI/UART, queue untuk ADC data dan UART RX, event group untuk task synchronization. Semua task berjalan dengan priority yang sesuai: interrupt handler prioritas tinggi, background task prioritas rendah. Stack size disesuaikan dengan kebutuhan masing-masing task. Heap FreeRTOS harus cukup untuk task, queue, semaphore, mutex, dan event group yang dibuat. Konfigurasi `configTOTAL_HEAP_SIZE` harus diperiksa jika terjadi kegagalan pembuatan RTOS primitive.

---

### Slide 34 — Alur Startup Project

Startup: init semua peripheral (GPIO, EXTI, ADC, DAC, I2C, SPI, UART). Create RTOS primitives (semaphore, mutex, queue, event group). Create semua tasks. Start scheduler (STM32) atau biarkan otomatis (ESP32). Tampilkan startup message via UART. Jika sensor I2C tidak terdeteksi, tandai offline tapi sistem tetap jalan. STM32 mulai baca sensor dan kirim data ke ESP32. ESP32 terima data dan tampilkan di monitor. Error handling: jika UART communication timeout, retry atau mark ESP32/STM32 offline. Sistem dirancang untuk tetap berjalan meskipun beberapa sensor mengalami gangguan, menunjukkan robustness RTOS dalam menangani error secara terpisah per task.

---

### Slide 35 — Alur Normal Project

Mode normal: STM32 baca ADC periodik, update encoder count, baca sensor I2C, baca sensor SPI. STM32 kirim data teraggregasi ke ESP32 via UART. ESP32 terima data, tampilkan di serial monitor, set DAC output jika ada command. ESP32 kirim command ke STM32 untuk kontrol LED, start/stop ADC, reset encoder counter. Loop non-blocking, gunakan delay task untuk kontrol frekuensi pembacaan. Data dikirim dengan protokol: [START][LENGTH][DATA...][CHECKSUM][END]. Queue digunakan untuk transfer data antar task tanpa blocking. Mutex memastikan akses shared resource (UART, I2C, SPI) tidak terjadi race condition. Event group untuk sinkronisasi task jika diperlukan.

---

### Slide 36 — Error Mode Project

Jika sensor error (NACK, timeout), tandai sensor offline, pakai data sensor lain yang masih aktif. Retry berkala: task sensor akan mencoba membaca ulang setelah delay tertentu. Jika UART communication error (timeout, checksum invalid), lakukan retry atau tandai MCU lain offline. Task watchdog untuk monitor task hang: jika task tidak memberikan feed dalam waktu tertentu, lakukan recovery atau reset task. Error count dicatat di variabel global atau dikirim ke monitor task. ISR error handling tanpa blocking: ISR hanya memberikan semaphore atau mengirim ke queue, pemrosesan error dilakukan di task level. Sistem dirancang untuk fault-tolerant: satu task atau peripheral error tidak menyebabkan seluruh sistem berhenti.

---

### Slide 37 — Troubleshooting Wajib

Troubleshooting: cek stack overflow dengan `uxTaskGetStackHighWaterMark()`, pastikan ISR-safe API (FromISR) dipakai di ISR, jangan gunakan `HAL_Delay()` di task RTOS, gunakan `vTaskDelay()` atau `osDelay()`. Pastikan mutex diambil dan dilepas dengan benar (take-release pair), hindari deadlock dengan urutan take mutex yang sama di semua task. Periksa priority inversion jika pakai mutex, pastikan priority inheritance bekerja. Cek configTOTAL_HEAP_SIZE cukup untuk semua task dan primitive. Debug dengan serial print, tetapi protect UART dengan mutex. Periksa context switch time jika performa kritis. Gunakan logic analyzer atau osiloskop untuk verifikasi timing interrupt dan task execution. Pastikan interrupt line tidak konflik dan priority interrupt sesuai.

---

### Slide 38 — Deliverable Laporan

Laporan: teori FreeRTOS, tabel 25 eksperimen, foto hardware, screenshot output, analisis error, source code, integrasi project. Bahasa Indonesia, penamaan konsisten Modul 09. Format laporan mengikuti template yang diberikan. Laporan harus mencakup: ringkasan teori RTOS, screenshot kode utama, tabel hasil 25 eksperimen dengan parameter yang diukur, foto wiring untuk setiap eksperimen, screenshot serial monitor atau osiloskop, analisis error yang ditemui dan solusinya, struktur kode dan penjelasan RTOS primitives yang digunakan, link repository source code, dan kesimpulan project. Laporan menunjukkan pemahaman mendalam tentang FreeRTOS dan aplikasinya dalam sistem embedded.

---

### Slide 39 — Tugas Video Modul 09

Video 20–35 menit: pembukaan, teori FreeRTOS, demo 10 STM32, 10 ESP32, 5 Multi, project final, troubleshooting, kesimpulan. Rekaman hardware dan screen recording wajib. Audio jelas, tampilan kode dan output terbaca. Video harus menunjukkan: pembukaan dengan identitas, ringkasan teori FreeRTOS (task, scheduler, ISR, semaphore, mutex, queue, event group), demo 10 eksperimen STM32 (ringkas, 30-60 detik per eksperimen), demo 10 eksperimen ESP32 (ringkas), demo 5 eksperimen Multi, demo project RTOS Sensor Hub System, penutup dengan kendala dan kesimpulan. Screen recording menunjukkan kode, serial monitor, proses build/upload. Rekaman hardware menunjukkan LED blink, button interrupt, encoder rotasi, potensiometer, osiloskop DAC, dan komunikasi antar-MCU.

---

### Slide 40 — Checklist Demo STM32

Demo STM32: GPIO task blink, external interrupt dengan semaphore, encoder 2-pin dengan queue, serial communication dengan queue, DAC sin wave di osiloskop, ADC reading dengan queue, I2C sensor dengan mutex, SPI communication dengan mutex, multi task GPIO-ADC-UART, semaphore dan mutex demonstration. Output: serial log menunjukkan task berjalan, LED blink terlihat, button toggle LED, encoder counter berubah, DAC bentuk gelombang di osiloskop, ADC nilai berubah saat potensiometer diputar, data sensor I2C tampil, data SPI tampil. Semua eksperimen harus menunjukkan penggunaan RTOS primitives yang sesuai dan output yang valid. Mahasiswa harus bisa menjelaskan kode dan konsep RTOS yang digunakan saat demo.

---

### Slide 41 — Checklist Demo ESP32

Demo ESP32: GPIO task blink, external interrupt dengan semaphore, encoder 2-pin dengan queue, serial communication dengan UART driver dan queue, DAC sin wave di osiloskop, ADC reading dengan queue, I2C sensor dengan mutex, SPI communication dengan mutex, multi task GPIO-ADC-UART, semaphore dan mutex demonstration. Output: serial log, LED blink, button toggle, encoder counter, DAC waveform, ADC value, I2C data, SPI data. ESP32 FreeRTOS native dengan task creation dan ISR-safe API. Mahasiswa harus menunjukkan perbedaan penggunaan FreeRTOS pada ESP32 vs STM32 jika ditanya. Demo harus menunjukkan sistem multi-task yang stabil dengan semua peripheral berjalan bersamaan.

---

### Slide 42 — Checklist Demo Multi

Demo multi: STM32 sensor node ke ESP32 gateway, RTOS task sync (LED blink sinkron), ADC-DAC communication (STM32 ADC → ESP32 DAC), I2C sensor sharing (STM32 baca, ESP32 akses data), SPI data exchange (STM32 SPI → ESP32), final integration RTOS Sensor Hub System. Data konsisten di STM32, ESP32, serial monitor. Komunikasi UART antar-MCU dengan protokol sederhana. Error handling ditunjukkan: jika satu MCU disconnect, MCU lain tetap berjalan. Project final menunjukkan integrasi semua konsep: GPIO, interrupt, encoder, serial, DAC, ADC, I2C, SPI dalam satu sistem RTOS yang robust dan terintegrasi.

---

### Slide 43 — Rubrik Penilaian

Penilaian: teori FreeRTOS (15%), demo 10 eksperimen STM32 (15%), demo 10 eksperimen ESP32 (15%), demo 5 eksperimen Multi (15%), demo project RTOS Sensor Hub System (20%), kualitas hardware demo dan wiring explanation (10%), kualitas audio/video/struktur presentasi (10%). Penalti: penamaan tidak konsisten Modul 09 (-5%), demo tidak lengkap proporsional jumlah yang hilang, tidak menjelaskan perbedaan semaphore dan mutex (-5%), tidak menjelaskan ISR-safe API (-5%), tidak ada output hardware (-25%), audio tidak jelas (-10%), video terlalu pendek (-10%), terlambat sesuai kebijakan kelas.

---

### Slide 44 — Referensi dan Best Practice

Referensi: FreeRTOS Official Documentation (https://www.freertos.org/documentation/), Espressif ESP-IDF Programming Guide (FreeRTOS, GPIO, UART, I2C, SPI, ADC, DAC), STM32 Reference Manual dan CMSIS-RTOS Documentation (FreeRTOS integration), Datasheet STM32F103/STM32F4, Datasheet ESP32. Best Practice: gunakan meaningful task names, pilih priority task dengan benar, gunakan stack size yang cukup (cek dengan uxTaskGetStackHighWaterMark()), hindari busy-wait di task, gunakan queue untuk data transfer, semaphore untuk signaling, mutex untuk shared resource protection, tangani error pada pengambilan semaphore/queue, debug dengan serial print namun protect UART dengan mutex, periksa heap RTOS cukup untuk semua task dan primitive.

---

### Slide 45 — Kesimpulan Modul 09

Modul 09 membangun keterampilan FreeRTOS lengkap: teori RTOS, task creation, scheduler, ISR handling, semaphore, mutex, queue, event group, GPIO tasks, external interrupt, encoder 2-pin interrupt, serial communication, DAC dan ADC dengan RTOS, I2C dan SPI dengan RTOS, serta integrasi project RTOS Sensor Hub System. Hasil akhir: 25 eksperimen terdokumentasi, project sistem sensor hub berbasis RTOS, dan pemahaman mendalam tentang pengembangan sistem embedded real-time dengan FreeRTOS pada platform STM32 dan ESP32. Mahasiswa kini memiliki kompetensi untuk mengembangkan sistem embedded kompleks dengan multiple task, interrupt, dan komunikasi antar-MCU menggunakan RTOS primitives secara efektif dan efisien.
