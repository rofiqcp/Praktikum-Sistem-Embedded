# Modul 04: ADC (Analog-to-Digital Converter) - Slide NotebookLM

## Slide 1 - Pengenalan ADC - Jembatan Dunia Analog ke Digital
ADC mengubah sinyal analog dari lingkungan real (suhu, cahaya, potensiometer) menjadi data digital diskret (0 dan 1) diproses CPU. Dunia nyata smooth kontinu, komputasi digital biner. ADC jembatan vital realitas analog proses digital, mengubah besaran fisik menjadi bytes untuk embedded systems industri.

## Slide 2 - Tahapan Konversi: Sampling, Kuantisasi, Encoding
Konversi ADC tiga tahap: Sampling ambil nilai pada interval waktu tertentu. Kuantisasi petakan nilai kontinu ke level diskret resolusi bit. Encoding ubah level kuantisasi menjadi code biner murni. Ketiga tahap terjadi internal hardware ADC sangat cepat hanya microsecond saja untuk presisi tinggi.

## Slide 3 - Teorema Nyquist - Frekuensi Sampling Minimal
Teorema Nyquist: frekuensi sampling â‰¥ 2Ã— frekuensi tertinggi sinyal (fs â‰¥ 2Ã—fmax). Jika dilanggar terjadi aliasing pembacaan palsu salah. Contoh: suara manusia 20kHz harus di-sample minimal 40kHz hasil akurat. Penting saat baca sensor AC sinyal variabel cepat aplikasi

## Slide 4 - Resolusi dan Formula Konversi ADC
Resolusi 12-bit = 2^12 = 4096 level (0-4095). Rumus konversi: V_mV = (raw Ã— 3300) / 4095 untuk Vref 3.3V. Setiap bit ekstra menambah presisi akurasi lebih baik. Tapi CPU lebih sibuk panas. Trade-off design sesuai kebutuhan aplikasi presisi tinggi rendah performa optimal.

## Slide 5 - Kualitas Sinyal - SNR dan Quantization Error
SNR 12-bit ≈ 74 dB bagus. Error kuantisasi ≈ 0.4 mV. INL DNL ukur linearitas ideal. ADC berkualitas INL < ±1 LSB presisi industri. Pahami parameter penting aplikasi demanding akurasi tinggi stabil measurement sensor terpercaya konstan.

## Slide 6 - Fitur ADC pada ESP32 - Atenuasi dan Kalibrasi
ESP32 punya dua ADC 18 channel total analog. ADC1 (GPIO32-39) aman tidak konflik WiFi. ADC2 berbagi modul WiFi haram dipakai saat aktif transmit. Atenuasi 0/2.5/6/11 dB perluas input voltage dari 0-3.3V. Kalibrasi eFuse otomatis kompensasi non-linearitas chip.

## Slide 7 - Fitur ADC pada STM32 - Watchdog dan DMA
STM32 punya ADC Analog Watchdog hardware otomatis detect threshold. Channel: TEMPSENSOR suhu, VREFINT 1.2V. DMA transfer data RAM tanpa CPU intervensi. SAR stabil linear reliable industri.

## Slide 8 - Teknik Filtering - Moving Average dan EMA
Moving Average: hitung rata-rata N sampel terakhir buffer circular update kontinyu smooth. Mengurangi noise halus tapi lag kecil response. EMA bobot sampel baru lebih tinggi responsif smooth optimal. Pilih sesuai: statis pakai moving average cepat EMA dinamis tracking.

## Slide 9 - Teknik Filtering Lanjut - Median dan Oversampling
Median Filter: ambil nilai tengah N sampel sangat efektif hapus spike outlier noise acak. Oversampling: sample 4Ã— rata-rata naik 1 bit resolusi. 16Ã— sampel 12-bit jadi 14-bit akurat sempurna. Trade-off proses lama sinyal bersih noise rendah

## Slide 10 - Perbandingan ESP32 vs STM32 - Kekuatan Kelemahan
ESP32: 9-12 bit selectable API mudah kalibrasi auto eFuse fleksibel. STM32: 12 bit fixed solid watchdog powerful linear sempurna trusted. ESP32 fleksibel IoT WiFi connected, STM32 presisi stabil industri aplikasi critical embedded system measurement.

## Slide 11 - Persiapan Hardware - Komponen dan Rangkaian Dasar
Komponen wajib: potensiometer 10kÎ©Ã—2, LDR, resistor 10kÎ©Ã—3, LED merah hijau, multimeter. Breadboard kabel jumper ESP32 DevKit V1 STM32 Blue Pill ST-Link V2. Setup: pot wiper GPIO34 (ESP32) / PA0 (STM32), VCC 3.3V, GND. Jangan gunakan 5V

