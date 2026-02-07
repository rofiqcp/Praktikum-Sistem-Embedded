# Rubrik Penilaian Project
## Modul 08: High-Speed Signal Acquisition System (DMA)

| Aspek | Bobot | Kriteria Penilaian | Skor (0-100) |
|-------|-------|--------------------|--------------|
| **Fungsionalitas Sistem (40%)** | | | |
| Konfigurasi DMA | 15% | - Menggunakan Circular Mode.<br>- Data width dan direction benar.<br>- Channel mapping sesuai. | |
| Data Acquisition | 15% | - ADC sampling berjalan otomatis.<br>- Dua channel terbaca simultan.<br>- Nilai sensor akurat/responsif. | |
| Serial Streaming | 10% | - Data terkirim ke UART.<br>- Format data kompatibel dengan Serial Plotter. | |
| **Logic & Algoritma (30%)** | | | |
| Double Buffering | 20% | - Implementasi Ping-Pong buffer benar.<br>- Memanfaatkan `HalfCpltCallback` & `CpltCallback`.<br>- Tidak ada race condition. | |
| Efisiensi CPU | 10% | - Tidak ada blocking code (`HAL_Delay` atau loop tunggu) dalam callback.<br>- Pemrosesan minimal. | |
| **Laporan & Demo (30%)** | | | |
| Demonstrasi | 15% | - Waveform di Serial Plotter mulus (sinusoid/potensio clean).<br>- Tidak ada glitch/putus (buffer underrun). | |
| Analisis | 15% | - Penjelasan alur data DMA vs Interrupt.<br>- Estimasi beban CPU (teoritis vs aktual). | |

## Total Skor: _____ / 100

**Catatan Evaluator:**
- **Bonus Point (+5):** Jika mahasiswa bisa mengimplementasikan UART Transmit juga menggunakan DMA.
- **Pengurangan Point (-10):** Jika masih menggunakan `HAL_ADC_PollForConversion()` atau `HAL_ADC_GetValue()` manual di dalam loop.
