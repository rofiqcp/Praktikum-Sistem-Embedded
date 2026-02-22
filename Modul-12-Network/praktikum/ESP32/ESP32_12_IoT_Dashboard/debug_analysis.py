#!/usr/bin/env python3
"""
============================================================================
IoT Dashboard Viewer & Data Logger with Real-time Plot
============================================================================
Deskripsi:
  Script Python untuk memantau data dari ESP32 IoT Dashboard.
  Mendukung MQTT subscriber, HTTP API polling, dan real-time
  matplotlib plotting untuk visualisasi data sensor.

Dependensi:
  pip install paho-mqtt requests matplotlib numpy

Penggunaan:
  python debug_analysis.py --mqtt                          # MQTT subscriber
  python debug_analysis.py --http --ip 192.168.1.100       # HTTP API polling
  python debug_analysis.py --mqtt --plot                   # MQTT + real-time plot
  python debug_analysis.py --analyze                       # Analyze saved data
  python debug_analysis.py --control led_on                # Send MQTT command

Author: Praktikum Sistem Embedded
============================================================================
"""

import argparse
import json
import time
import sys
import threading
from datetime import datetime

try:
    import paho.mqtt.client as mqtt
    HAS_MQTT = True
except ImportError:
    HAS_MQTT = False
    print("[WARNING] 'paho-mqtt' not installed. Install: pip install paho-mqtt")

try:
    import requests
    HAS_REQUESTS = True
except ImportError:
    HAS_REQUESTS = False

try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] 'matplotlib/numpy' not installed. Plot features disabled.")

# ============================================================================
# Konfigurasi
# ============================================================================
MQTT_BROKER = "test.mosquitto.org"
MQTT_PORT = 1883
MQTT_TOPIC_DATA = "esp32/sensor/data"
MQTT_TOPIC_CONTROL = "esp32/control/cmd"
MQTT_TOPIC_STATUS = "esp32/status"

DATA_LOG_FILE = "iot_sensor_log.csv"
MAX_PLOT_POINTS = 100  # Max data points di real-time plot

# Data storage
data_store = {
    'timestamps': [],
    'temp': [],
    'hum': [],
    'light': [],
    'heap': [],
    'ids': [],
}
data_lock = threading.Lock()
start_time = time.time()


# ============================================================================
# MQTT Subscriber
# ============================================================================
class MQTTSubscriber:
    def __init__(self, broker=MQTT_BROKER, port=MQTT_PORT):
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.broker = broker
        self.port = port

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"[OK] Connected to MQTT broker: {self.broker}")
            client.subscribe(MQTT_TOPIC_DATA)
            client.subscribe(MQTT_TOPIC_STATUS)
            print(f"[OK] Subscribed to: {MQTT_TOPIC_DATA}")
            print(f"[OK] Subscribed to: {MQTT_TOPIC_STATUS}")
        else:
            print(f"[ERROR] MQTT connect failed, rc={rc}")

    def on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
            elapsed = time.time() - start_time

            if msg.topic == MQTT_TOPIC_DATA:
                with data_lock:
                    data_store['timestamps'].append(elapsed)
                    data_store['temp'].append(payload.get('temp', 0))
                    data_store['hum'].append(payload.get('hum', 0))
                    data_store['light'].append(payload.get('light', 0))
                    data_store['heap'].append(payload.get('heap', 0))
                    data_store['ids'].append(payload.get('id', 0))

                print(f"[{elapsed:>7.1f}s] #{payload.get('id',0):>4d} | "
                      f"Temp: {payload.get('temp',0):>5.1f}°C | "
                      f"Hum: {payload.get('hum',0):>5.1f}% | "
                      f"Light: {payload.get('light',0):>4d} | "
                      f"LED: {'ON' if payload.get('led') else 'OFF'}")

                # Log to CSV
                with open(DATA_LOG_FILE, 'a') as f:
                    f.write(f"{elapsed:.2f},{payload.get('id',0)},"
                            f"{payload.get('temp',0)},{payload.get('hum',0)},"
                            f"{payload.get('light',0)},{payload.get('heap',0)}\n")

            elif msg.topic == MQTT_TOPIC_STATUS:
                print(f"  [STATUS] {payload}")

        except json.JSONDecodeError:
            print(f"  [RAW] {msg.topic}: {msg.payload.decode()}")

    def start(self):
        # Init log file
        with open(DATA_LOG_FILE, 'w') as f:
            f.write("timestamp,id,temp,hum,light,heap\n")

        self.client.connect(self.broker, self.port, 60)
        self.client.loop_start()

    def stop(self):
        self.client.loop_stop()
        self.client.disconnect()

    def publish_command(self, command):
        self.client.publish(MQTT_TOPIC_CONTROL, command, qos=1)
        print(f"[TX] Sent command: {command} -> {MQTT_TOPIC_CONTROL}")


