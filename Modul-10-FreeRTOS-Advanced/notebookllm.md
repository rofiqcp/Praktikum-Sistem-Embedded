# Modul 10: FreeRTOS Advanced — NotebookLM 45 Slide

## Bagian 1 — Teori FreeRTOS Advanced (Slide 1–15)

### Slide 1 — Modul 10 dan Target Akhir

Modul 10 membahas fitur lanjutan FreeRTOS yang digunakan dalam sistem embedded industri modern. Target akhir pembelajaran modul ini adalah penyelesaian **25 eksperimen final** yang terdiri dari 10 eksperimen STM32, 10 eksperimen ESP32, dan 5 eksperimen Multi STM32-ESP32 yang dirancang untuk membangun kompetensi praktis mahasiswa dalam mengimplementasikan fitur RTOS lanjutan. Eksperimen mencakup Event Groups, Software Timers, Task Notifications, Semaphore varieties, Mutex dan Priority Inheritance, Memory Management, Static Allocation, Critical Section dan Task Suspension, Message Buffers, Stream Buffers, dan Queue Sets. Setiap eksperimen diimplementasikan pada platform STM32 dan ESP32 untuk membandingkan perilaku FreeRTOS di kedua arsitektur. Eksperimen Multi mengintegrasikan kedua MCU dengan koordinasi menggunakan objek RTOS lanjutan via UART. Semua percobaan diarahkan untuk mendukung pengembangan project akhir berupa Advanced RTOS Industrial System yang mengintegrasikan semua fitur RTOS lanjutan untuk monitoring dan kontrol industri. Mahasiswa diwajibkan memahami teori setiap objek RTOS, mengimplementasikannya pada semua platform, dan mendokumentasikan hasilnya dalam laporan praktikum.

---

### Slide 2 — Event Groups

Event Groups adalah objek FreeRTOS yang menggunakan bit flags untuk sinkronisasi antar task, dengan 24 bit yang dapat diset, cleared, atau ditunggu oleh task. Keunggulannya adalah dapat melakukan sinkronisasi multiple event dalam satu objek, mendukung logika AND/OR untuk kombinasi bit, dan lebih ringan dari multiple semaphore. Fungsi utama meliputi `xEventGroupCreate()` untuk membuat Event Group, `xEventGroupSetBits()` untuk mengatur bit (tersedia versi ISR `xEventGroupSetBitsFromISR()`), `xEventGroupWaitBits()` untuk menunggu bit tertentu, dan `xEventGroupClearBits()` untuk membersihkan bit manual. Contoh kasus penggunaan Event Groups adalah untuk sinkronisasi button press event, mode switch industri, dan koordinasi multiple task yang bergantung pada event berbeda. Pada praktikum, Event Groups diuji pada STM32_01 dan ESP32_01 untuk menyinkronisasikan LED indikator dengan button press menggunakan bit 0 dan bit 1.

---

### Slide 3 — Software Timers

Software Timers adalah timer yang dikelola oleh FreeRTOS kernel, berjalan di task daemon timer dengan prioritas yang dapat dikonfigurasi melalui `configTIMER_TASK_PRIORITY`. Keunggulannya adalah non-blocking, tidak memakan CPU saat menunggu, dan satu task daemon dapat mengelola semua Software Timer secara bersamaan. Fungsi utama meliputi `xTimerCreate()` untuk membuat timer (one-shot atau periodik), `xTimerStart()` untuk memulai timer, `xTimerStop()` untuk menghentikan timer, dan `xTimerChangePeriod()` untuk mengubah periode timer. Perbedaan dengan task periodik adalah Software Timer tidak memiliki state sendiri dan lebih ringan untuk eksekusi berkala sederhana, sedangkan task periodik memiliki kontrol penuh atas eksekusi dan stack sendiri. Pada praktikum, Software Timers diuji pada STM32_02 dan ESP32_02 untuk toggle LED setiap 1 detik tanpa memblokir task lain.

---

### Slide 4 — Task Notifications

Task Notifications adalah mekanisme komunikasi antar task paling ringan di FreeRTOS, menggunakan struktur pada TCB task tujuan tanpa objek eksplisit tambahan. Keunggulannya adalah 45% lebih cepat dan 45% lebih hemat memory dibandingkan semaphore atau queue, serta mendukung pengiriman nilai 32-bit antar task. Fungsi utama meliputi `xTaskNotifyGive()` untuk mengirim notifikasi (mirip increment semaphore), `ulTaskNotifyTake()` untuk menerima notifikasi (mirip decrement semaphore), `xTaskNotify()` untuk mengirim notifikasi dengan nilai 32-bit, dan `xTaskNotifyWait()` untuk menunggu notifikasi dengan nilai tertentu. Keterbatasannya adalah hanya bisa mengirim notifikasi ke task tertentu (unicast) dan tidak mendukung broadcast seperti Event Groups. Pada praktikum, Task Notifications diuji pada STM32_03 dan ESP32_03 untuk menggantikan binary semaphore dengan respon yang lebih cepat.

