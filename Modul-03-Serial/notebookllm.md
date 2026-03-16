# NotebookLLM Modul 03 - Komunikasi Serial UART

## SLIDE 1:
UART itu seperti antrean kasir: bit lewat satu per satu, bukan serombongan. Walau terlihat lambat, cara ini stabil untuk kabel panjang dan perangkat berbeda. Di modul ini kita belajar mengirim data dengan ritme yang disepakati.

## SLIDE 2:
Frame UART punya urutan bak amplop surat: start bit membuka, data bit membawa isi, parity jadi satpam pengecek, stop bit menutup. Format paling umum 8N1 berarti 8 data, tanpa parity, 1 stop bit pada 115200 baud.

## SLIDE 3:
Baud rate ibarat kecepatan bicara dua orang. Jika satu bicara terlalu cepat dan lawan lebih lambat, pesan jadi salah dengar. Karena itu ESP32 dan STM32 wajib set baud sama, misalnya 115200, supaya ritme kirim-terima sinkron.

## SLIDE 4:
ESP32 punya 3 UART dengan pin fleksibel, seperti stopkontak yang bisa dipindah tempat. STM32F103 juga punya 3 USART, tetapi jalur pin lebih tetap. Keduanya sama kuat, beda gaya konfigurasi dan cara menangani event data.

## SLIDE 5:
Pada ESP32, uart_driver_install dan uart_read_bytes adalah pintu utama. Pada STM32, HAL_UART_Receive dan HAL_UART_Transmit jadi andalan. Anggap API ini seperti bahasa resmi resepsionis agar data masuk, dicatat, lalu dikirim balik.

## SLIDE 6:
Perintah build-upload dasar: pio run -e esp32dev -t upload untuk ESP32, dan pio run -e bluepill_f103c8 -t upload untuk STM32. Buka monitor dengan pio device monitor -b 115200. Ini ritual wajib sebelum analisis hasil tiap percobaan.

## SLIDE 7:
Percobaan 01 Echo polling: MCU menunggu byte, lalu langsung memantulkan balik. Seperti petugas loket yang hanya melayani saat dipanggil, polling sederhana namun CPU sering menunggu. Uji dengan ketik Hello dan karakter cepat beruntun.

## SLIDE 8:
Di ESP32_01, uart_read_bytes timeout 20 ms lalu uart_write_bytes echo data. Di STM32_01, HAL_UART_Receive blocking dan LED PC13 toggle saat byte masuk. Ini memberi indikator visual bahwa jalur RX benar-benar aktif menerima karakter.

## SLIDE 9:
Percobaan 02 pindah ke interrupt/event. Ibarat bel pintu: CPU tidak perlu menunggu terus, cukup merespons saat ada sinyal. Hasilnya lebih responsif saat data padat, dan peluang kehilangan karakter berkurang dibanding polling murni.

## SLIDE 10:
ESP32_02 memakai queue event UART_DATA, FIFO_OVF, BUFFER_FULL, PARITY_ERR, FRAME_ERR. STM32_02 memakai HAL_UART_RxCpltCallback lalu re-arm HAL_UART_Receive_IT. Jika tidak re-arm, penerimaan berhenti setelah satu byte seperti mikrofon dimute.

## SLIDE 11:
Percobaan 03 ring buffer seperti tandon air antara ISR dan pemroses utama. ISR cukup menuang data cepat ke buffer, loop utama menyedot pelan. Dengan pola ini, lonjakan data tidak langsung membuat program panik atau kehilangan ritme.

## SLIDE 12:
ESP32_03 punya rb_push, rb_pop, statistik overflow periodik. STM32_03 mendorong byte dari callback ke buffer lalu main loop echo. Konsep head-tail-count wajib dipahami, karena ini fondasi parser, bridge, dan protokol paket di tahap lanjut.

## SLIDE 13:
Percobaan 04 printf redirect memudahkan debugging. STM32 mengalihkan _write ke UART seperti menyambung megafon ke serial monitor. ESP32 sudah nyaman dengan printf dan log. Output tabel sensor membuat data mudah dibaca, bukan teks acak.

## SLIDE 14:
Uji perintah cepat: pada ESP32_04 ketik ON atau OFF untuk LED GPIO2. Pada STM32_04 gunakan t untuk toggle LED dan r untuk report. Pendekatan ini melatih pola request-response sederhana sebelum masuk parser perintah penuh.

## SLIDE 15:
Percobaan 05 command parser mengubah UART jadi mini terminal. Perintah LED ON, LED OFF, BEEP, STATUS, HELP dipetakan ke handler. Ibarat resepsionis hotel, parser menerima kalimat, mengenali intent, lalu menjalankan layanan tepat.

