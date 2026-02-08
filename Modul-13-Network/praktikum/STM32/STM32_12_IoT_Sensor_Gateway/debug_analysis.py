#!/usr/bin/env python3
"""
STM32_12_IoT_Sensor_Gateway - IoT Data Receiver & Dashboard
=============================================================
Tools:
1. HTTP server to receive sensor data
2. Real-time dashboard with matplotlib
3. Data logger with CSV export
4. Gateway health monitor

Usage:
  python debug_analysis.py --server           # Start HTTP data receiver
  python debug_analysis.py --dashboard        # Real-time matplotlib dashboard
  python debug_analysis.py --analyze          # Analyze gateway operation
  python debug_analysis.py --serial PORT      # Monitor gateway serial output
"""

import json
import time
import sys
import os
import argparse
import csv
from datetime import datetime
from collections import deque
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading

# Global data store
sensor_data = deque(maxlen=1000)
data_lock = threading.Lock()

class SensorDataHandler(BaseHTTPRequestHandler):
    """HTTP handler for receiving sensor data from IoT gateway."""

    def do_POST(self):
        """Handle POST /api/sensor."""
        if self.path == "/api/sensor":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode("utf-8")

            try:
                data = json.loads(body)
                data["received_at"] = datetime.now().isoformat()

                with data_lock:
                    sensor_data.append(data)

                ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                print(f"[{ts}] RX: gw={data.get('gw', '?')} "
                      f"temp={data.get('temp', '?')}C "
                      f"ext_v={data.get('ext_v', '?')}V "
                      f"ext_pct={data.get('ext_pct', '?')}% "
                      f"seq={data.get('seq', '?')}")

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                response = json.dumps({"status": "ok", "count": len(sensor_data)})
                self.wfile.write(response.encode())

            except json.JSONDecodeError as e:
                print(f"[ERROR] Invalid JSON: {e}")
                self.send_response(400)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": "Invalid JSON"}).encode())
        else:
            self.send_response(404)
            self.end_headers()

    def do_GET(self):
        """Handle GET requests for data retrieval."""
        if self.path == "/api/data":
            with data_lock:
                data_list = list(sensor_data)

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(data_list[-50:]).encode())

        elif self.path == "/api/stats":
            with data_lock:
                count = len(sensor_data)
                if count > 0:
                    temps = [d.get("temp", 0) for d in sensor_data if "temp" in d]
                    ext_vs = [d.get("ext_v", 0) for d in sensor_data if "ext_v" in d]
                    stats = {
                        "count": count,
                        "temp_min": min(temps) if temps else 0,
                        "temp_max": max(temps) if temps else 0,
                        "temp_avg": sum(temps) / len(temps) if temps else 0,
                        "ext_v_min": min(ext_vs) if ext_vs else 0,
                        "ext_v_max": max(ext_vs) if ext_vs else 0,
                        "ext_v_avg": sum(ext_vs) / len(ext_vs) if ext_vs else 0,
                    }
                else:
                    stats = {"count": 0}

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(stats).encode())

        elif self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            html = """
            <html><head><title>IoT Gateway Dashboard</title>
            <style>body{font-family:monospace;padding:20px;background:#1a1a2e;color:#e0e0e0;}
            h1{color:#00d4ff;}table{border-collapse:collapse;width:100%;}
            th,td{border:1px solid #333;padding:8px;text-align:left;}
            th{background:#16213e;}</style>
            <script>
            async function refresh(){
                let r=await fetch('/api/data');let d=await r.json();
                let t='<tr><th>Time</th><th>GW</th><th>Temp(C)</th><th>ExtV(V)</th><th>Ext%</th><th>Seq</th></tr>';
                d.reverse().forEach(e=>{
                    t+=`<tr><td>${e.received_at||''}</td><td>${e.gw||''}</td>
                    <td>${e.temp||''}</td><td>${e.ext_v||''}</td>
                    <td>${e.ext_pct||''}</td><td>${e.seq||''}</td></tr>`;
                });
                document.getElementById('data').innerHTML=t;
                let s=await(await fetch('/api/stats')).json();
                document.getElementById('stats').innerHTML=
                    `Count:${s.count} | Temp:${(s.temp_avg||0).toFixed(1)}C (${(s.temp_min||0).toFixed(1)}-${(s.temp_max||0).toFixed(1)}) | ExtV:${(s.ext_v_avg||0).toFixed(2)}V`;
            }
            setInterval(refresh,2000);refresh();
            </script></head>
            <body><h1>IoT Sensor Gateway Dashboard</h1>
            <p id="stats">Loading...</p>
            <table id="data"><tr><td>Loading...</td></tr></table>
            </body></html>"""
            self.wfile.write(html.encode())
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, format, *args):
        """Suppress default HTTP log messages."""
        pass