---

### Slide 5 — Semaphore Varieties

FreeRTOS menyediakan tiga jenis semaphore: Binary Semaphore, Counting Semaphore, dan Mutex. Binary Semaphore memiliki nilai 0 atau 1, digunakan untuk sinkronisasi (misalnya ISR ke task), dan tidak memiliki Priority Inheritance. Counting Semaphore memiliki nilai 0 hingga max count, digunakan untuk mengontrol akses ke multiple resource identik, dan dibuat dengan `xSemaphoreCreateCounting()`. Mutex mirip Binary Semaphore tetapi memiliki mekanisme Priority Inheritance, digunakan untuk akses shared resource kritis, dan hanya owner yang boleh mengembalikan mutex. Fungsi umum meliputi `xSemaphoreCreateBinary()`, `xSemaphoreCreateMutex()`, `xSemaphoreTake()`, dan `xSemaphoreGive()`. Pada praktikum, ketiga jenis semaphore diuji pada STM32_04 dan ESP32_04 untuk membandingkan perilaku sinkronisasi dan kontrol resource.

---

### Slide 6 — Mutex dan Priority Inheritance

Priority Inversion terjadi ketika task low priority memegang mutex, task high priority menunggu mutex, dan task medium priority menunda task low priority sehingga task high priority ikut tertunda. Mutex FreeRTOS menggunakan Priority Inheritance: saat task high priority menunggu mutex yang dipegang task low priority, prioritas task low priority dinaikkan ke prioritas task high priority sementara hingga mutex dikembalikan. Ini mencegah task medium priority menunda task low priority saat memegang mutex. Contoh alur: Task Low (prio 1) ambil mutex, Task High (prio 3) coba ambil mutex → tertunda, prioritas Task Low dinaikkan ke 3, Task Medium (prio 2) tidak bisa jalan karena prioritas Task Low lebih tinggi, Task Low selesai kembalikan mutex → prioritas kembali ke 1, Task High jalan. Pada praktikum, mekanisme ini diuji pada STM32_05 dan ESP32_05.

---

### Slide 7 — Memory Management

FreeRTOS menyediakan 5 jenis heap management: heap_1 (hanya alokasi, tidak bisa free, untuk sistem alokasi sekali), heap_2 (bisa free tetapi fragmentasi tinggi, tidak direkomendasi), heap_3 (wrapper malloc/free standar, thread-safe), heap_4 (dinamis dengan coalescence, fragmentasi rendah, direkomendasi umum), dan heap_5 (multiple memory region, untuk STM32 dengan RAM terpisah). Fungsi terkait meliputi `pvPortMalloc()` untuk alokasi, `vPortFree()` untuk membebaskan, `xPortGetFreeHeapSize()` untuk cek sisa heap, dan `xPortGetMinimumEverFreeHeapSize()` untuk cek heap minimum pernah tersisa. Pada praktikum, semua jenis heap diuji pada STM32_06 dan ESP32_06 untuk menganalisis fragmentasi dan penggunaan memory.

---

### Slide 8 — Static Allocation

Static Allocation membuat objek FreeRTOS (task, queue, semaphore, timer) dengan memory yang dialokasikan sebelumnya (array statis), bukan dari heap dinamis. Ini cocok untuk sistem deterministik yang tidak ingin fragmentasi heap. Langkah membuat task statis: deklarasi `StaticTask_t xTaskBuffer` dan `StackType_t xStack[STACK_SIZE]`, lalu buat task dengan `xTaskCreateStatic()` yang membutuhkan parameter buffer statis. Objek lain seperti queue, semaphore, dan timer juga memiliki versi `CreateStatic()` dengan parameter buffer statis. Keunggulannya adalah waktu eksekusi deterministik tanpa alokasi dinamis, dan cocok untuk sistem safety-critical. Pada praktikum, Static Allocation diuji pada STM32_07 dan ESP32_07 untuk membuat task dan queue tanpa menyentuh heap dinamis.

---

### Slide 9 — Critical Section dan Task Suspension

Critical Section adalah mekanisme proteksi data paling ringan di FreeRTOS — menonaktifkan scheduler/interrupt sementara agar operasi berjalan atomis. Fungsi utama: `taskENTER_CRITICAL()` dan `taskEXIT_CRITICAL()` untuk proteksi singkat. Versi ISR: `taskENTER_CRITICAL_FROM_ISR()` dan `taskEXIT_CRITICAL_FROM_ISR()`. Perbedaan dengan Mutex: Critical Section tidak memanggil RTOS API di dalamnya (karena scheduler dinonaktifkan), sementara Mutex dapat memanggil RTOS API dan mendukung Priority Inheritance. Critical Section cocok untuk operasi sangat singkat (increment counter, set flag), Mutex untuk operasi lebih lama yang perlu memanggil fungsi RTOS. Task Suspension (`vTaskSuspend()` / `vTaskResume()`) memungkinkan kontrol eksekusi task dari task lain tanpa menghapus task. Pada praktikum, Critical Section dan Task Suspension diuji pada STM32_08 dan ESP32_08 untuk demonstrasi race condition dan perlindungan data atomis.