## SLIDE 16:
ESP32_05 menormalkan input ke uppercase, trim spasi, lalu parsing token. STM32_05 memakai command table dan function pointer agar rapi. Keuntungan model tabel: tambah fitur cukup daftar entri baru, tidak perlu if-else panjang berantai.

## SLIDE 17:
Percobaan 06 JSON protocol membuat data terstruktur seperti formulir digital. Contoh kirim: {"cmd":"set","pin":2,"val":1} atau {"cmd":"get","pin":2}. Kelebihan JSON: mudah dibaca manusia dan mesin, cocok untuk gateway dan logging.

## SLIDE 18:
ESP32_06 mengumpulkan karakter sampai } lalu parse manual key cmd, pin, val tanpa library berat. STM32_06 juga parse manual dan dukung set, get, toggle pin 13. Teknik ini hemat memori, relevan untuk embedded dengan resource terbatas.

## SLIDE 19:
Percobaan 07 line editor membuat serial terasa seperti shell. Ada echo, backspace, dan history via panah. Analogi: bukan lagi walkie-talkie satu arah, tapi mesin ketik interaktif yang mengingat perintah terakhir untuk mempercepat kerja operator.

## SLIDE 20:
ESP32_07 simpan 5 history, dukung help, history, clear, info, serta ESC sequence untuk panah atas-bawah. STM32_07 serupa dengan callback interrupt. Uji: ketik beberapa command, tekan panah atas, lalu cek recall berjalan benar.

## SLIDE 21:
Percobaan 08 framing STX ETX adalah memberi pagar pada paket biner. STX menandai awal, ETX menandai akhir. Jika data memuat byte khusus, gunakan DLE byte stuffing. Ini seperti menaruh escape character agar isi tidak disangka delimiter.

## SLIDE 22:
ESP32_08 dan STM32_08 sama-sama encode-decode serta tampilkan hex dump. State machine receiver memisah mode idle, receiving, escape. Uji data yang mengandung 0x02, 0x03, 0x10 untuk memastikan stuffing dan unstuffing bekerja konsisten.

## SLIDE 23:
Percobaan 09 CRC-8 adalah sidik jari paket. Walau data lewat kabel benar, noise bisa mengubah bit. CRC memverifikasi integritas saat tiba. Jika sidik jari beda, paket ditolak. Ini lebih kuat dari checksum sederhana untuk deteksi error umum.

## SLIDE 24:
Di kode digunakan polynomial 0x07 dan init 0x00. Format paket praktik: [LENGTH][DATA][CRC]. ESP32_09 menampilkan pass-fail dan statistik, STM32_09 juga self-test. Uji korupsi byte untuk melihat CRC FAIL muncul sesuai ekspektasi.

## SLIDE 25:
Percobaan 10 timeout parser mendeteksi batas paket dari jeda, mirip mendengar kalimat lalu menunggu hening. Jika tidak ada byte dalam TIMEOUT_MS, paket dianggap selesai. Metode ini cocok saat protokol tidak punya delimiter atau panjang tetap.

## SLIDE 26:
ESP32_10 memakai esp_timer presisi dan restart timer setiap byte masuk. STM32_10 memakai HAL_GetTick untuk cek elapsed time. Uji dengan kirim cepat jadi satu paket, lalu beri jeda agar terbaca sebagai paket terpisah otomatis.

## SLIDE 27:
Percobaan 11 multi-UART bridge membuat MCU jadi penerjemah dua kanal. Data dari UART cepat diteruskan ke UART lain, dan sebaliknya. Ini penting saat PC memakai 115200 sedangkan perangkat lapangan hanya 9600 dengan format lebih sederhana.

## SLIDE 28:
ESP32_11 memakai UART0 115200 dan UART1 GPIO17 TX, GPIO16 RX pada 9600, dengan dua task forward dua arah. STM32_11 menjembatani USART1 PA9 PA10 dan USART2 PA2 PA3 via interrupt, sambil menghitung statistik byte per arah.

## SLIDE 29:
Percobaan 12 error statistics memonitor kesehatan link: parity, framing, overrun, noise, buffer full. Analogi panel kesehatan pasien, kita tidak hanya tahu data datang, tetapi tahu kualitas jalurnya sehat, waspada, atau kritis.

## SLIDE 30:
ESP32_12 aktifkan parity even dan event queue error. STM32_12 set UART 9-bit dengan parity even, override HAL_UART_ErrorCallback untuk hitung PE, FE, ORE, NE. Coba kirim dari terminal tanpa parity agar parity error sengaja terpicu.