## Slide 12 - P01: ADC Single Read - Membaca Nilai Mentah ADC
Percobaan dasar ADC. API ESP32 adc_oneshot_read() atau STM32 HAL_ADC_Start(). Setup: atenuasi 11dB baca 0-3.3V. Compile upload open Serial Monitor 115200 baud. Putar potensiometer smooth 0-10kÎ© amati raw 0-4095. Catat tabel: posisi raw tegangan hitung manual konversi akurat.

## Slide 13 - P01 Lanjut - Implementasi ESP32 dan STM32 Dual
Rumus ESP32: V_mV = (raw Ã— 3300) Ã· 4095. Contoh raw 2048 â†’ 1650mV. ESP32 pakai adc_cali_raw_to_voltage() akurat. STM32 hitung manual atau library SPL valid. Multimeter digital referensi bandingkan error persen. Biasanya 1-5% okeh

## Slide 14 - P02: Voltage Display - Konversi Tegangan Serial
Lanjutan fokus accuracy display hasil. Tampil ADC millivolt (mV) Serial Monitor. Rumus V_mV = (raw Ã— 3300) Ã· 4095. Compile upload putar potensio tegangan naik turun terlihat jelas. Multimeter ukur verifikasi pin. Bandingkan pembacaan ADC multimeter hitung deviasi error.

## Slide 15 - P02 Lanjut - Verifikasi Akurat dengan Multimeter Digital
Ambil 5 posisi potensio: 0% 25% 50% 75% 100%. Catat: V_multimeter raw V_ADC hitung. Error(%) = |V_ADC - V_ref| / V_ref Ã— 100. Analisa posisi error terbesar. Ujung range 0V 3.3V non-linear error besar. STM32 presisi

## Slide 16 - P03: Moving Average - Filter Peredam Noise Sinyal
Filter digital moving average smooth pembacaan noisy. Buffer circular N=10 sampel update kontinyu. Raw fluktuasi 100-110, filtered stabil 105 terlihat. SNR naik sqrt(N), 10Ã— â†’ 3.16Ã— improvement SNR pengurangan noise. Observe raw vs filtered perbedaan nyata stabilitas.

## Slide 17 - P03 Lanjut - Perhitungan Standar Deviasi Filter Detail
Catat 20 sampel raw filtered posisi 50%. Hitung: minimum maksimum mean standar deviasi Ïƒ. Formula: Ïƒ = sqrt(Î£(x-mean)Â²/N). Bandingkan Ïƒ raw besar Ïƒ filtered kecil noise berkurang. Test window N=5 fast N=20 smooth lag. Trade-off responsif.

## Slide 18 - P04: Multi Channel - Pembacaan Dua Sensor Simultan
Baca dua channel ADC simultan sumber berbeda. Pot1 GPIO34/PA0, Pot2 GPIO35/PA1 VCC 3.3V. Tabel 4 kolom: CH1_raw CH1_mV CH2_raw CH2_mV update kontinyu. Putar pot1: CH2 tetap stabil. Tidak crosstalk bagus hardware isolated well akurat.

## Slide 19 - P04 Lanjut - Deteksi Crosstalk Antar Channel ADC
Test crosstalk: pot1 minimum, pot2 maksimum catat. Tukar: pot1 maksimum, pot2 minimum. CH1 sama sempurna tidak crosstalk, beda ada coupling buruk design. STM32 Scan simultaneous lebih bagus, ESP32 polling sequential lag sedikit acceptable.

## Slide 20 - P05: ADC Calibration - Kalibrasi eFuse Akurasi Meningkat
Kalibrasi ADC tingkatkan akurasi kompensasi non-linearitas pabrik. ESP32: adc_cali_create_scheme_curve_fitting() load eFuse otomatis. STM32: HAL_ADCEx_Calibration_Start() sequence internal. Amati: raw â†’ V_tanpa â†’ V_dengan. Multimeter: error tanpa â‰ˆ5%, dengan â‰ˆ1%.

## Slide 21 - P05 Lanjut - Perbandingan Sebelum Sesudah Kalibrasi Akurat
Ambil 5 posisi: 0% 25% 50% 75% 100%. Catat: V_ref V_tanpa V_dengan. Error sebelum besar ujung, sesudah minimal rata. Jenis: eFuse Two Point presisi, Vref, Default buruk. STM32 factory kalibrasi bagus pabrik.

## Slide 22 - P06: Continuous DMA - Sampling Berkecepatan Tinggi Cepat
DMA transfer data RAM otomatis CPU tidak polling overhead. Buffer 256 sampel penuh callback batch. esp_adc_continuous_new_handle() config sample_freq_hz 20000. Compile upload data transmisi jauh cepat P01 single-shot polling lambat sekali. DMA ideal akuisisi.