---

### Slide 10 — Message Buffers dan Stream Buffers

Message Buffer digunakan untuk transfer data dengan panjang variabel antar task atau antara ISR dan task. Setiap pengiriman menulis pesan dengan header panjang pesan otomatis, sehingga penerima tahu panjang pesan yang diterima. Fungsi utama meliputi `xMessageBufferCreate()` untuk membuat buffer, `xMessageBufferSend()` untuk mengirim pesan (tersedia versi ISR), dan `xMessageBufferReceive()` untuk menerima pesan. Keunggulannya adalah otomatis mendeteksi panjang pesan, dapat menangani pesan 1 byte hingga ukuran buffer penuh, dan thread-safe untuk ISR dan task. Cocok untuk pesan variabel seperti komando, log, atau data sensor dengan panjang berubah. Pada praktikum, Message Buffers diuji pada STM32_08 dan ESP32_08 untuk mengirim pesan 1-128 byte antar task.

---

### Slide 10 — Message Buffers dan Stream Buffers

Message Buffer digunakan untuk transfer data dengan panjang variabel antar task atau ISR. Header panjang otomatis ditambahkan saat pengiriman sehingga penerima tahu panjang pesan. Fungsi: `xMessageBufferCreate()`, `xMessageBufferSend()`, `xMessageBufferReceive()`. Cocok untuk pesan variabel seperti komando, log, JSON. Stream Buffer digunakan untuk transfer byte stream kontinu tanpa header panjang — hanya byte mentah. Fungsi: `xStreamBufferCreate(size, triggerLevel)`, `xStreamBufferSend()`, `xStreamBufferReceive()`. Trigger level menentukan minimum byte sebelum task penerima bangun. Stream Buffer cocok untuk audio, sensor stream, data serial. Pada praktikum, keduanya diuji bersama pada STM32_09 dan ESP32_09 untuk membandingkan perilaku masing-masing dengan data variabel dan stream kontinu.

---

### Slide 11 — Queue Sets

Queue Set memungkinkan satu task menunggu event dari multiple queue, semaphore, atau mutex dalam satu panggilan tanpa perlu polling masing-masing objek. Langkah penggunaan: buat Queue Set dengan `xQueueCreateSet()` (parameter ukuran total semua objek), tambahkan objek ke Queue Set dengan `xQueueAddToSet()`, lalu task menunggu event dengan `xQueueSelectFromSet()` dan membaca objek yang aktif setelah event terdeteksi. Keunggulannya adalah menghindari polling multiple objek (menghemat CPU), dan satu task dapat menangani banyak sumber event sekaligus. Cocok untuk task gateway yang menerima input dari banyak sumber seperti queue data, semaphore event, dan notifikasi. Pada praktikum, Queue Sets diuji pada STM32_10 dan ESP32_10 untuk memantau 2 queue dan 1 semaphore dalam satu task.

---

### Slide 12 — FreeRTOS pada STM32

STM32 mendukung FreeRTOS melalui STM32Cube HAL, dengan integrasi penuh pada STM32CubeIDE. Konfigurasi FreeRTOS dilakukan melalui graphical configurator: pengaturan tick rate, prioritas timer task, heap size, dan jumlah idle task. STM32 memiliki RAM terbatas sehingga pemilihan heap management (heap_4 direkomendasi) dan penggunaan Static Allocation untuk task kritis sangat penting. Implementasi pada STM32 menggunakan HAL FreeRTOS API yang kompatibel dengan standar FreeRTOS API, dengan tambahan fitur seperti CMSIS-RTOS wrapper jika diperlukan. Eksperimen STM32 pada Modul 10 menggunakan STM32F103C8/STM32F4 dengan FreeRTOS yang dikonfigurasi melalui STM32CubeIDE, dan mahasiswa diwajibkan membandingkan perilaku objek RTOS dengan teori yang dipelajari.

---

### Slide 13 — FreeRTOS pada ESP32

ESP32 mendukung FreeRTOS melalui ESP-IDF dan Arduino framework, dengan penyesuaian untuk dual-core ESP32 (xCoreID untuk menentukan core task berjalan). ESP-IDF menyediakan FreeRTOS extended API untuk fitur lanjutan seperti ring buffer (mirip Stream Buffer), queue sets, dan task notifications. ESP32 memiliki RAM lebih besar dibanding STM32 Blue Pill, sehingga lebih fleksibel untuk alokasi dinamis, tetapi Static Allocation tetap disarankan untuk task kritis. Konfigurasi FreeRTOS pada ESP-IDF dilakukan melalui menuconfig: pengaturan tick rate, stack size default, heap size, dan fitur lanjutan. Eksperimen ESP32 pada Modul 10 menggunakan ESP-IDF/Arduino, dan mahasiswa diwajibkan membandingkan efisiensi Task Notifications vs Semaphore pada ESP32.

