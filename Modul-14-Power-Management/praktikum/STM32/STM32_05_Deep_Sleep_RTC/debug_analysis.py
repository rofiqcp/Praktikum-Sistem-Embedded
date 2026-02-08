#!/usr/bin/env python3
"""
STM32_05_Deep_Sleep_RTC - Standby Cycle Tracker & Power Profiler
Parses serial output to analyze Standby mode + RTC wakeup behavior.
Tracks boot source (standby vs normal), RTC counter persistence, power profile.
"""

import sys
import re
from datetime import datetime


class StandbyCycleTracker:
    def __init__(self):
        self.start_time = datetime.now()
        self.boot_events = []
        self.standby_entries = []
        self.rtc_counters = []
        self.current_boot = None

    def parse_line(self, line):
        """Parse a single serial output line."""
        line = line.strip()
        if not line:
            return

        # Normal boot detection
        if '[BOOT] Normal boot' in line:
            self.current_boot = {
                'type': 'NORMAL',
                'timestamp': datetime.now()
            }
            self.boot_events.append(self.current_boot)
            return

        # Standby wakeup detection
        if 'WAKEUP FROM STANDBY DETECTED' in line:
            self.current_boot = {
                'type': 'STANDBY_WAKE',
                'timestamp': datetime.now()
            }
            self.boot_events.append(self.current_boot)
            return

        # SRAM loss info
        if 'All SRAM data was lost' in line and self.current_boot:
            self.current_boot['sram_lost'] = True
            return

        # RTC counter survived
        rtc_match = re.search(r'\[RTC\] Counter value: (\d+)', line)
        if rtc_match:
            val = int(rtc_match.group(1))
            self.rtc_counters.append(val)
            if self.current_boot:
                self.current_boot['rtc_counter'] = val
            return

        # HCLK info
        hclk_match = re.search(r'\[INFO\] HCLK: (\d+) Hz', line)
        if hclk_match and self.current_boot:
            self.current_boot['hclk'] = int(hclk_match.group(1))
            return

        # FreeRTOS ticks
        tick_match = re.search(r'\[INFO\] FreeRTOS ticks: (\d+)', line)
        if tick_match and self.current_boot:
            self.current_boot['frticks'] = int(tick_match.group(1))
            return

        # RTC alarm configuration
        alarm_match = re.search(r'\[RTC\] Alarm set: current=(\d+), alarm=(\d+) \(\+(\d+) sec\)', line)
        if alarm_match:
            entry = {
                'current': int(alarm_match.group(1)),
                'alarm': int(alarm_match.group(2)),
                'interval': int(alarm_match.group(3)),
                'timestamp': datetime.now()
            }
            self.standby_entries.append(entry)
            return

        # Entering standby
        if 'Entering STANDBY now' in line:
            if self.standby_entries:
                self.standby_entries[-1]['confirmed'] = True
            return

    def print_summary(self):
        """Print analysis summary."""
        elapsed = (datetime.now() - self.start_time).total_seconds()

        normal_boots = sum(1 for b in self.boot_events if b['type'] == 'NORMAL')
        standby_wakes = sum(1 for b in self.boot_events if b['type'] == 'STANDBY_WAKE')

        print("\n" + "=" * 60)
        print("  STANDBY MODE CYCLE TRACKER & POWER PROFILER")
        print("=" * 60)
        print(f"  Analysis duration  : {elapsed:.1f} seconds")
        print(f"  Total boot events  : {len(self.boot_events)}")
        print(f"    Normal boots     : {normal_boots}")
        print(f"    Standby wakeups  : {standby_wakes}")
        print(f"  Standby entries    : {len(self.standby_entries)}")

        # Boot event details
        if self.boot_events:
            print(f"\n  --- Boot Event History ---")
            for i, boot in enumerate(self.boot_events):
                delta = (boot['timestamp'] - self.start_time).total_seconds()
                rtc = boot.get('rtc_counter', 'N/A')
                sram = "LOST" if boot.get('sram_lost') else "OK"
                print(f"    Boot #{i+1} [{delta:6.1f}s]: {boot['type']}"
                      f" | RTC={rtc} | SRAM={sram}")

        # RTC counter progression
        if len(self.rtc_counters) > 1:
            print(f"\n  --- RTC Counter Progression ---")
            for i, val in enumerate(self.rtc_counters):
                delta_rtc = val - self.rtc_counters[0]
                print(f"    Reading #{i+1}: {val} (delta: +{delta_rtc})")
            total_rtc_delta = self.rtc_counters[-1] - self.rtc_counters[0]
            print(f"    Total RTC elapsed: {total_rtc_delta} seconds")

        # Power profile
        if self.standby_entries:
            intervals = [e['interval'] for e in self.standby_entries]
            avg_interval = sum(intervals) / len(intervals)
            active_est = 8.0  # estimated seconds active per cycle
            total_cycle = avg_interval + active_est

            print(f"\n  --- Power Profile ---")
            print(f"    Standby interval : {avg_interval:.0f} sec")
            print(f"    Est. active time : {active_est:.0f} sec")
            print(f"    Total cycle      : {total_cycle:.0f} sec")
            print(f"    Standby duty     : {avg_interval / total_cycle * 100:.1f}%")
            print(f"    Active duty      : {active_est / total_cycle * 100:.1f}%")
            print()
            print(f"    Active current   : ~30.0 mA")
            print(f"    Standby current  : ~0.002 mA (2 uA)")
            avg_curr = (active_est / total_cycle * 30) + (avg_interval / total_cycle * 0.002)
            print(f"    Average current  : ~{avg_curr:.3f} mA")
            print(f"    Battery life est : (capacity_mAh / {avg_curr:.3f}) hours")

            # Example with CR2032
            cr2032_mah = 225
            hours = cr2032_mah / avg_curr
            days = hours / 24
            print(f"    CR2032 ({cr2032_mah}mAh)  : ~{days:.0f} days")

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
    tracker = StandbyCycleTracker()

    if len(sys.argv) > 1:
        if tracker.process_file(sys.argv[1]):
            tracker.print_summary()
    else:
        print("[StandbyCycleTracker] Demo mode")
        print("Usage: python debug_analysis.py <serial_log.txt>")
        print()

        sample = [
            "[BOOT] Normal boot (not from Standby)",
            "[INFO] HCLK: 72000000 Hz",
            "[INFO] FreeRTOS ticks: 5023",
            "[RTC] Alarm set: current=0, alarm=10 (+10 sec)",
            "[STANDBY] Entering STANDBY now...",
            "!!! WAKEUP FROM STANDBY DETECTED !!!",
            "[WAKE] All SRAM data was lost",
            "[RTC] Counter value: 12 (survived standby)",
            "[INFO] HCLK: 72000000 Hz",
            "[INFO] FreeRTOS ticks: 4987",
            "[RTC] Alarm set: current=15, alarm=25 (+10 sec)",
            "[STANDBY] Entering STANDBY now...",
            "!!! WAKEUP FROM STANDBY DETECTED !!!",
            "[WAKE] All SRAM data was lost",
            "[RTC] Counter value: 27 (survived standby)",
        ]

        for line in sample:
            tracker.parse_line(line)

        tracker.print_summary()


if __name__ == "__main__":
    main()
