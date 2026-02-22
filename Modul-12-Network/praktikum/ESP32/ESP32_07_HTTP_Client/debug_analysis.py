#!/usr/bin/env python3
"""
============================================================================
ESP32_07_HTTP_Client - Debug & Analysis Tool
Modul 13 - Network & IoT
============================================================================

Script ini menyediakan:
1. Mock HTTP Server   - Server lokal yang bisa diakses ESP32 untuk testing
2. Serial Monitor     - Monitor serial output dan parse data request
3. Response Analysis  - Analisis response time dari log serial
4. Visualisasi        - Grafik response time dan memory usage

Cara Pakai:
  python debug_analysis.py --mode server --port 8080
  python debug_analysis.py --mode monitor --serial /dev/ttyUSB0
  python debug_analysis.py --mode analyze --logfile serial_log.txt

============================================================================
"""

import json
import time
import argparse
import sys
import re
import os
from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading

# Serial opsional
try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

# Matplotlib opsional
try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class MockHTTPHandler(BaseHTTPRequestHandler):
    """
    Mock HTTP server handler yang meniru httpbin.org
    ESP32 bisa request ke server ini untuk testing lokal (tanpa internet)
    """

    # Counter untuk tracking requests
    request_count = 0
    request_log = []

    def log_message(self, format, *args):
        """Override default logging"""
        timestamp = datetime.now().isoformat()
        msg = format % args
        print(f"[{timestamp}] {msg}")

    def do_GET(self):
        """Handle GET requests - meniru beberapa endpoint httpbin"""
        MockHTTPHandler.request_count += 1
        client_ip = self.client_address[0]
        timestamp = datetime.now().isoformat()

        # Log request
        log_entry = {
            "timestamp": timestamp,
            "method": "GET",
            "path": self.path,
            "client_ip": client_ip,
            "headers": dict(self.headers)
        }
        MockHTTPHandler.request_log.append(log_entry)

        if self.path == "/get" or self.path == "/":
            # Meniru httpbin.org/get
            response_data = {
                "args": {},
                "headers": dict(self.headers),
                "origin": client_ip,
                "url": f"http://{self.headers.get('Host', 'localhost')}{self.path}",
                "server_time": timestamp,
                "request_number": MockHTTPHandler.request_count
            }
            self._send_json(200, response_data)

        elif self.path == "/ip":
            # Meniru httpbin.org/ip
            response_data = {"origin": client_ip}
            self._send_json(200, response_data)

        elif self.path == "/headers":
            # Meniru httpbin.org/headers
            response_data = {"headers": dict(self.headers)}
            self._send_json(200, response_data)

        elif self.path == "/status":
            # Custom endpoint: server status
            response_data = {
                "server": "ESP32 Mock HTTP Server",
                "total_requests": MockHTTPHandler.request_count,
                "uptime": "running",
                "timestamp": timestamp
            }
            self._send_json(200, response_data)

        elif self.path == "/delay/2":
            # Delayed response untuk test timeout
            time.sleep(2)
            response_data = {"delayed": True, "delay_sec": 2}
            self._send_json(200, response_data)

        elif self.path == "/sensor":
            # Simulasi sensor data (ESP32 bisa fetch ini)
            import random
            response_data = {
                "temperature": round(20 + random.random() * 15, 1),
                "humidity": round(40 + random.random() * 40, 1),
                "pressure": round(1000 + random.random() * 30, 1),
                "timestamp": timestamp
            }
            self._send_json(200, response_data)

        else:
            self._send_json(404, {"error": "Not found", "path": self.path})

    def _send_json(self, status_code, data):
        """Helper: kirim JSON response"""
        body = json.dumps(data, indent=2).encode('utf-8')
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Server', 'ESP32-MockServer/1.0')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(body)


def run_mock_server(port=8080):
    """Jalankan mock HTTP server"""
    server = HTTPServer(('0.0.0.0', port), MockHTTPHandler)
    print(f"\n{'='*60}")
    print(f"  ESP32 Mock HTTP Server")
    print(f"  Listening on port {port}")
    print(f"{'='*60}")
    print(f"\n  Available endpoints:")
    print(f"    GET /get      - Echo request info (like httpbin)")
    print(f"    GET /ip       - Return client IP")
    print(f"    GET /headers  - Echo request headers")
    print(f"    GET /status   - Server status")
    print(f"    GET /sensor   - Simulated sensor data")
    print(f"    GET /delay/2  - 2-second delayed response")
    print(f"\n  Ubah HTTP_URL_GET di main.c ke:")
    print(f"    http://<PC_IP>:{port}/get")
    print(f"\n  Ctrl+C untuk berhenti\n")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print(f"\n[SERVER] Stopped. Total requests: {MockHTTPHandler.request_count}")
        # Simpan log
        with open("mock_server_log.json", 'w') as f:
            json.dump(MockHTTPHandler.request_log, f, indent=2)
        print(f"[LOG] Request log saved to mock_server_log.json")
        server.server_close()


