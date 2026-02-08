# Referensi Modul 13: Network & IoT

## Buku Referensi Utama

1. **Kolban, N.** *Kolban's Book on ESP32*
   - Pages 138-199: WiFi — Station/AP mode, scanning, event handler, IP assignment
   - Pages 170-190: TCP/UDP socket programming, lwIP, BSD sockets
   - Pages 190-199: HTTP server (httpd_start), HTTP client (esp_http_client)
   - Pages 200-250: Bluetooth Classic & BLE — GATT, GAP, advertising, scanning
   - Pages 454-470: MQTT — esp_mqtt_client, publish/subscribe, QoS, LWT
   - Pages 470-475: mDNS — hostname, service advertisement

2. **de Oliveira, C.** *Mastering STM32*. 2nd Edition
   - UART/USART communication (untuk AT commands ke ESP-01/HM-10)
   - SPI communication (untuk W5500 Ethernet)
   - DMA dan interrupt untuk komunikasi efisien

## Dokumentasi Online

### ESP-IDF (Espressif)
- ESP-IDF Programming Guide: https://docs.espressif.com/projects/esp-idf/en/stable/
- WiFi API: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html
- TCP/IP Adapter: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_netif.html
- HTTP Server: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html
- HTTP Client: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_client.html
- MQTT: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/mqtt.html
- BLE: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/index.html

### STM32 (STMicroelectronics)
- STM32CubeF1 HAL Reference: https://www.st.com/resource/en/user_manual/um1850.pdf
- STM32F103 Reference Manual: https://www.st.com/resource/en/reference_manual/rm0008.pdf
- STM32F103 Datasheet: https://www.st.com/resource/en/datasheet/stm32f103c8.pdf

### Modul External
- ESP8266 AT Command Set: https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/
- W5500 Datasheet: https://docs.wiznet.io/Product/iEthernet/W5500/datasheet
- HM-10 BLE Module: http://www.jnhuamao.cn/bluetooth.asp

### Protokol
- MQTT v3.1.1 Specification: https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
- Bluetooth Core Specification: https://www.bluetooth.com/specifications/specs/core-specification/
- HTTP/1.1 (RFC 7230-7235): https://datatracker.ietf.org/doc/html/rfc7230
- WebSocket (RFC 6455): https://datatracker.ietf.org/doc/html/rfc6455

## Tools & Software

| Tool | Fungsi | Link |
|------|--------|------|
| PlatformIO | IDE embedded development | https://platformio.org |
| Mosquitto | MQTT broker | https://mosquitto.org |
| MQTT Explorer | MQTT GUI client | https://mqtt-explorer.com |
| nRF Connect | BLE scanner/client | Google Play / App Store |
| Postman | HTTP API testing | https://www.postman.com |
| Wireshark | Network packet analyzer | https://www.wireshark.org |
| curl | HTTP command-line tool | Built-in Linux/Mac |
| netcat (nc) | TCP/UDP command-line tool | Built-in Linux/Mac |
| Python paho-mqtt | MQTT Python library | `pip install paho-mqtt` |
| Python requests | HTTP Python library | `pip install requests` |
| Python bleak | BLE Python library | `pip install bleak` |
