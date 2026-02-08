# Project Modul 03: Wireless Sensor Network Gateway

## 📋 Informasi Project

| Item | Keterangan |
|------|------------|
| **Judul** | Wireless Sensor Network Gateway System |
| **Modul** | 03 - Serial UART Communication |
| **Platform** | STM32F103C8T6 + ESP32 DevKitC |
| **Tingkat Kesulitan** | ⭐⭐⭐⭐ (Lanjut) |
| **Durasi Pengerjaan** | 2 minggu |
| **Jenis** | Sistem Dual-MCU Terintegrasi |

---

## 🎯 Deskripsi Project

Mahasiswa akan mengembangkan **Wireless Sensor Network Gateway** yang terdiri dari:

- **STM32F103C8T6** sebagai **Sensor Node Controller** - mengumpulkan data dari multiple sensors dan mengirim via UART dengan protokol yang terdefinisi
- **ESP32 DevKitC** sebagai **Gateway Hub** - menerima data dari STM32, melakukan aggregation, dan menyediakan web interface untuk monitoring

Sistem ini mengimplementasikan protokol komunikasi yang robust dengan error detection, acknowledgment system, dan data buffering.

---

## 🎯 Tujuan Project

Setelah menyelesaikan project ini, mahasiswa mampu:

1. Merancang protokol komunikasi serial yang reliable
2. Mengimplementasikan error detection dan recovery mechanisms
3. Membangun buffer management untuk data streaming
4. Mengintegrasikan komunikasi antar-MCU dengan minimal data loss
5. Mengembangkan web-based monitoring interface
6. Menerapkan JSON serialization untuk data exchange
7. Melakukan throughput dan reliability testing

---

## 📐 Arsitektur Sistem

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                    WIRELESS SENSOR NETWORK GATEWAY                            │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   SENSOR INPUTS                STM32 NODE                    ESP32 GATEWAY   │
│   ─────────────                CONTROLLER                    ───────────────│
│                                                                              │
│   ┌─────────┐                ┌───────────────┐              ┌───────────┐   │
│   │Temp/Hum │────────ADC────►│               │              │           │   │
│   │ DHT22   │                │  Data         │    UART      │  Protocol │   │
│   └─────────┘                │  Collection   │◄────────────►│  Handler  │   │
│                              │               │   115200     │           │   │
│   ┌─────────┐                │  Protocol     │   8N1        │  Data     │   │
│   │  Light  │────────ADC────►│  Encoder      │              │  Parser   │   │
│   │  LDR    │                │               │              │           │   │
│   └─────────┘                │  TX Buffer    │              │  Web      │   │
│                              │  Management   │              │  Server   │   │
│   ┌─────────┐                │               │              │           │   │
│   │ Motion  │───────GPIO────►│  Error        │              │  JSON     │   │
│   │  PIR    │                │  Handler      │              │  API      │   │
│   └─────────┘                │               │              │           │   │
│                              └───────┬───────┘              └─────┬─────┘   │
│   ┌─────────┐                        │                            │         │
│   │  Soil   │────────ADC────►        │                            │         │
│   │Moisture │                        │                   ┌────────▼───────┐ │
│   └─────────┘                        │                   │   Web Browser  │ │
│                                      │                   │   Dashboard    │ │
│                              ┌───────▼───────┐           └────────────────┘ │
│                              │ Serial Debug  │                              │
│                              │   Console     │                              │
│                              └───────────────┘                              │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Spesifikasi Hardware

### Komponen Utama

| No | Komponen | Jumlah | Fungsi |
|----|----------|--------|--------|
| 1 | STM32F103C8T6 | 1 | Sensor Node Controller |
| 2 | ESP32 DevKitC | 1 | Gateway Hub |
| 3 | DHT22 | 1 | Temperature & Humidity |
| 4 | LDR + 10kΩ | 1 | Light sensor |
| 5 | PIR HC-SR501 | 1 | Motion detection |
| 6 | Soil Moisture Sensor | 1 | Soil humidity |
| 7 | LED RGB | 1 | Status indicator |
| 8 | Buzzer | 1 | Alert audio |
| 9 | OLED 0.96" I2C | 1 | Local display (optional) |
| 10 | Resistor 10kΩ | 4 | Pull-up/dividers |
| 11 | Resistor 330Ω | 3 | LED current limiting |

