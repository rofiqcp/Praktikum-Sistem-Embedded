#!/usr/bin/env python3
"""
STM32_09_Dynamic_Frequency - Frequency vs Performance Chart
============================================================
Parses frequency scaling benchmark data and generates a
performance comparison chart.

Usage: python debug_analysis.py [PORT]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import re
from datetime import datetime

# Configuration
PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD = 115200

# Benchmark data
benchmarks = []
current_bench = {}


def parse_line(line):
    """Parse serial output for frequency/benchmark data."""
    global current_bench

    if "Switching to:" in line:
        current_bench = {"mode": line.split("Switching to:")[-1].strip().strip("-").strip()}

    elif "[CLK] SYSCLK:" in line:
        m = re.search(r"SYSCLK:\s*(\d+)", line)
        if m:
            current_bench["sysclk_hz"] = int(m.group(1))

    elif "[BENCH] Loops/sec:" in line:
        m = re.search(r"Loops/sec:\s*(\d+)", line)
        if m:
            current_bench["loops"] = int(m.group(1))
            if "sysclk_hz" in current_bench:
                benchmarks.append(current_bench.copy())
            current_bench = {}


def print_chart():
    """Print frequency vs performance chart."""
    if not benchmarks:
        return

    # Get latest set (last 4 entries)
    latest = benchmarks[-4:] if len(benchmarks) >= 4 else benchmarks

    print("\n" + "=" * 70)
    print("  Frequency vs Performance Analysis")
    print("=" * 70)

    max_loops = max(b["loops"] for b in latest) if latest else 1

    print(f"\n  {'Mode':<25} {'Freq(MHz)':<12} {'Loops/s':<12} {'Perf%':<8}")
    print("  " + "-" * 58)
    for b in latest:
        freq_mhz = b["sysclk_hz"] / 1e6
        perf = b["loops"] / max_loops * 100
        print(f"  {b['mode']:<25} {freq_mhz:<12.1f} {b['loops']:<12} {perf:<7.1f}%")

    # ASCII bar chart
    print(f"\n  Performance Bar Chart (normalized to max):")
    print("  " + "-" * 55)
    for b in latest:
        perf = b["loops"] / max_loops
        bar_len = int(perf * 40)
        freq_mhz = b["sysclk_hz"] / 1e6
        bar = "█" * bar_len + "░" * (40 - bar_len)
        print(f"  {freq_mhz:>5.0f}MHz |{bar}| {perf * 100:.0f}%")

    # Power efficiency
    print(f"\n  Estimated Power Efficiency:")
    print("  " + "-" * 55)
    power_est = {"8": 8, "36": 18, "72": 30}
    for b in latest:
        freq_mhz = b["sysclk_hz"] / 1e6
        current = power_est.get(str(int(freq_mhz)), 15)
        efficiency = b["loops"] / current if current > 0 else 0
        print(f"  {freq_mhz:>5.0f}MHz: {b['loops']:>8} loops/s / {current:>2}mA = "
              f"{efficiency:>8.0f} loops/s/mA")

    print("=" * 70 + "\n")


def main():
    print(f"Frequency vs Performance Chart")
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
        bench_count = 0
        while True:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    print(f"[SERIAL] {line}")
                    parse_line(line)
                    if len(benchmarks) > bench_count and len(benchmarks) % 4 == 0:
                        bench_count = len(benchmarks)
                        print_chart()
    except KeyboardInterrupt:
        print("\nStopped.")
        print_chart()
    finally:
        if 'ser' in dir() and ser.is_open:
            ser.close()


def demo_output():
    """Demo mode with simulated data."""
    demo_data = [
        {"mode": "HSE 8 MHz", "sysclk_hz": 8000000, "loops": 45000},
        {"mode": "HSI 8 MHz", "sysclk_hz": 8000000, "loops": 44500},
        {"mode": "PLL 36 MHz", "sysclk_hz": 36000000, "loops": 198000},
        {"mode": "PLL 72 MHz", "sysclk_hz": 72000000, "loops": 395000},
    ]
    max_loops = max(d["loops"] for d in demo_data)

    print("=" * 70)
    print("  Frequency vs Performance Analysis (DEMO)")
    print("=" * 70)
    print(f"\n  {'Mode':<25} {'Freq(MHz)':<12} {'Loops/s':<12} {'Perf%':<8}")
    print("  " + "-" * 58)
    for b in demo_data:
        freq_mhz = b["sysclk_hz"] / 1e6
        perf = b["loops"] / max_loops * 100
        print(f"  {b['mode']:<25} {freq_mhz:<12.1f} {b['loops']:<12} {perf:<7.1f}%")

    print(f"\n  Performance Bar Chart:")
    print("  " + "-" * 55)
    for b in demo_data:
        perf = b["loops"] / max_loops
        bar_len = int(perf * 40)
        freq_mhz = b["sysclk_hz"] / 1e6
        bar = "█" * bar_len + "░" * (40 - bar_len)
        print(f"  {freq_mhz:>5.0f}MHz |{bar}| {perf * 100:.0f}%")
    print("=" * 70)


if __name__ == "__main__":
    main()
