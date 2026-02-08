#!/usr/bin/env python3
"""
STM32_07_Touch_Wakeup - Wakeup Event Tracker
==============================================
Monitors WKUP pin (PA0) wakeup events from STM32 Standby mode.
Tracks wakeup count, timing, and displays wakeup history.

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
MAX_EVENTS = 100

# Wakeup event storage
wakeup_events = deque(maxlen=MAX_EVENTS)
current_event = {}


def parse_line(line):
    """Parse serial output and extract wakeup data."""
    global current_event

    if "[WAKEUP]" in line and "Woke from STANDBY" in line:
        current_event = {
            "timestamp": datetime.now().isoformat(),
            "type": "WKUP_PIN",
            "epoch": time.time()
        }
    elif "[BOOT]" in line and "Fresh" in line:
        current_event = {
            "timestamp": datetime.now().isoformat(),
            "type": "FRESH_BOOT",
            "epoch": time.time()
        }
    elif "[INFO] Wakeup count:" in line:
        m = re.search(r"Wakeup count:\s*(\d+)", line)
        if m:
            current_event["wakeup_count"] = int(m.group(1))
    elif "[INFO] Total awake time:" in line:
        m = re.search(r"Total awake time:\s*(\d+)", line)
        if m:
            current_event["total_awake_ms"] = int(m.group(1))
    elif "[STANDBY] Entering STANDBY" in line:
        if current_event:
            wakeup_events.append(current_event.copy())
            current_event = {}


def print_summary():
    """Print wakeup event summary."""
    print("\n" + "=" * 60)
    print("  WKUP Pin Wakeup Event Tracker")
    print("=" * 60)
    print(f"  Total events recorded: {len(wakeup_events)}")

    if not wakeup_events:
        print("  No events yet. Waiting for data...\n")
        return

    # Calculate intervals
    intervals = []
    for i in range(1, len(wakeup_events)):
        dt = wakeup_events[i]["epoch"] - wakeup_events[i - 1]["epoch"]
        intervals.append(dt)

    print(f"\n  {'#':<4} {'Time':<20} {'Type':<12} {'Count':<8} {'Awake(ms)':<10}")
    print("  " + "-" * 56)
    for i, evt in enumerate(wakeup_events):
        ts = evt.get("timestamp", "N/A")[:19]
        etype = evt.get("type", "?")
        count = evt.get("wakeup_count", "?")
        awake = evt.get("total_awake_ms", "?")
        print(f"  {i + 1:<4} {ts:<20} {etype:<12} {count:<8} {awake:<10}")

    if intervals:
        avg_interval = sum(intervals) / len(intervals)
        print(f"\n  Average interval: {avg_interval:.1f} seconds")
        print(f"  Min interval:     {min(intervals):.1f} seconds")
        print(f"  Max interval:     {max(intervals):.1f} seconds")

    print("=" * 60 + "\n")


def main():
    print(f"WKUP Pin Wakeup Event Tracker")
    print(f"Connecting to {PORT} @ {BAUD} baud...")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        print("Connected! Monitoring wakeup events...\n")
    except serial.SerialException as e:
        print(f"Error: {e}")
        print("\n[DEMO MODE] Showing sample output:\n")
        demo_output()
        return

    event_count = 0
    try:
        while True:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    print(f"[SERIAL] {line}")
                    parse_line(line)

                    if len(wakeup_events) > event_count:
                        event_count = len(wakeup_events)
                        print_summary()
    except KeyboardInterrupt:
        print("\nStopped by user.")
        print_summary()
    finally:
        if 'ser' in dir() and ser.is_open:
            ser.close()


def demo_output():
    """Demo mode when no serial port available."""
    print("=" * 60)
    print("  WKUP Pin Wakeup Event Tracker (DEMO)")
    print("=" * 60)
    print(f"  Total events recorded: 5\n")
    print(f"  {'#':<4} {'Time':<20} {'Type':<12} {'Count':<8} {'Awake(ms)':<10}")
    print("  " + "-" * 56)
    for i in range(5):
        t = f"2026-02-07 10:{i:02d}:00"
        print(f"  {i + 1:<4} {t:<20} {'WKUP_PIN':<12} {i:<8} {(i + 1) * 5000:<10}")
    print(f"\n  Average interval: 60.0 seconds")
    print("=" * 60)


if __name__ == "__main__":
    main()