## Slide 23 - P06 Lanjut - Mode DMA Buffer Handler Performa Optimal
P01 ~100 sps, P06 ~10000 sps 100Ã— lebih cepat luar biasa! Buffer kecil overflow, besar delay lama. Optimal 256-512 sampel batch processing. Callback setiap penuh process batch data. DMA capture transisi smooth single-shot terlewat banyak. Time-critical akuisisi.

## Slide 24 - P07: Threshold Alert - LED Nyalakan Melampaui Batas Ambang
Sistem peringatan ambang ADC (threshold). LOW=1000 (~0.8V), HIGH=3000 (~2.4V). Status: NORMAL off, LOW biru, HIGH merah alert. Putar pot min-max LED nyalakan balik normal otomatis. STM32 Watchdog hardware otomatis efisien reliable polling.

## Slide 25 - P07 Lanjut - Analog Watchdog STM32 Otomatis Hardware Monitor
Watchdog hardware monitor range otomatis unik. Setup: WatchdogMode SINGLE_REG set High/Low, Channel, ITMode. Keluar range trigger callback otomatis instant cepat. Keuntungan: zero CPU overhead pure hardware monitoring response. Safety-critical guaranteed akurat.

## Slide 26 - P08: Battery Monitor - Monitor Tegangan Baterai Cadangan UPS
Voltage divider Vbat 4.2V â†’ 2.1V aman GPIO 3.3V. Vbat---[10k]---+---[10k]---GND, ADC tengah. Vbat = 2 Ã— Vadc pembagi. Baca detik log estimasi kapasitas Li-Ion curve discharge. Compile monitor turun sesuai discharge persen akurat kalibrasi.

## Slide 27 - P08 Lanjut - Voltage Divider Estimasi Kapasitas Baterai
Mapping Li-Ion: 4.2V=100% 4.0V=80% 3.7V=50% 3.4V=20% 3.0V=0%. Interpolasi linier. Charge naik, discharge turun amati. Bandingkan BMS bawaan mirip harusnya sama. Setiap 1 menit sample cukup optimal. Log SD historical battery health degradation.

## Slide 28 - P09: Temperature Internal - Sensor Suhu Chip Terintegrasi Gratis
ESP32 STM32 sensor suhu internal chip built-in gratis. ESP32 API atau rumus manual. STM32 channel TEMPSENSOR formula T = (V - V25) / Slope + 25. Compile baca kontinyu normal 25-30Â°C. Bekerja CPU naik terlihat. Detect overheat sebelum thermal trigger.

## Slide 29 - P09 Lanjut - TEMPSENSOR STM32 Presisi Akurat Tinggi
TEMPSENSOR STM32 lebih akurat built-in. Channel ADC TEMPSENSOR Vref 1.2V internal stabil. Sample 239.5 cycle max presisi tinggi. Rumus: T = (Vs - 0.76) / (-2.5mV) + 25 datasheet. Catat 60 sampel 1 menit observe stabil. Heat test observe naik.

## Slide 30 - P10: Sampling Rate - Kecepatan Sampling Kuplik Maksimal Sistem
Ukur samples/second (SPS) sistem achieve. Loop baca ADC timestamp N=1000 hitung SPS = 1000/elapsed. ESP32 single ~100, DMA ~10000. STM32 single ~10000, DMA ~100000 potensial. Terbatas: ADC time, serial transmit UART, loop delay. Display log.

## Slide 31 - P10 Lanjut - Perhitungan SPS Presisi Akurat Measurement
Timing precision micros() SysTick STM32. Code: start=micros(), N=1000 baca, SPS=1000000Ã—N/(end-start). Catat rata variasi. Bandingkan measured vs theoretical. Pembatas: ADC, RAM bandwidth, UART 115200 bottleneck baud rate serial. Optimize DMA buffer.

## Slide 32 - P11: Light Sensor LDR - Deteksi Intensitas Cahaya Ambient
LDR resistansi turun terang naik gelap non-linear response. Divider: 3.3V---[10k]---+---[LDR]---GND. Terang LDR R kecil tegangan ADC tinggi. Gelap R besar tegangan ADC rendah. Compile buka tutup cahaya responsif immediate real-time. Otomatis lamp on threshold.

## Slide 33 - P11 Lanjut - Voltage Divider Resistor Tetap Design Optimal
Design optimal R1 = R_LDR_putih geometrik. Terang 1k gelap 100k pilih R1=10k tengah. Vadc = Vcc Ã— R_LDR / (R1+R_LDR). Test 5 gelap semi normal window terang matahari. Catat ADC buat curve mapping brightness% akurat.

## Slide 34 - P12: Statistical Analysis - Min Max Rata Stdev Nilai Data
Analisis statistik kualitas data sinyal analog. Kumpul N=100 sampel array normal. Hitung: minimum maksimun mean standar deviasi Ïƒ. Ïƒ = sqrt(Î£(x-mean)Â²/N). Bandingkan raw noisy Ïƒ besar filtered smooth Ïƒ. SNR = mean/Ïƒ besar bagus signal quality.

