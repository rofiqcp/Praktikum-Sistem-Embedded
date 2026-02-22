# Project Modul 09: FreeRTOS Task Management

## 🎯 Judul Project
**"Multi-Platform Task Orchestrator: Sistem Monitoring dan Kontrol Real-Time dengan STM32 dan ESP32"**

---

## 📋 Deskripsi Project

Mahasiswa diminta membuat sistem monitoring dan kontrol real-time yang mengintegrasikan **STM32** sebagai *Local Controller* dan **ESP32** sebagai *IoT Gateway*. Sistem ini mendemonstrasikan penggunaan FreeRTOS task management untuk mengelola multiple sensor, aktuator, dan komunikasi antar device.

### Skenario Aplikasi
Sistem Smart Environmental Monitoring untuk ruangan laboratorium yang mencakup:
- Monitoring suhu, kelembaban, dan kualitas udara
- Kontrol pencahayaan dan ventilasi otomatis
- Logging data ke cloud dan tampilan dashboard
- Alert system untuk kondisi abnormal

---

## 🏗️ Arsitektur Sistem

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ARSITEKTUR SISTEM                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────────────────┐    UART    ┌──────────────────────┐       │
│  │    STM32F103C8T6     │◄──────────►│       ESP32          │       │
│  │  (Local Controller)  │            │    (IoT Gateway)     │       │
│  └──────────┬───────────┘            └──────────┬───────────┘       │
│             │                                    │                   │
│      ┌──────┴──────┐                      ┌──────┴──────┐           │
│      │   Sensors   │                      │    WiFi     │           │
│      │  & Actors   │                      │   Cloud     │           │
│      └─────────────┘                      └─────────────┘           │
│                                                                      │
│  STM32 Tasks:                           ESP32 Tasks:                 │
│  ├─ SensorTask (Pri 3)                  ├─ UARTReceiveTask (Pri 3)  │
│  ├─ ActuatorTask (Pri 2)                ├─ WiFiTask (Pri 2, Core 0) │
│  ├─ UARTTransmitTask (Pri 2)            ├─ CloudTask (Pri 2, Core 1)│
│  ├─ AlertTask (Pri 4)                   ├─ DisplayTask (Pri 1)      │
│  └─ MonitorTask (Pri 1)                 └─ MonitorTask (Pri 1)      │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 📌 Spesifikasi Teknis

### Hardware Requirements

#### STM32 Side:
| Komponen | Qty | Fungsi |
|----------|-----|--------|
| STM32F103C8T6 | 1 | Main controller |
| DHT22/DHT11 | 1 | Sensor suhu & kelembaban |
| MQ-135 | 1 | Sensor kualitas udara |
| LDR + Resistor | 1 | Sensor cahaya |
| LED RGB | 1 | Status indicator |
| LED (4x) | 4 | Output indicators |
| Relay Module | 2 | Kontrol fan & lamp |
| Buzzer | 1 | Audio alert |
| Push Button | 2 | Manual override |

#### ESP32 Side:
| Komponen | Qty | Fungsi |
|----------|-----|--------|
| ESP32 DevKit | 1 | IoT gateway |
| OLED 0.96" I2C | 1 | Local display |
| LED (2x) | 2 | WiFi & Cloud status |
| Push Button | 1 | Mode selection |

### Koneksi STM32:
```
STM32F103C8T6:
├── PA0: DHT22 Data
├── PA1: MQ-135 Analog
├── PA2: LDR Analog
├── PA3: Button 1 (Pull-up)
├── PA4: LED Red
├── PA5: LED Yellow
├── PA6: LED Green
├── PA7: LED Blue
├── PA8: Relay 1 (Fan)
├── PA9: UART1 TX → ESP32 RX
├── PA10: UART1 RX → ESP32 TX
├── PB0: Relay 2 (Lamp)
├── PB1: Buzzer
└── PB10: Button 2 (Pull-up)
```

