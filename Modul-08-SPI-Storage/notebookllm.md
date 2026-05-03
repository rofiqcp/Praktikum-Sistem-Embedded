# Modul 07: SPI Bus dan Storage — NotebookLM (45 Slide Compact)

---

## GRUP 1: TEORI FUNDAMENTAL (Slide 1-15)

### SLIDE 1: SPI Protocol Basics
4-kabel: MOSI (M→S), MISO (S→M), SCLK (clock), CS (LOW=active). Full-duplex sync, 80+ MHz. Master kontrol clock+CS. Topologi: 1 Master, N Slave via CS. Storage/display/sensor cepat. Analogi: komandan + 3 radio + N lampu sinyal prajurit.

### SLIDE 2: CPOL dan CPHA Modes
4 mode skema timing. **Mode 0** (CPOL=0, CPHA=0): clock LOW idle, sample rising edge. **Mode 1**: LOW, falling. **Mode 2**: HIGH, falling. **Mode 3**: HIGH, rising. **Mode 0 paling umum** (SD, flash, OLED). Must match Master↔Slave. Mismatch=corrupt. Lihat datasheet!

### SLIDE 3: SPI vs I2C vs UART
**SPI**: 80MHz speed, 4-pin, full-duplex, multi-device CS, cocok storage. **I2C**: 3MHz, 2-pin, half-duplex, sensor kecil. **UART**: 115k baud, 2-pin, debug PC. **Pilih**: SPI perf-critical, I2C cheap multi-sensor, UART console. Praktikum: **SPI storage utama**.

### SLIDE 4: Master-Slave Topologi
Master generate SCLK, kontrol CS. Slaves listen saat CS=LOW sendiri. 1 Master hanya (not multi-master). Per-slave CS terpisah **elegant tangkas**. Multi-drop collision-free. Weakness: CS pin per tambah device.

### SLIDE 5: Hardware Grounding Best Practices
High-freq noise-sensitive. **(1) Star-point ground**, **(2) Bypass 100nF VCC**, **(3) MISO 4.7k pull-up**, **(4) CS debounce**, **(5) Kabel <1m**, **(6) Scope**. PCB teliti!

### SLIDE 6: ESP32 SPI 4 Controller
SPI0/1 flash. **VSPI (SPI3)**: 18=SCLK, 19=MISO, 23=MOSI, 5=CS. **HSPI**: 14, 12, 13, 15. GPIO flexible remap. Speed 80MHz, praktik 10-20MHz. API: `spi_bus_initialize()` + `spi_bus_add_device()`.

### SLIDE 7: STM32F103 SPI 2 Controller Fixed
**SPI1** (APB2 72MHz): PA5=SCLK, 7=MOSI, 6=MISO. **SPI2** (APB1 36MHz): PB13, 15, 14. Pin fixed tidak fleksibel ESP32. Prescaler /2-/256 adjust speed. Keuntungan: DMA, Analog Watchdog, reliable industrial.

### SLIDE 8: Clock Prescaler Formula
**STM32**: `SPI_Clock = APB_Clock / Prescaler`. APB2=72M, APB1=36M. Contoh: 72M÷8=9MHz. Manual calculate. **ESP32**: specify `clock_speed_hz`, driver auto. STM32 precision, ESP32 lazy-programmer.

### SLIDE 9: DMA FIFO Features
**STM32**: DMA1/2 auto buffer→memory (high-throughput wajib). **ESP32**: GDMA modern + burst. **Interrupt**: TxCplt/RxCplt async. 8B polling OK; 4KB **DMA mandatory** prevent lag.

### SLIDE 10: Platform Comparison
**ESP32**: pin flexibility, 40+ MHz, WiFi/BLE, Arduino IDE. **STM32**: ultra low-power, robust, reliability, cost $2-3. **Praktikum**: **ESP32 hub** (fast) + **STM32 backup** (robust) redundansi optimal.

