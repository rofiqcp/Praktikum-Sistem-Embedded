#!/usr/bin/env python3
"""
STM32_11_ESP01_MQTT - MQTT Broker Monitor & Packet Decoder
============================================================
Tools:
1. MQTT packet encoder/decoder (raw bytes)
2. MQTT broker monitor (subscribe and display)
3. ESP-01 AT command simulator
4. Protocol analysis and visualization

Usage:
  python debug_analysis.py --decode HEX       # Decode MQTT packet hex
  python debug_analysis.py --monitor          # Monitor MQTT broker
  python debug_analysis.py --analyze          # Analyze MQTT protocol
  python debug_analysis.py --build            # Build sample MQTT packets
"""

import struct
import time
import sys
import argparse
from datetime import datetime

# MQTT Packet Types
MQTT_TYPES = {
    0x10: "CONNECT",
    0x20: "CONNACK",
    0x30: "PUBLISH",
    0x40: "PUBACK",
    0x50: "PUBREC",
    0x60: "PUBREL",
    0x70: "PUBCOMP",
    0x80: "SUBSCRIBE",
    0x82: "SUBSCRIBE",
    0x90: "SUBACK",
    0xA0: "UNSUBSCRIBE",
    0xB0: "UNSUBACK",
    0xC0: "PINGREQ",
    0xD0: "PINGRESP",
    0xE0: "DISCONNECT",
}

def encode_remaining_length(length):
    """Encode MQTT remaining length field."""
    encoded = bytearray()
    while True:
        byte = length % 128
        length = length // 128
        if length > 0:
            byte |= 0x80
        encoded.append(byte)
        if length == 0:
            break
    return bytes(encoded)

def decode_remaining_length(data, offset=1):
    """Decode MQTT remaining length field. Returns (value, bytes_consumed)."""
    multiplier = 1
    value = 0
    idx = offset
    while idx < len(data):
        encoded_byte = data[idx]
        value += (encoded_byte & 127) * multiplier
        multiplier *= 128
        idx += 1
        if (encoded_byte & 128) == 0:
            break
    return value, idx - offset

def build_connect_packet(client_id, keepalive=60):
    """Build MQTT CONNECT packet."""
    # Variable header
    var_header = bytearray()
    var_header.extend(b'\x00\x04MQTT')  # Protocol name
    var_header.append(0x04)              # Protocol level (3.1.1)
    var_header.append(0x02)              # Flags: clean session
    var_header.extend(struct.pack(">H", keepalive))  # Keep alive

    # Payload
    payload = bytearray()
    client_bytes = client_id.encode("utf-8")
    payload.extend(struct.pack(">H", len(client_bytes)))
    payload.extend(client_bytes)

    remaining = bytes(var_header) + bytes(payload)
    packet = bytearray([0x10])
    packet.extend(encode_remaining_length(len(remaining)))
    packet.extend(remaining)
    return bytes(packet)

def build_publish_packet(topic, payload_str, qos=0, retain=False, packet_id=None):
    """Build MQTT PUBLISH packet."""
    flags = 0x30
    if retain:
        flags |= 0x01
    flags |= (qos << 1)

    remaining = bytearray()
    topic_bytes = topic.encode("utf-8")
    remaining.extend(struct.pack(">H", len(topic_bytes)))
    remaining.extend(topic_bytes)

    if qos > 0 and packet_id is not None:
        remaining.extend(struct.pack(">H", packet_id))

    remaining.extend(payload_str.encode("utf-8"))

    packet = bytearray([flags])
    packet.extend(encode_remaining_length(len(remaining)))
    packet.extend(remaining)
    return bytes(packet)

def build_subscribe_packet(topic, packet_id=1, qos=0):
    """Build MQTT SUBSCRIBE packet."""
    remaining = bytearray()
    remaining.extend(struct.pack(">H", packet_id))

    topic_bytes = topic.encode("utf-8")
    remaining.extend(struct.pack(">H", len(topic_bytes)))
    remaining.extend(topic_bytes)
    remaining.append(qos)

    packet = bytearray([0x82])
    packet.extend(encode_remaining_length(len(remaining)))
    packet.extend(remaining)
    return bytes(packet)

def build_pingreq():
    """Build MQTT PINGREQ packet."""
    return b'\xC0\x00'