def mqtt_monitor(enable_plot=False):
    """Monitor data sensor via MQTT subscriber."""
    print(f"\n{'='*60}")
    print(f"  IoT MQTT Data Monitor")
    print(f"  Broker: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"  Topic:  {MQTT_TOPIC_DATA}")
    print(f"  Press Ctrl+C to stop")
    print(f"{'='*60}\n")

    sub = MQTTSubscriber()
    sub.start()

    if enable_plot and HAS_MATPLOTLIB:
        # Real-time plot
        fig, axes = plt.subplots(3, 1, figsize=(12, 8), sharex=True)
        fig.suptitle("ESP32 IoT Dashboard - Real-time Data", fontsize=13)

        lines_temp, = axes[0].plot([], [], 'r-', label='Temperature (°C)')
        lines_hum,  = axes[1].plot([], [], 'b-', label='Humidity (%)')
        lines_light, = axes[2].plot([], [], 'orange', label='Light Level')

        for ax in axes:
            ax.legend(loc='upper left')
            ax.grid(True, alpha=0.3)
            ax.set_xlim(0, 60)

        axes[0].set_ylim(15, 40)
        axes[0].set_ylabel('Temp (°C)')
        axes[1].set_ylim(30, 90)
        axes[1].set_ylabel('Humidity (%)')
        axes[2].set_ylim(0, 1100)
        axes[2].set_ylabel('Light')
        axes[2].set_xlabel('Time (seconds)')

        def update_plot(frame):
            with data_lock:
                if not data_store['timestamps']:
                    return lines_temp, lines_hum, lines_light

                t = data_store['timestamps'][-MAX_PLOT_POINTS:]
                temp = data_store['temp'][-MAX_PLOT_POINTS:]
                hum = data_store['hum'][-MAX_PLOT_POINTS:]
                light = data_store['light'][-MAX_PLOT_POINTS:]

            lines_temp.set_data(t, temp)
            lines_hum.set_data(t, hum)
            lines_light.set_data(t, light)

            if t:
                for ax in axes:
                    ax.set_xlim(max(0, t[-1] - 120), t[-1] + 5)

            return lines_temp, lines_hum, lines_light

        ani = animation.FuncAnimation(fig, update_plot, interval=1000, blit=True)

        try:
            plt.tight_layout()
            plt.show()
        except KeyboardInterrupt:
            pass
    else:
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            pass

    sub.stop()
    print_statistics()


def http_poll(ip_address, interval=5, duration=60):
    """Poll data dari ESP32 HTTP API endpoint."""
    if not HAS_REQUESTS:
        print("[ERROR] 'requests' library required. Install: pip install requests")
        return

    url = f"http://{ip_address}/api/data"
    print(f"\n{'='*60}")
    print(f"  HTTP API Polling")
    print(f"  URL: {url}")
    print(f"  Interval: {interval}s | Duration: {duration}s")
    print(f"{'='*60}\n")

    with open(DATA_LOG_FILE, 'w') as f:
        f.write("timestamp,id,temp,hum,light,heap\n")

    end_time = time.time() + duration

    try:
        while time.time() < end_time:
            try:
                resp = requests.get(url, timeout=3)
                data = resp.json()
                elapsed = time.time() - start_time

                print(f"[{elapsed:>6.1f}s] Temp: {data.get('temp',0):.1f}°C | "
                      f"Hum: {data.get('hum',0):.1f}% | "
                      f"Light: {data.get('light',0)} | "
                      f"LED: {'ON' if data.get('led') else 'OFF'} | "
                      f"Heap: {data.get('heap',0)}")

                with open(DATA_LOG_FILE, 'a') as f:
                    f.write(f"{elapsed:.2f},{data.get('reading',0)},"
                            f"{data.get('temp',0)},{data.get('hum',0)},"
                            f"{data.get('light',0)},{data.get('heap',0)}\n")

                data_store['timestamps'].append(elapsed)
                data_store['temp'].append(data.get('temp', 0))
                data_store['hum'].append(data.get('hum', 0))
                data_store['light'].append(data.get('light', 0))

            except requests.exceptions.RequestException as e:
                print(f"[ERROR] Request failed: {e}")

            time.sleep(interval)

    except KeyboardInterrupt:
        pass

    print_statistics()


def send_mqtt_command(command):
    """Kirim command ke ESP32 via MQTT."""
    if not HAS_MQTT:
        print("[ERROR] paho-mqtt required. Install: pip install paho-mqtt")
        return

    print(f"[INFO] Sending command: {command}")
    sub = MQTTSubscriber()
    sub.start()
    time.sleep(1)
    sub.publish_command(command)
    time.sleep(2)
    sub.stop()
    print("[OK] Command sent!")


def print_statistics():
    """Tampilkan statistik dari data yang sudah dikumpulkan."""
    with data_lock:
        if not data_store['temp']:
            print("\n[INFO] No data collected.")
            return

        print(f"\n{'='*60}")
        print(f"  Data Collection Statistics")
        print(f"{'='*60}")
        print(f"  Total samples  : {len(data_store['temp'])}")
        print(f"  Duration       : {data_store['timestamps'][-1]:.1f} seconds")
        print(f"  Temperature    : min={min(data_store['temp']):.1f} "
              f"max={max(data_store['temp']):.1f} "
              f"avg={sum(data_store['temp'])/len(data_store['temp']):.1f}°C")
        print(f"  Humidity       : min={min(data_store['hum']):.1f} "
              f"max={max(data_store['hum']):.1f} "
              f"avg={sum(data_store['hum'])/len(data_store['hum']):.1f}%")
        if data_store['light']:
            print(f"  Light          : min={min(data_store['light'])} "
                  f"max={max(data_store['light'])} "
                  f"avg={sum(data_store['light'])/len(data_store['light']):.0f}")
        print(f"  Data saved to  : {DATA_LOG_FILE}")


def analyze_saved_data():
    """Analisis dan plot data yang sudah disimpan."""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib required. Install: pip install matplotlib numpy")
        return

    try:
        data = np.genfromtxt(DATA_LOG_FILE, delimiter=',', skip_header=1)
        timestamps = data[:, 0]
        temp = data[:, 2]
        hum = data[:, 3]
        light = data[:, 4]
        heap = data[:, 5]
    except Exception as e:
        print(f"[ERROR] Cannot load {DATA_LOG_FILE}: {e}")
        return

    print(f"[INFO] Loaded {len(timestamps)} samples from {DATA_LOG_FILE}")

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle("ESP32 IoT Dashboard - Data Analysis", fontsize=14, fontweight='bold')

    # Temperature
    ax = axes[0][0]
    ax.plot(timestamps, temp, 'r-', linewidth=1, label='Temperature')
    ax.axhline(y=np.mean(temp), color='darkred', linestyle='--',
               label=f'Mean: {np.mean(temp):.1f}°C')
    ax.fill_between(timestamps, temp, alpha=0.2, color='red')
    ax.set_ylabel('Temperature (°C)')
    ax.set_title('Temperature Over Time')
    ax.legend()
    ax.grid(True, alpha=0.3)

    # Humidity
    ax = axes[0][1]
    ax.plot(timestamps, hum, 'b-', linewidth=1, label='Humidity')
    ax.axhline(y=np.mean(hum), color='darkblue', linestyle='--',
               label=f'Mean: {np.mean(hum):.1f}%')
    ax.fill_between(timestamps, hum, alpha=0.2, color='blue')
    ax.set_ylabel('Humidity (%)')
    ax.set_title('Humidity Over Time')
    ax.legend()
    ax.grid(True, alpha=0.3)

    # Light
    ax = axes[1][0]
    ax.plot(timestamps, light, 'orange', linewidth=1, label='Light')
    ax.axhline(y=np.mean(light), color='darkorange', linestyle='--',
               label=f'Mean: {np.mean(light):.0f}')
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('Light Level')
    ax.set_title('Light Level Over Time')
    ax.legend()
    ax.grid(True, alpha=0.3)

    # Free Heap Memory
    ax = axes[1][1]
    ax.plot(timestamps, heap / 1024, 'g-', linewidth=1, label='Free Heap')
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('Free Heap (KB)')
    ax.set_title('Memory Usage Over Time')
    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    filename = 'iot_dashboard_analysis.png'
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    print(f"[INFO] Plot saved to {filename}")
    plt.show()


def main():
    parser = argparse.ArgumentParser(description="IoT Dashboard Viewer & Data Logger")
    parser.add_argument('--mqtt', action='store_true', help='Monitor via MQTT subscriber')
    parser.add_argument('--http', action='store_true', help='Monitor via HTTP API polling')
    parser.add_argument('--ip', type=str, help='ESP32 IP address (for HTTP mode)')
    parser.add_argument('--plot', action='store_true', help='Enable real-time plotting')
    parser.add_argument('--analyze', action='store_true', help='Analyze saved data file')
    parser.add_argument('--control', type=str, help='Send MQTT command (led_on/led_off/status)')
    parser.add_argument('--duration', type=int, default=120, help='Monitor duration (seconds)')
    parser.add_argument('--broker', type=str, default=MQTT_BROKER, help='MQTT broker address')
    args = parser.parse_args()

    global MQTT_BROKER
    if args.broker:
        MQTT_BROKER = args.broker

    if args.analyze:
        analyze_saved_data()
    elif args.control:
        send_mqtt_command(args.control)
    elif args.http:
        if not args.ip:
            print("[ERROR] --ip required for HTTP mode")
            sys.exit(1)
        http_poll(args.ip, duration=args.duration)
        if data_store['temp'] and HAS_MATPLOTLIB:
            analyze_saved_data()
    elif args.mqtt:
        mqtt_monitor(enable_plot=args.plot)
    else:
        print("Usage: python debug_analysis.py --mqtt [--plot]")
        print("       python debug_analysis.py --http --ip <ESP32_IP>")
        print("       python debug_analysis.py --control <command>")
        print("       python debug_analysis.py --analyze")
        parser.print_help()


if __name__ == "__main__":
    main()
