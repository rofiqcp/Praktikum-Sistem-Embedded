#!/usr/bin/env python3
"""
STM32_11_Battery_Powered_Logger - Battery Discharge & Data Dashboard
=====================================================================
Parses CSV data from the battery-powered logger and displays
battery discharge curve and data logger dashboard.

Usage: python debug_analysis.py [PORT]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import re
from collections import deque
from datetime import datetime

# Configuration
PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD = 115200
MAX_ENTRIES = 500

# Data storage
log_entries = deque(maxlen=MAX_ENTRIES)


def parse_line(line):
    """Parse CSV log line: log,battery_mV,temp_x10C,status"""
    # Skip comments and headers
    if line.startswith("#") or line.startswith("log,") or not line.strip():
        return

    parts = line.split(",")
    if len(parts) >= 4:
        try:
            entry = {
                "log_num": int(parts[0]),
                "battery_mV": int(parts[1]),
                "temp_x10": int(parts[2]),
                "status": parts[3].strip(),
                "timestamp": datetime.now().isoformat()
            }
            log_entries.append(entry)
        except (ValueError, IndexError):
            pass


def print_dashboard():
    """Print data logger dashboard."""
    if not log_entries:
        return

    print("\n" + "=" * 70)
    print("  Battery-Powered Data Logger Dashboard")
    print("=" * 70)

    # Latest data
    latest = log_entries[-1]
    print(f"\n  Latest Reading (Log #{latest['log_num']}):")
    print(f"    Battery:     {latest['battery_mV']} mV")
    temp = latest['temp_x10']
    print(f"    Temperature: {temp // 10}.{abs(temp) % 10} °C")
    print(f"    Status:      {latest['status']}")
    print(f"    Total logs:  {len(log_entries)}")

    # Battery trend
    batteries = [e["battery_mV"] for e in log_entries]
    temps = [e["temp_x10"] for e in log_entries]

    if len(batteries) > 1:
        print(f"\n  Battery Discharge Curve:")
        print("  " + "-" * 55)
        max_v = max(batteries) if batteries else 3300
        min_v = min(batteries) if batteries else 2800
        v_range = max(max_v - min_v, 100)

        # Show last 20 entries as chart
        recent = list(log_entries)[-20:]
        for entry in recent:
            v = entry["battery_mV"]
            bar_len = int((v - min_v) / v_range * 40) if v_range > 0 else 20
            bar_len = max(0, min(40, bar_len))
            bar = "█" * bar_len + "░" * (40 - bar_len)
            print(f"  #{entry['log_num']:>4} |{bar}| {v}mV")

    # Statistics
    if batteries:
        print(f"\n  Statistics:")
        print("  " + "-" * 40)
        print(f"  Battery - Min: {min(batteries)}mV  Max: {max(batteries)}mV  "
              f"Avg: {sum(batteries) // len(batteries)}mV")
        if temps:
            avg_t = sum(temps) // len(temps)
            print(f"  Temp    - Min: {min(temps) // 10}.{abs(min(temps)) % 10}°C  "
                  f"Max: {max(temps) // 10}.{abs(max(temps)) % 10}°C  "
                  f"Avg: {avg_t // 10}.{abs(avg_t) % 10}°C")

        # Discharge rate
        if len(batteries) >= 2:
            rate = batteries[0] - batteries[-1]
            per_log = rate / (len(batteries) - 1) if len(batteries) > 1 else 0
            print(f"\n  Discharge rate: {rate}mV over {len(batteries)} logs")
            print(f"  Per log: ~{per_log:.1f}mV/log")

    # Status count
    statuses = {}
    for e in log_entries:
        s = e["status"]
        statuses[s] = statuses.get(s, 0) + 1
    print(f"\n  Status Summary:")
    for s, cnt in sorted(statuses.items()):
        print(f"    {s}: {cnt} ({cnt * 100 // len(log_entries)}%)")

    print("=" * 70 + "\n")


def main():
    print(f"Battery-Powered Data Logger Dashboard")
    print(f"Connecting to {PORT} @ {BAUD} baud...")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        print("Connected!\n")
    except serial.SerialException as e:
        print(f"Error: {e}")
        print("\n[DEMO MODE]\n")
        demo_output()
        return

    try:
        count = 0
        while True:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    print(f"[SERIAL] {line}")
                    parse_line(line)
                    if len(log_entries) > count:
                        count = len(log_entries)
                        if count % 5 == 0:
                            print_dashboard()
    except KeyboardInterrupt:
        print("\nStopped.")
        print_dashboard()
    finally:
        if 'ser' in dir() and ser.is_open:
            ser.close()


def demo_output():
    """Demo mode with simulated data."""
    print("=" * 70)
    print("  Battery-Powered Data Logger Dashboard (DEMO)")
    print("=" * 70)

    # Simulate discharge
    print(f"\n  Battery Discharge Curve:")
    print("  " + "-" * 55)
    for i in range(20):
        v = 3300 - i * 15
        bar_len = int((v - 2800) / 500 * 40)
        bar_len = max(0, min(40, bar_len))
        bar = "█" * bar_len + "░" * (40 - bar_len)
        print(f"  #{i + 1:>4} |{bar}| {v}mV")

    print(f"\n  Statistics:")
    print(f"  Battery - Min: 3015mV  Max: 3300mV  Avg: 3157mV")
    print(f"  Temp    - Min: 23.5°C  Max: 26.1°C  Avg: 24.8°C")
    print(f"\n  Discharge rate: 285mV over 20 logs")
    print(f"  Per log: ~15.0mV/log")
    print("=" * 70)


if __name__ == "__main__":
    main()
