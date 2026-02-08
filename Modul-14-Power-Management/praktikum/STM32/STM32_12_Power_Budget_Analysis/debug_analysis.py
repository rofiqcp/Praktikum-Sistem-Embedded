#!/usr/bin/env python3
"""
STM32_12_Power_Budget_Analysis - Power Budget Calculator & Estimator
=====================================================================
Parses power budget report data, generates state timeline,
battery life estimates, and comprehensive power analysis.

Usage: python debug_analysis.py [PORT]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import re
from collections import deque, OrderedDict
from datetime import datetime

# Configuration
PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD = 115200
MAX_REPORTS = 50

# Report storage
reports = deque(maxlen=MAX_REPORTS)
current_report = {}
current_states = OrderedDict()


def parse_line(line):
    """Parse power budget report lines."""
    global current_report, current_states

    # State data: "Active (Sensor)       85     2.0%   30.000 mA"
    m = re.match(r"\s*([\w\s()]+?)\s+(\d+)\s+([\d.]+)%\s+([\d.]+)\s*mA", line)
    if m:
        name = m.group(1).strip()
        time_ms = int(m.group(2))
        duty = float(m.group(3))
        current = float(m.group(4))
        current_states[name] = {
            "time_ms": time_ms, "duty": duty, "current_mA": current
        }

    # Average current
    if "Avg current:" in line:
        m2 = re.search(r"Avg current:\s+([\d.]+)\s*mA", line)
        if m2:
            current_report["avg_current"] = float(m2.group(1))

    # Energy per cycle
    if "Energy/cycle:" in line:
        m2 = re.search(r"Energy/cycle:\s+([\d.]+)\s*mJ", line)
        if m2:
            current_report["energy_mJ"] = float(m2.group(1))

    if "Cycle duration:" in line:
        m2 = re.search(r"Cycle duration:\s+(\d+)", line)
        if m2:
            current_report["cycle_ms"] = int(m2.group(1))

    if "Avg power:" in line:
        m2 = re.search(r"Avg power:\s+([\d.]+)\s*mW", line)
        if m2:
            current_report["avg_power_mW"] = float(m2.group(1))

    # End of report
    if "Next analysis cycle" in line:
        if current_states:
            current_report["states"] = dict(current_states)
            current_report["timestamp"] = datetime.now().isoformat()
            reports.append(current_report.copy())
            current_report = {}
            current_states = OrderedDict()


def print_analysis():
    """Print comprehensive power analysis."""
    if not reports:
        return

    r = reports[-1]
    states = r.get("states", {})

    print("\n" + "=" * 70)
    print("  Power Budget Calculator & Battery Life Estimator")
    print("=" * 70)

    # State timeline bar
    if states:
        total_ms = sum(s["time_ms"] for s in states.values())
        if total_ms == 0:
            total_ms = 1

        print(f"\n  State Timeline (cycle = {total_ms}ms):")
        print("  " + "-" * 60)
        timeline_width = 50
        timeline = ""
        symbols = {"Active": "A", "Processing": "P", "UART": "U",
                    "Sleep": "S", "Stop": "Z"}
        for name, data in states.items():
            width = max(1, int(data["time_ms"] / total_ms * timeline_width))
            sym = "?"
            for key, val in symbols.items():
                if key in name:
                    sym = val
                    break
            timeline += sym * width

        print(f"  |{timeline:<{timeline_width}}|")
        print(f"  A=Active P=Processing U=UART S=Sleep Z=Stop\n")

        # Detailed state breakdown
        print(f"  {'State':<20} {'Time(ms)':<10} {'Duty%':<8} {'I(mA)':<10} {'P(mW)':<10}")
        print("  " + "-" * 60)
        for name, data in states.items():
            power = data["current_mA"] * 3.3  # assuming 3.3V
            print(f"  {name:<20} {data['time_ms']:<10} {data['duty']:<7.1f}% "
                  f"{data['current_mA']:<10.3f} {power:<10.1f}")

    # Power summary
    avg_i = r.get("avg_current", 0)
    energy = r.get("energy_mJ", 0)
    avg_p = r.get("avg_power_mW", 0)
    cycle_ms = r.get("cycle_ms", 0)

    print(f"\n  Power Summary:")
    print("  " + "-" * 40)
    print(f"  Average current: {avg_i:.3f} mA")
    print(f"  Average power:   {avg_p:.2f} mW")
    print(f"  Energy/cycle:    {energy:.2f} mJ")
    print(f"  Cycle duration:  {cycle_ms} ms")

    # Battery life estimates
    print(f"\n  Battery Life Estimates:")
    print("  " + "-" * 55)
    print(f"  {'Battery':<14} {'Hours':<12} {'Days':<10} {'Months':<10}")
    print("  " + "-" * 55)
    for cap in [200, 500, 1000, 2000, 3000, 5000]:
        hours = cap / avg_i if avg_i > 0 else 0
        days = hours / 24
        months = days / 30
        print(f"  {cap:>6} mAh {hours:>10.1f} {days:>10.1f} {months:>10.1f}")

    # Optimization score
    if states:
        sleep_time = sum(s["time_ms"] for name, s in states.items()
                         if "Sleep" in name or "Stop" in name)
        sleep_pct = sleep_time / total_ms * 100 if total_ms > 0 else 0
        print(f"\n  Sleep Efficiency: {sleep_pct:.1f}% of cycle")
        if sleep_pct < 50:
            print(f"  [!] Consider increasing sleep time for better battery life")
        elif sleep_pct > 90:
            print(f"  [✓] Excellent sleep efficiency!")
        else:
            print(f"  [~] Good, but room for improvement")

    # Trend (if multiple reports)
    if len(reports) > 1:
        print(f"\n  Trend ({len(reports)} reports):")
        print("  " + "-" * 40)
        currents = [rp.get("avg_current", 0) for rp in reports]
        print(f"  Avg current range: {min(currents):.3f} - {max(currents):.3f} mA")

    print("=" * 70 + "\n")


def main():
    print(f"Power Budget Calculator & Battery Life Estimator")
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
        report_count = 0
        while True:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    print(f"[SERIAL] {line}")
                    parse_line(line)
                    if len(reports) > report_count:
                        report_count = len(reports)
                        print_analysis()
    except KeyboardInterrupt:
        print("\nStopped.")
        print_analysis()
    finally:
        if 'ser' in dir() and ser.is_open:
            ser.close()


def demo_output():
    """Demo mode."""
    print("=" * 70)
    print("  Power Budget Calculator (DEMO)")
    print("=" * 70)

    states = [
        ("Active (Sensor)", 85, 1.9, 30.0),
        ("Processing", 150, 3.4, 30.0),
        ("UART TX", 120, 2.7, 30.0),
        ("Sleep (WFI)", 2000, 45.5, 2.0),
        ("Stop Mode", 2040, 46.4, 0.020),
    ]
    total_ms = sum(s[1] for s in states)

    print(f"\n  State Timeline (cycle = {total_ms}ms):")
    print("  |AAPPPUUUSSSSSSSSSSSSSSSSSSZZZZZZZZZZZZZZZZZZZZZZZ|")
    print(f"\n  {'State':<20} {'Time(ms)':<10} {'Duty%':<8} {'I(mA)':<10}")
    print("  " + "-" * 50)
    for name, t, d, i in states:
        print(f"  {name:<20} {t:<10} {d:<7.1f}% {i:<10.3f}")

    avg_i = sum(s[3] * s[2] / 100 for s in states)
    print(f"\n  Average current: {avg_i:.3f} mA")
    print(f"\n  Battery Life:")
    print(f"  {'Battery':<14} {'Hours':<12} {'Days':<10}")
    print("  " + "-" * 40)
    for cap in [500, 1000, 2000, 3000]:
        h = cap / avg_i if avg_i > 0 else 0
        print(f"  {cap:>6} mAh {h:>10.1f} {h / 24:>10.1f}")
    print("=" * 70)


if __name__ == "__main__":
    main()