---

### Slide 14 — Pemilihan Objek RTOS

Pemilihan objek RTOS yang tepat bergantung pada kasus penggunaan:
- Event Groups: sinkronisasi multiple event/flags.
- Software Timers: eksekusi berkala/penundaan non-blocking.
- Task Notifications: komunikasi unicast ringan antar task.
- Binary Semaphore: sinkronisasi 1:1 (ISR ke task).
- Counting Semaphore: kontrol multiple resource identik.
- Mutex: shared resource dengan priority inheritance.
- Message Buffer: pesan variabel antar task/ISR.
- Stream Buffer: byte stream kontinu antar task/ISR.
- Queue Sets: pantau multiple queue/semaphore dalam satu task.

Kesalahan pemilihan objek dapat menyebabkan inefisiensi, priority inversion, atau kehabisan memory. Mahasiswa diwajibkan menganalisis kasus penggunaan industri dan memilih objek RTOS yang paling tepat.

---

### Slide 15 — Error Handling RTOS

Error umum pada FreeRTOS lanjutan meliputi:
- Heap habis: gunakan `xPortGetFreeHeapSize()` dan pilih heap_4, atau gunakan Static Allocation.
- Buffer overrun/underrun: sesuaikan ukuran Message/Stream Buffer, atau implementasikan overrun handling.
- Priority Inversion: gunakan Mutex bukan Binary Semaphore untuk shared resource.
- Task deadlock: hindari mengambil multiple mutex secara bersamaan, atau gunakan priority inheritance.
- Queue Set miss: pastikan semua objek ditambahkan ke Queue Set, dan `xQueueSelectFromSet()` dipanggil dengan timeout yang tepat.

Pada praktikum, mahasiswa diwajibkan menguji skenario error dan mengimplementasikan penanganan error yang sesuai untuk memastikan sistem tetap stabil meskipun terjadi error.

---

## Bagian 2 — Praktikum 20 Eksperimen (Slide 16–30)

### Slide 16 — Struktur Final Praktikum

Modul 10 memiliki tepat **25 eksperimen** yang terdiri dari STM32_01 sampai STM32_10 (10 eksperimen), ESP32_01 sampai ESP32_10 (10 eksperimen), dan MULTI_01 sampai MULTI_05 (5 eksperimen Multi STM32-ESP32). Struktur ini memastikan mahasiswa memahami FreeRTOS lanjutan dari sisi STM32, ESP32, dan integrasi dua MCU secara bertahap. Eksperimen STM32 dan ESP32 mencakup: Event Groups, Software Timers, Task Notifications, Semaphore Varieties, Mutex+Priority Inheritance, Memory Management, Static Allocation, Critical Section+Task Suspension, Message+Stream Buffers, dan Queue Sets. Eksperimen Multi mencakup: Distributed Event Group Sync, Multi-MCU Timer Network, Task Notification Pipeline, Priority Inheritance Network, dan Full Advanced Industrial System. Mahasiswa diwajibkan menyelesaikan semua 25 eksperimen untuk mendapatkan nilai lengkap.

---

### Slide 17 — STM32_01 dan STM32_02

STM32_01 adalah Event Groups Basic: membuat Event Group, set bit 0 saat button ditekan, task menunggu bit 0 lalu toggle LED PB0. Output berupa LED yang berubah status sesuai event bit. STM32_02 adalah Software Timers: membuat timer periodik 1 detik untuk toggle LED PB0, mengukur jeda waktu toggle, dan membandingkan dengan task periodik biasa. Output berupa LED toggle tepat 1 detik tanpa blocking task lain. Mahasiswa diwajibkan menguji penambahan event bit 1 pada STM32_01 dan timer one-shot pada STM32_02 untuk memahami fleksibilitas kedua objek RTOS ini.

---

### Slide 18 — STM32_03 dan STM32_04

STM32_03 adalah Task Notifications: taskA mengirim notifikasi saat button ditekan, taskB menunggu notifikasi lalu toggle LED. Bandingkan respon time dengan binary semaphore. STM32_04 adalah Semaphore Varieties: buat Binary Semaphore untuk sinkronisasi button ke LED PB0, Counting Semaphore max count 3 untuk LED PB1. Uji perbedaan perilaku kedua semaphore. Output menunjukkan Binary Semaphore sinkronisasi 1:1, Counting Semaphore mengizinkan multiple resource. Mahasiswa diwajibkan mencatat perbedaan efisiensi Task Notifications vs Semaphore pada laporan.

---

### Slide 19 — STM32_05 dan STM32_06

