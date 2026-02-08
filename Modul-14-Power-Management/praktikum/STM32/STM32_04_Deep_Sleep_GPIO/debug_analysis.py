#!/usr/bin/env python3
"""
STM32_04_Deep_Sleep_GPIO - GPIO Wakeup Event Tracker
Parses serial output to analyze Stop mode + GPIO EXTI wakeup events.
Tracks wakeup latency, GPIO event count, and state transitions.
"""

import sys
import re
from datetime import datetime


class GPIOWakeupTracker:
    def __init__(self):
        self.start_time = datetime.now()
        self.stop_entries = []
        self.wake_events = []
        self.state_transitions = []
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
            self.state_transitions.append(('STOP', datetime.now()))
            return

        # Wake event
        if '[WAKE] Woken up from STOP mode' in line:
            self.wake_events.append({
                'timestamp': datetime.now()
            })
            self.current_state = "ACTIVE"
            self.state_transitions.append(('WAKE', datetime.now()))
            return

        # Wakeup source
        src_match = re.search(r'\[WAKE\] Source: (.+)', line)
        if src_match and self.wake_events:
            self.wake_events[-1]['source'] = src_match.group(1).strip()
            return

        # EXTI flag
        exti_match = re.search(r'EXTI flag\s*:\s*(\w+)', line)
        if exti_match and self.wake_events:
            self.wake_events[-1]['exti_flag'] = exti_match.group(1)
            return

        # Total STOP entries
        total_stop = re.search(r'Total STOP entries\s*:\s*(\d+)', line)
        if total_stop and self.wake_events:
            self.wake_events[-1]['total_stops'] = int(total_stop.group(1))
            return

        # Total GPIO wakeups
        total_gpio = re.search(r'Total GPIO wakeups\s*:\s*(\d+)', line)
        if total_gpio and self.wake_events:
            self.wake_events[-1]['total_gpio'] = int(total_gpio.group(1))
            return

        # Last active duration
        active_match = re.search(r'Last active duration\s*:\s*(\d+)\s*ms', line)
        if active_match and self.wake_events:
            self.wake_events[-1]['active_duration'] = int(active_match.group(1))
            return

        # HCLK after wake
        hclk_match = re.search(r'HCLK after wake\s*:\s*(\d+)\s*Hz', line)
        if hclk_match and self.wake_events:
            self.wake_events[-1]['hclk'] = int(hclk_match.group(1))
            return

        # Stack HWM
        stack_match = re.search(r'Stack HWM\s*:\s*(\d+)\s*words', line)
        if stack_match and self.wake_events:
            self.wake_events[-1]['stack_hwm'] = int(stack_match.group(1))
            return

        # Phase detection
        if 'PRE-SLEEP' in line:
            self.current_state = "PRE-SLEEP"
            self.state_transitions.append(('PRE-SLEEP', datetime.now()))
        elif 'POST-WAKE' in line:
            self.current_state = "POST-WAKE"
            self.state_transitions.append(('POST-WAKE', datetime.now()))

    def print_summary(self):
        """Print analysis summary."""
        elapsed = (datetime.now() - self.start_time).total_seconds()

        print("\n" + "=" * 60)
        print("  GPIO WAKEUP EVENT TRACKER")
        print("=" * 60)
        print(f"  Analysis duration  : {elapsed:.1f} seconds")
        print(f"  Current state      : {self.current_state}")
        print(f"  STOP entries       : {len(self.stop_entries)}")
        print(f"  Wake events        : {len(self.wake_events)}")

        # Wakeup source analysis
        if self.wake_events:
            print(f"\n  --- Wakeup Source Analysis ---")
            gpio_wakes = sum(1 for w in self.wake_events
                           if w.get('source', '').startswith('GPIO'))
            other_wakes = len(self.wake_events) - gpio_wakes
            print(f"    GPIO PA0 wakeups : {gpio_wakes}")
            print(f"    Other wakeups    : {other_wakes}")

            # EXTI flag verification
            exti_set = sum(1 for w in self.wake_events
                         if w.get('exti_flag') == 'SET')
            print(f"    EXTI flag SET     : {exti_set}")

        # Active duration stats
        active_durs = [w['active_duration'] for w in self.wake_events
                      if 'active_duration' in w]
        if active_durs:
            print(f"\n  --- Active Duration Stats ---")
            print(f"    Min    : {min(active_durs)} ms")
            print(f"    Max    : {max(active_durs)} ms")
            print(f"    Avg    : {sum(active_durs) / len(active_durs):.0f} ms")

        # Clock recovery verification
        hclk_vals = [w['hclk'] for w in self.wake_events if 'hclk' in w]
        if hclk_vals:
            all_ok = all(v == 72000000 for v in hclk_vals)
            print(f"\n  --- Clock Recovery ---")
            print(f"    All 72MHz: {'YES' if all_ok else 'FAILED!'}")

        # Stack usage
        stacks = [w['stack_hwm'] for w in self.wake_events if 'stack_hwm' in w]
        if stacks:
            print(f"\n  --- Stack Usage ---")
            print(f"    Min HWM : {min(stacks)} words")
            print(f"    Current : {stacks[-1]} words")

        # State transition timeline
        if self.state_transitions:
            print(f"\n  --- Recent Transitions ---")
            for state, ts in self.state_transitions[-8:]:
                delta = (ts - self.start_time).total_seconds()
                print(f"    [{delta:8.2f}s] {state}")

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
    tracker = GPIOWakeupTracker()

    if len(sys.argv) > 1:
        if tracker.process_file(sys.argv[1]):
            tracker.print_summary()
    else:
        print("[GPIOWakeupTracker] Demo mode")
        print("Usage: python debug_analysis.py <serial_log.txt>")
        print()

        sample = [
            ">>> PRE-SLEEP: Slow blink (preparing to sleep)...",
            "[STOP] Entering STOP mode (cycle #1)",
            "[STOP] Waiting for PA0 button press...",
            "[WAKE] Woken up from STOP mode!",
            "[WAKE] Source: GPIO PA0 (EXTI)",
            "  Total STOP entries  : 1",
            "  Total GPIO wakeups  : 1",
            "  EXTI flag           : SET",
            "  Last active duration: 4200 ms",
            "  HCLK after wake     : 72000000 Hz",
            "  Stack HWM           : 180 words",
            ">>> POST-WAKE: Fast blink (awake state)...",
            ">>> PRE-SLEEP: Slow blink (preparing to sleep)...",
            "[STOP] Entering STOP mode (cycle #2)",
            "[WAKE] Woken up from STOP mode!",
            "[WAKE] Source: GPIO PA0 (EXTI)",
            "  Total STOP entries  : 2",
            "  Total GPIO wakeups  : 2",
            "  EXTI flag           : SET",
            "  Last active duration: 3800 ms",
            "  HCLK after wake     : 72000000 Hz",
            "  Stack HWM           : 178 words",
        ]

        for line in sample:
            tracker.parse_line(line)

        tracker.print_summary()


if __name__ == "__main__":
    main()