### Koneksi ESP32:
```
ESP32:
├── GPIO4: LED WiFi Status
├── GPIO5: LED Cloud Status
├── GPIO16: UART2 RX → STM32 TX
├── GPIO17: UART2 TX → STM32 RX
├── GPIO21: I2C SDA (OLED)
├── GPIO22: I2C SCL (OLED)
└── GPIO23: Mode Button
```

---

## 📝 Spesifikasi Software

### STM32 Tasks (5 Tasks Minimum)

#### Task 1: SensorTask (Priority 3, Period 500ms)
```c
void vSensorTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    SensorData_t data;
    
    for(;;)
    {
        // Baca semua sensor
        data.temperature = readDHT22_Temperature();
        data.humidity = readDHT22_Humidity();
        data.airQuality = readMQ135();
        data.lightLevel = readLDR();
        data.timestamp = xTaskGetTickCount();
        
        // Kirim ke queue untuk task lain
        xQueueSend(xSensorQueue, &data, 0);
        
        // Log ke serial
        printf("[Sensor] T:%.1f H:%.1f AQ:%d L:%d\n",
               data.temperature, data.humidity,
               data.airQuality, data.lightLevel);
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}
```

#### Task 2: ActuatorTask (Priority 2, Event-driven)
```c
void vActuatorTask(void *pvParameters)
{
    ActuatorCmd_t cmd;
    
    for(;;)
    {
        // Tunggu command dari queue
        if(xQueueReceive(xActuatorQueue, &cmd, portMAX_DELAY))
        {
            switch(cmd.device)
            {
                case DEVICE_FAN:
                    HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, cmd.state);
                    break;
                case DEVICE_LAMP:
                    HAL_GPIO_WritePin(LAMP_PORT, LAMP_PIN, cmd.state);
                    break;
                case DEVICE_BUZZER:
                    controlBuzzer(cmd.state, cmd.duration);
                    break;
            }
            
            printf("[Actuator] Device %d -> %s\n", 
                   cmd.device, cmd.state ? "ON" : "OFF");
        }
    }
}
```