def decode_mqtt_packet(data):
    """Decode an MQTT packet from raw bytes."""
    if not data or len(data) < 2:
        return {"error": "Packet too short"}

    packet_type = data[0] & 0xF0
    flags = data[0] & 0x0F

    type_name = MQTT_TYPES.get(data[0] & 0xF0, MQTT_TYPES.get(data[0], f"UNKNOWN(0x{data[0]:02X})"))

    remaining_len, rl_bytes = decode_remaining_length(data, 1)
    header_len = 1 + rl_bytes

    result = {
        "type": type_name,
        "type_byte": data[0],
        "flags": flags,
        "remaining_length": remaining_len,
        "header_length": header_len,
        "total_length": header_len + remaining_len,
        "hex": " ".join(f"{b:02X}" for b in data),
    }

    payload_start = header_len

    if packet_type == 0x10:  # CONNECT
        if len(data) > payload_start + 6:
            proto_len = struct.unpack(">H", data[payload_start:payload_start + 2])[0]
            proto_name = data[payload_start + 2:payload_start + 2 + proto_len].decode("utf-8", errors="replace")
            proto_level = data[payload_start + 2 + proto_len]
            conn_flags = data[payload_start + 3 + proto_len]
            keepalive = struct.unpack(">H", data[payload_start + 4 + proto_len:payload_start + 6 + proto_len])[0]

            var_end = payload_start + 6 + proto_len
            if len(data) > var_end + 2:
                cid_len = struct.unpack(">H", data[var_end:var_end + 2])[0]
                client_id = data[var_end + 2:var_end + 2 + cid_len].decode("utf-8", errors="replace")
                result["client_id"] = client_id

            result["protocol"] = f"{proto_name} v{proto_level}"
            result["keepalive"] = keepalive
            result["clean_session"] = bool(conn_flags & 0x02)

    elif packet_type == 0x30:  # PUBLISH
        if len(data) > payload_start + 2:
            topic_len = struct.unpack(">H", data[payload_start:payload_start + 2])[0]
            topic = data[payload_start + 2:payload_start + 2 + topic_len].decode("utf-8", errors="replace")
            qos = (flags >> 1) & 0x03
            msg_start = payload_start + 2 + topic_len
            if qos > 0:
                msg_start += 2
            message = data[msg_start:header_len + remaining_len].decode("utf-8", errors="replace")
            result["topic"] = topic
            result["qos"] = qos
            result["retain"] = bool(flags & 0x01)
            result["message"] = message

    elif data[0] == 0x82:  # SUBSCRIBE
        if len(data) > payload_start + 4:
            pkt_id = struct.unpack(">H", data[payload_start:payload_start + 2])[0]
            topic_len = struct.unpack(">H", data[payload_start + 2:payload_start + 4])[0]
            topic = data[payload_start + 4:payload_start + 4 + topic_len].decode("utf-8", errors="replace")
            sub_qos = data[payload_start + 4 + topic_len] if len(data) > payload_start + 4 + topic_len else 0
            result["packet_id"] = pkt_id
            result["topic"] = topic
            result["qos"] = sub_qos

    return result

def analyze_protocol():
    """Demonstrate MQTT protocol analysis."""
    print("=" * 60)
    print("  MQTT Protocol Analysis")
    print("=" * 60)

    # Build and decode sample packets
    packets = [
        ("CONNECT", build_connect_packet("stm32_iot_01", 60)),
        ("PUBLISH", build_publish_packet("stm32/sensor/data", '{"temp":25.5,"hum":60}')),
        ("SUBSCRIBE", build_subscribe_packet("stm32/cmd/#", 1)),
        ("PINGREQ", build_pingreq()),
    ]

    for name, pkt_bytes in packets:
        print(f"\n--- {name} Packet ---")
        print(f"  Raw ({len(pkt_bytes)} bytes): {' '.join(f'{b:02X}' for b in pkt_bytes)}")

        decoded = decode_mqtt_packet(pkt_bytes)
        print(f"  Type     : {decoded['type']}")
        print(f"  Flags    : 0x{decoded['flags']:01X}")
        print(f"  Rem.Len  : {decoded['remaining_length']}")

        if "client_id" in decoded:
            print(f"  Client ID: {decoded['client_id']}")
            print(f"  Protocol : {decoded['protocol']}")
            print(f"  Keepalive: {decoded['keepalive']}s")
            print(f"  Clean Ses: {decoded['clean_session']}")

        if "topic" in decoded:
            print(f"  Topic    : {decoded['topic']}")
        if "message" in decoded:
            print(f"  Message  : {decoded['message']}")
            print(f"  QoS      : {decoded.get('qos', 0)}")
            print(f"  Retain   : {decoded.get('retain', False)}")
        if "packet_id" in decoded:
            print(f"  Packet ID: {decoded['packet_id']}")

    # Show MQTT flow
    print(f"\n\n{'=' * 60}")
    print("  MQTT Connection Flow (STM32 -> ESP-01 -> Broker)")
    print(f"{'=' * 60}")
    flow = [
        ("STM32", "ESP-01", "AT+RST",                        "Reset ESP-01"),
        ("STM32", "ESP-01", 'AT+CWJAP="SSID","PASS"',        "Connect WiFi"),
        ("STM32", "ESP-01", 'AT+CIPSTART="TCP","broker",1883',"Open TCP"),
        ("STM32", "ESP-01", "AT+CIPSEND=N",                   "Send MQTT CONNECT"),
        ("ESP-01","Broker", "[CONNECT packet bytes]",          "-> broker"),
        ("Broker","ESP-01", "[CONNACK packet]",                "<- broker"),
        ("STM32", "ESP-01", "AT+CIPSEND=N",                   "Send MQTT SUBSCRIBE"),
        ("STM32", "ESP-01", "AT+CIPSEND=N",                   "Send MQTT PUBLISH"),
        ("STM32", "ESP-01", "AT+CIPSEND=2",                   "Send MQTT PINGREQ"),
    ]
    for src, dst, data, desc in flow:
        print(f"  {src:8s} -> {dst:8s} : {data:38s} [{desc}]")

    # ESP-01 AT command sequence
    print(f"\n\n{'=' * 60}")
    print("  ESP-01 AT Command Quick Reference")
    print(f"{'=' * 60}")
    at_cmds = [
        ("AT",                         "Test"),
        ("AT+RST",                     "Reset module"),
        ("AT+CWMODE=1",                "Station mode"),
        ('AT+CWJAP="ssid","pass"',     "Join WiFi"),
        ("AT+CIFSR",                   "Get IP address"),
        ('AT+CIPSTART="TCP","host",port', "Open TCP"),
        ("AT+CIPSEND=len",             "Send data (wait for >)"),
        ("AT+CIPCLOSE",                "Close connection"),
    ]
    for cmd, desc in at_cmds:
        print(f"  {cmd:38s} {desc}")