class SerialMonitor:
    """Monitor serial output dari ESP32 dan parse data HTTP"""

    def __init__(self, port="/dev/ttyUSB0", baudrate=115200):
        if not HAS_SERIAL:
            print("[ERROR] pyserial not installed. Run: pip install pyserial")
            sys.exit(1)
        self.port = port
        self.baudrate = baudrate
        self.data_log = []
        self.request_times = []
        self.heap_values = []

    def monitor(self, duration=300, logfile="serial_log.txt"):
        """Monitor serial dan parse output HTTP client"""
        print(f"\n{'='*60}")
        print(f"  Serial Monitor: {self.port} @ {self.baudrate} baud")
        print(f"  Duration: {duration}s | Log: {logfile}")
        print(f"{'='*60}\n")

        try:
            ser = serial.Serial(self.port, self.baudrate, timeout=1)
        except serial.SerialException as e:
            print(f"[ERROR] Cannot open {self.port}: {e}")
            return

        start = time.time()
        with open(logfile, 'w') as f:
            while (time.time() - start) < duration:
                try:
                    line = ser.readline().decode(errors='replace').strip()
                    if not line:
                        continue

                    timestamp = datetime.now().isoformat()
                    f.write(f"[{timestamp}] {line}\n")
                    print(f"  {line}")

                    # Parse HTTP request data
                    self._parse_line(line, timestamp)

                except KeyboardInterrupt:
                    break
                except Exception as e:
                    print(f"[WARN] Read error: {e}")

        ser.close()
        print(f"\n[DONE] Log saved to {logfile}")
        print(f"  Requests captured: {len(self.request_times)}")
        print(f"  Heap readings: {len(self.heap_values)}")

        # Analisis dan plot
        self._analyze()

    def _parse_line(self, line, timestamp):
        """Parse log line untuk mengekstrak data"""
        # Parse response time: "Status: 200, Content-Length: xxx, Time: 123 ms"
        time_match = re.search(r'Time:\s*(\d+)\s*ms', line)
        if time_match:
            self.request_times.append({
                "timestamp": timestamp,
                "time_ms": int(time_match.group(1))
            })

        # Parse heap: "Free heap: 123456 bytes"
        heap_match = re.search(r'[Ff]ree heap:\s*(\d+)', line)
        if heap_match:
            self.heap_values.append({
                "timestamp": timestamp,
                "heap": int(heap_match.group(1))
            })

        # Parse HTTP status
        status_match = re.search(r'Status:\s*(\d+)', line)
        if status_match:
            self.data_log.append({
                "timestamp": timestamp,
                "status": int(status_match.group(1)),
                "line": line
            })

    def _analyze(self):
        """Analisis data yang terkumpul"""
        if not self.request_times and not self.heap_values:
            print("[INFO] Tidak ada data untuk dianalisis")
            return

        if self.request_times:
            times = [r["time_ms"] for r in self.request_times]
            print(f"\n  Response Time Stats:")
            print(f"    Count : {len(times)}")
            print(f"    Avg   : {sum(times)/len(times):.1f} ms")
            print(f"    Min   : {min(times)} ms")
            print(f"    Max   : {max(times)} ms")

        if HAS_MATPLOTLIB and (self.request_times or self.heap_values):
            self._plot()

    def _plot(self):
        """Visualisasi data serial"""
        fig, axes = plt.subplots(1, 2, figsize=(14, 5))
        fig.suptitle("ESP32 HTTP Client - Serial Analysis", fontsize=13, fontweight='bold')

        # Plot 1: Response times
        ax1 = axes[0]
        if self.request_times:
            times = [r["time_ms"] for r in self.request_times]
            ax1.bar(range(1, len(times) + 1), times, color='#3498db', alpha=0.8)
            avg_t = sum(times) / len(times)
            ax1.axhline(y=avg_t, color='red', linestyle='--', label=f"Avg: {avg_t:.0f} ms")
            ax1.set_xlabel("Request #")
            ax1.set_ylabel("Response Time (ms)")
            ax1.set_title("HTTP Response Times")
            ax1.legend()
            ax1.grid(True, alpha=0.3)
        else:
            ax1.text(0.5, 0.5, "No request data", ha='center', va='center')

        # Plot 2: Heap memory
        ax2 = axes[1]
        if self.heap_values:
            heaps = [h["heap"] / 1024 for h in self.heap_values]
            ax2.plot(range(1, len(heaps) + 1), heaps, 'g-o', markersize=4)
            ax2.set_xlabel("Reading #")
            ax2.set_ylabel("Free Heap (KB)")
            ax2.set_title("Heap Memory Over Time")
            ax2.grid(True, alpha=0.3)
        else:
            ax2.text(0.5, 0.5, "No heap data", ha='center', va='center')

        plt.tight_layout()
        plt.savefig("http_client_analysis.png", dpi=150)
        print("[PLOT] Saved: http_client_analysis.png")
        plt.show()


