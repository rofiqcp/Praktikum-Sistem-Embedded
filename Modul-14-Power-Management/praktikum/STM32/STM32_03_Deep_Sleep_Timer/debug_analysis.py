#!/usr/bin/env python3
"""
STM32_03_Deep_Sleep_Timer - Stop Mode Cycle Tracker
Parses serial output to analyze Stop mode + RTC alarm wakeup cycles.
Tracks stop/wake transitions, clock reconfiguration, and power savings.
"""

import sys
import re
from datetime import datetime


class StopModeCycleTracker:
    def __init__(self):
        self.start_time = datetime.now()
        self.stop_entries = []
        self.wake_events = []
        self.rtc_alarms = []
        self.clock_reconfigs = 0
        self.current_state = "BOOT"

    def parse_line(self, line):
        """Parse a single serial output line."""
        line = line.strip()
        if not line:
            return

        # Stop mode entry
        stop_match = re.search(r'\[STOP\] Entering STOP mode \(cycle #(\d+)\)', line)
        if stop_match:
            cycle = int(stop_match.group(1))
            self.stop_entries.append({
                'cycle': cycle,
                'timestamp': datetime.now()
            })
            self.current_state = "STOP"
            return

        # RTC alarm set
        rtc_match = re.search(r'\[RTC\] Alarm set for (\d+) seconds', line)
        if rtc_match:
            seconds = int(rtc_match.group(1))
            self.rtc_alarms.append({
                'seconds': seconds,
                'timestamp': datetime.now()
            })
            return

        # RTC counter info
        ctr_match = re.search(r'\[RTC\] Current counter: (\d+), Alarm at: (\d+)', line)
        if ctr_match:
            if self.rtc_alarms:
                self.rtc_alarms[-1]['counter'] = int(ctr_match.group(1))
                self.rtc_alarms[-1]['alarm_at'] = int(ctr_match.group(2))
            return

        # Wake event
        if '[WAKE] Woken up from STOP mode' in line:
            self.wake_events.append({
                'timestamp': datetime.now()
            })
            self.current_state = "ACTIVE"
            return

        # Clock reconfigured
        if 'Clock reconfigured to 72MHz' in line:
            self.clock_reconfigs += 1
            return

        # Wakeup source
        src_match = re.search(r'\[WAKE\] Source: (.+)', line)
        if src_match and self.wake_events:
            self.wake_events[-1]['source'] = src_match.group(1).strip()
            return

        # Stop cycles count
        cyc_match = re.search(r'\[WAKE\] Stop cycles: (\d+)', line)
        if cyc_match and self.wake_events:
            self.wake_events[-1]['total_cycles'] = int(cyc_match.group(1))
            return

        # HCLK after wake
        hclk_match = re.search(r'\[WAKE\] HCLK: (\d+) Hz', line)
        if hclk_match and self.wake_events:
            self.wake_events[-1]['hclk'] = int(hclk_match.group(1))
            return

    def print_summary(self):
        """Print analysis summary."""
        elapsed = (datetime.now() - self.start_time).total_seconds()

        print("\n" + "=" * 60)
        print("  STOP MODE CYCLE TRACKER")
        print("=" * 60)
        print(f"  Analysis duration    : {elapsed:.1f} seconds")
        print(f"  Current state        : {self.current_state}")
        print(f"  Total STOP entries   : {len(self.stop_entries)}")
        print(f"  Total WAKE events    : {len(self.wake_events)}")
        print(f"  Clock reconfigurations: {self.clock_reconfigs}")

        # RTC alarm stats
        if self.rtc_alarms:
            print(f"\n  --- RTC Alarm Configuration ---")
            for i, alarm in enumerate(self.rtc_alarms[-5:]):
                print(f"    Alarm #{i+1}: {alarm['seconds']}s interval", end="")
                if 'counter' in alarm:
                    print(f" (cnt:{alarm['counter']} -> {alarm['alarm_at']})", end="")
                print()

        # Wake source analysis
        if self.wake_events:
            print(f"\n  --- Wakeup Sources ---")
            sources = {}
            for w in self.wake_events:
                src = w.get('source', 'Unknown')
                sources[src] = sources.get(src, 0) + 1
            for src, count in sources.items():
                print(f"    {src}: {count} times")

        # HCLK verification
        hclk_vals = [w['hclk'] for w in self.wake_events if 'hclk' in w]
        if hclk_vals:
            all_72mhz = all(v == 72000000 for v in hclk_vals)
            print(f"\n  --- Clock Recovery ---")
            print(f"    All restored to 72MHz: {'YES' if all_72mhz else 'NO'}")
            if not all_72mhz:
                for v in hclk_vals:
                    if v != 72000000:
                        print(f"    WARNING: HCLK = {v} Hz (not 72MHz!)")

        # Power estimation
        if self.rtc_alarms:
            avg_sleep = sum(a['seconds'] for a in self.rtc_alarms) / len(self.rtc_alarms)
            active_est = 5.0  # ~5 seconds active per cycle
            duty_sleep = avg_sleep / (avg_sleep + active_est) * 100
            print(f"\n  --- Power Estimation ---")
            print(f"    Avg sleep period : {avg_sleep:.0f} sec")
            print(f"    Est. active time : {active_est:.0f} sec/cycle")
            print(f"    Sleep duty cycle : {duty_sleep:.1f}%")
            print(f"    Active current   : ~30 mA")
            print(f"    Stop current     : ~20 uA")
            avg_ma = (100 - duty_sleep) / 100 * 30 + duty_sleep / 100 * 0.02
            print(f"    Avg current est  : {avg_ma:.2f} mA")

        print("\n" + "=" * 60)

    def process_file(self, filename):
        try:
            with open(filename, 'r') as f:
                for line in f:
                    self.parse_line(line)
        except FileNotFoundError:
            print(f"File not found: {filename}")
            return False
        return True


def main():
    tracker = StopModeCycleTracker()

    if len(sys.argv) > 1:
        if tracker.process_file(sys.argv[1]):
            tracker.print_summary()
    else:
        print("[StopModeCycleTracker] Demo mode")
        print("Usage: python debug_analysis.py <serial_log.txt>")
        print()

        sample = [
            "[RTC] Alarm set for 10 seconds",
            "[RTC] Current counter: 100, Alarm at: 110",
            "[STOP] Entering STOP mode (cycle #1)",
            "[WAKE] Woken up from STOP mode!",
            "[WAKE] Clock reconfigured to 72MHz",
            "[WAKE] Source: RTC Alarm",
            "[WAKE] Stop cycles: 1",
            "[WAKE] HCLK: 72000000 Hz",
            "[RTC] Alarm set for 10 seconds",
            "[RTC] Current counter: 115, Alarm at: 125",
            "[STOP] Entering STOP mode (cycle #2)",
            "[WAKE] Woken up from STOP mode!",
            "[WAKE] Clock reconfigured to 72MHz",
            "[WAKE] Source: RTC Alarm",
            "[WAKE] Stop cycles: 2",
            "[WAKE] HCLK: 72000000 Hz",
        ]

        for line in sample:
            tracker.parse_line(line)

        tracker.print_summary()


if __name__ == "__main__":
    main()