def monitor_broker(broker="test.mosquitto.org", port=1883, topic="stm32/#"):
    """Monitor MQTT broker for messages."""
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        print("ERROR: paho-mqtt not installed. Run: pip install paho-mqtt")
        print("\nRunning demo mode...\n")
        monitor_demo()
        return

    def on_connect(client, userdata, flags, rc):
        codes = {0: "OK", 1: "Bad protocol", 2: "Client ID rejected",
                 3: "Server unavailable", 4: "Bad credentials", 5: "Not authorized"}
        print(f"[{datetime.now().strftime('%H:%M:%S')}] Connected: {codes.get(rc, f'code={rc}')}")
        client.subscribe(topic)
        print(f"Subscribed to: {topic}\n")

    def on_message(client, userdata, msg):
        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        try:
            payload = msg.payload.decode("utf-8")
        except UnicodeDecodeError:
            payload = msg.payload.hex()
        print(f"[{ts}] {msg.topic}: {payload}")

    client = mqtt.Client(client_id="stm32_monitor_" + str(int(time.time()) % 10000))
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"Connecting to {broker}:{port}...")
    try:
        client.connect(broker, port, 60)
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nDisconnecting...")
        client.disconnect()
    except Exception as e:
        print(f"ERROR: {e}")
        monitor_demo()

def monitor_demo():
    """Demo MQTT monitoring output."""
    print("=== MQTT Monitor Demo ===\n")
    print(f"Broker: test.mosquitto.org:1883")
    print(f"Topic:  stm32/#\n")

    messages = [
        ("stm32/sensor/data", '{"id":"stm32_iot_01","temp":25.5,"hum":60.0,"seq":0}'),
        ("stm32/sensor/data", '{"id":"stm32_iot_01","temp":25.8,"hum":60.5,"seq":1}'),
        ("stm32/sensor/data", '{"id":"stm32_iot_01","temp":26.1,"hum":61.0,"seq":2}'),
        ("stm32/cmd/led",     '{"action":"toggle"}'),
        ("stm32/sensor/data", '{"id":"stm32_iot_01","temp":26.4,"hum":61.5,"seq":3}'),
    ]

    for topic, payload in messages:
        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        print(f"[{ts}] {topic}: {payload}")
        time.sleep(0.5)

def decode_hex(hex_str):
    """Decode MQTT packet from hex string."""
    hex_str = hex_str.replace(" ", "").replace("0x", "").replace(",", "")
    try:
        data = bytes.fromhex(hex_str)
    except ValueError as e:
        print(f"ERROR: Invalid hex: {e}")
        return

    decoded = decode_mqtt_packet(data)
    if "error" in decoded:
        print(f"Error: {decoded['error']}")
    else:
        print(f"Type       : {decoded['type']}")
        print(f"Flags      : 0x{decoded['flags']:01X}")
        print(f"Total Len  : {decoded['total_length']} bytes")
        for key in ["client_id", "protocol", "keepalive", "clean_session",
                     "topic", "message", "qos", "retain", "packet_id"]:
            if key in decoded:
                print(f"{key:11s}: {decoded[key]}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MQTT Protocol Analyzer & Monitor")
    parser.add_argument("--analyze", action="store_true", help="Analyze MQTT protocol")
    parser.add_argument("--monitor", action="store_true", help="Monitor MQTT broker")
    parser.add_argument("--decode", type=str, metavar="HEX", help="Decode MQTT packet hex")
    parser.add_argument("--build", action="store_true", help="Build sample MQTT packets")
    args = parser.parse_args()

    if args.decode:
        decode_hex(args.decode)
    elif args.monitor:
        monitor_broker()
    elif args.build:
        analyze_protocol()
    elif args.analyze:
        analyze_protocol()
    else:
        analyze_protocol()
