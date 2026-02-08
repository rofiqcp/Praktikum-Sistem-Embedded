#!/usr/bin/env python3
"""
STM32_01_Current_Measurement - Power Baseline Analyzer
Parses serial output to analyze active current measurement data.
Tracks HCLK frequency, SysTick values, LED toggles, and CPU load.
"""

import sys
import re
import time
from collections import defaultdict
from datetime import datetime


class PowerBaselineAnalyzer:
    def __init__(self):
        self.reports = []
        self.start_time = datetime.now()
        self.hclk_values = []
        self.systick_vals = []
        self.led_toggles = []
        self.cpu_loads = []
        self.button_states = []

    def parse_line(self, line):
        """Parse a single serial output line."""
        line = line.strip()
        if not line:
            return

        # Parse report header
        report_match = re.search(r'Report #(\d+) \(Uptime: (\d+) ms\)', line)
        if report_match:
            report_num = int(report_match.group(1))
            uptime_ms = int(report_match.group(2))
            self.reports.append({
                'number': report_num,
                'uptime_ms': uptime_ms,
                'timestamp': datetime.now()
            })
            return

        # Parse HCLK
        hclk_match = re.search(r'HCLK\s+(?:Frequency\s+)?:\s*(\d+)\s*Hz', line)
        if hclk_match:
            self.hclk_values.append(int(hclk_match.group(1)))
            return

        # Parse SysTick VAL
        systick_match = re.search(r'SysTick VAL\s*:\s*(\d+)', line)
        if systick_match:
            self.systick_vals.append(int(systick_match.group(1)))
            return

        # Parse LED toggles
        led_match = re.search(r'LED Toggles\s*:\s*(\d+)', line)
        if led_match:
            self.led_toggles.append(int(led_match.group(1)))
            return

        # Parse CPU load
        cpu_match = re.search(r'CPU Load Test\s*:\s*(\d+)', line)
        if cpu_match:
            self.cpu_loads.append(int(cpu_match.group(1)))
            return

        # Parse button state
        btn_match = re.search(r'Button \(PA0\)\s*:\s*(\w+)', line)
        if btn_match:
            self.button_states.append(btn_match.group(1))
            return

    def print_summary(self):
        """Print analysis summary."""
        elapsed = (datetime.now() - self.start_time).total_seconds()

        print("\n" + "=" * 60)
        print("  POWER BASELINE ANALYSIS SUMMARY")
        print("=" * 60)
        print(f"  Analysis duration : {elapsed:.1f} seconds")
        print(f"  Total reports     : {len(self.reports)}")

        if self.hclk_values:
            avg_hclk = sum(self.hclk_values) / len(self.hclk_values)
            print(f"\n  HCLK Frequency:")
            print(f"    Samples  : {len(self.hclk_values)}")
            print(f"    Average  : {avg_hclk / 1e6:.1f} MHz")
            print(f"    Min      : {min(self.hclk_values) / 1e6:.1f} MHz")
            print(f"    Max      : {max(self.hclk_values) / 1e6:.1f} MHz")
            stable = all(v == self.hclk_values[0] for v in self.hclk_values)
            print(f"    Stable   : {'YES' if stable else 'NO'}")

        if self.led_toggles:
            print(f"\n  LED Toggle Count:")
            print(f"    First    : {self.led_toggles[0]}")
            print(f"    Last     : {self.led_toggles[-1]}")
            if len(self.led_toggles) > 1:
                rate = (self.led_toggles[-1] - self.led_toggles[0]) / max(1, len(self.led_toggles) - 1)
                print(f"    Avg/cycle: {rate:.1f}")

        if self.systick_vals:
            avg_st = sum(self.systick_vals) / len(self.systick_vals)
            print(f"\n  SysTick VAL:")
            print(f"    Samples  : {len(self.systick_vals)}")
            print(f"    Average  : {avg_st:.0f}")

        if self.cpu_loads:
            print(f"\n  CPU Load Test (dummy sum):")
            consistent = all(v == self.cpu_loads[0] for v in self.cpu_loads)
            print(f"    Consistent: {'YES' if consistent else 'NO'}")
            print(f"    Value     : {self.cpu_loads[0]}")

        if self.button_states:
            pressed = self.button_states.count('PRESSED')
            released = self.button_states.count('RELEASED')
            print(f"\n  Button States:")
            print(f"    Pressed  : {pressed}")
            print(f"    Released : {released}")

        # Power estimation
        print(f"\n  --- Power Estimation ---")
        print(f"  Active @ 72MHz : ~30 mA (measure with multimeter)")
        print(f"  At 3.3V supply : ~99 mW estimated")
        print(f"  Measurement tip: Use inline ammeter on 3.3V rail")

        print("\n" + "=" * 60)

    def process_file(self, filename):
        """Process a log file."""
        try:
            with open(filename, 'r') as f:
                for line in f:
                    self.parse_line(line)
        except FileNotFoundError:
            print(f"File not found: {filename}")
            return False
        return True

    def process_stdin(self):
        """Process from stdin (pipe from serial monitor)."""
        print("[PowerBaselineAnalyzer] Reading from stdin (Ctrl+C to stop)...")
        try:
            for line in sys.stdin:
                self.parse_line(line)
                # Print parsed data in real-time
                if self.reports and len(self.reports) % 5 == 0:
                    self.print_summary()
        except KeyboardInterrupt:
            pass


def main():
    analyzer = PowerBaselineAnalyzer()

    if len(sys.argv) > 1:
        if analyzer.process_file(sys.argv[1]):
            analyzer.print_summary()
    else:
        # Demo mode with sample data
        print("[PowerBaselineAnalyzer] Demo mode - no input file specified")
        print("Usage: python debug_analysis.py <serial_log.txt>")
        print("   or: cat serial_log.txt | python debug_analysis.py")
        print()

        sample_lines = [
            "--- Report #1 (Uptime: 3000 ms) ---",
            "  HCLK Frequency  : 72000000 Hz",
            "  SysTick VAL     : 45231",
            "  LED Toggles     : 6",
            "  CPU Load Test   : 704982704 (dummy sum)",
            "  Button (PA0)    : RELEASED",
            "--- Report #2 (Uptime: 6000 ms) ---",
            "  HCLK Frequency  : 72000000 Hz",
            "  SysTick VAL     : 31005",
            "  LED Toggles     : 12",
            "  CPU Load Test   : 704982704 (dummy sum)",
            "  Button (PA0)    : RELEASED",
            "--- Report #3 (Uptime: 9000 ms) ---",
            "  HCLK Frequency  : 72000000 Hz",
            "  SysTick VAL     : 67890",
            "  LED Toggles     : 18",
            "  CPU Load Test   : 704982704 (dummy sum)",
            "  Button (PA0)    : PRESSED",
        ]

        for line in sample_lines:
            analyzer.parse_line(line)

        analyzer.print_summary()


if __name__ == "__main__":
    main()