### Pin Assignment

**STM32F103C8T6 (Sensor Node):**
| Pin | Fungsi | Mode | Keterangan |
|-----|--------|------|------------|
| PA0 | Soil Moisture | ADC | Analog input |
| PA1 | LDR | ADC | Analog input |
| PA4 | DHT22 Data | GPIO | Digital I/O |
| PA5 | PIR Sensor | GPIO Input | Motion detect |
| PA2 | TX to ESP32 | USART2 TX | 115200 baud |
| PA3 | RX from ESP32 | USART2 RX | 115200 baud |
| PB3 | Status LED R | PWM | Error indicator |
| PB4 | Status LED G | PWM | OK indicator |
| PB5 | Status LED B | PWM | Activity |
| PC13 | Built-in LED | GPIO | Heartbeat |

**ESP32 DevKitC (Gateway):**
| Pin | Fungsi | Mode | Keterangan |
|-----|--------|------|------------|
| GPIO16 | RX from STM32 | UART2 RX | 115200 baud |
| GPIO17 | TX to STM32 | UART2 TX | 115200 baud |
| GPIO21 | OLED SDA | I2C | Optional display |
| GPIO22 | OLED SCL | I2C | Optional display |
| GPIO4 | Status LED | GPIO | Connection status |
| GPIO5 | Alert LED | GPIO | Alert indicator |
| GPIO2 | Built-in LED | GPIO | Activity |
| GPIO19 | Buzzer | PWM | Audio alert |

---

## 📝 Spesifikasi Protokol Komunikasi

### Protocol Frame Format

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        SERIAL PROTOCOL FRAME                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────┬──────┬────────┬──────────┬─────────┬──────────┬─────┬─────┐       │
│  │START│ SEQ  │  TYPE  │  LENGTH  │  PAYLOAD │ CHECKSUM │ END │ END │       │
│  │ 0x02│ 1B   │  1B    │   2B     │  N bytes │   2B     │ 0x03│ 0x0A│       │
│  │(STX)│      │        │          │          │ (CRC16)  │(ETX)│(LF) │       │
│  └─────┴──────┴────────┴──────────┴──────────┴──────────┴─────┴─────┘       │
│                                                                             │
│  Minimum Frame: 9 bytes (empty payload)                                     │
│  Maximum Frame: 265 bytes (256 byte payload)                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Message Types

| Type Code | Name | Direction | Description |
|-----------|------|-----------|-------------|
| 0x01 | DATA | Node→Gateway | Sensor data packet |
| 0x02 | ACK | Gateway→Node | Acknowledgment |
| 0x03 | NAK | Gateway→Node | Negative acknowledge |
| 0x04 | CMD | Gateway→Node | Command to node |
| 0x05 | STATUS | Both | Status request/response |
| 0x06 | CONFIG | Gateway→Node | Configuration update |
| 0x07 | ALERT | Node→Gateway | Alert/alarm condition |
| 0xFF | HEARTBEAT | Both | Keep-alive packet |

### Data Packet Payload (Type 0x01)

```c
typedef struct __attribute__((packed)) {
    uint32_t timestamp;      // 4 bytes - millis since boot
    int16_t  temperature;    // 2 bytes - temp × 100 (25.50°C = 2550)
    uint16_t humidity;       // 2 bytes - hum × 100 (65.50% = 6550)
    uint16_t light;          // 2 bytes - ADC value (0-4095)
    uint16_t soilMoisture;   // 2 bytes - ADC value (0-4095)
    uint8_t  motion;         // 1 byte  - 0=no motion, 1=motion
    uint8_t  batteryLevel;   // 1 byte  - percentage (0-100)
    uint8_t  reserved[2];    // 2 bytes - future use
} SensorDataPayload;         // Total: 16 bytes
```