### SLIDE 11: W25Q32 Flash Memory
**W25Q32** 4MB NOR. 4096 sector, 256 block. Program 256B, 1-2ms. Erase 100-400ms. Read instant, write slow. Persistent power-off. Cmd: 0x9F JEDEC, 0x06 WrEn, 0x05 Status, 0x20 Erase, 0x02 Prog, 0x03 Read.

### SLIDE 12: SD Card SPI Mode
**SD** compatible SPI (no 4-bit). Init: 400kHz→CMD0→CMD8→ACMD41→speed up. **ESP32**: VFS FatFS auto. STM32: bare metal. Advantage: PC, 32GB. Disadvantage: complex init.

### SLIDE 13: OLED SSD1306 128×64
**Monochrome** 5-pin SPI + DC/RST. Init 20 cmd. Frame 1024B. Update <10ms realtime.

### SLIDE 14: MCP3208 ADC 8-Channel
**12-bit 8-channel** range 0-4095 @ 3.3V VREF. Extend single internal. SPI 3-byte frame. Speed 100k-3.6MHz, ~200k sample/sec. Accuracy ±1/2 LSB. Sensor analog semua (potensio, LDR, thermistor, light, pressure).

### SLIDE 15: MCP4921 DAC 12-Bit
**12-bit 1-output** 0-VREF range. SPI 2-byte. Generate audio waveform test, proportional alarm LED brightness. Update 100kHz practical. Waveform via LUT sine atau realtime calculate. Analogi voltage knob digital.

---

## GRUP 2: PRAKTIKUM 13 EXPERIMENTS (Slide 16-30)

### SLIDE 16: P01 Loopback Self-Test
Verify bus sebelum slave. MOSI→MISO 1 jumper. 5 test: counter 0xFF, alternating 0xAAAA, all 0xFF/0x00, random, null. Expect TX=RX. Fail? Config/hardware STOP! Serial hex dump TX vs RX, "PASS".

### SLIDE 17: P02 OLED Display Text
Tampilkan teks+counter OLED. Init 1-2MHz→cmd (DC=LOW)→20+ init→clear 1024B→draw→push frame. Loop: count++, send, 1sec delay. Font 5×7 = 20 char/line. Blank? RST timing, DC pin, sequence error.

### SLIDE 18: P03 Flash Erase-Program
Eksternal 4MB. Cmd: 0x06 Enable, 0x05 Status, 0x20 Erase, 0x02 Program, 0x03 Read, 0x9F JEDEC. JEDEC expect 0xEF-40-16. Erase 400ms, program 5ms, poll, read, verify. First persistent external data!

### SLIDE 19: P04 SD Card FAT
Init 400kHz→CMD0→CMD8→ACMD41→25MHz. **ESP32**: `esp_vfs_fat_sdspi_mount()` auto. Create "log.csv"→append 100→read. Physical: eject SD, PC mount CSV readable! Survive reset!

### SLIDE 20: P05 MCP3208 8-Channel
Read semua channel tabel. Calc: (raw/4095)×3.3V. Display 8-ch realtime + min/max/avg. Potensio CH0 naik-turun verify. Timing: 8ch @ 500kHz ≤ 50ms/read. VREF 3.3V correct!

### SLIDE 21: P06 DAC Waveform 4-Type
Generate sine/sawtooth/triangle/square per 5 detik rotate. SPI 2-byte cmd+12-bit=16bit. LUT sine atau formula. 256 sample/sec = 256Hz. Value: dac=2048+(amp×sample[i]), clamp 0-4095. Scope observe clean edge challenge?

### SLIDE 22: P07 Multi-Slave 2-3 Device
W25Q33 CS1, MCP3208 CS2, OLED CS3. Pattern: cmd slave1 resp, cmd slave2 resp. Measure CS timing. Test: flash+ADC concurrent verify scaling.

### SLIDE 23: P08 NVS Key-Value
Config persistent boot counter. **ESP32 NVS** simple. STM32 flash manual. Read (default 0)→increment→write→reset. Expected: boot #1→#2→#3 persist! Advanced: struct, string, blob.