STM32_05 adalah Mutex and Priority Inheritance: buat 3 task Low, Medium, High. Low ambil mutex, High coba ambil mutex → amati Priority Inheritance (Medium tidak menunda High). Bandingkan dengan Binary Semaphore. STM32_06 adalah Memory Management: uji heap_1 hingga heap_5, alokasi dan free memory berulang, catat sisa heap dan fragmentasi. Pilih heap_4 sebagai rekomendasi. Output berupa laporan penggunaan heap dan rekomendasi untuk aplikasi industri.

---

### Slide 20 — STM32_07 dan STM32_08

STM32_07 adalah Static Allocation: alokasi statis TCB dan stack untuk task, buat task dengan `xTaskCreateStatic()`, buat queue statis. Toggle LED saat data diterima via queue statis. STM32_08 adalah Critical Section dan Task Suspension: demonstrasikan race condition pada variabel `shared_counter` tanpa proteksi, lalu gunakan `taskENTER_CRITICAL()`/`taskEXIT_CRITICAL()` untuk perbaiki. Tambah `vTaskSuspend()`/`vTaskResume()` untuk kontrol eksekusi task dari task lain. Output menunjukkan nilai counter konsisten dengan critical section; task dapat di-suspend/resume runtime.

---

### Slide 21 — STM32_09 dan STM32_10

STM32_09 adalah Message Buffers dan Stream Buffers: bagian 1 buat Message Buffer kirim pesan variabel 1-128 byte; bagian 2 buat Stream Buffer kirim byte stream 100 byte/100ms; bandingkan perilaku keduanya; demonstrasikan overrun. STM32_10 adalah Queue Sets Multiplexing: buat 2 queue + 1 semaphore, tambahkan ke Queue Set, task monitor gunakan `xQueueSelectFromSet()`, identifikasi sumber event (Data/Status/Button), toggle LED sesuai event. Output menunjukkan task menangani multiple objek tanpa polling per objek.

---

### Slide 22 — ESP32_01 dan ESP32_02

ESP32_01 adalah Event Groups Basic: sama dengan STM32_01 tetapi pada ESP32, menggunakan GPIO2 untuk LED dan GPIO34 untuk button. Output LED berubah sesuai event bit. ESP32_02 adalah Software Timers: timer periodik 1 detik toggle LED GPIO2, ukur jeda waktu. Output LED toggle tepat tanpa blocking. Mahasiswa diwajibkan membandingkan perilaku Event Groups dan Software Timers antara STM32 dan ESP32.

---

### Slide 23 — ESP32_03 dan ESP32_04

ESP32_03 adalah Task Notifications: sama dengan STM32_03 pada ESP32. Bandingkan respon time dengan semaphore. ESP32_04 adalah Semaphore Varieties: Binary dan Counting Semaphore pada ESP32, uji perilaku sinkronisasi dan kontrol resource. Output menunjukkan perbedaan kedua semaphore sesuai teori.

---

### Slide 24 — ESP32_05 dan ESP32_06

ESP32_05 adalah Mutex and Priority Inheritance: sama dengan STM32_05 pada ESP32, amati Priority Inheritance. ESP32_06 adalah Memory Management: uji heap FreeRTOS pada ESP32, ESP32 default menggunakan heap_4. Catat sisa heap dan fragmentasi. Output berupa laporan penggunaan heap ESP32.

---

### Slide 25 — ESP32_07 dan ESP32_08

ESP32_07 adalah Static Allocation: task dan queue statis pada ESP32, sama dengan STM32_07. ESP32_08 adalah Critical Section dan Task Suspension: sama dengan STM32_08 pada ESP32 — demonstrasikan race condition, perbaiki dengan `taskENTER_CRITICAL()`, tambah `vTaskSuspend()`/`vTaskResume()`. Output menunjukkan nilai counter konsisten; task dapat di-suspend/resume.

---

### Slide 26 — ESP32_09 dan ESP32_10

ESP32_09 adalah Message Buffers dan Stream Buffers pada ESP32: sama dengan STM32_09. Uji kecepatan stream buffer 100 byte/detik dan 1000 byte/detik. ESP32_10 adalah Queue Sets Multiplexing pada ESP32: sama dengan STM32_10, toggle LED GPIO2 saat data event, LED GPIO4 saat status event. Output menunjukkan task menangani multiple queue tanpa polling.

---

### Slide 27 — MULTI_01 dan MULTI_02

MULTI_01 adalah Distributed Event Group Synchronization: STM32 set event bit via button + software timer, kirim event code ke ESP32 via UART; ESP32 set event group lokal sesuai kode diterima, kirim ACK ke STM32. Round-trip event sync <10 ms, LED kedua MCU mencerminkan state event. MULTI_02 adalah Multi-MCU Software Timer Network: ESP32 master timer coordinator kirim perintah START/STOP/CHANGE_PERIOD ke STM32; STM32 jalankan software timer sesuai perintah dan lapor event kembali ke ESP32; ESP32 log timestamp timer event. Demonstrasikan pengendalian software timer antar MCU via UART command.

---

### Slide 28 — MULTI_03 dan MULTI_04

