# Modul 12: Network dan IoT

## Daftar Isi
1. [Pendahuluan](#1-pendahuluan)
2. [Arsitektur IoT](#2-arsitektur-iot)
3. [Model TCP/IP dan OSI](#3-model-tcpip-dan-osi)
4. [WiFi — Wireless Fidelity](#4-wifi--wireless-fidelity)
5. [Protokol TCP dan UDP](#5-protokol-tcp-dan-udp)
6. [Protokol HTTP dan REST API](#6-protokol-http-dan-rest-api)
7. [Protokol MQTT](#7-protokol-mqtt)
8. [Bluetooth Low Energy (BLE)](#8-bluetooth-low-energy-ble)
9. [WebSocket](#9-websocket)
10. [Perbandingan ESP32 vs STM32 untuk Networking](#10-perbandingan-esp32-vs-stm32-untuk-networking)
11. [STM32 Network via Modul External](#11-stm32-network-via-modul-external)
12. [Keamanan Jaringan](#12-keamanan-jaringan)
13. [Daftar Percobaan](#13-daftar-percobaan)

---

## 1. Pendahuluan

Konektivitas jaringan merupakan komponen fundamental dalam sistem embedded modern. Kemampuan perangkat embedded untuk berkomunikasi melalui jaringan memungkinkan terciptanya ekosistem Internet of Things (IoT) yang menghubungkan perangkat fisik dengan layanan cloud dan antarmuka pengguna.

Modul ini membahas berbagai protokol dan teknologi jaringan yang umum digunakan dalam embedded systems, dengan fokus pada dua platform:

- **ESP32**: Memiliki WiFi dan Bluetooth/BLE built-in, cocok untuk aplikasi IoT langsung
- **STM32**: Tidak memiliki WiFi/BLE bawaan, memerlukan modul external (ESP-01, W5500, HM-10)

### Tujuan Pembelajaran

Setelah menyelesaikan modul ini, mahasiswa diharapkan mampu:
1. Memahami arsitektur IoT dan protokol jaringan dasar
2. Mengimplementasikan koneksi WiFi (station/AP) pada ESP32
3. Membangun server dan client TCP/UDP
4. Mengimplementasikan HTTP server/client dan REST API
5. Menggunakan MQTT untuk komunikasi publish/subscribe
6. Mengimplementasikan BLE advertising dan GATT server
7. Menggunakan modul external (ESP-01, W5500, HM-10) pada STM32

---

## 2. Arsitektur IoT

### 2.1 Layer Arsitektur IoT

```
┌─────────────────────────────────────────────────┐
│              APPLICATION LAYER                   │
│    Dashboard, Mobile App, Analytics, AI/ML       │
├─────────────────────────────────────────────────┤
│              CLOUD / PLATFORM LAYER              │
│    AWS IoT, Google Cloud IoT, ThingsBoard        │
│    Data Storage, Rules Engine, Visualization     │
├─────────────────────────────────────────────────┤
│              NETWORK / TRANSPORT LAYER           │
│    WiFi, Cellular, LoRa, Ethernet, BLE           │
│    TCP/UDP, HTTP, MQTT, CoAP, WebSocket          │
├─────────────────────────────────────────────────┤
│              PERCEPTION / DEVICE LAYER           │
│    Sensor, Actuator, Microcontroller             │
│    ESP32, STM32, Arduino, Raspberry Pi           │
└─────────────────────────────────────────────────┘
```

### 2.2 Alur Data IoT

```
Sensor → MCU → Network → Gateway → Cloud → Dashboard
                                              ↓
                                          Actuator ← MCU ← Command
```

**Contoh alur lengkap:**
1. Sensor suhu DHT22 membaca temperatur = 28.5°C
2. ESP32 memproses data dan membentuk paket JSON
3. ESP32 mengirim via MQTT ke broker (test.mosquitto.org)
4. Dashboard (Node-RED/Grafana) subscribe ke topic dan menampilkan grafik
5. Jika suhu > 30°C, dashboard kirim command ke ESP32 untuk nyalakan kipas

### 2.3 Komponen Arsitektur

| Komponen | Fungsi | Contoh |
|----------|--------|--------|
| **Sensor** | Mengukur parameter fisik | DHT22, BMP280, LDR, PIR |
| **Mikrokontroler** | Memproses data sensor | ESP32, STM32, Arduino |
| **Gateway** | Menghubungkan device ke cloud | Router WiFi, LoRa Gateway |
| **Protokol** | Mengatur format komunikasi | MQTT, HTTP, CoAP, WebSocket |
| **Cloud** | Menyimpan dan mengolah data | AWS IoT, Firebase, ThingsBoard |
| **Dashboard** | Menampilkan data ke user | Grafana, Node-RED, Web App |

---

## 3. Model TCP/IP dan OSI

### 3.1 Model OSI (7 Layer)

```
┌──────────────────────────────────────────────┐
│ 7. Application  │ HTTP, MQTT, DNS, FTP       │
├──────────────────────────────────────────────┤
│ 6. Presentation │ SSL/TLS, encoding          │
├──────────────────────────────────────────────┤
│ 5. Session      │ Session management         │
├──────────────────────────────────────────────┤
│ 4. Transport    │ TCP, UDP                   │
├──────────────────────────────────────────────┤
│ 3. Network      │ IP, ICMP, ARP              │
├──────────────────────────────────────────────┤
│ 2. Data Link    │ Ethernet, WiFi (802.11)    │
├──────────────────────────────────────────────┤
│ 1. Physical     │ Radio, kabel, sinyal       │
└──────────────────────────────────────────────┘
```

### 3.2 Model TCP/IP (4 Layer)

| TCP/IP Layer | Protokol | Fungsi |
|-------------|----------|--------|
| **Application** | HTTP, MQTT, DNS, DHCP | Aplikasi dan layanan |
| **Transport** | TCP, UDP | Pengiriman data end-to-end |
| **Internet** | IP, ICMP, ARP | Pengalamatan dan routing |
| **Network Access** | Ethernet, WiFi | Akses fisik jaringan |

### 3.3 IP Addressing

- **IPv4**: 32-bit address (contoh: 192.168.1.100)
- **Subnet Mask**: Menentukan network vs host portion (255.255.255.0 = /24)
- **Gateway**: Router yang menghubungkan ke jaringan lain
- **DHCP**: Pemberian IP otomatis oleh server
- **Static IP**: IP dikonfigurasi manual

---

## 4. WiFi — Wireless Fidelity

### 4.1 Dasar WiFi (IEEE 802.11)

WiFi menggunakan gelombang radio pada frekuensi 2.4 GHz dan/atau 5 GHz untuk komunikasi nirkabel. ESP32 mendukung 802.11 b/g/n pada 2.4 GHz.

### 4.2 Mode Operasi WiFi

| Mode | Deskripsi | Penggunaan |
|------|-----------|------------|
| **Station (STA)** | Terhubung ke Access Point yang ada | ESP32 sebagai client WiFi |
| **Access Point (AP)** | Membuat jaringan WiFi sendiri | ESP32 sebagai hotspot |
| **STA+AP** | Kedua mode bersamaan | Repeater / bridge |

### 4.3 WiFi Scanning

Proses scanning memungkinkan perangkat menemukan Access Point yang tersedia:

```c
/* ESP-IDF: WiFi Scan */
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

void wifi_scan(void) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,           // Scan semua SSID
        .bssid = NULL,          // Scan semua BSSID
        .channel = 0,           // Scan semua channel
        .show_hidden = true,    // Tampilkan hidden SSID
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };
    
    esp_wifi_scan_start(&scan_config, true);
    
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    
    wifi_ap_record_t *ap_list = malloc(ap_count * sizeof(wifi_ap_record_t));
    esp_wifi_scan_get_ap_records(&ap_count, ap_list);
    
    for (int i = 0; i < ap_count; i++) {
        printf("SSID: %-32s | RSSI: %d | CH: %d\n",
               ap_list[i].ssid, ap_list[i].rssi, ap_list[i].primary);
    }
    free(ap_list);
}
```

### 4.4 WiFi Station Mode (ESP-IDF)

```c
/* Inisialisasi WiFi Station */
static void wifi_init_sta(void) {
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &event_handler, NULL, NULL);
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "MySSID",
            .password = "MyPassword",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}
```

### 4.5 WiFi Event Handler

```c
static void event_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "Disconnected, retrying...");
                esp_wifi_connect();
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        }
    }
}
```

### 4.6 WiFi Access Point Mode

```c
static void wifi_init_softap(void) {
    esp_netif_create_default_wifi_ap();
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32_AP",
            .ssid_len = strlen("ESP32_AP"),
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = 6,
        },
    };
    
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
}
```

### 4.7 RSSI dan Kualitas Sinyal

| RSSI (dBm) | Kualitas | Keterangan |
|------------|----------|------------|
| -30 to -50 | Excellent | Sangat dekat dengan AP |
| -50 to -60 | Good | Sinyal bagus |
| -60 to -70 | Fair | Cukup untuk browsing |
| -70 to -80 | Weak | Koneksi tidak stabil |
| < -80 | Poor | Sering putus |

---

## 5. Protokol TCP dan UDP

### 5.1 TCP (Transmission Control Protocol)

TCP adalah protokol connection-oriented yang menjamin pengiriman data secara berurutan dan tanpa error.

**Karakteristik TCP:**
- Connection-oriented (3-way handshake)
- Reliable (acknowledgment, retransmission)
- Ordered (data diterima sesuai urutan)
- Flow control & congestion control
- Overhead lebih besar dari UDP

**3-Way Handshake:**
```
Client              Server
  |--- SYN ---------->|
  |<-- SYN+ACK -------|
  |--- ACK ---------->|
  |   (Connected!)     |
```

### 5.2 UDP (User Datagram Protocol)

UDP adalah protokol connectionless yang mengirim datagram tanpa jaminan pengiriman.

**Karakteristik UDP:**
- Connectionless (kirim langsung)
- Unreliable (tidak ada ACK)
- Unordered (bisa sampai tidak berurutan)
- Low overhead, latency rendah
- Cocok untuk streaming, broadcast, sensor data

### 5.3 TCP vs UDP

| Aspek | TCP | UDP |
|-------|-----|-----|
| Koneksi | Connection-oriented | Connectionless |
| Reliabilitas | Guaranteed delivery | Best effort |
| Urutan | Ordered | Unordered |
| Overhead | Tinggi (20+ byte header) | Rendah (8 byte header) |
| Kecepatan | Lebih lambat | Lebih cepat |
| Use Case | HTTP, MQTT, file transfer | DNS, streaming, broadcast |

### 5.4 TCP Socket Programming (ESP-IDF)

```c
#include "lwip/sockets.h"

/* TCP Server */
void tcp_server_task(void *pvParam) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        char buffer[128];
        int len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            send(client_fd, buffer, len, 0);  // Echo back
        }
        close(client_fd);
    }
}
```

### 5.5 UDP Socket Programming (ESP-IDF)

```c
/* UDP Server */
void udp_server_task(void *pvParam) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(5000),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    
    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    while (1) {
        char buffer[128];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr *)&client_addr, &client_len);
        if (len > 0) {
            buffer[len] = '\0';
            // Send response
            sendto(sock, buffer, len, 0,
                   (struct sockaddr *)&client_addr, client_len);
        }
    }
}
```

---

## 6. Protokol HTTP dan REST API

### 6.1 HTTP (HyperText Transfer Protocol)

HTTP adalah protokol request-response di layer aplikasi yang berjalan di atas TCP.

**Struktur HTTP Request:**
```
GET /api/status HTTP/1.1
Host: 192.168.1.100
Content-Type: application/json
\r\n
```

**Struktur HTTP Response:**
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 42
\r\n
{"temperature": 28.5, "humidity": 65}
```

### 6.2 HTTP Methods

| Method | Fungsi | Contoh |
|--------|--------|--------|
| **GET** | Mengambil data | GET /api/sensor |
| **POST** | Mengirim data baru | POST /api/led {"state": "on"} |
| **PUT** | Update data | PUT /api/config {"interval": 5} |
| **DELETE** | Menghapus data | DELETE /api/log/123 |

### 6.3 HTTP Status Codes

| Code | Arti | Keterangan |
|------|------|------------|
| 200 | OK | Request berhasil |
| 201 | Created | Resource berhasil dibuat |
| 400 | Bad Request | Request tidak valid |
| 404 | Not Found | Resource tidak ditemukan |
| 500 | Internal Server Error | Error di server |

### 6.4 HTTP Server (ESP-IDF)

```c
#include "esp_http_server.h"

/* Handler untuk GET /api/status */
static esp_err_t status_handler(httpd_req_t *req) {
    char response[128];
    snprintf(response, sizeof(response),
             "{\"heap\":%lu,\"uptime\":%lld}",
             esp_get_free_heap_size(),
             esp_timer_get_time() / 1000000);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, strlen(response));
}

/* Handler untuk POST /api/led */
static esp_err_t led_handler(httpd_req_t *req) {
    char buf[64];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    buf[len] = '\0';
    // Parse dan kontrol LED
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

/* Start HTTP server */
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    
    httpd_start(&server, &config);
    
    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_handler,
    };
    httpd_register_uri_handler(server, &status_uri);
    
    return server;
}
```

### 6.5 HTTP Client (ESP-IDF)

```c
#include "esp_http_client.h"

void http_get_request(void) {
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .method = HTTP_METHOD_GET,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "Status=%d, Length=%d", status, length);
    }
    
    esp_http_client_cleanup(client);
}
```

---

## 7. Protokol MQTT

### 7.1 Apa itu MQTT?

MQTT (Message Queuing Telemetry Transport) adalah protokol messaging ringan berbasis publish/subscribe yang dirancang untuk koneksi terbatas dan bandwidth rendah.

### 7.2 Arsitektur MQTT

```
┌──────────┐    publish    ┌──────────┐    deliver    ┌──────────┐
│ Publisher │──────────────>│  Broker  │──────────────>│Subscriber│
│ (ESP32)  │  topic:       │(Mosquitto)│  topic:       │(Dashboard)│
│          │  sensor/temp  │          │  sensor/temp  │          │
└──────────┘               └──────────┘               └──────────┘
```

### 7.3 Konsep MQTT

| Konsep | Deskripsi |
|--------|-----------|
| **Broker** | Server yang menerima dan mendistribusikan pesan |
| **Publisher** | Client yang mengirim pesan ke topic |
| **Subscriber** | Client yang menerima pesan dari topic |
| **Topic** | Hierarchical string untuk routing pesan (e.g., `home/living/temp`) |
| **QoS** | Quality of Service (0, 1, 2) |
| **Retain** | Broker menyimpan pesan terakhir untuk subscriber baru |
| **Last Will (LWT)** | Pesan yang dikirim broker jika client disconnect tidak normal |

### 7.4 MQTT QoS Levels

| QoS | Nama | Jaminan | Overhead |
|-----|------|---------|----------|
| 0 | At most once | Fire and forget | Minimal |
| 1 | At least once | Acknowledged delivery | Moderate |
| 2 | Exactly once | 4-way handshake | Tinggi |

### 7.5 MQTT Topic Hierarchy

```
home/                       ← Root
├── living/                 ← Room
│   ├── temperature         ← Sensor type
│   ├── humidity
│   └── light
├── kitchen/
│   ├── temperature
│   └── gas
└── control/
    ├── ac                  ← Actuator
    └── fan
```

**Wildcard:**
- `+` : Single level wildcard (`home/+/temperature` = semua suhu di semua ruangan)
- `#` : Multi level wildcard (`home/#` = semua data di bawah home)

### 7.6 MQTT Client (ESP-IDF)

```c
#include "mqtt_client.h"

static esp_mqtt_client_handle_t client;

static void mqtt_event_handler(void *args, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            esp_mqtt_client_subscribe(client, "sensor/control", 1);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Data: %.*s", event->data_len, event->data);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected");
            break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org",
        .credentials.client_id = "esp32_client",
    };
    
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                    mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

/* Publish data */
void publish_sensor_data(float temp, float humidity) {
    char payload[64];
    snprintf(payload, sizeof(payload),
             "{\"temp\":%.1f,\"hum\":%.1f}", temp, humidity);
    esp_mqtt_client_publish(client, "sensor/data", payload, 0, 1, 0);
}
```

---

## 8. Bluetooth Low Energy (BLE)

### 8.1 BLE vs Bluetooth Classic

| Aspek | BLE | Bluetooth Classic |
|-------|-----|-------------------|
| Bandwidth | 1 Mbps | 2-3 Mbps |
| Range | ~100m | ~100m |
| Power | Sangat rendah | Moderate |
| Latency | ~6ms | ~100ms |
| Use Case | Sensor, beacon, wearable | Audio, file transfer |

### 8.2 BLE Architecture

```
┌─────────────────────────────────────┐
│         APPLICATION                  │
├─────────────────────────────────────┤
│    GAP (Generic Access Profile)      │
│    Advertising, Scanning, Connecting │
├─────────────────────────────────────┤
│    GATT (Generic Attribute Profile)  │
│    Services, Characteristics         │
├─────────────────────────────────────┤
│    ATT (Attribute Protocol)          │
├─────────────────────────────────────┤
│    L2CAP (Logical Link Control)      │
├─────────────────────────────────────┤
│    HCI (Host Controller Interface)   │
├─────────────────────────────────────┤
│    Link Layer                        │
├─────────────────────────────────────┤
│    Physical Layer (2.4 GHz)          │
└─────────────────────────────────────┘
```

### 8.3 GAP (Generic Access Profile)

GAP mengatur discovery dan koneksi antar perangkat BLE.

**Roles:**
- **Peripheral**: Mengiklankan keberadaannya (advertising)
- **Central**: Melakukan scanning dan inisiasi koneksi
- **Broadcaster**: Hanya advertising, tidak bisa di-connect
- **Observer**: Hanya scanning, tidak melakukan koneksi

**Advertising Data:**
- Device Name
- TX Power Level
- Service UUIDs
- Manufacturer Specific Data
- Appearance

### 8.4 GATT (Generic Attribute Profile)

GATT mengatur pertukaran data setelah koneksi terjalin.

```
Server
├── Service 1 (UUID: 0x180A - Device Info)
│   ├── Characteristic: Manufacturer Name (Read)
│   ├── Characteristic: Model Number (Read)
│   └── Characteristic: Firmware Rev (Read)
│
└── Service 2 (UUID: 0x00FF - Custom)
    ├── Characteristic: Sensor Data (Read, Notify)
    │   └── Descriptor: CCCD (Client Config)
    └── Characteristic: LED Control (Read, Write)
```

**Operasi GATT:**
- **Read**: Client membaca nilai dari server
- **Write**: Client menulis nilai ke server
- **Notify**: Server mengirim update ke client tanpa diminta
- **Indicate**: Seperti notify tapi dengan acknowledgment

### 8.5 BLE pada ESP32 (ESP-IDF)

```c
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

/* Konfigurasi Advertising */
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,     // 20ms
    .adv_int_max = 0x40,     // 40ms
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* Start advertising */
void ble_start_advertising(void) {
    esp_ble_gap_set_device_name("ESP32_BLE");
    esp_ble_gap_start_advertising(&adv_params);
}
```

### 8.6 BLE pada STM32 (via HM-10 Module)

STM32 tidak memiliki BLE built-in. Gunakan modul HM-10 via UART:

```c
/* AT commands untuk HM-10 */
// Test koneksi
UART_SendString("AT\r\n");           // Response: "OK"

// Set nama device
UART_SendString("AT+NAMEMyBLE\r\n"); // Response: "OK+Set:MyBLE"

// Set role (0=Peripheral, 1=Central)
UART_SendString("AT+ROLE0\r\n");     // Response: "OK+Set:0"

// Cek status koneksi
UART_SendString("AT+CONN?\r\n");     // Response: "OK+CONN" or "OK+LOST"
```

---

## 9. WebSocket

### 9.1 Apa itu WebSocket?

WebSocket adalah protokol komunikasi full-duplex yang berjalan di atas TCP. Berbeda dengan HTTP yang request-response, WebSocket memungkinkan komunikasi dua arah secara real-time.

### 9.2 HTTP vs WebSocket

| Aspek | HTTP | WebSocket |
|-------|------|-----------|
| Komunikasi | Request-Response | Full-duplex |
| Koneksi | Short-lived | Persistent |
| Overhead | Header setiap request | Minimal setelah handshake |
| Latency | Tinggi (buka-tutup koneksi) | Rendah (koneksi tetap) |
| Use Case | REST API, web pages | Real-time data, chat, games |

### 9.3 WebSocket Handshake

```
Client → Server:
GET /ws HTTP/1.1
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13

Server → Client:
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

### 9.4 WebSocket Server (ESP-IDF)

```c
#include "esp_http_server.h"

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        // Handshake - return ESP_OK untuk accept
        return ESP_OK;
    }
    
    // Receive WebSocket frame
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    httpd_ws_recv_frame(req, &ws_pkt, 0);
    uint8_t *buf = calloc(1, ws_pkt.len + 1);
    ws_pkt.payload = buf;
    httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    
    // Echo back
    httpd_ws_send_frame(req, &ws_pkt);
    free(buf);
    return ESP_OK;
}
```

---

## 10. Perbandingan ESP32 vs STM32 untuk Networking

### 10.1 Tabel Perbandingan

| Aspek | ESP32 | STM32 (F103/F411) |
|-------|-------|-------------------|
| **WiFi** | Built-in 802.11 b/g/n | ❌ Tidak ada (perlu ESP-01/ESP8266) |
| **Bluetooth Classic** | Built-in v4.2 | ❌ Tidak ada |
| **BLE** | Built-in v4.2 | ❌ Perlu HM-10 / nRF52832 |
| **Ethernet** | RMII (ESP32 original) | SPI (W5500) / RMII (F4+ETH) |
| **TCP/UDP Stack** | lwIP (built-in) | lwIP (F4+ETH) / AT cmd (ESP-01) |
| **HTTP** | esp_http_server/client | Via WiFi module / Ethernet |
| **MQTT** | esp_mqtt component | Via WiFi module |
| **TLS/SSL** | mbedTLS (built-in) | mbedTLS (manual) / AT+CIPSSLSIZE |
| **WiFi API** | esp_wifi.h (native) | AT commands via UART |
| **BLE API** | esp_gap_ble_api.h (native) | AT commands (HM-10) |
| **Complexity** | Simple (native) | Complex (external modules) |
| **Cost** | ~$3-5 (all-in-one) | ~$1-2 + module cost |

### 10.2 Kapan Menggunakan Masing-masing?

**Gunakan ESP32 ketika:**
- Butuh WiFi/BLE langsung tanpa hardware tambahan
- Prototyping cepat untuk aplikasi IoT
- Budget terbatas (all-in-one solution)
- Aplikasi consumer IoT (smart home, wearable)

**Gunakan STM32 + Modul External ketika:**
- Butuh kontrol hardware yang lebih presisi
- Aplikasi industrial yang butuh reliabilitas tinggi
- Sudah ada infrastruktur STM32 yang mature
- Butuh real-time performance yang deterministik
- Koneksi Ethernet preferred (W5500 + STM32)

---

## 11. STM32 Network via Modul External

### 11.1 ESP-01 (ESP8266) via UART AT Commands

**Wiring:**
```
STM32F103          ESP-01 (ESP8266)
---------          ----------------
PA2 (TX) --------> RX
PA3 (RX) <-------- TX
3.3V -------------> VCC
GND --------------> GND
3.3V -------------> CH_PD (Enable)
                    GPIO0 → Float (normal mode)
                    GPIO0 → GND (flash mode)
```

**AT Command Reference:**

| Command | Fungsi | Response |
|---------|--------|----------|
| `AT` | Test | OK |
| `AT+RST` | Reset module | OK |
| `AT+GMR` | Version info | AT version:... |
| `AT+CWMODE=1` | Set station mode | OK |
| `AT+CWJAP="SSID","PASS"` | Connect WiFi | WIFI CONNECTED, WIFI GOT IP |
| `AT+CIFSR` | Get IP address | +CIFSR:STAIP,"192.168.1.x" |
| `AT+CIPSTART="TCP","ip",port` | Open TCP | CONNECT |
| `AT+CIPSEND=length` | Prepare send | > (prompt) |
| `AT+CIPCLOSE` | Close connection | CLOSED |

### 11.2 W5500 Ethernet Module via SPI

**Wiring:**
```
STM32F103          W5500 Module
---------          ------------
PA5 (SCK) -------> SCLK
PA6 (MISO) <------ MISO
PA7 (MOSI) ------> MOSI
PA4 (CS) ---------> CS
PB0 --------------> RST
PB1 <-------------- INT
3.3V -------------> VCC
GND --------------> GND
```

**W5500 SPI Frame Format:**
```
┌─────────────────┬────────────────┬──────────────┐
│ Address (16-bit) │ Control (8-bit) │ Data (8-bit+) │
│ Offset Address   │ BSB+RWB+OM     │ Register Data │
└─────────────────┴────────────────┴──────────────┘

Control Byte:
  BSB[4:0]: Block Select Bits (Common=00000, Socket n=n*4+1)
  RWB: Read(0) / Write(1)
  OM[1:0]: SPI Mode (00=Variable, 01=1byte, 10=2byte, 11=4byte)
```

### 11.3 HM-10 BLE Module via UART

**Wiring:**
```
STM32F103          HM-10 Module
---------          ------------
PA2 (TX) --------> RX
PA3 (RX) <-------- TX
3.3V -------------> VCC
GND --------------> GND
```

**AT Commands:**

| Command | Fungsi |
|---------|--------|
| `AT` | Test koneksi |
| `AT+NAME?` | Get device name |
| `AT+NAMExxxx` | Set device name |
| `AT+ROLE0` | Set peripheral mode |
| `AT+ROLE1` | Set central mode |
| `AT+DISC?` | Discover devices |
| `AT+CONNaddress` | Connect to device |
| `AT+UUID?` | Get service UUID |

---

## 12. Keamanan Jaringan

### 12.1 WiFi Security

| Mode | Keamanan | Keterangan |
|------|----------|------------|
| Open | Tidak ada | Tidak direkomendasikan |
| WEP | Lemah | Sudah deprecated |
| WPA | Moderate | TKIP encryption |
| WPA2-PSK | Kuat | AES encryption, pre-shared key |
| WPA3 | Sangat kuat | SAE (belum umum di ESP32) |

### 12.2 TLS/SSL

TLS (Transport Layer Security) mengenkripsi komunikasi antara client dan server:
- HTTPS = HTTP + TLS
- MQTTS = MQTT + TLS (port 8883)
- WSS = WebSocket + TLS

### 12.3 Best Practices Keamanan IoT

1. Selalu gunakan WPA2 untuk WiFi
2. Gunakan TLS/HTTPS untuk transfer data sensitif
3. Jangan hardcode password di source code (gunakan NVS/Flash)
4. Update firmware secara berkala (OTA)
5. Gunakan unique client ID untuk MQTT
6. Implementasi authentication di REST API
7. Gunakan certificate pinning untuk koneksi HTTPS

---

## 13. ESP-NOW — Peer-to-Peer Communication

ESP-NOW adalah protokol proprietary Espressif yang memungkinkan komunikasi langsung antar ESP32 **tanpa router WiFi**. Sangat ideal untuk sensor network dan remote control.

### 13.1 Karakteristik ESP-NOW

| Fitur | Spesifikasi |
|-------|-------------|
| **Range** | ~200m (open area), ~50m (indoor) |
| **Data rate** | 1 Mbps |
| **Max payload** | 250 bytes per packet |
| **Max peers** | 20 (encrypted: 10) |
| **Latency** | < 1 ms |
| **Enkripsi** | CCMP (optional) |
| **Power** | Sangat rendah (bisa dengan deep sleep) |

### 13.2 Implementasi ESP-NOW

**Sender (Pengirim):**

```c
#include "esp_now.h"
#include "esp_wifi.h"

/* MAC address receiver — ganti dengan MAC receiver sebenarnya */
static uint8_t peer_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_data_t;

void espnow_send_cb(const uint8_t *mac, esp_now_send_status_t status)
{
    printf("Send to %02X:%02X:%02X:%02X:%02X:%02X: %s\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
           status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void espnow_init_sender(void)
{
    /* WiFi harus diinisialisasi (STA atau AP mode) */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    esp_now_init();
    esp_now_register_send_cb(espnow_send_cb);

    /* Tambah peer */
    esp_now_peer_info_t peer = {
        .channel = 0,
        .encrypt = false,
    };
    memcpy(peer.peer_addr, peer_mac, 6);
    esp_now_add_peer(&peer);
}

void send_sensor_data(float temp, float hum)
{
    sensor_data_t data = {
        .temperature = temp,
        .humidity    = hum,
        .timestamp   = esp_timer_get_time() / 1000,
    };
    esp_now_send(peer_mac, (uint8_t *)&data, sizeof(data));
}
```

**Receiver (Penerima):**

```c
void espnow_recv_cb(const esp_now_recv_info_t *info,
                    const uint8_t *data, int len)
{
    sensor_data_t *sensor = (sensor_data_t *)data;
    printf("From %02X:%02X: Temp=%.1f°C, Hum=%.1f%%\n",
           info->src_addr[4], info->src_addr[5],
           sensor->temperature, sensor->humidity);
}

void espnow_init_receiver(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    esp_now_init();
    esp_now_register_recv_cb(espnow_recv_cb);
}
```

---

## 14. OTA — Over-The-Air Update

OTA memungkinkan update firmware tanpa koneksi fisik — sangat penting untuk perangkat IoT yang sudah deploy di lapangan.

### 14.1 OTA pada ESP32

ESP32 menggunakan dual partition scheme (OTA_0 dan OTA_1) untuk safe update:

```
OTA Process:
┌──────────────┐     ┌──────────────┐
│  OTA_0       │     │  OTA_1       │
│  (running)   │     │  (empty)     │
│  v1.0        │     │              │
└──────┬───────┘     └──────────────┘
       │  Download new firmware
       ▼                    │
┌──────────────┐     ┌──────┴───────┐
│  OTA_0       │     │  OTA_1       │
│  (old)       │     │  (new v1.1)  │
│  v1.0        │     │  ← Running!  │
└──────────────┘     └──────────────┘
       │  If v1.1 fails → rollback to v1.0
```

**Implementasi OTA HTTP:**

```c
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

void ota_update_task(void *param)
{
    const char *url = "https://server.com/firmware.bin";
    
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = server_cert_pem,  /* TLS certificate */
    };
    
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    
    printf("Starting OTA update from %s\n", url);
    esp_err_t ret = esp_https_ota(&ota_config);
    
    if (ret == ESP_OK) {
        printf("OTA Success! Restarting...\n");
        esp_restart();
    } else {
        printf("OTA Failed: %s\n", esp_err_to_name(ret));
    }
    vTaskDelete(NULL);
}

/* Validasi firmware setelah boot */
void validate_ota(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    esp_ota_get_state_partition(running, &state);
    
    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        /* Firmware baru — lakukan self-test */
        if (self_test_passed()) {
            esp_ota_mark_app_valid_cancel_rollback();
            printf("Firmware validated!\n");
        } else {
            printf("Self-test failed, rolling back...\n");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }
}
```

### 14.2 Bootloader dan Booting Process

Proses boot pada mikrokontroler:

```
STM32 Boot Process:
┌────────────────────────────────────────────────┐
│ 1. Reset → Check BOOT pins                    │
│    BOOT0=0 → Boot from Flash (normal)          │
│    BOOT0=1 → Boot from System Memory (DFU)     │
│                                                │
│ 2. Vector Table → Stack Pointer (0x08000000)   │
│ 3. Reset Handler → SystemInit() → main()       │
│                                                │
│ Custom Bootloader (optional):                  │
│    0x08000000: Bootloader (checks for update)  │
│    0x08004000: Application (user code)         │
└────────────────────────────────────────────────┘

ESP32 Boot Process:
┌────────────────────────────────────────────────┐
│ 1. ROM Code (1st stage bootloader)             │
│ 2. 2nd Stage Bootloader (0x1000)               │
│    → Reads partition table (0x8000)             │
│    → Selects app partition (factory/OTA)        │
│ 3. Application startup                         │
│    → ESP-IDF init → FreeRTOS → app_main()      │
└────────────────────────────────────────────────┘
```

---

## 15. Daftar Percobaan

### ESP32 (12 Percobaan — Native WiFi/BLE):

| No | Program | Konsep Utama |
|----|---------|-------------|
| 01 | WiFi_Scan | Scan AP, RSSI, channel, auth mode |
| 02 | WiFi_Station | Connect to AP, event handler, DHCP |
| 03 | WiFi_Access_Point | Soft-AP, DHCP server, client monitor |
| 04 | TCP_Client_Server | TCP socket, send/receive, echo |
| 05 | UDP_Communication | UDP datagram, broadcast |
| 06 | HTTP_Server | Web server, REST API, HTML page |
| 07 | HTTP_Client | HTTP GET, parse response |
| 08 | MQTT_Pub_Sub | MQTT publish/subscribe, QoS |
| 09 | BLE_Advertising | BLE GAP advertising, scan response |
| 10 | BLE_GATT_Server | GATT service/characteristic, notify |
| 11 | WebSocket_Server | Real-time bidirectional communication |
| 12 | IoT_Dashboard | Full IoT: sensor → MQTT → dashboard |

### STM32 (12 Percobaan — Via Modul External):

| No | Program | Hardware Tambahan | Konsep Utama |
|----|---------|------------------|-------------|
| 01 | UART_AT_Command | ESP-01 | AT command interface |
| 02 | ESP01_WiFi_Connect | ESP-01 + Router | WiFi via AT commands |
| 03 | ESP01_TCP_Client | ESP-01 | TCP connection via AT |
| 04 | ESP01_HTTP_GET | ESP-01 | HTTP GET via AT commands |
| 05 | W5500_Ethernet_Init | W5500 (SPI) | Ethernet init, IP config |
| 06 | W5500_TCP_Server | W5500 | TCP server via Ethernet |
| 07 | W5500_UDP | W5500 | UDP via Ethernet |
| 08 | W5500_HTTP_Server | W5500 | HTTP server via Ethernet |
| 09 | UART_Bridge_ESP32 | ESP32 | Custom UART protocol bridge |
| 10 | BLE_HM10 | HM-10 | BLE via UART AT commands |
| 11 | ESP01_MQTT | ESP-01 + Broker | MQTT via AT commands |
| 12 | IoT_Sensor_Gateway | ESP-01/W5500 + Sensor | Full IoT gateway |

---

## Referensi

1. Kolban, N. *Kolban's Book on ESP32*. Pages 138-250 (WiFi, TCP/UDP, HTTP, BLE)
2. Kolban, N. *Kolban's Book on ESP32*. Pages 454-475 (MQTT, mDNS)
3. ESP-IDF Programming Guide: https://docs.espressif.com/projects/esp-idf/
4. MQTT Specification: https://mqtt.org/mqtt-specification/
5. Bluetooth SIG: https://www.bluetooth.com/specifications/
6. W5500 Datasheet: https://www.wiznet.io/product-item/w5500/
7. HM-10 Datasheet: http://www.jnhuamao.cn/bluetooth.asp
8. de Oliveira, C. *Mastering STM32*. 2nd Edition.
