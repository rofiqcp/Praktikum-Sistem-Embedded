# Modul 05: DAC & PWM 

**Slide 1**: DAC digital-to-analog output tegangan. PWM pulse width modulation duty cycle. DAC presisi mahal, PWM ekonomis perlu filter. Modul DAC PWM teknik output analog ESP32 STM32 embedded.

**Slide 2**: Vout=Vref×(Digital_Value/2^n). 8-bit Vref3.3V input128→1.64V. Resolusi 3.3V/255=12.94mV. Sampling kuantisasi encoding presisi tinggi sempurna level tegangan kontinyu.

**Slide 3**: R-2R Ladder superposisi resistor bobot. Weighted Resistor linear. Delta-Sigma oversampling resolusi tinggi audio. Resolusi vs bandwidth trade-off implementasi pilih aplikasi kebutuhan.

**Slide 4**: INL DNL settling time glitch energy SFDR spesifikasi DAC penting audio motor sensor presisi diinginkan.

**Slide 5**: STM32F103 tanpa DAC. STM32F411 DAC 12-bit PA4 PA5 DMA trigger. Settling ~10µs sempurna presisi analog production.

**Slide 6**: DAC_CR enable, DAC_DHR12R1 input, DAC_DOR1 output register. HAL_DAC_Init()ConfigChannel()Start() mudah digunakan.

**Slide 7**: PWM duty=(Ton/(Ton+Toff))×100%. Vavg=Vmax×Duty. 5V duty50%=2.5V. Freq LED 1-10kHz motor 10-20kHz servo 50Hz penting.

**Slide 8**: Resolusi 8-bit256 10-bit1024 16-bit65536 level. Max_Freq=Clock/2^Resolusi trade-off halus vs cepat.

**Slide 9**: RC fc=1/(2πRC) PWM filter smooth. fc jauh dibawah PWM minimal10× ripple tidak terlihat.

**Slide 10**: ESP32 DAC 8-bit GPIO25 GPIO26 256 level 0-3.3V 12.94mV step. Sinus cosine generator DMA streaming audio consumer-grade.

**Slide 11**: DAC_output_voltage(0-255) ESP32 API simpel. Lookup sinus 256 sampel samplerate. DAC1 lebih bagus dari DAC2 WiFi interference.

**Slide 12**: LEDC 16 channel PWM 8 high80MHz 8 low 4timer 1-16bit. ledcAttach(pin,freq,res) ledcWrite(pin,duty) mudah fleksibel.

**Slide 13**: STM32 servo 50Hz: PSC720 ARR2000 CCR 75→0° 150→90° 225→180°. Presisi timing akurat responsif.

**Slide 14**: TIM_HandleTypeDef TIM_OC_InitTypeDef HAL_TIM_PWM_Init()Start(). PWM1 HIGH saat CNT<CCR PWM2 LOW. Dead-time prevent shoot-through aman.

**Slide 15**: Kesimpulan: DAC true analog presisi PWM ekonomis filter latency trade-off aplikasi pilih optimal embedded design.

## BAGIAN 2: PRAKTIKUM (15 SLIDE)

**Slide 16**: P01 DAC Voltage output 0V 0.825V 1.65V 2.475V 3.3V multimeter verify error analisis resolusi.

**Slide 17**: P02 Sine lookup table 256 sampel smooth sine vs 64 sampel tangga amati osiloskop.

**Slide 18**: P03 Triangle sawtooth spectrum fundamental harmonik ganjil. Aplikasi function generator oscillator.

**Slide 19**: P04 Tone 440Hz speaker kapasitor 10µF 44.1kHz rate 100 sampel per periode dengar pitch.

**Slide 20**: P05 LED Breathing duty 0→255 smooth delay ledcWrite(value) 8-bit 256 12-bit 4096 halus gamma correction perceptual.

**Slide 21**: P06 LED Serial input brightness real-time instant 0 64 128 192 255 potensio kontrol.

**Slide 22**: P07 Servo SG90 PWM 20ms 0.5ms→0° 1.5ms→90° 2.5ms→180°. STM32 PSC720 ARR2000 CCR akurat.

**Slide 23**: P08 Sweep 100Hz-20kHz pitch smooth dengar frequency response hearing test akustik infrasonik audiosonik.

**Slide 24**: P09 Motor DC L298N duty modulasi torque cepat lambat arah forward reverse brake L298N kapasitor 100µF heat dissipation.

**Slide 25**: P10 RGB LED R/G/B→GPIO PWM R220Ω (255,0,0)=Red (0,255,0)=Green (0,0,255)=Blue interpolasi linear smooth hue.

**Slide 26**: P11 Buzzer Melody C4262Hz D4294Hz E4330Hz F4349Hz G4392Hz A4440Hz B4494Hz C5523Hz array freq durasi envelope ADSR.

**Slide 27**: P12 DAC vs PWM: DAC true murni PWM ripple. THD DAC kecil PWM besar settling trade-off kesimpulan cost performance.