## SLIDE 31:
Perbandingan inti: ESP32 unggul fleksibilitas pin dan event queue FreeRTOS, STM32 unggul alur HAL yang deterministik dan ringan. Pilih ESP32 untuk gateway multitugas, pilih STM32 untuk node kontrol real-time yang sederhana dan stabil.

## SLIDE 32:
Pola desain aman: ISR singkat, parsing di task utama, buffer jelas batasnya, dan selalu ada timeout. Jangan menaruh proses berat di callback. Prinsip ini seperti dapur restoran: pesanan dicatat cepat, memasak detail di stasiun terpisah.

## SLIDE 33:
Checklist praktikum: cek wiring TX-RX silang, satukan GND, samakan baud, upload firmware benar env, lalu monitor 115200. Jika tidak ada output, uji echo paling dasar dulu. Debug bertahap lebih efektif daripada langsung melompat ke CRC.

## SLIDE 34:
Urutan belajar ideal modul ini: Echo, Interrupt, Ring Buffer, Printf, Parser, JSON, Line Editor, Framing, CRC, Timeout, Bridge, Error Monitor. Urutan tersebut membangun pondasi seperti menyusun rumah dari pondasi ke atap, bukan kebalik.

## SLIDE 35:
Ringkasan teori sampai praktikum: komunikasi serial bukan cuma kirim byte, tetapi mengelola antrean, format paket, validasi, dan keandalan link. Saat semua teknik digabung, MCU mampu jadi sistem komunikasi yang tangguh di lapangan.

## SLIDE 36:
Project menggabungkan semua percobaan menjadi gateway monitoring lab kimia. STM32 bertindak sensor node, ESP32 jadi hub operator. Data sensor periodik dikirim, divalidasi, ditampilkan JSON, dan dipakai memicu alarm saat threshold terlampaui.

## SLIDE 37:
Format paket project: STX NODE_ID MSG_TYPE LENGTH DATA CRC8 ETX. Node kirim data atau alert, gateway kirim command set sampling, set threshold, status, reset. Mekanisme ini membuat komunikasi disiplin, mudah di-debug, dan bisa diskalakan.

## SLIDE 38:
Langkah demo project: hidupkan dua board, uji handshake PING-ACK, tampilkan data masuk, ubah sampling lewat command operator, lalu paksa kondisi suhu tinggi untuk memicu buzzer. Tunjukkan juga statistik error per node agar evaluasi link objektif.

## SLIDE 39:
Arsitektur modular wajib: frame_protocol, command_parser, ring_buffer, json_display, error_monitor dipisah file. Analogi tim kerja: tiap anggota punya tugas jelas. Kode jadi mudah diuji, mudah diperbaiki, dan aman saat menambah fitur baru.

## SLIDE 40:
Target penilaian project: link UART andal, parser berjalan, bridge aktif dua arah, alarm bekerja, dokumentasi jelas. Fokus bukan sekadar nyala, tetapi bukti terukur melalui log, statistik, dan demo skenario nyata sesuai rubrik modul.

## SLIDE 41:
Tugas video berdurasi 15 sampai 25 menit, format screen recording plus webcam dan demo hardware. Buka dengan identitas, lalu jelaskan gambaran modul UART. Pastikan suara jernih, teks monitor terbaca, dan wajah presenter terlihat konsisten.

## SLIDE 42:
Bagian inti video adalah demo 12 percobaan untuk ESP32 dan STM32: tujuan, potongan kode kunci, hasil serial monitor, serta analisa singkat. Gunakan alur tetap agar penonton tidak bingung, seperti episode berulang dengan format sama.

## SLIDE 43:
Untuk percobaan berbasis perintah, tampilkan input nyata: LED ON OFF, BEEP, JSON set get, history panah, framing hex, CRC pass fail, timeout paket. Penilaian tinggi datang dari bukti run yang jelas, bukan narasi tanpa demonstrasi layar.

## SLIDE 44:
Segmen project di video harus menunjukkan integrasi total: node kirim data, gateway parsing dan validasi CRC, operator kirim command balik, lalu alarm aktif saat anomali. Tutup dengan perbandingan STM32 vs ESP32 serta kendala dan solusi.

## SLIDE 45:
Sebelum submit, cek checklist: YouTube unlisted, durasi sesuai, webcam aktif, hardware tampak, semua percobaan dan project tampil, audio bersih. Hindari penalti utama: tanpa webcam, tanpa demo hardware, atau pengumpulan terlambat.
