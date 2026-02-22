#!/usr/bin/env python3
"""
============================================================================
ESP32_08_MQTT_Pub_Sub - Debug & Analysis Tool
Modul 13 - Network & IoT
============================================================================

Script ini menyediakan:
1. MQTT Monitor       - Subscribe dan monitor semua pesan dari ESP32
2. MQTT Commander     - Kirim perintah ke ESP32 via MQTT
3. Data Logger        - Catat data sensor ke file JSON
4. Visualisasi        - Real-time dan post-hoc chart sensor data

Dependency:
  pip install paho-mqtt matplotlib

Cara Pakai:
  python debug_analysis.py --mode monitor
  python debug_analysis.py --mode command --topic esp32/led --message "on"
  python debug_analysis.py --mode analyze --logfile mqtt_log.json

============================================================================
"""

import json
import time
import argparse
import sys
import os
from datetime import datetime

# Paho MQTT client
try:
    import paho.mqtt.client as mqtt
    HAS_PAHO = True
except ImportError:
    HAS_PAHO = False
    print("[WARN] paho-mqtt tidak tersedia. Install: pip install paho-mqtt")

# Matplotlib opsional
try:
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


# Default broker (sama dengan yang di ESP32)
DEFAULT_BROKER = "test.mosquitto.org"
DEFAULT_PORT = 1883

# Topics yang di-monitor
SUBSCRIBE_TOPICS = [
    ("esp32/sensor/data", 0),
    ("esp32/status", 0),
    ("esp32/command/#", 0),
    ("esp32/led", 0),
]