MULTI_03 adalah Task Notification Pipeline: STM32 ADC task baca sensor → task notification → UART TX task → ESP32 RX ISR → task notification → processor task → Message Buffer → analyzer task. Pipeline menggunakan hanya Task Notification + Message Buffer tanpa semaphore konvensional; latensi pipeline <5 ms. MULTI_04 adalah Priority Inheritance dan Critical Section Network: STM32 demonstrasikan priority inheritance (3 task, Low pegang mutex, High tertunda, Medium blocked); kirim log ke ESP32; ESP32 gunakan critical section untuk update statistik atomis dan tampilkan laporan inheritance events. Kedua MCU membuktikan mekanisme RTOS lanjutan dalam skenario jaringan.

---

### Slide 29 — MULTI_05 Full Advanced Industrial System

MULTI_05 adalah integrasi penuh semua fitur Modul 10 dalam satu sistem industri. STM32 node: Static Allocation untuk task kritis, Critical Section untuk stats atomis, Message Buffer untuk paket data variabel, Task Notification untuk pipeline ADC. ESP32 gateway: Event Group state machine (IDLE/ACTIVE/ALARM), Software Timer watchdog 5 detik, Queue Set pantau multiple sumber data, Stream Buffer logging kontinu. UART bridge bawa data + perintah antar MCU. LED indicators: hijau (ACTIVE), kuning (IDLE), merah (ALARM). Watchdog ESP32 trigger ALARM jika STM32 tidak kirim data 5 detik. Demonstrasikan semua 10+ objek RTOS aktif secara bersamaan dalam satu sistem terintegrasi.

---

### Slide 30 — Data Pengamatan Praktikum

Mahasiswa diwajibkan membandingkan implementasi FreeRTOS lanjutan antara STM32 dan ESP32:
- Event Groups: perilaku sama, hanya perbedaan pin GPIO.
- Software Timers: sama pada kedua platform.
- Task Notifications: lebih efisien dari semaphore pada kedua platform.
- Semaphores/Mutex: Priority Inheritance bekerja sama.
- Memory Management: STM32 lebih terbatas RAM, ESP32 lebih fleksibel.
- Static Allocation: sama pada kedua platform.
- Message/Stream Buffers: sama pada kedua platform.
- Queue Sets: sama pada kedua platform.

Perbandingan ini membantu mahasiswa memahami portabilitas FreeRTOS antar platform.

---

### Slide 30 — Data Pengamatan Praktikum

Setiap eksperimen wajib dicatat: kode eksperimen, platform, objek RTOS yang digunakan, wiring, output serial/LED, error yang muncul dan solusi. Tabel **25 eksperimen** (10 STM32 + 10 ESP32 + 5 Multi) wajib lengkap. Foto wiring dan screenshot serial/LED sebagai bukti. Untuk eksperimen Multi, catat protokol UART yang digunakan dan timing komunikasi antar MCU. Mahasiswa diwajibkan dokumentasi penggunaan heap dengan `xPortGetFreeHeapSize()` dan stack usage dengan `uxTaskGetStackHighWaterMark()` untuk setiap task di semua eksperimen. Data disimpan dalam folder terstruktur per eksperimen.

---

### Slide 29 — Troubleshooting RTOS

Troubleshooting umum pada praktikum Modul 10:
- Task tidak jalan: cek prioritas task, pastikan task tidak阻塞 selamanya.
- Heap habis: gunakan `xPortGetFreeHeapSize()`, pilih heap_4 atau Static Allocation.
- Buffer overrun: besarkan ukuran Message/Stream Buffer.
- Priority Inversion: gunakan Mutex dengan Priority Inheritance.
- Queue Set tidak mendeteksi event: pastikan semua objek ditambahkan ke Queue Set.
- Task Notifications tidak diterima: pastikan task tujuan benar, dan `ulTaskNotifyTake()` dipanggil.

Mahasiswa diwajibkan mendokumentasikan error dan solusi pada laporan praktikum.

---

### Slide 30 — Deliverable Laporan

Laporan praktikum Modul 10 harus memuat:
1. Cover dan identitas.
2. Ringkasan teori FreeRTOS Advanced.
3. Tabel hasil 20 eksperimen.
4. Foto wiring tiap eksperimen.
5. Screenshot serial/LED output.
6. Analisis error dan troubleshooting.
7. Source code utama atau link repository.
8. Integrasi project Advanced RTOS Industrial System.
9. Kesimpulan dan perbandingan STM32 vs ESP32.

Bahasa laporan Indonesia, penamaan file konsisten Modul 10.

---

## Bagian 3 — Project, Video, dan Evaluasi (Slide 31–45)

### Slide 31 — Project Advanced RTOS Industrial System

