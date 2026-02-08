#!/usr/bin/env python3
"""
STM32_10_Peripheral_Power_Gate - Peripheral Power Budget Pie Chart
===================================================================
Parses peripheral clock gating data and displays power budget
as a text-based pie chart and comparison table.

Usage: python debug_analysis.py [PORT]
Default port: /dev/ttyUSB0
"""

import serial
import sys
import re

# Configuration
PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD = 115200

# Peripheral data
peripherals_all = {}
peripherals_opt = {}
summary = {}


def parse_line(line):
    """Parse peripheral clock status lines."""
    # Match peripheral lines: "  GPIOA  : ON   (~0.5 mA)"
    m = re.match(r"\s+(\w+)\s*:\s*(ON|OFF)\s+\(~([\d.]+)\s*mA\)", line)
    if m:
        name = m.group(1)
        state = m.group(2)
        current = float(m.group(3))
        # Determine phase by checking if SAVED marker present
        if "<-- SAVED" in line:
            peripherals_opt[name] = {"state": state, "current": current, "saved": True}
        elif name not in peripherals_opt:
            peripherals_all[name] = {"state": state, "current": current}

    # Summary lines
    if "All clocks ON:" in line:
        m2 = re.search(r"~([\d.]+)\s*mA", line)
        if m2:
            summary["all_on"] = float(m2.group(1))
    elif "Optimized:" in line:
        m2 = re.search(r"~([\d.]+)\s*mA", line)
        if m2:
            summary["optimized"] = float(m2.group(1))
    elif "Current saved:" in line:
        m2 = re.search(r"~([\d.]+)\s*mA", line)
        if m2:
            summary["saved"] = float(m2.group(1))
    elif "Saving percentage:" in line:
        m2 = re.search(r"~([\d.]+)%", line)
        if m2:
            summary["pct"] = float(m2.group(1))


def print_budget():
    """Print power budget analysis."""
    if not summary:
        return

    print("\n" + "=" * 65)
    print("  Peripheral Power Budget Analysis")
    print("=" * 65)

    total_all = summary.get("all_on", 0)
    total_opt = summary.get("optimized", 0)
    saved = summary.get("saved", 0)
    pct = summary.get("pct", 0)

    print(f"\n  All peripherals ON:  {total_all:.1f} mA")
    print(f"  Optimized:           {total_opt:.1f} mA")
    print(f"  Saved:               {saved:.1f} mA ({pct:.1f}%)")

    # Text pie chart - segments
    if peripherals_all:
        print(f"\n  Power Distribution (All ON):")
        print("  " + "-" * 50)
        sorted_p = sorted(peripherals_all.items(),
                          key=lambda x: x[1]["current"], reverse=True)
        for name, info in sorted_p:
            if info["state"] == "ON" and total_all > 0:
                pct_p = info["current"] / total_all * 100
                bar = "█" * int(pct_p / 2.5)
                print(f"  {name:<10} {info['current']:>5.1f}mA "
                      f"{pct_p:>5.1f}% |{bar}")

    # Comparison bar
    if total_all > 0:
        print(f"\n  Before vs After Comparison:")
        print("  " + "-" * 50)
        bar_all = "█" * 40
        bar_opt_len = int(total_opt / total_all * 40)
        bar_opt = "█" * bar_opt_len + "░" * (40 - bar_opt_len)
        print(f"  ALL ON  |{bar_all}| {total_all:.1f}mA")
        print(f"  OPTIM   |{bar_opt}| {total_opt:.1f}mA")
        print(f"  SAVED   {'':>41} -{saved:.1f}mA")

    # System-level estimate
    cpu_ma = 25.0  # CPU core at 72MHz
    print(f"\n  System-Level Power Estimate:")
    print("  " + "-" * 50)
    print(f"  CPU core (72MHz):     {cpu_ma:.1f} mA")
    print(f"  Peripherals (all):   +{total_all:.1f} mA")
    print(f"  Total (all on):       {cpu_ma + total_all:.1f} mA")
    print(f"  Total (optimized):    {cpu_ma + total_opt:.1f} mA")
    print(f"  System saving:        {saved:.1f} mA "
          f"({saved / (cpu_ma + total_all) * 100:.1f}% of system)")

    print("=" * 65 + "\n")


def main():
    print(f"Peripheral Power Budget Analyzer")
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
        while True:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    print(f"[SERIAL] {line}")
                    parse_line(line)
                    if "Saving percentage" in line:
                        print_budget()
    except KeyboardInterrupt:
        print("\nStopped.")
        print_budget()
    finally:
        if 'ser' in dir() and ser.is_open:
            ser.close()


def demo_output():
    """Demo mode."""
    print("=" * 65)
    print("  Peripheral Power Budget Analysis (DEMO)")
    print("=" * 65)

    demo_periphs = [
        ("USB", 1.5), ("ADC1", 1.0), ("ADC2", 1.0), ("SPI1", 0.8),
        ("SPI2", 0.8), ("I2C1", 0.6), ("I2C2", 0.6), ("GPIOA", 0.5),
        ("GPIOB", 0.5), ("USART2", 0.5), ("USART3", 0.5),
        ("TIM1", 0.3), ("TIM2", 0.3), ("TIM3", 0.3),
    ]
    total = sum(p[1] for p in demo_periphs)

    print(f"\n  Power Distribution:")
    print("  " + "-" * 50)
    for name, current in demo_periphs:
        pct = current / total * 100
        bar = "█" * int(pct / 2)
        print(f"  {name:<10} {current:>5.1f}mA {pct:>5.1f}% |{bar}")

    saved = 7.2
    opt = total - saved
    print(f"\n  ALL ON:    {total:.1f} mA")
    print(f"  Optimized: {opt:.1f} mA")
    print(f"  Saved:     {saved:.1f} mA ({saved / total * 100:.1f}%)")
    print("=" * 65)


if __name__ == "__main__":
    main()