### SLIDE 24: P09 SPIFFS CSV Logger
Logger persistent internal 7-day. **ESP32**: mount→`fopen/write` normal. Create→append 100/sec→read→check size. Advanced: rotate >64KB. Warning: high-freq write = wear! Buffer+flush 16KB/10sec.

### SLIDE 25: P10 Speed Benchmark
Measure throughput clock+buffer. Loopback. Clock: 1/5/10/20/40MHz. Buffer: 16/256/4096B. Table: clock vs throughput vs efficiency. 16B overhead big→low, 4096B amortize→high. Insight: **MHz ≠ Megabyte/sec**!

### SLIDE 26: P11 Interrupt Non-Blocking
Compare blocking vs interrupt CPU work. Blocking 100% idle. Interrupt background. Test 1024B: (0 work) vs (N loop background). Insight: **parallel**—CPU update LED saat SPI background. Tradeoff overhead small transfer.

### SLIDE 27: P12 Triple-Tier Logger
Full integration: sensor→buffer→flush→safe. **ESP32 3-task**: read→queue→file, queue→SPIFFS, Display+alarm. STM32 timer→buffer→flash manual. Metrics: entry count, storage %, flush time, latency. CSV. Run 300sec: 50-100KB archive.

### SLIDE 28: P13 ESP32 ADC Attenuation
ADC2 WiFi conflict + 4 attenuation level: 0dB (<1.1V), 2.5dB, 6dB, 11dB (<3.3V). Setup: potensio ADC1 (safe), ADC2 test (conflict). WiFi OFF: normal. WiFi ON: UNAVAILABLE! Recommendation: critical→ADC1 only!

### SLIDE 29: P13b STM32 Analog Watchdog
ADC threshold interrupt out-of-range. **Zero-overhead 100% hardware** level. Channel, high (2.5V), low (0.5V). When >high or <low→ISR auto. Potensio cross→"ALERT!" Callback. Use case: battery low, temp critical. Zero polling!

### SLIDE 30: Rekapitulasi 13 Experiments
(1) Loopback verify, (2) Display OLED, (3) Storage W25Q32, (4) Sensor ADC, (5) Multi-slave, (6) DAC waveform, (7) NVS config, (8) SPIFFS logger, (9) Benchmark, (10) Interrupt, (11) Triple-redundancy, (12) Watchdog/attenuation. **Outcome**: sensor→log→display→alarm! Protocol, storage, reliability, performance mastered.

---

## GRUP 3: ADVANCED + PROJECT + VIDEO (Slide 31-45)

### SLIDE 31: Troubleshooting Common
**A: MISO 0xFF** → CS?, MISO wire?, mode?, clock?. **B: Corrupt** → jitter?, tight wire?. **C: Slow** → CS <1µs?, settling?. **Solution**: downgrade 1-2MHz diagnose. **Tools**: scope, analyzer.

### SLIDE 32: Best Practices Hardware
**(1) Star-point**, **(2) Bypass 100nF**, **(3) MISO pull-up**, **(4) CS debounce**, **(5) Timing ≥1µs**, **(6) Datasheet strict**, **(7) Scope check**.

### SLIDE 33: Testing Pyramid
**Foundation**: P01 loopback. **Layer 2**: device isolated. **Layer 3**: persistence write→read. **Layer 4**: performance. **Layer 5**: integration. **Tools**: scope, hex, analyzer. **TDD**: test first.

### SLIDE 34: Code Architecture 3-Layer
**(A) HAL**: `init()`, `write()`, `read()`, `speed()` generic. **(B) Driver**: per-device, isolated test. **(C) App**: orchestrate. **Folders**: `hal/`, `drivers/`, `main.c`.