### Command Packet Payload (Type 0x04)

```c
typedef struct __attribute__((packed)) {
    uint8_t  commandId;      // 1 byte  - command identifier
    uint8_t  param1;         // 1 byte  - parameter 1
    uint16_t param2;         // 2 bytes - parameter 2
    uint8_t  data[12];       // 12 bytes - additional data
} CommandPayload;            // Total: 16 bytes

// Command IDs:
// 0x01 = Set sample interval
// 0x02 = Set LED state
// 0x03 = Request immediate read
// 0x04 = Reset statistics
// 0x05 = Enter low power mode
// 0x10 = Reboot node
```

### Error Codes (NAK Payload)

| Code | Description |
|------|-------------|
| 0x01 | Unknown message type |
| 0x02 | Invalid payload length |
| 0x03 | CRC mismatch |
| 0x04 | Sequence error |
| 0x05 | Command execution failed |
| 0x06 | Sensor read error |
| 0x07 | Buffer overflow |
| 0xFF | General error |

---

## 📋 Spesifikasi Fungsional

### F1: Sensor Data Collection (STM32)

**Deskripsi:** STM32 mengumpulkan data dari semua sensor dengan interval tertentu.

**Fitur:**
- Configurable sample interval (100ms - 60s)
- Moving average filter untuk noise reduction
- Sensor health monitoring (detect disconnection)
- Automatic retry pada read failure

**Implementasi:**
```c
// Sensor reading with filtering
#define FILTER_SAMPLES 5

typedef struct {
    float samples[FILTER_SAMPLES];
    uint8_t index;
    float average;
} SensorFilter;

float updateFilter(SensorFilter* filter, float newValue) {
    filter->samples[filter->index] = newValue;
    filter->index = (filter->index + 1) % FILTER_SAMPLES;
    
    float sum = 0;
    for (int i = 0; i < FILTER_SAMPLES; i++) {
        sum += filter->samples[i];
    }
    filter->average = sum / FILTER_SAMPLES;
    return filter->average;
}
```

### F2: Protocol Handler (STM32)

**Deskripsi:** Encode sensor data ke protocol frame dan handle responses.

**Fitur:**
- Frame construction dengan CRC16
- Sequence number tracking
- ACK timeout dan retransmission
- Maximum retry before giving up

**Implementasi:**
```c
// Build and send data frame
uint8_t sendDataPacket(SensorDataPayload* data) {
    uint8_t frame[64];
    uint16_t frameLen = 0;
    
    frame[frameLen++] = 0x02;  // STX
    frame[frameLen++] = sequenceNumber++;
    frame[frameLen++] = 0x01;  // DATA type
    frame[frameLen++] = sizeof(SensorDataPayload) & 0xFF;
    frame[frameLen++] = (sizeof(SensorDataPayload) >> 8) & 0xFF;
    
    memcpy(&frame[frameLen], data, sizeof(SensorDataPayload));
    frameLen += sizeof(SensorDataPayload);
    
    uint16_t crc = calculateCRC16(frame + 1, frameLen - 1);
    frame[frameLen++] = crc & 0xFF;
    frame[frameLen++] = (crc >> 8) & 0xFF;
    
    frame[frameLen++] = 0x03;  // ETX
    frame[frameLen++] = 0x0A;  // LF
    
    // STM32 HAL: send frame via USART2
    HAL_UART_Transmit(&huart2, frame, frameLen, HAL_MAX_DELAY);
    
    return waitForAck(500);  // 500ms timeout
}
```

### F3: Gateway Data Handler (ESP32)

**Deskripsi:** Receive, parse, dan store sensor data.

**Fitur:**
- Frame validation dan CRC check
- Data buffering dengan circular buffer
- Automatic ACK/NAK generation
- Statistics tracking (packets received, errors, etc.)