Project akhir Modul 10 adalah Advanced RTOS Industrial System untuk monitoring dan kontrol industri. ESP32 berperan sebagai gateway dan koordinator sistem, STM32 sebagai node sensor deterministik. Keduanya menggunakan objek RTOS lanjutan untuk menangani event, timer, notifikasi, sinkronisasi, dan manajemen memory. Fitur sistem meliputi deteksi event button dengan Event Groups, pengecekan periodik dengan Software Timers, notifikasi data ready dengan Task Notifications, kontrol akses shared resource dengan Mutex, logging data dengan Stream/Message Buffers, dan pantau multiple input dengan Queue Sets. Output ditampilkan melalui LED indikator dan serial monitor, serta komunikasi antar MCU menggunakan objek RTOS yang tepat.

---

### Slide 32 — Arsitektur Project

Arsitektur sistem terdiri dari ESP32 gateway dan STM32 node yang berkomunikasi melalui UART menggunakan Queue Sets atau Message Buffers. ESP32 menangani Event Groups, Software Timers, dan Task Notifications untuk koordinasi sistem. STM32 menangani Mutex untuk shared resource, Memory Management, dan Static Allocation untuk task kritis. Kedua platform mengimplementasikan Message/Stream Buffers untuk logging data, dan Queue Sets untuk pantau multiple input. Sensor simulasi (potensiometer) dan button event dihubungkan ke kedua MCU, dengan LED indikator untuk setiap objek RTOS yang aktif. Sistem dirancang agar tetap stabil meskipun salah satu objek RTOS mengalami error.

---

### Slide 33 — Objek RTOS pada Project

Objek RTOS yang diimplementasikan pada project:
- Event Groups: sinkronisasi button panic event dan mode switch.
- Software Timers: pengecekan sensor periodik 1 detik.
- Task Notifications: notifikasi data sensor ready ke task processing.
- Mutex: kontrol akses shared serial untuk menghindari priority inversion.
- Heap_4: manajemen memory efisien dengan fragmentasi rendah.
- Static Allocation: task kritis (watchdog, logging) dialokasikan statis.
- Message Buffers: pengiriman pesan variabel antar task.
- Stream Buffers: logging data sensor stream.
- Queue Sets: pantau multiple input dari button, queue data, dan semaphore.

Semua objek RTOS yang dipelajari pada teori dan praktikum diintegrasikan ke dalam project ini.

---

### Slide 34 — Alur Operasi Project

Alur startup: init semua objek RTOS, buat task dengan prioritas sesuai, start Software Timers, tampilkan status sistem via serial.
Alur normal: task monitor button set event bit pada Event Group, Software Timers trigger pengecekan periodik, task sensor kirim notifikasi saat data siap, task kontrol akses shared resource dengan Mutex, task logging tulis data ke Stream/Message Buffer, task gateway pantau Queue Set untuk multiple input.
Alur error: jika heap habis, gunakan Static Allocation untuk task baru; jika buffer penuh, implementasikan overrun handling; jika priority inversion terdeteksi, pastikan Mutex digunakan dengan benar.

---

### Slide 35 — Fitur Minimal Project

Fitur wajib yang ditampilkan pada demo project:
1. Event Groups merespons button press dengan LED indikator.
2. Software Timers berjalan periodik tanpa blocking task lain.
3. Task Notifications lebih efisien dari semaphore (ditunjukkan dengan respon time).
4. Mutex mencegah priority inversion (ditunjukkan dengan percobaan Priority Inheritance).
5. Memory Management menunjukkan penggunaan heap_4 dengan sisa heap yang cukup.
6. Static Allocation task/queue kritis berjalan tanpa alokasi dinamis.
7. Message Buffer menangani pesan variabel 1-128 byte utuh.
8. Stream Buffer menangani byte stream sensor tanpa overrun.
9. Queue Sets menangani 2 queue + 1 semaphore dalam satu task.
10. Komunikasi antar MCU berjalan stabil menggunakan objek RTOS.

---

### Slide 36 — Tugas Video Modul 10

Video 20–35 menit: pembukaan, teori FreeRTOS Advanced, demo 10 eksperimen STM32, demo 10 eksperimen ESP32, demo project final, troubleshooting, kesimpulan. Rekaman hardware dan screen recording wajib. Audio jelas, tampilan kode dan output terbaca. Bahasa presentasi Indonesia. Video mengikuti struktur yang sama dengan Modul 07, dengan penyesuaian isi ke topik FreeRTOS Advanced. Link video diunggah ke YouTube Unlisted atau e-learning sesuai instruksi dosen, bersama dengan link repository source code dan laporan PDF/MD.

---

### Slide 37 — Checklist Demo STM32

Demo STM32: Event Groups (LED response button), Software Timers (LED toggle 1 detik), Task Notifications (respon lebih cepat dari semaphore), Semaphore Varieties (Binary vs Counting), Mutex/Priority Inheritance (High task tidak tertunda Medium), Memory Management (heap_4, sisa heap), Static Allocation (task statis), Message Buffers (pesan variabel), Stream Buffers (byte stream), Queue Sets (multiple objek). Output serial log dan LED terlihat jelas, error handling ditunjukkan jika diperlukan.