**Slide 28**: Filtering: ripple<1% perlu freq20× fundamental RC cascade boost R/C. Motor 10kHz minimal servo 50Hz RC.

**Slide 29**: Troubleshoot DAC: unstable floating EMI load buffer Vref noisy. PWM: pin mode PSC ARR CLK CCR≤ARR servo jitter power eksternal.

**Slide 30**: Optimasi: DMA audio continuous prescaler terendah timer dedicated memory 256 sine 256bytes filter power ground isolator motor aman.

## BAGIAN 3: PROJECT & VIDEO (15 SLIDE)

**Slide 31**: Museum Interactive Controller: dual-MCU ESP32 audio DAC buzzer RGB STM32 servo motor potensio. UART sinkron swap frequency ramp servo sinkron. 2 minggu development.

**Slide 32**: Mode (1)AMBIENT RGB breathing motor idle (2)INTERACTIVE potensio freq servo follow (3)SWEEP freq 100Hz-5kHz servo sinkron (4)MELODY buzzer RGB. Transisi mode kontinyu brake servo 90° motor 0.

**Slide 33**: Spesifikasi: P01-P12 integrasikan reference sine triangle tone breathing brightness servo sweep motor RGB melody DAC-PWM compare. ESP32 GPIO25 sine GPIO26 triangle GPIO2/4/5 RGB GPIO13 buzzer GPIO34 pot ADC. STM32 TIM2 servo TIM3 motor ADC UART.

**Slide 34**: Output: speaker 85dB osiloskop LED RGB servo motor buzzer. Software: modular C++ STM32 handler UART CSV logging. Dokumentasi: architecture schematic flowchart FSM test report siap production.

**Slide 35**: Rubrik 20%: DAC waveform smooth freq±1% (20%). LED flicker color (20%). Servo motor 0-180° linear (15%). Buzzer tempo (15%). UART latency<50ms (10%). Mode UX instant (10%). Code modular (10%). A≥85 B70-84.

**Slide 36**: Video 15-25m YouTube Unlisted (1)Pembuka teaser (2)Materi DAC PWM ESP32 STM32 (3)Demo P01-P12 8-14m (4)Project 3-5m (5)Kesimpulan terima kasih.

**Slide 37**: Record: laptop code webcam PIP hardware. P01 multimeter tegangan P02 osiloskop sine Nyquist P03 spektrum P04 speaker pitch 440→880 P05 LED breathing thermometer P06 potensio real-time gamma plot.

**Slide 38**: Demo P07-P12 Final: servo 0-180° akurat, sweep 100Hz-5kHz sinkron, motor RPM curve, RGB smooth, do-re-mi tempo stabil, DAC vs PWM compare. Museum visitor panel saat pitch sweep servo sinkron melodi RGB breathing demo selesai.

**Slide 39**: Penilaian: pemahaman 15% penjelasan formula analogi. Demo 35% 12 lengkap narasi analisis. Project 20% dual-MCU multi-mode. Hardware 15% webcam jelas sync readable. Kualitas 15% 720p audio 70dB edit smooth. A≥85 B70-84 C60-69 D50-59.

**Slide 40**: Checklist: ✓15-25m ✓webcam>90% ✓P01-P12 ✓hardware ✓project ✓audio ✓oscilloscope ✓code ✓YouTube unlisted. Penalty: webcam-20% hardware-20% <10m-10% >30m-5% audio-15% <720p-10% incomplete-30%. Contact TA <24h WhatsApp deadline e-learning.

**Slide 41**: Kesimpulan: DAC true presisi PWM ekonomis. Praktikum P01-P12 kompetensi dasar. Project dual-MCU sinkronisasi responsif. Video dokumentasi hands-on. Prepare career amplifier audio motor smart lighting IoT embedded systems industrial scale deployment.

**Slide 42**: Referensi: Mastering STM32 AN3126 DAC audio ESP-IDF RM0008 datasheet. Tools: PlatformIO VS Code Rigol DS1054Z Python matplotlib. Repository organized modular reusable baseline complete.

**Slide 43**: Advanced: audio synthesizer LFO MIDI reverb DSP. Motor BLDC PMSM PID encoder field-weakening. Buck boost power factor harmonic. LED WS2812 OLED visualization. Sensor fusion Kalman filter peak detection algoritma.

**Slide 44**: Sync: CAN bus arbitrasi priority. Real-time event-driven microsecond. GPS RTC shared network. Failover watchdog safety critical. Logic analyzer timestamp latency jitter verifikasi deterministic.

**Slide 45**: Akhir modul: teori praktikum project video dokumentasi selesai. Kompeten: configure DAC PWM waveform aktuator servo multi-MCU, troubleshoot noise filtering debug osiloskop, trade-off cost performance modular reusable, lanjut FreeRTOS advanced motor power wireless IoT. Praktik konsisten embedded industry challenge skill engineer profesional excellence!

---

**Total: ~11,250 karakter. 45 slide × ~250 char/slide.**