#### Task 3: AlertTask (Priority 4 - Highest)
```c
void vAlertTask(void *pvParameters)
{
    SensorData_t data;
    AlertLevel_t level;
    
    for(;;)
    {
        // Peek sensor data (tanpa remove)
        if(xQueuePeek(xSensorQueue, &data, pdMS_TO_TICKS(100)))
        {
            level = evaluateAlertLevel(&data);
            
            if(level >= ALERT_WARNING)
            {
                // Trigger buzzer
                ActuatorCmd_t cmd = {DEVICE_BUZZER, 1, 500};
                xQueueSend(xActuatorQueue, &cmd, 0);
                
                // Update LED status
                setAlertLED(level);
                
                printf("[ALERT] Level %d triggered!\n", level);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

#### Task 4: UARTTransmitTask (Priority 2, Period 1000ms)
```c
void vUARTTransmitTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    SensorData_t data;
    char txBuffer[128];
    
    for(;;)
    {
        if(xQueueReceive(xSensorQueue, &data, 0) == pdPASS)
        {
            // Format data untuk ESP32
            sprintf(txBuffer, "DATA:%lu,%.1f,%.1f,%d,%d\n",
                    data.timestamp,
                    data.temperature,
                    data.humidity,
                    data.airQuality,
                    data.lightLevel);
            
            HAL_UART_Transmit(&huart1, (uint8_t*)txBuffer, 
                             strlen(txBuffer), 100);
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}
```

#### Task 5: MonitorTask (Priority 1, Period 5000ms)
```c
void vMonitorTask(void *pvParameters)
{
    for(;;)
    {
        printf("\n========== SYSTEM STATUS ==========\n");
        printf("Free Heap: %d bytes\n", xPortGetFreeHeapSize());
        
        // Print stack high water marks
        printf("Stack HWM:\n");
        printf("  Sensor: %d\n", uxTaskGetStackHighWaterMark(xSensorHandle));
        printf("  Actuator: %d\n", uxTaskGetStackHighWaterMark(xActuatorHandle));
        printf("  Alert: %d\n", uxTaskGetStackHighWaterMark(xAlertHandle));
        printf("  UART: %d\n", uxTaskGetStackHighWaterMark(xUARTHandle));
        printf("===================================\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### ESP32 Tasks (5 Tasks Minimum)

#### Task 1: UARTReceiveTask (Priority 3, Core 1)
```cpp
void UARTReceiveTask(void *pvParameters) {
    char rxBuffer[128];
    SensorData_t data;
    
    for(;;) {
        if(Serial2.available()) {
            int len = Serial2.readBytesUntil('\n', rxBuffer, 127);
            rxBuffer[len] = '\0';
            
            // Parse data dari STM32
            if(strncmp(rxBuffer, "DATA:", 5) == 0) {
                sscanf(rxBuffer + 5, "%lu,%f,%f,%d,%d",
                       &data.timestamp,
                       &data.temperature,
                       &data.humidity,
                       &data.airQuality,
                       &data.lightLevel);
                
                // Kirim ke queues
                xQueueSend(xDisplayQueue, &data, 0);
                xQueueSend(xCloudQueue, &data, 0);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
```

#### Task 2: WiFiTask (Priority 2, Core 0)
```cpp
void WiFiTask(void *pvParameters) {
    for(;;) {
        if(WiFi.status() != WL_CONNECTED) {
            digitalWrite(LED_WIFI, LOW);
            
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            
            int retry = 0;
            while(WiFi.status() != WL_CONNECTED && retry < 20) {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                retry++;
            }
            
            if(WiFi.status() == WL_CONNECTED) {
                digitalWrite(LED_WIFI, HIGH);
                Serial.println("WiFi Connected: " + WiFi.localIP().toString());
            }
        }
        
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
```

#### Task 3: CloudTask (Priority 2, Core 1)
```cpp
void CloudTask(void *pvParameters) {
    SensorData_t data;
    HTTPClient http;
    
    for(;;) {
        if(xQueueReceive(xCloudQueue, &data, pdMS_TO_TICKS(1000))) {
            if(WiFi.status() == WL_CONNECTED) {
                // Kirim ke cloud (ThingSpeak/Firebase/Custom)
                String url = String(CLOUD_URL) + 
                            "?temp=" + String(data.temperature) +
                            "&hum=" + String(data.humidity) +
                            "&aq=" + String(data.airQuality) +
                            "&light=" + String(data.lightLevel);
                
                http.begin(url);
                int httpCode = http.GET();
                
                if(httpCode == 200) {
                    digitalWrite(LED_CLOUD, HIGH);
                    Serial.println("[Cloud] Data uploaded");
                } else {
                    digitalWrite(LED_CLOUD, LOW);
                    Serial.println("[Cloud] Upload failed");
                }
                
                http.end();
            }
        }
    }
}
```

#### Task 4: DisplayTask (Priority 1, Core 1)
```cpp
void DisplayTask(void *pvParameters) {
    SensorData_t data;
    
    for(;;) {
        if(xQueueReceive(xDisplayQueue, &data, pdMS_TO_TICKS(500))) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setCursor(0, 0);
            
            display.println("=== ENV MONITOR ===");
            display.printf("Temp: %.1f C\n", data.temperature);
            display.printf("Hum:  %.1f %%\n", data.humidity);
            display.printf("AQ:   %d\n", data.airQuality);
            display.printf("Light:%d\n", data.lightLevel);
            display.println("==================");
            display.printf("WiFi: %s\n", 
                WiFi.status() == WL_CONNECTED ? "OK" : "DISC");
            
            display.display();
        }
    }
}
```

#### Task 5: MonitorTask (Priority 1, Core 0)
```cpp
void MonitorTask(void *pvParameters) {
    for(;;) {
        Serial.println("\n======== ESP32 STATUS ========");
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());
        Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        
        // Task stack info
        Serial.println("Task Stack High Water Marks:");
        Serial.printf("  UART: %d\n", uxTaskGetStackHighWaterMark(xUARTHandle));
        Serial.printf("  WiFi: %d\n", uxTaskGetStackHighWaterMark(xWiFiHandle));
        Serial.printf("  Cloud: %d\n", uxTaskGetStackHighWaterMark(xCloudHandle));
        Serial.printf("  Display: %d\n", uxTaskGetStackHighWaterMark(xDisplayHandle));
        Serial.println("==============================\n");
        
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
```

---

## 🎯 Deliverables

### 1. Source Code (40%)
- [ ] Kode STM32 lengkap dengan semua task
- [ ] Kode ESP32 lengkap dengan semua task
- [ ] File konfigurasi (platformio.ini, FreeRTOSConfig.h)
- [ ] README dengan instruksi build dan upload

### 2. Dokumentasi (30%)
- [ ] Laporan teknis (min. 15 halaman)
  - Pendahuluan dan latar belakang
  - Desain sistem dan arsitektur
  - Implementasi detail setiap task
  - Analisis timing dan prioritas
  - Pengujian dan hasil
  - Kesimpulan dan saran
- [ ] Diagram (flowchart, state diagram, timing diagram)
- [ ] Skematik rangkaian

### 3. Video Demonstrasi (20%)
- [ ] Durasi: 5-10 menit
- [ ] Konten:
  - Penjelasan arsitektur sistem
  - Demo fungsi setiap task
  - Interaksi STM32-ESP32
  - Monitoring cloud/dashboard
  - Troubleshooting scenario

### 4. Presentasi (10%)
- [ ] Slide presentasi (max 15 slide)
- [ ] Live demo di kelas
- [ ] Q&A session

---

## 📊 Kriteria Penilaian

| Aspek | Bobot | Kriteria |
|-------|-------|----------|
| **Fungsionalitas STM32** | 20% | Semua task berjalan, sensor terbaca, aktuator bekerja |
| **Fungsionalitas ESP32** | 20% | Dual-core utilized, WiFi stable, cloud connected |
| **Integrasi Sistem** | 15% | Komunikasi UART lancar, data sinkron |
| **Task Management** | 15% | Prioritas tepat, no starvation, stack optimal |
| **Dokumentasi** | 15% | Lengkap, detail, analisis mendalam |
| **Video & Presentasi** | 10% | Jelas, demonstratif, professional |
| **Inovasi** | 5% | Fitur tambahan, optimisasi, kreativitas |

---

## 📅 Timeline

| Minggu | Aktivitas |
|--------|-----------|
| 1 | Desain sistem, persiapan hardware |
| 2 | Implementasi STM32 tasks |
| 3 | Implementasi ESP32 tasks |
| 4 | Integrasi dan testing |
| 5 | Dokumentasi dan video |
| 6 | Presentasi dan demo |

---

## 💡 Tips dan Hints

1. **Mulai dari task sederhana** - Buat LED blink task dulu, baru tambah kompleksitas
2. **Monitor stack usage** - Gunakan `uxTaskGetStackHighWaterMark()` selalu
3. **Debug via UART** - Print status task secara berkala
4. **Test incremental** - Satu task dulu, baru tambah yang lain
5. **Backup sering** - Gunakan version control (Git)

---

## 📚 Resources

- FreeRTOS API Reference: https://freertos.org/a00106.html
- STM32 HAL Documentation
- ESP32 Arduino Core Documentation
- ThingSpeak API untuk cloud logging

---

## ⚠️ Catatan Penting

1. **WAJIB** menggunakan kedua platform (STM32 DAN ESP32)
2. **WAJIB** minimal 5 task per platform
3. **WAJIB** ada komunikasi antar device
4. Plagiarisme akan mendapat nilai 0
5. Keterlambatan pengumpulan: -10% per hari
