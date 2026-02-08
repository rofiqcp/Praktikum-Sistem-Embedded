#!/usr/bin/env python3
"""
STM32_08_ULP_Coprocessor - IWDG Wakeup Cycle Analyzer
======================================================
Analyzes IWDG-based periodic wakeup cycles from STM32 Standby mode.
Tracks cycle count, sensor data, timing, and power estimates.

Usage: python debug_analysis.py [PORT]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import time
import re
from datetime import datetime
from collections import deque

# Configuration
PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD = 115200
MAX_CYCLES = 200

# Cycle data
cycles = deque(maxlen=MAX_CYCLES)
current_cycle = {}


def parse_line(line):
    """Parse serial output and extract cycle data."""
    global current_cycle

    if "IWDG timeout reset" in line:
        m = re.search(r"cycle #(\d+)", line)
        current_cycle = {
            "timestamp": datetime.now().isoformat(),
            "epoch": time.time(),
            "type": "IWDG_RESET",
            "cycle": int(m.group(1)) if m else 0
        }
    elif "Fresh start" in line:
        current_cycle = {
            "timestamp": datetime.now().isoformat(),
            "epoch": time.time(),
            "type": "FRESH_BOOT",
            "cycle": 0
        }
    elif "Simulated reading:" in line:
        m = re.search(r"reading:\s*(\d+)", line)
        if m:
            current_cycle["sensor"] = int(m.group(1))
    elif "Duty cycle:" in line:
        m = re.search(r"Duty cycle:\s*([\d.]+)", line)
        if m:
            current_cycle["duty"] = float(m.group(1))
    elif "Estimated avg current:" in line:
        m = re.search(r"~([\d.]+)\s*mA", line)
        if m:
            current_cycle["avg_current_mA"] = float(m.group(1))
    elif "Entering Standby" in line:
        if current_cycle:
            cycles.append(current_cycle.copy())
            current_cycle = {}


def print_analysis():
    """Print IWDG cycle analysis."""
    print("\n" + "=" * 65)
    print("  IWDG Wakeup Cycle Analyzer")
    print("=" * 65)
    print(f"  Total cycles: {len(cycles)}")

    if not cycles:
        print("  No cycles yet.\n")
        return

    iwdg_cycles = [c for c in cycles if c.get("type") == "IWDG_RESET"]
    print(f"  IWDG resets:  {len(iwdg_cycles)}")

    # Timing analysis
    intervals = []
    for i in range(1, len(cycles)):
        dt = cycles[i]["epoch"] - cycles[i - 1]["epoch"]
        intervals.append(dt)

    print(f"\n  {'Cycle':<8} {'Time':<20} {'Type':<12} {'Sensor':<8} {'Duty%':<8}")
    print("  " + "-" * 58)
    for c in list(cycles)[-10:]:  # Last 10
        ts = c.get("timestamp", "")[:19]
        print(f"  {c.get('cycle', '?'):<8} {ts:<20} {c.get('type', '?'):<12} "
              f"{c.get('sensor', '?'):<8} {c.get('duty', '?'):<8}")

    if intervals:
        avg = sum(intervals) / len(intervals)
        print(f"\n  Average cycle time: {avg:.1f} seconds")
        print(f"  Expected (~13s):    IWDG 10s + awake 3s")
        deviation = abs(avg - 13.0) / 13.0 * 100
        print(f"  Deviation:          {deviation:.1f}%")

    # Power estimate
    if iwdg_cycles and iwdg_cycles[-1].get("avg_current_mA"):
        avg_i = iwdg_cycles[-1]["avg_current_mA"]
        print(f"\n  Estimated avg current: {avg_i:.3f} mA")
        for cap in [500, 1000, 2000]:
            hours = cap / avg_i if avg_i > 0 else 0
            print(f"  {cap} mAh battery: {hours:.0f} hours ({hours / 24:.1f} days)")

    print("=" * 65 + "\n")


def main():
    print(f"IWDG Wakeup Cycle Analyzer")
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
        cycle_count = 0
        while True:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    print(f"[SERIAL] {line}")
                    parse_line(line)
                    if len(cycles) > cycle_count:
                        cycle_count = len(cycles)
                        print_analysis()
    except KeyboardInterrupt:
        print("\nStopped.")
        print_analysis()
    finally:
        if 'ser' in dir() and ser.is_open:
            ser.close()


def demo_output():
    """Demo mode."""
    print("=" * 65)
    print("  IWDG Wakeup Cycle Analyzer (DEMO)")
    print("=" * 65)
    print(f"  Total cycles: 10, IWDG resets: 9\n")
    print(f"  {'Cycle':<8} {'Type':<12} {'Sensor':<8} {'Duty%':<8}")
    print("  " + "-" * 40)
    for i in range(10):
        t = "IWDG_RESET" if i > 0 else "FRESH_BOOT"
        sensor = (i * 37 + 128) % 4096
        print(f"  {i:<8} {t:<12} {sensor:<8} {'23.1':<8}")
    print(f"\n  Average cycle time: 13.2 seconds")
    print(f"  Estimated avg current: 6.93 mA")
    print(f"  1000 mAh battery: 144 hours (6.0 days)")
    print("=" * 65)


if __name__ == "__main__":
    main()