class MQTTMonitor:
    """Monitor dan logger untuk pesan MQTT dari ESP32"""

    def __init__(self, broker=DEFAULT_BROKER, port=DEFAULT_PORT):
        if not HAS_PAHO:
            print("[ERROR] Install paho-mqtt: pip install paho-mqtt")
            sys.exit(1)

        self.broker = broker
        self.port = port
        self.messages = []
        self.sensor_data = []  # Khusus data sensor untuk plotting
        self.running = True

        # Buat MQTT client
        self.client = mqtt.Client(client_id="python_monitor_modul13",
                                  protocol=mqtt.MQTTv311)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect

        print(f"[MQTT Monitor] Broker: {broker}:{port}")

    def _on_connect(self, client, userdata, flags, rc):
        """Callback saat berhasil konek ke broker"""
        if rc == 0:
            print(f"\n[CONNECTED] Broker: {self.broker}:{self.port}")
            # Subscribe ke semua topics
            for topic, qos in SUBSCRIBE_TOPICS:
                client.subscribe(topic, qos)
                print(f"  Subscribed: {topic} (QoS={qos})")
            print()
        else:
            print(f"[ERROR] Connection failed, rc={rc}")

    def _on_message(self, client, userdata, msg):
        """Callback saat menerima pesan MQTT"""
        timestamp = datetime.now().isoformat()
        payload = msg.payload.decode(errors='replace')

        # Log pesan
        entry = {
            "timestamp": timestamp,
            "topic": msg.topic,
            "payload": payload,
            "qos": msg.qos,
            "retain": msg.retain,
            "size": len(msg.payload)
        }
        self.messages.append(entry)

        # Display
        qos_str = f"QoS{msg.qos}"
        retain_str = " [RETAINED]" if msg.retain else ""
        print(f"[{timestamp[11:19]}] {msg.topic} ({qos_str}{retain_str})")
        print(f"  Payload: {payload}")

        # Parse sensor data jika topic sensor
        if "sensor/data" in msg.topic:
            try:
                data = json.loads(payload)
                data["_timestamp"] = timestamp
                self.sensor_data.append(data)
                print(f"  >> Temp: {data.get('temp','?')}°C | "
                      f"Hum: {data.get('hum','?')}% | "
                      f"Light: {data.get('light','?')} | "
                      f"Heap: {data.get('heap','?')}")
            except json.JSONDecodeError:
                pass
        print()

    def _on_disconnect(self, client, userdata, rc):
        """Callback saat disconnect"""
        if rc != 0:
            print(f"[DISCONNECTED] Unexpected (rc={rc}), reconnecting...")
        else:
            print("[DISCONNECTED] Clean disconnect")

    def start_monitor(self, duration=300, logfile="mqtt_log.json"):
        """Mulai monitoring MQTT messages"""
        print(f"\n{'='*65}")
        print(f"  MQTT Monitor - Listening for ESP32 Messages")
        print(f"  Duration: {duration}s | Log: {logfile}")
        print(f"{'='*65}\n")

        try:
            self.client.connect(self.broker, self.port, keepalive=60)
        except Exception as e:
            print(f"[ERROR] Cannot connect to broker: {e}")
            return

        self.client.loop_start()

        try:
            start = time.time()
            while self.running and (time.time() - start) < duration:
                time.sleep(1)
                elapsed = int(time.time() - start)
                if elapsed % 30 == 0 and elapsed > 0:
                    print(f"--- {elapsed}/{duration}s | "
                          f"Messages: {len(self.messages)} | "
                          f"Sensor readings: {len(self.sensor_data)} ---\n")
        except KeyboardInterrupt:
            print("\n[STOP] Ctrl+C received")

        self.client.loop_stop()
        self.client.disconnect()

        # Simpan log
        log_data = {
            "broker": self.broker,
            "duration": duration,
            "total_messages": len(self.messages),
            "messages": self.messages,
            "sensor_data": self.sensor_data
        }
        with open(logfile, 'w') as f:
            json.dump(log_data, f, indent=2)
        print(f"\n[LOG] Saved {len(self.messages)} messages to {logfile}")

        # Analisis dan plot
        if self.sensor_data:
            self._analyze_sensor_data()

    def _analyze_sensor_data(self):
        """Analisis statistik data sensor"""
        temps = [d["temp"] for d in self.sensor_data if "temp" in d]
        hums = [d["hum"] for d in self.sensor_data if "hum" in d]
        heaps = [d["heap"] for d in self.sensor_data if "heap" in d]

        print(f"\n{'='*65}")
        print(f"  Sensor Data Analysis ({len(self.sensor_data)} readings)")
        print(f"{'='*65}")

        if temps:
            print(f"  Temperature: {min(temps):.1f} - {max(temps):.1f}°C "
                  f"(avg: {sum(temps)/len(temps):.1f}°C)")
        if hums:
            print(f"  Humidity   : {min(hums):.1f} - {max(hums):.1f}% "
                  f"(avg: {sum(hums)/len(hums):.1f}%)")
        if heaps:
            print(f"  Heap Memory: {min(heaps)/1024:.1f} - {max(heaps)/1024:.1f} KB")

        # Analisis QoS distribution
        qos_counts = {0: 0, 1: 0, 2: 0}
        for msg in self.messages:
            qos_counts[msg.get("qos", 0)] = qos_counts.get(msg.get("qos", 0), 0) + 1
        print(f"  QoS Distribution: QoS0={qos_counts[0]} | QoS1={qos_counts[1]} | QoS2={qos_counts[2]}")

        # Topic distribution
        topic_counts = {}
        for msg in self.messages:
            t = msg["topic"]
            topic_counts[t] = topic_counts.get(t, 0) + 1
        print(f"  Topics:")
        for t, c in sorted(topic_counts.items()):
            print(f"    {t}: {c} messages")
        print(f"{'='*65}\n")

        if HAS_MATPLOTLIB:
            self._plot_sensor_data()

    def _plot_sensor_data(self):
        """Visualisasi data sensor MQTT"""
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle("ESP32 MQTT Sensor Data Analysis", fontsize=14, fontweight='bold')

        temps = [d.get("temp", 0) for d in self.sensor_data]
        hums = [d.get("hum", 0) for d in self.sensor_data]
        lights = [d.get("light", 0) for d in self.sensor_data]
        heaps = [d.get("heap", 0) for d in self.sensor_data]
        seqs = [d.get("seq", i+1) for i, d in enumerate(self.sensor_data)]

        # Plot 1: Temperature
        ax1 = axes[0][0]
        ax1.plot(seqs, temps, 'r-o', markersize=4, label="Temperature")
        ax1.set_xlabel("Sequence")
        ax1.set_ylabel("Temperature (°C)")
        ax1.set_title("Temperature Over Time")
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # Plot 2: Humidity
        ax2 = axes[0][1]
        ax2.plot(seqs, hums, 'b-s', markersize=4, label="Humidity")
        ax2.set_xlabel("Sequence")
        ax2.set_ylabel("Humidity (%)")
        ax2.set_title("Humidity Over Time")
        ax2.legend()
        ax2.grid(True, alpha=0.3)

        # Plot 3: Light level
        ax3 = axes[1][0]
        ax3.bar(seqs, lights, color='#f39c12', alpha=0.7, label="Light (ADC)")
        ax3.set_xlabel("Sequence")
        ax3.set_ylabel("Light Level (0-4095)")
        ax3.set_title("Light Sensor Readings")
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # Plot 4: Heap memory
        ax4 = axes[1][1]
        ax4.plot(seqs, [h/1024 for h in heaps], 'g-^', markersize=4, label="Free Heap")
        ax4.set_xlabel("Sequence")
        ax4.set_ylabel("Free Heap (KB)")
        ax4.set_title("ESP32 Memory Usage")
        ax4.legend()
        ax4.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig("mqtt_analysis.png", dpi=150)
        print("[PLOT] Saved: mqtt_analysis.png")
        plt.show()