---

### Slide 38 — Checklist Demo ESP32

Demo ESP32: sama dengan STM32, dengan penyesuaian pin GPIO ESP32. Tunjukkan perbedaan penggunaan FreeRTOS antara STM32 dan ESP32, terutama pada Memory Management dan Static Allocation. Output serial dan LED ESP32 terlihat jelas, komunikasi antar MCU ditunjukkan dengan pengiriman data antara ESP32 dan STM32 menggunakan Queue Sets atau Message Buffers.

---

### Slide 39 — Checklist Demo Project

Demo project Advanced RTOS Industrial System: tampilkan hardware ESP32 dan STM32 aktif, semua objek RTOS bekerja sesuai alur, LED indikator merespons event yang sesuai, serial monitor menampilkan log sistem lengkap, error handling ditunjukkan dengan simulasi error (misal: heap habis, buffer penuh). Pastikan sistem tetap stabil meskipun beberapa objek RTOS mengalami error ringan.

---

### Slide 40 — Rubrik Penilaian

| Komponen | Bobot |
|---|---:|
| Pemahaman teori FreeRTOS Advanced lengkap | 15% |
| Demo 10 eksperimen STM32 | 15% |
| Demo 10 eksperimen ESP32 | 15% |
| Demo project Advanced RTOS Industrial System | 20% |
| Kualitas hardware demo dan wiring explanation | 10% |
| Kualitas audio/video/struktur presentasi | 10% |
| Laporan praktikum lengkap | 10% |
| Penalti kesalahan penamaan/teori | sesuai aturan |

---

### Slide 41 — Penalti Video

| Pelanggaran | Penalti |
|---|---:|
| Tidak ada demo hardware | −25% |
| Tidak menunjukkan 20 eksperimen | proporsional jumlah yang hilang |
| Salah menyebut Modul 09, bukan Modul 10 | −5% |
| Tidak menjelaskan Priority Inheritance | −5% |
| Tidak ada output serial/LED | −10% |
| Audio tidak jelas | −10% |
| Video terlalu pendek (<15 menit) | −10% |
| Terlambat pengumpulan | sesuai kebijakan kelas |

---

### Slide 42 — Referensi Modul 10

1. FreeRTOS Official Documentation — Semua topik FreeRTOS Advanced.
2. Espressif ESP-IDF Programming Guide — FreeRTOS on ESP32.
3. STM32Cube Reference Manual — FreeRTOS integration with STM32 HAL.
4. Book: "Mastering the FreeRTOS Real Time Kernel" by Richard Barry.
5. Datasheet STM32F103/STM32F4 dan ESP32 untuk konfigurasi GPIO dan memory.

Mahasiswa diwajibkan membaca referensi tersebut untuk memperdalam pemahaman FreeRTOS Advanced.

---

### Slide 43 — Tips Implementasi RTOS

1. Selalu cek return value dari fungsi FreeRTOS (misal: `xQueueSend()` return `pdPASS` atau `errQUEUE_FULL`).
2. Gunakan `configASSERT()` untuk debugging task/queue/semaphore creation.
3. Atur prioritas task dengan benar: task kritis memiliki prioritas lebih tinggi.
4. Hindari penggunaan `vTaskDelay()` untuk sinkronisasi, gunakan objek RTOS yang tepat.
5. Untuk sistem deterministik, gunakan Static Allocation untuk task/queue kritis.
6. Selalu uji error scenario untuk memastikan sistem stabil.

---

### Slide 44 — Perbandingan RTOS Lanjutan vs Dasar

FreeRTOS Basic: task, queue, semaphore dasar, delay. Cocok untuk sistem sederhana.
FreeRTOS Advanced (Modul 10): Event Groups, Software Timers, Task Notifications, Mutex dengan Priority Inheritance, Memory Management lanjutan, Static Allocation, Message/Stream Buffers, Queue Sets. Cocok untuk sistem industri kompleks yang membutuhkan sinkronisasi canggih, manajemen memory efisien, dan determinisme.

Mahasiswa diwajibkan memahami perbedaan ini untuk memilih fitur RTOS yang tepat sesuai kompleksitas aplikasi.

---

### Slide 45 — Kesimpulan Modul 10

Modul 10 membangun keterampilan FreeRTOS lanjutan lengkap: teori dan implementasi Event Groups, Software Timers, Task Notifications, Semaphore varieties, Mutex/Priority Inheritance, Memory Management, Static Allocation, Message/Stream Buffers, dan Queue Sets pada STM32 dan ESP32. Hasil akhir adalah Advanced RTOS Industrial System dengan 20 eksperimen terdokumentasi, yang mempersiapkan mahasiswa untuk mengimplementasikan RTOS pada sistem industri nyata. Mahasiswa diharapkan mampu memilih objek RTOS yang tepat, menangani error RTOS, dan membangun sistem embedded deterministik yang stabil dan efisien.