def run_server(host="0.0.0.0", port=8080):
    """Start HTTP server for receiving IoT data."""
    print(f"=" * 60)
    print(f"  IoT Sensor Gateway Data Receiver")
    print(f"  Listening on http://{host}:{port}")
    print(f"  POST /api/sensor  - receive sensor data")
    print(f"  GET  /api/data    - get recent data (JSON)")
    print(f"  GET  /api/stats   - get statistics")
    print(f"  GET  /            - web dashboard")
    print(f"=" * 60)
    print()

    server = HTTPServer((host, port), SensorDataHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print(f"\n\nServer stopped. Total data points: {len(sensor_data)}")

        # Export to CSV
        if sensor_data:
            filename = f"sensor_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
            with open(filename, "w", newline="") as f:
                writer = csv.DictWriter(f, fieldnames=["received_at", "gw", "temp", "ext_v", "ext_pct", "ts", "seq"])
                writer.writeheader()
                for d in sensor_data:
                    writer.writerow({k: d.get(k, "") for k in writer.fieldnames})
            print(f"Data exported to {filename}")
        server.server_close()

def run_dashboard():
    """Real-time dashboard using matplotlib."""
    try:
        import matplotlib
        matplotlib.use("TkAgg")
        import matplotlib.pyplot as plt
        import matplotlib.animation as animation
    except ImportError:
        print("ERROR: matplotlib not installed. Run: pip install matplotlib")
        print("\nRunning text-based dashboard...\n")
        run_text_dashboard()
        return

    fig, axes = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle("IoT Sensor Gateway - Real-Time Dashboard", fontsize=14)

    temp_data = deque(maxlen=100)
    ext_data = deque(maxlen=100)
    time_data = deque(maxlen=100)

    def update(frame):
        # Simulate data for demo
        t = time.time()
        import math
        temp = 25.0 + 5.0 * math.sin(t * 0.1) + (hash(str(t)) % 100) * 0.01
        ext_v = 1.65 + 1.0 * math.cos(t * 0.05) + (hash(str(t + 1)) % 100) * 0.005

        temp_data.append(temp)
        ext_data.append(ext_v)
        time_data.append(datetime.now().strftime("%H:%M:%S"))

        axes[0].clear()
        axes[0].plot(list(temp_data), "r-", linewidth=2)
        axes[0].set_title("Internal Temperature (°C)")
        axes[0].set_ylabel("°C")
        axes[0].grid(True, alpha=0.3)
        if temp_data:
            axes[0].axhline(y=sum(temp_data) / len(temp_data), color="orange",
                           linestyle="--", alpha=0.5, label="avg")
            axes[0].legend()

        axes[1].clear()
        axes[1].plot(list(ext_data), "b-", linewidth=2)
        axes[1].set_title("External ADC Voltage (V)")
        axes[1].set_ylabel("V")
        axes[1].set_xlabel("Samples")
        axes[1].grid(True, alpha=0.3)
        axes[1].set_ylim(0, 3.3)

        fig.tight_layout()

    ani = animation.FuncAnimation(fig, update, interval=1000)
    plt.show()

def run_text_dashboard():
    """Text-based real-time dashboard."""
    import math
    print("=== IoT Gateway Text Dashboard ===")
    print("Press Ctrl+C to stop\n")

    cycle = 0
    try:
        while True:
            t = time.time()
            temp = 25.0 + 5.0 * math.sin(t * 0.1)
            ext_v = 1.65 + 1.0 * math.cos(t * 0.05)
            ext_pct = ext_v * 100.0 / 3.3

            ts = datetime.now().strftime("%H:%M:%S")
            bar_temp = "#" * int(temp / 2)
            bar_ext = "#" * int(ext_pct / 5)

            print(f"\r[{ts}] Temp: {temp:5.1f}C |{bar_temp:20s}| "
                  f"ExtV: {ext_v:4.2f}V ({ext_pct:5.1f}%) |{bar_ext:20s}| "
                  f"seq={cycle}", end="", flush=True)

            cycle += 1
            time.sleep(1)
    except KeyboardInterrupt:
        print(f"\n\nDashboard stopped after {cycle} readings")

def analyze_gateway():
    """Analyze IoT gateway operation."""
    print("=" * 60)
    print("  IoT Sensor Gateway Analysis")
    print("=" * 60)

    print("\n--- System Architecture ---")
    print("  ┌────────────┐    UART2    ┌────────┐    WiFi    ┌────────┐")
    print("  │   STM32    │───────────→ │ ESP-01 │──────────→ │ Server │")
    print("  │ Blue Pill  │  PA2/PA3    │ (AT)   │   HTTP     │ (HTTP) │")
    print("  └──────┬─────┘             └────────┘            └────────┘")
    print("         │")
    print("    ┌────┴────┐")
    print("    │ Sensors  │")
    print("    ├─────────┤")
    print("    │ PA0:ADC  │  External sensor (pot)")
    print("    │ CH16:Int │  Internal temperature")
    print("    └─────────┘")

    print("\n--- Data Flow ---")
    steps = [
        "1. SensorTask reads ADC channels every 5 seconds",
        "2. Internal temp: ADC CH16 -> V_sense -> temperature formula",
        "3. External ADC: PA0 -> voltage -> percentage",
        "4. Data packaged as JSON and queued",
        "5. UploadTask dequeues and sends via HTTP POST",
        "6. ESP-01 AT commands: CIPSTART -> CIPSEND -> HTTP data",
        "7. Server receives JSON, stores, and displays",
        "8. MonitorTask prints statistics every 10 seconds",
    ]
    for step in steps:
        print(f"  {step}")

    print("\n--- JSON Data Format ---")
    sample = {
        "gw": "gw_stm32_01",
        "temp": 26.5,
        "ext_v": 1.65,
        "ext_pct": 50.0,
        "ts": 12345678,
        "seq": 42
    }
    print(f"  {json.dumps(sample, indent=2)}")

    print("\n--- Temperature Calculation ---")
    print("  V_sense = ADC_raw × 3.3 / 4096")
    print("  Temp = ((1.43 - V_sense) / 0.0043) + 25")
    print("  Example: ADC=1750 -> V=1.409V -> Temp=29.9°C")

    print("\n--- Error Handling ---")
    errors = [
        ("WiFi failure",     "Retry every 5s, PB12 LED on"),
        ("HTTP POST fail",   "Re-queue data, retry on next cycle"),
        ("3 consecutive ERR","Reinitialize WiFi connection"),
        ("Queue full",       "Drop oldest reading, log warning"),
        ("ADC read error",   "Use previous reading, set flag"),
    ]
    for err, handling in errors:
        print(f"  {err:20s} -> {handling}")

    print("\n--- LED Indicators ---")
    print("  PC13 (active low): Toggle on each sensor reading (activity)")
    print("  PB12 (active high): ON = upload error, OFF = OK")

    print("\n--- Performance Estimates ---")
    print("  ADC read:    ~20us per channel (239.5 cycles @ 12MHz)")
    print("  JSON build:  ~100us (snprintf)")
    print("  WiFi send:   ~500ms-2s (AT command sequence)")
    print("  Total cycle:  ~2-3s per upload")
    print("  Data rate:   ~1 reading every 10s")

def monitor_serial(port_name="/dev/ttyUSB0", baudrate=115200):
    """Monitor gateway serial debug output."""
    try:
        import serial
    except ImportError:
        print("ERROR: pyserial not installed. Run: pip install pyserial")
        return

    print(f"Monitoring {port_name} at {baudrate} baud...")
    try:
        ser = serial.Serial(port_name, baudrate, timeout=0.1)
    except serial.SerialException as e:
        print(f"ERROR: {e}")
        return

    log_file = f"gateway_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
    print(f"Logging to {log_file}\n")

    try:
        with open(log_file, "w") as f:
            while True:
                data = ser.read(256)
                if data:
                    text = data.decode("utf-8", errors="replace")
                    sys.stdout.write(text)
                    sys.stdout.flush()
                    f.write(text)
                    f.flush()
    except KeyboardInterrupt:
        print(f"\n\nLog saved to {log_file}")
        ser.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="IoT Sensor Gateway Tools")
    parser.add_argument("--server", action="store_true", help="Start HTTP data receiver")
    parser.add_argument("--dashboard", action="store_true", help="Real-time dashboard")
    parser.add_argument("--analyze", action="store_true", help="Analyze gateway operation")
    parser.add_argument("--serial", type=str, metavar="PORT", help="Monitor serial output")
    parser.add_argument("--port", type=int, default=8080, help="HTTP server port")
    args = parser.parse_args()

    if args.server:
        run_server(port=args.port)
    elif args.dashboard:
        run_dashboard()
    elif args.serial:
        monitor_serial(args.serial)
    elif args.analyze:
        analyze_gateway()
    else:
        analyze_gateway()
