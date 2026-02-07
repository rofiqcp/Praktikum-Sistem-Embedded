# Prompt untuk Pembuatan PPT - Bagian 1
## Modul 08: Direct Memory Access (DMA)

**Instruksi Umum:**
Buat slide presentasi yang clean, profesional, dan visual. Gunakan diagram blok di mana perlu.

### Slide 1: Judul
- **Judul:** Modul 08 - Direct Memory Access (DMA)
- **Subjudul:** Praktikum Sistem Embedded
- **Visual:** Ilustrasi "Fast Lane" atau jalur data cepat di samping jalur lambat.
- **Poin Utama:**
  - Konsep Transfer Data Hardware.
  - Offloading CPU.
  - High-Speed Data Acquisition.

### Slide 2: Apa itu DMA?
- **Judul:** Definisi & Konsep
- **Konten:**
  - Fitur hardware yang memindahkan data antar lokasi memori tanpa melibatkan CPU.
  - **Lokasi Transfer:** Periferal ↔ Memori, Memori ↔ Memori.
- **Analogi:**
  - **Tanpa DMA:** Chef (CPU) memotong sayur DAN mengantar makanan ke meja. (Lambat).
  - **Dengan DMA:** Chef (CPU) hanya masa, Pelayan (DMA) mengantar makanan. (Efisien).

### Slide 3: Mengapa Menggunakan DMA?
- **Judul:** Keuntungan Menggunakan DMA
- **Daftar Poin:**
  1. **CPU Efficiency:** CPU bisa sleep atau proses data lain saat transfer berlangsung.
  2. **Faster Transfer:** Kecepatan transfer mendekati batas bandwidth bus sistem.
  3. **Deterministic:** Waktu transfer lebih terprediksi dibanding software polling.
  4. **Low Power:** Mendukung mode hemat daya (Sleep Mode).

### Slide 4: Arsitektur Sistem (Bus Matrix)
- **Judul:** Bagaimana DMA Bekerja?
- **Visual:** Diagram Bus Matrix sederhana.
  - Tunjukkan "DMA Controller" terhubung ke Bus Matrix.
  - Tunjukkan "CPU", "SRAM", "Flash", dan "Peripherals" juga terhubung.
- **Penjelasan:**
  - DMA bertindak sebagai "Bus Master" (sama seperti CPU).
  - DMA request akses bus -> Arbiter memberikan akses -> Data ditransfer.

### Slide 5: Konfigurasi Dasar DMA
- **Judul:** 3 Parameter Utama Transaksi
- **Konten:**
  1. **Source Address:** Alamat asal data (Misal: Register Data ADC, &ADC1->DR).
  2. **Destination Address:** Alamat tujuan (Misal: Buffer Array RAM).
  3. **Data Size / Length:** Jumlah unit data yang akan dipindah (Misal: 100 sample).
- **Tambahan:**
  - **Data Width:** Byte (8-bit), Half-Word (16-bit), Word (32-bit).
  - **Increment Mode:** Apakah alamat bertambah otomatis setelah setiap transfer?
