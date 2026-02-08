# Rubrik Penilaian Project — Modul 13: Network & IoT

## Komponen Penilaian

### 1. Fungsionalitas (40%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) |
|----------|--------------|----------|----------|----------|
| **Konektivitas** | WiFi/BLE/Ethernet stabil, auto-reconnect | Koneksi stabil, tanpa auto-reconnect | Koneksi sering putus | Gagal connect |
| **Protokol** | TCP/UDP/HTTP/MQTT/BLE berfungsi sempurna | Sebagian besar protokol berfungsi | Hanya 1-2 protokol berfungsi | Protokol tidak berfungsi |
| **Data Flow** | Sensor → network → cloud → dashboard lengkap | Sensor → network → partial cloud | Hanya komunikasi lokal | Tidak ada data flow |
| **Error Handling** | Timeout, retry, fallback sempurna | Timeout dan retry ada | Minimal error handling | Tidak ada error handling |
| **Fitur Tambahan** | Ada fitur kreatif di luar requirement | Semua requirement terpenuhi | Sebagian requirement | Banyak yang kurang |

### 2. Kode Program (25%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) |
|----------|--------------|----------|----------|----------|
| **Struktur** | Modular, fungsi terpisah, clean code | Cukup terstruktur | Kurang terstruktur | Semua di satu fungsi |
| **Framework** | ESP-IDF/HAL native, API benar | Framework benar, sedikit issue | Campuran framework | Arduino / salah framework |
| **Dokumentasi** | Komentar lengkap, header jelas | Komentar cukup | Komentar minim | Tanpa komentar |
| **Kompilasi** | Zero warning, zero error | Compile sukses, ada warning | Compile dengan modifikasi | Tidak bisa compile |

### 3. Dokumentasi & Laporan (20%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) |
|----------|--------------|----------|----------|----------|
| **Diagram** | Blok diagram, wiring, flowchart lengkap | Ada diagram utama | Diagram minimal | Tanpa diagram |
| **Screenshot** | Serial output, dashboard, tools capture | Screenshot utama ada | Sedikit screenshot | Tanpa screenshot |
| **Analisis** | Analisis mendalam, perbandingan, optimasi | Analisis cukup | Deskripsi tanpa analisis | Tanpa analisis |
| **Format** | Rapi, terstruktur, profesional | Cukup rapi | Kurang terstruktur | Berantakan |

### 4. Presentasi & Demo (15%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) |
|----------|--------------|----------|----------|----------|
| **Demo Live** | Semua fitur demo berjalan | Sebagian besar berjalan | Hanya sebagian | Demo gagal |
| **Penjelasan** | Jelas, teknis, menjawab pertanyaan | Cukup jelas | Kurang jelas | Tidak bisa menjelaskan |
| **Python Scripts** | Debug scripts berjalan, visualisasi bagus | Scripts berjalan | Partial scripts | Tidak ada scripts |

## Skala Nilai

| Rentang Skor | Nilai | Keterangan |
|-------------|-------|------------|
| 90-100 | A | Excellent — semua kriteria terpenuhi |
| 80-89 | B | Good — sebagian besar terpenuhi |
| 70-79 | C | Fair — memenuhi minimum requirement |
| 60-69 | D | Poor — banyak kekurangan |
| < 60 | E | Fail — tidak memenuhi standar |

## Formula Perhitungan
```
Nilai = (Fungsionalitas × 0.4) + (Kode × 0.25) + (Dokumentasi × 0.2) + (Presentasi × 0.15)
Nilai Final = (Total Skor / 16) × 100
```