**Implementasi:**
```cpp
// Circular buffer for received data
#define DATA_BUFFER_SIZE 100

SensorDataPayload dataBuffer[DATA_BUFFER_SIZE];
volatile uint16_t bufferHead = 0;
volatile uint16_t bufferTail = 0;

void storeData(SensorDataPayload* data) {
    uint16_t next = (bufferHead + 1) % DATA_BUFFER_SIZE;
    if (next != bufferTail) {  // Not full
        memcpy(&dataBuffer[bufferHead], data, sizeof(SensorDataPayload));
        bufferHead = next;
    }
}

SensorDataPayload* getLatestData() {
    if (bufferHead == bufferTail) return NULL;
    uint16_t latest = (bufferHead - 1 + DATA_BUFFER_SIZE) % DATA_BUFFER_SIZE;
    return &dataBuffer[latest];
}
```

### F4: Web Server Interface (ESP32)

**Deskripsi:** HTTP server menyediakan web dashboard dan REST API.

**Endpoints:**
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | / | Dashboard HTML page |
| GET | /api/current | Latest sensor data (JSON) |
| GET | /api/history | Historical data (JSON) |
| GET | /api/stats | Statistics (JSON) |
| POST | /api/command | Send command to node |
| GET | /ws | WebSocket for real-time updates |

**JSON Response Format:**
```json
{
    "timestamp": 1234567890,
    "sensors": {
        "temperature": 25.5,
        "humidity": 65.0,
        "light": 2048,
        "soilMoisture": 1500,
        "motion": false
    },
    "status": {
        "batteryLevel": 85,
        "signalQuality": "good",
        "lastUpdate": "2024-01-15T10:30:00Z"
    }
}
```

### F5: Dashboard Web Interface

**Features:**
- Real-time sensor data display
- Historical charts (last 24 hours)
- Alert configuration
- Node status monitoring
- Manual command sending

**Dashboard Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│            SENSOR NETWORK DASHBOARD                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │ Temperature │ │  Humidity   │ │    Light    │           │
│  │   25.5°C    │ │    65%      │ │   2048 lux  │           │
│  │  ▲ +0.5°C   │ │  ▼ -2%      │ │  ── stable  │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
│                                                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │    Soil     │ │   Motion    │ │   Battery   │           │
│  │    1500     │ │  Detected   │ │     85%     │           │
│  │  ── normal  │ │  ⚠ Alert    │ │  █████░░░   │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Temperature History (24h)               │   │
│  │  30°──────────────────────────────────              │   │
│  │  25°─────╱╲────╱╲────────╱╲──────────              │   │
│  │  20°────╱──╲──╱──╲──────╱──╲─────────              │   │
│  │     00:00   06:00   12:00   18:00   24:00          │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  Status: ● Connected    Last Update: 10:30:25              │
│  Packets: 15,234 received | 12 errors | 0.08% error rate   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 Deliverables

### D1: Source Code (40%)

| Item | Deskripsi | Bobot |
|------|-----------|-------|
| STM32 Firmware | Sensor collection, protocol encoder | 15% |
| ESP32 Firmware | Protocol handler, web server | 15% |
| Protocol Library | Shared protocol implementation | 5% |
| Web Interface | HTML/CSS/JS dashboard | 5% |

**Struktur Project:**
```
Project-03-Sensor-Gateway/
├── STM32-Sensor-Node/
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.c
│   │   ├── sensors.c
│   │   ├── protocol.c
│   │   └── uart_handler.c
│   └── include/
│       ├── config.h
│       ├── sensors.h
│       ├── protocol.h
│       └── uart_handler.h
│
├── ESP32-Gateway/
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.c
│   │   ├── protocol.c
│   │   ├── web_server.c
│   │   └── data_manager.c
│   ├── include/
│   │   ├── config.h
│   │   └── protocol.h
│   └── data/
│       ├── index.html
│       ├── style.css
│       └── app.js
│
├── shared/
│   ├── protocol_common.h
│   └── crc16.h
│
└── docs/
    ├── protocol_spec.md
    ├── api_reference.md
    └── wiring_diagram.png
```

### D2: Dokumentasi Teknis (25%)