class MQTTCommander:
    """Kirim perintah ke ESP32 via MQTT"""

    def __init__(self, broker=DEFAULT_BROKER, port=DEFAULT_PORT):
        if not HAS_PAHO:
            print("[ERROR] Install paho-mqtt: pip install paho-mqtt")
            sys.exit(1)

        self.client = mqtt.Client(client_id="python_commander_modul13")
        self.broker = broker
        self.port = port

    def send_command(self, topic, message, qos=1):
        """Kirim satu pesan MQTT ke topic tertentu"""
        try:
            self.client.connect(self.broker, self.port)
            result = self.client.publish(topic, message, qos=qos)
            result.wait_for_publish()
            print(f"[SENT] Topic: {topic} | Message: {message} | QoS: {qos}")
            self.client.disconnect()
        except Exception as e:
            print(f"[ERROR] {e}")

    def interactive_mode(self):
        """Mode interaktif - ketik perintah dari terminal"""
        self.client.connect(self.broker, self.port)
        self.client.loop_start()

        print(f"\n{'='*65}")
        print(f"  MQTT Interactive Commander")
        print(f"  Broker: {self.broker}:{self.port}")
        print(f"  Commands: 'led on', 'led off', 'q' to quit")
        print(f"  Or type: <topic> <message>")
        print(f"{'='*65}\n")

        try:
            while True:
                cmd = input("MQTT> ").strip()
                if not cmd:
                    continue
                if cmd.lower() in ('q', 'quit', 'exit'):
                    break

                if cmd == "led on":
                    self.client.publish("esp32/led", "on", qos=1)
                    print("  -> LED ON command sent")
                elif cmd == "led off":
                    self.client.publish("esp32/led", "off", qos=1)
                    print("  -> LED OFF command sent")
                else:
                    parts = cmd.split(maxsplit=1)
                    if len(parts) == 2:
                        topic, msg = parts
                        self.client.publish(topic, msg, qos=1)
                        print(f"  -> Published to {topic}")
                    else:
                        print("  Usage: <topic> <message> or 'led on/off'")
        except (KeyboardInterrupt, EOFError):
            pass

        self.client.loop_stop()
        self.client.disconnect()
        print("\n[DONE] Commander disconnected")


def analyze_logfile(logfile):
    """Analisis file log MQTT yang sudah direkam"""
    if not os.path.exists(logfile):
        print(f"[ERROR] File not found: {logfile}")
        return

    with open(logfile, 'r') as f:
        data = json.load(f)

    sensor_data = data.get("sensor_data", [])
    messages = data.get("messages", [])

    print(f"\n{'='*65}")
    print(f"  MQTT Log Analysis: {logfile}")
    print(f"  Total Messages: {len(messages)}")
    print(f"  Sensor Readings: {len(sensor_data)}")
    print(f"{'='*65}\n")

    if not sensor_data:
        print("[INFO] No sensor data to analyze")
        return

    if not HAS_MATPLOTLIB:
        print("[SKIP] Install matplotlib for visualization")
        return

    # Reuse plot logic
    monitor = MQTTMonitor.__new__(MQTTMonitor)
    monitor.sensor_data = sensor_data
    monitor.messages = messages
    monitor._analyze_sensor_data()


def main():
    parser = argparse.ArgumentParser(
        description="ESP32 MQTT Pub/Sub - Debug & Analysis Tool")
    parser.add_argument("--mode", choices=["monitor", "command", "interactive", "analyze"],
                        default="monitor", help="Mode operasi")
    parser.add_argument("--broker", default=DEFAULT_BROKER,
                        help="MQTT broker address")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help="MQTT broker port")
    parser.add_argument("--duration", type=int, default=300,
                        help="Monitor duration (seconds)")
    parser.add_argument("--topic", default="esp32/led",
                        help="MQTT topic (mode=command)")
    parser.add_argument("--message", default="on",
                        help="Message to send (mode=command)")
    parser.add_argument("--logfile", default="mqtt_log.json",
                        help="Log file path")
    args = parser.parse_args()

    if args.mode == "monitor":
        monitor = MQTTMonitor(args.broker, args.port)
        monitor.start_monitor(duration=args.duration, logfile=args.logfile)

    elif args.mode == "command":
        cmd = MQTTCommander(args.broker, args.port)
        cmd.send_command(args.topic, args.message)

    elif args.mode == "interactive":
        cmd = MQTTCommander(args.broker, args.port)
        cmd.interactive_mode()

    elif args.mode == "analyze":
        analyze_logfile(args.logfile)


if __name__ == "__main__":
    main()