## Slide 35 - P12 Lanjut - SNR Analisis Kualitas Sinyal Lengkap Detail
SNR = 20Ã—log10(mean/Ïƒ) dB. Ambient noise Ïƒ besar SNR kecil quality rendah environment. Filter DMA cepat kurangi Ïƒ naik SNR improvement. 100 sampel loop hitung statistik monitor. Log CSV Excel chart histogram analisis distribusi Gaussian.

## Slide 36 - Project: Sistem Monitoring Udara Gedung Perkantoran Hijau Indonesia
GreenAir Indonesia kontrak monitoring gedung 3 lantai. Setiap lantai ESP32 hub baca 4: suhu cahaya gas baterai. Filter moving average kalibrasi eFuse statistik real-time kontinyu 24/7. STM32 alert monitor redundant threshold suhu>35 gas>70 LED buzzer.

## Slide 37 - Project: Dual MCU Architecture Hub dan Alert Controller Sistem
ESP32 Hub akuisisi 4 channel ADC parallel setup filter moving average kalibrasi eFuse konversi. 1 detik normal continuous DMA anomali. TX serial data STM32. STM32 Alert terima parse threshold logic LED buzzer logging timestamp event anomali.

## Slide 38 - Project: Mode Normal vs Mode Anomali Otomatis Switching Smart
Mode Normal: sampling 1 detik tabel 4ch raw mV filtered mean stddev periodik. Mode Anomali: gas>70% trigger switch continuous DMA 10000sps ESP32. STM32 LED merah buzzer nonstop tone lain. Otomatis balik normal 30 detik stabil gas turun kembali.

## Slide 39 - Project: Rangkaian Hardware Pin Mapping Lengkap Detail Akurat
E: GPIO34 suhu CH6, GPIO35 cahaya CH7, GPIO32 gas CH4, GPIO33 baterai CH5, GPIO2 LED, GPIO4 buzzer. S: PA0 suhu, PA1 gas, PA4 baterai, PB0 LED, PB1 buzzer, PC13, PA9 TX, PA10 RX. Cross UART E TXâ†’S RX. GND reference modular rapi.

## Slide 40 - Project: Rencana Eksekusi Integrasi Lengkap 2 Minggu Timeline
Week1: P01-P07 dasar dual-platform verify. P04 multi-channel crosstalk test. P05-P06 kalibrasi DMA verify improvement. P07 threshold LED nyala. Week2: dual-MCU UART komunikasi test mode switching normal anomali hardware assembly final kode optimization tuning.

## Slide 41 - Tugas Video: Struktur dan Durasi 15-25 Menit Lengkap Sempurna
Laporan video demonstrasi praktikum project modul 04 YouTube Unlisted 15-25 menit. Wajib: webcam PiP terlihat bicara code selalu visible. Wajib: hardware fisik meja live demo pot LDR LED aktual running bekerja real. Screen vs code kode jelas serial visible.

## Slide 42 - Tugas Video: Demonstrasi Teori dan Materi ADC Lengkap Sempurna
2-3 menit teori: sampling kuantisasi encoding Nyquist resolusi 12-bit V=rawÃ—3300Ã·4095 formula. Diagram visual sinyal sampling quantization jelas. ADC ESP32 ADC1 ADC2 atenuasi kalibrasi WiFi issue. STM32 channel internal suhu watchdog DMA filtering ringkas.

## Slide 43 - Tugas Video: Demonstrasi 13 Experiment Praktikum Lengkap Semua
8-14 menit demo P01-P12 satu-satu berurutan live compile upload serial capture. P01 raw linear. P02 tegangan verify. P03 filter noise reduction. P04 dual crosstalk independent. P05 kalibrasi akurasi. P06 DMA cepat. P07 threshold LED. P08-P12 sensor stats demo semua.

## Slide 44 - Tugas Video: Demo Project dan Hardware Fisik Rangkaian Aktual Real
3-5 menit project: mode normal ESP32 4 channel 1 detik tabel real-time hardware camera pot1 suhu pot2 gas LDR cahaya LED breadboard. Gas ekstrem >70% anomali trigger LED merah buzzer bunyi STM32 aktif. E switch DMA cepat. Serial penuh. Screenshot tabel hasil.

## Slide 45 - Tugas Video: Checklist Penilaian Bobot Penalti Jelas Terukur
Penilaian: materi 15% eksperimen 35% project 20% hardware 15% video 15% total 100. Penalti: webcam -20% hardware -20% durasi<10 -10% durasi>30 -5% terlambat -10%/hari. Checklist: âœ“YouTube Unlisted âœ“15-25 âœ“webcam audio âœ“code screen âœ“serial âœ“12exp âœ“project âœ“hardware âœ“link tepat waktu.