| Item | Deskripsi | Bobot |
|------|-----------|-------|
| Protocol Specification | Frame format, messages, error codes | 10% |
| API Documentation | REST endpoints, JSON formats | 8% |
| Wiring Diagram | Complete circuit diagram | 7% |

### D3: Laporan Project (20%)

| Section | Konten | Bobot |
|---------|--------|-------|
| Abstrak | Ringkasan 200 kata | 2% |
| Pendahuluan | Background, objectives | 3% |
| Metodologi | Design, implementation | 5% |
| Hasil | Data, testing, analysis | 7% |
| Kesimpulan | Summary, future work | 3% |

### D4: Video Demonstrasi (15%)

| Aspek | Deskripsi | Durasi |
|-------|-----------|--------|
| System Overview | Architecture explanation | 1-2 menit |
| Hardware Setup | Wiring demonstration | 2-3 menit |
| Protocol Demo | Serial traffic analysis | 2-3 menit |
| Web Interface | Dashboard features | 2-3 menit |
| Testing | Error handling demo | 2 menit |

**Total durasi video: 9-13 menit**

---

## 🧪 Kriteria Pengujian

### Test Case 1: Protocol Reliability (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC1.1 | Send 1000 packets | >99% delivery rate |
| TC1.2 | Corrupt CRC | NAK sent, data rejected |
| TC1.3 | Simulate disconnect | Auto reconnect within 5s |
| TC1.4 | Burst send (100 rapid) | All data buffered & processed |

### Test Case 2: Sensor Accuracy (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC2.1 | Compare DHT22 reading | ±1°C accuracy |
| TC2.2 | Light sensor calibration | Linear response verified |
| TC2.3 | Motion detection | <1s response time |
| TC2.4 | Sensor disconnect | Error reported, no crash |

### Test Case 3: Web Interface (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC3.1 | Dashboard load | <2s load time |
| TC3.2 | Real-time update | Update within 1s |
| TC3.3 | API response | Correct JSON format |
| TC3.4 | Multiple clients | Support 5+ simultaneous |

### Test Case 4: Long-term Stability (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC4.1 | 24-hour continuous run | No memory leak |
| TC4.2 | Stress test | Handle peak load |
| TC4.3 | Power cycle recovery | Auto resume operation |
| TC4.4 | WiFi disconnect | Buffer data, resume TX |

---

## ⏰ Timeline Pengerjaan

### Minggu 1

| Hari | Kegiatan |
|------|----------|
| 1-2 | Protocol specification design |
| 3-4 | STM32 sensor reading & encoding |
| 5-6 | ESP32 protocol handler |
| 7 | Integration testing part 1 |

### Minggu 2

| Hari | Kegiatan |
|------|----------|
| 1-2 | Web server & API implementation |
| 3-4 | Dashboard development |
| 5 | Full integration & debugging |
| 6 | Testing & documentation |
| 7 | Video recording & submission |

---

## 📚 Referensi

### Dokumentasi
1. STM32F103 Reference Manual - USART Chapter
2. ESP32 Technical Reference - UART & WiFi
3. ESP-IDF Web Server Documentation

### Libraries
1. cJSON (ESP-IDF built-in) - JSON handling
2. ESP HTTP Server (ESP-IDF built-in) - Web server
3. DHT sensor library - Temperature/humidity

### Standards
1. NMEA Protocol - Reference for message format
2. Modbus RTU - Industrial protocol reference

---

## ⚠️ Catatan Penting

1. **Protocol Reliability:**
   - Implement proper ACK/timeout mechanism
   - Use sequence numbers for packet ordering
   - Buffer data when connection lost

2. **Power Considerations:**
   - STM32 should handle sleep mode between reads
   - ESP32 WiFi is power hungry - optimize usage

3. **Security:**
   - Web interface should be on local network only
   - Consider authentication for API endpoints

4. **Testing:**
   - Test with actual sensor values, not just simulation
   - Verify CRC implementation dengan online calculator
   - Use logic analyzer untuk protocol debugging