def analyze_logfile(logfile):
    """Analisis file log serial yang sudah direkam"""
    if not os.path.exists(logfile):
        print(f"[ERROR] File not found: {logfile}")
        return

    print(f"\n{'='*60}")
    print(f"  Analyzing: {logfile}")
    print(f"{'='*60}\n")

    request_times = []
    heap_values = []
    statuses = []

    with open(logfile, 'r') as f:
        for line in f:
            time_match = re.search(r'Time:\s*(\d+)\s*ms', line)
            if time_match:
                request_times.append(int(time_match.group(1)))

            heap_match = re.search(r'[Ff]ree heap:\s*(\d+)', line)
            if heap_match:
                heap_values.append(int(heap_match.group(1)))

            status_match = re.search(r'Status:\s*(\d+)', line)
            if status_match:
                statuses.append(int(status_match.group(1)))

    print(f"  Requests found  : {len(request_times)}")
    print(f"  Heap readings   : {len(heap_values)}")
    print(f"  HTTP statuses   : {statuses}")

    if request_times:
        print(f"\n  Response Time:")
        print(f"    Avg: {sum(request_times)/len(request_times):.1f} ms")
        print(f"    Min: {min(request_times)} ms | Max: {max(request_times)} ms")

    if heap_values:
        print(f"\n  Heap Memory:")
        print(f"    Start: {heap_values[0]/1024:.1f} KB")
        print(f"    End  : {heap_values[-1]/1024:.1f} KB")
        print(f"    Delta: {(heap_values[-1]-heap_values[0])/1024:.1f} KB")

    if HAS_MATPLOTLIB and (request_times or heap_values):
        fig, axes = plt.subplots(1, 2, figsize=(12, 4))
        fig.suptitle("HTTP Client Log Analysis", fontsize=13)

        if request_times:
            axes[0].plot(request_times, 'b-o', markersize=3)
            axes[0].set_title("Response Times")
            axes[0].set_ylabel("ms")
            axes[0].grid(True, alpha=0.3)

        if heap_values:
            axes[1].plot([h/1024 for h in heap_values], 'g-s', markersize=3)
            axes[1].set_title("Free Heap")
            axes[1].set_ylabel("KB")
            axes[1].grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig("logfile_analysis.png", dpi=150)
        print("[PLOT] Saved: logfile_analysis.png")
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description="ESP32 HTTP Client - Debug & Analysis Tool")
    parser.add_argument("--mode", choices=["server", "monitor", "analyze"],
                        default="server", help="Mode operasi")
    parser.add_argument("--port", type=int, default=8080,
                        help="Port untuk mock server")
    parser.add_argument("--serial", default="/dev/ttyUSB0",
                        help="Serial port (mode=monitor)")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Baud rate")
    parser.add_argument("--duration", type=int, default=300,
                        help="Monitor duration (seconds)")
    parser.add_argument("--logfile", default="serial_log.txt",
                        help="Log file path")
    args = parser.parse_args()

    if args.mode == "server":
        run_mock_server(args.port)

    elif args.mode == "monitor":
        mon = SerialMonitor(args.serial, args.baud)
        mon.monitor(duration=args.duration, logfile=args.logfile)

    elif args.mode == "analyze":
        analyze_logfile(args.logfile)


if __name__ == "__main__":
    main()
