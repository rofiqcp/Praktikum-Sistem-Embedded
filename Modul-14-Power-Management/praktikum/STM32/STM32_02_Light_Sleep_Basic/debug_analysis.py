#!/usr/bin/env python3
"""
STM32_02_Light_Sleep_Basic - Sleep/Wake Cycle Analyzer
Parses serial output to analyze sleep mode WFI cycles.
Tracks sleep durations, wake counts, and power state transitions.
"""

import sys
import re
from datetime import datetime


class SleepWakeAnalyzer:
    def __init__(self):
        self.start_time = datetime.now()
        self.sleep_events = []
        self.wake_events = []
        self.total_sleep_ms = 0
        self.total_wake_irqs = 0
        self.current_state = "UNKNOWN"
        self.state_log = []

    def parse_line(self, line):
        """Parse a single serial output line."""
        line = line.strip()
        if not line:
            return

        # Detect sleep entry
        if '[SLEEP] Entering SLEEP mode' in line:
            self.sleep_events.append({
                'timestamp': datetime.now(),
                'type': 'SLEEP_ENTRY'
            })
            self.current_state = "SLEEPING"
            self.state_log.append(('SLEEP', datetime.now()))
            return

        # Detect wake event
        wake_match = re.search(r'\[WAKE\] Woken up from SLEEP mode', line)
        if wake_match:
            self.wake_events.append({
                'timestamp': datetime.now(),
                'type': 'WAKE'
            })
            self.current_state = "ACTIVE"
            self.state_log.append(('WAKE', datetime.now()))
            return

        # Parse sleep duration
        dur_match = re.search(r'Sleep duration\s*:\s*~?(\d+)\s*ms', line)
        if dur_match:
            duration = int(dur_match.group(1))
            if self.wake_events:
                self.wake_events[-1]['duration_ms'] = duration
            return

        # Parse total sleeps
        total_match = re.search(r'Total sleeps\s*:\s*(\d+)', line)
        if total_match:
            count = int(total_match.group(1))
            if self.wake_events:
                self.wake_events[-1]['total_sleeps'] = count
            return

        # Parse total wake IRQs
        irq_match = re.search(r'Total wake IRQs\s*:\s*(\d+)', line)
        if irq_match:
            self.total_wake_irqs = int(irq_match.group(1))
            return

        # Parse total sleep ms
        tsleep_match = re.search(r'Total sleep ms\s*:\s*(\d+)', line)
        if tsleep_match:
            self.total_sleep_ms = int(tsleep_match.group(1))
            return

        # Detect active phase
        if 'ACTIVE phase' in line:
            self.current_state = "ACTIVE_BLINK"
            return

        # Detect wakeup phase
        if 'WAKEUP phase' in line:
            self.current_state = "WAKE_BLINK"
            return

    def print_summary(self):
        """Print analysis summary."""
        elapsed = (datetime.now() - self.start_time).total_seconds()

        print("\n" + "=" * 60)
        print("  SLEEP/WAKE CYCLE ANALYSIS")
        print("=" * 60)
        print(f"  Analysis duration   : {elapsed:.1f} seconds")
        print(f"  Current state       : {self.current_state}")
        print(f"  Total sleep entries : {len(self.sleep_events)}")
        print(f"  Total wake events   : {len(self.wake_events)}")
        print(f"  Total wake IRQs     : {self.total_wake_irqs}")
        print(f"  Cumulative sleep    : {self.total_sleep_ms} ms")

        # Sleep duration statistics
        durations = [w.get('duration_ms', 0) for w in self.wake_events if 'duration_ms' in w]
        if durations:
            print(f"\n  --- Sleep Duration Stats ---")
            print(f"    Count    : {len(durations)}")
            print(f"    Min      : {min(durations)} ms")
            print(f"    Max      : {max(durations)} ms")
            print(f"    Average  : {sum(durations) / len(durations):.1f} ms")

        # State transition timeline
        if self.state_log:
            print(f"\n  --- State Transitions ---")
            for i, (state, ts) in enumerate(self.state_log[-10:]):
                delta = (ts - self.start_time).total_seconds()
                print(f"    [{delta:8.2f}s] {state}")

        # Power estimation
        if elapsed > 0 and self.total_sleep_ms > 0:
            sleep_pct = (self.total_sleep_ms / 1000.0 / elapsed) * 100
            active_pct = 100.0 - sleep_pct
            print(f"\n  --- Power Duty Cycle ---")
            print(f"    Active   : {active_pct:.1f}% (~30 mA)")
            print(f"    Sleep    : {sleep_pct:.1f}% (~2 mA)")
            avg_current = (active_pct / 100 * 30) + (sleep_pct / 100 * 2)
            print(f"    Avg Est. : {avg_current:.1f} mA")

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
        """Process from stdin."""
        print("[SleepWakeAnalyzer] Reading from stdin (Ctrl+C to stop)...")
        try:
            for line in sys.stdin:
                self.parse_line(line)
        except KeyboardInterrupt:
            pass


def main():
    analyzer = SleepWakeAnalyzer()

    if len(sys.argv) > 1:
        if analyzer.process_file(sys.argv[1]):
            analyzer.print_summary()
    else:
        print("[SleepWakeAnalyzer] Demo mode")
        print("Usage: python debug_analysis.py <serial_log.txt>")
        print()

        sample = [
            ">>> ACTIVE phase: LED slow blink for 5 sec...",
            "[SLEEP] Entering SLEEP mode (WFI)...",
            "[SLEEP] CPU halted, peripherals remain active",
            "[SLEEP] Press PA0 button to wake up",
            "[WAKE] Woken up from SLEEP mode!",
            "[WAKE] Sleep duration : ~4523 ms",
            "[WAKE] Total sleeps   : 1",
            "[WAKE] Total wake IRQs: 1",
            "[WAKE] Total sleep ms : 4523",
            ">>> WAKEUP phase: LED fast blink for 3 sec...",
            ">>> ACTIVE phase: LED slow blink for 5 sec...",
            "[SLEEP] Entering SLEEP mode (WFI)...",
            "[WAKE] Woken up from SLEEP mode!",
            "[WAKE] Sleep duration : ~8102 ms",
            "[WAKE] Total sleeps   : 2",
            "[WAKE] Total wake IRQs: 2",
            "[WAKE] Total sleep ms : 12625",
        ]

        for line in sample:
            analyzer.parse_line(line)

        analyzer.print_summary()


if __name__ == "__main__":
    main()