### SLIDE 35: Programming Gotchas
**(1) Endian**: little-endian both, swap if mix. **(2) Timing**: device 100µs-ms respon, polling+timeout not blind. **(3) Buffer**: ring/queue water-mark. **(4) Flash wear**: 100k erase cycle, buffer 1-day flush = 1000-day life. **(5) Floating**: tie pullup/down**.

### SLIDE 36: Project Vulkanologi Overview
**Semeru monitor** ≥4 sensor (T,vib,humid,gas), 3-redundansi (NVS, SPIFFS 7d, SD), OLED+buzz, **dual-MCU** (ESP32+STM32), UART sync. Flow: ADC→avg→log (1Hz normal, 10Hz anomaly >70%)→3-tier→display→buzz. **Redundancy**: independent read, W25Q32 backup, UART sync.

### SLIDE 37: Hardware Dual-MCU SPI Bus
**Single bus** shared MOSI/MISO/SCLK, multiple CS. **ESP32**: MCP3208 CS1, DAC CS2, OLED CS3. **STM32**: MCP3208 CS4, W25Q32 CS5. **UART**: 10sec sync. **Pin**: GPIO18/23/19 shared, 5/17/21=CS, 16/17=UART. **Decouple**: 100nF.

### SLIDE 38: Software FreeRTOS+Interrupt
**ESP32 3-task**: Reader 100ms→queue, Logger→write, Display 500ms. **UART**: 10sec sync. **STM32**: timer→callback→flash. **Sync**: handshake, 5sec timeout independent.

### SLIDE 39: Testing Checklist Pre-Demo
**HW**: SCLK stable, CS timing, bypass cap. **FW**: P01 PASS, OLED, W25Q 0xEF, SD CSV, 8ch, DAC, NVS 5× reset, SPIFFS, bench >80KB/s, no crash 10min. **Demo**: pot→OLED <1s, eject SD, reset→resume.

### SLIDE 40: Rubrik Penilaian Bobot
| HW SPI | 15% | Clean |
| Sensor | 20% | 4ch ±5% |
| Storage | 25% | 3-tier |
| DAC | 10% | Buzz |
| UART | 10% | Sync |
| Logger | 10% | 30min |
| Code | 10% | Modular |
**Deliver**: GitHub, schematic, README, PDF, YouTube 15-25min.

### SLIDE 41: Tugas Video Format Requirements
**Format**: screen + webcam PiP + hardware cam. **Duration**: 15-25m. **Resolution**: ≥720p audio clear. **Sections**: Intro 1m, Teori 2-3m, Demo 8-14m, Project 3-5m, Closing 1m. **Style**: casual, medium pace, natural.

### SLIDE 42: Teori Segment 2-3 Minute
**S1**: 4-wire diagram, CPOL/CPHA. **S2**: SPI vs I2C MHz. **S3**: NVS→SPIFFS→SD. **S4**: ESP32+STM32 bus. **Narasi**: "SPI komandan + 3 radio + 3 lampu sinyal 3 prajurit..." Konsep!

### SLIDE 43: Demo Percobaan Each ≤45 Sec
**P01**: hex PASS. **P02**: counter rapid. **P03**: JEDEC OK. **P04**: CSV PC. **P05**: 8ch realtime. **P06**: waveform scope. **P07**: CS timing. **P08**: boot persist. **P09**: size %. **P10**: throughput. **P11**: blocking vs background. **P12**: 5min stable. **P13**: ADC conflict.

### SLIDE 44: Project Demo Finale 4-Minute
**Setup 1m**: board, schematic, pins. **Startup 1m**: Init, loopback, MONITORING. **Realtime 1.5m**: pot→OLED <1s, 8ch, storage %, gas>70%→buzz, alert. **Redundancy 1m**: SD→SPIFFS, reset→integrity, 10Hz stable.

### SLIDE 45: Penutup Lessons Learned
**Skills**: SPI, storage, redundancy, dual-MCU! **Key**: Test P01 first, datasheet gospel, redundancy critical, buffer prevent loss. **Future**: RTC, WiFi, encrypt. **Thanks**—Modul 07! 🚀
