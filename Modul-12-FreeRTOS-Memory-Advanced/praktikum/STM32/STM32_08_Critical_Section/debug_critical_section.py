#!/usr/bin/env python3
"""
debug_critical_section.py - Critical Section Debug & Analysis Tool

Detects data corruption patterns from STM32_08_Critical_Section.
Compares Phase 1 (no protection) vs Phase 2 (with critical section).
Shows corruption rates, field values, and timeline analysis.
Logs data to CSV for post-analysis.

Usage:
    python debug_critical_section.py [--port /dev/ttyUSB0] [--baud 115200] [--csv log.csv]
"""

import serial
import re
import sys
import argparse
import csv
import time
from datetime import datetime
from collections import deque

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not available, plotting disabled")


class CriticalSectionMonitor:
    def __init__(self, port, baud, csv_file=None):
        self.port = port
        self.baud = baud
        self.csv_file = csv_file
        self.csv_writer = None
        self.csv_fh = None

        # Phase tracking
        self.current_phase = 0
        self.phase_results = []

        # Current phase data
        self.corruption_events = deque(maxlen=500)
        self.timestamps = deque(maxlen=500)

        # All corruption events for plotting
        self.all_corruptions = []
        self.phase_boundaries = []

        # Per-phase counters
        self.write_a_count = 0
        self.write_b_count = 0
        self.verify_count = 0
        self.corruption_count = 0
        self.corruption_rate = 0.0

        self.start_time = time.time()

        # Regex patterns
        self.re_phase1_start = re.compile(r'PHASE 1: WITHOUT Critical Section')
        self.re_phase2_start = re.compile(r'PHASE 2: WITH Critical Section')
        self.re_corrupt = re.compile(
            r'\[CORRUPT\] f1=0x([0-9A-Fa-f]+) f2=0x([0-9A-Fa-f]+) '
            r'f3=0x([0-9A-Fa-f]+) f4=0x([0-9A-Fa-f]+) '
            r'cs=0x([0-9A-Fa-f]+) expected_cs=0x([0-9A-Fa-f]+)')
        self.re_result_write_a = re.compile(
            r'\[RESULT\] Write A count\s*:\s*(\d+)')
        self.re_result_write_b = re.compile(
            r'\[RESULT\] Write B count\s*:\s*(\d+)')
        self.re_result_verify = re.compile(
            r'\[RESULT\] Verify count\s*:\s*(\d+)')
        self.re_result_corrupt = re.compile(
            r'\[RESULT\] Corruptions\s*:\s*(\d+)')
        self.re_result_rate = re.compile(
            r'\[RESULT\] Corruption rate:\s*(\d+)\.(\d+)%')
        self.re_result_status = re.compile(
            r'\[RESULT\] Status\s*:\s*(.+)')
        self.re_phase1_results = re.compile(
            r'PHASE 1 RESULTS \(NO CRITICAL SECTION\)')
        self.re_phase2_results = re.compile(
            r'PHASE 2 RESULTS \(WITH CRITICAL SECTION\)')

    def init_csv(self):
        if self.csv_file:
            self.csv_fh = open(self.csv_file, 'w', newline='')
            self.csv_writer = csv.writer(self.csv_fh)
            self.csv_writer.writerow([
                'timestamp', 'elapsed_s', 'phase', 'event_type',
                'field1', 'field2', 'field3', 'field4',
                'checksum', 'expected_checksum',
                'write_a_count', 'write_b_count', 'verify_count',
                'corruption_count'
            ])

    def log_csv(self, event_type, f1='', f2='', f3='', f4='',
                cs='', exp_cs=''):
        if self.csv_writer:
            elapsed = time.time() - self.start_time
            self.csv_writer.writerow([
                datetime.now().isoformat(), f'{elapsed:.3f}',
                self.current_phase, event_type,
                f1, f2, f3, f4, cs, exp_cs,
                self.write_a_count, self.write_b_count,
                self.verify_count, self.corruption_count
            ])
            self.csv_fh.flush()

    def analyze_corruption(self, f1, f2, f3, f4, cs, exp_cs):
        """Analyze the type of corruption that occurred."""
        fields = [f1, f2, f3, f4]
        unique = len(set(fields))

        # Check if fields belong to different writers
        from_a = sum(1 for f in fields if (f & 0x80000000) == 0)
        from_b = sum(1 for f in fields if (f & 0x80000000) != 0)

        if from_a > 0 and from_b > 0:
            return f"TORN WRITE: {from_a} fields from A, {from_b} from B"
        elif cs != exp_cs and unique == 1:
            return "CHECKSUM MISMATCH: fields consistent but checksum stale"
        elif unique > 1:
            return f"PARTIAL UPDATE: {unique} different values in fields"
        else:
            return "UNKNOWN corruption pattern"

    def parse_line(self, line):
        elapsed = time.time() - self.start_time

        # Phase transitions
        if self.re_phase1_start.search(line):
            self.current_phase = 1
            self.corruption_events.clear()
            self.phase_boundaries.append((elapsed, 1, 'start'))
            print(f"\n  [{elapsed:8.2f}s] ========== PHASE 1: NO PROTECTION ==========")
            print(f"           Expecting data corruptions...")
            self.log_csv('phase_start')
            return

        if self.re_phase2_start.search(line):
            self.current_phase = 2
            self.corruption_events.clear()
            self.phase_boundaries.append((elapsed, 2, 'start'))
            print(f"\n  [{elapsed:8.2f}s] ========== PHASE 2: CRITICAL SECTION ==========")
            print(f"           Should be corruption-free...")
            self.log_csv('phase_start')
            return

        # Corruption events
        m = self.re_corrupt.search(line)
        if m:
            f1 = int(m.group(1), 16)
            f2 = int(m.group(2), 16)
            f3 = int(m.group(3), 16)
            f4 = int(m.group(4), 16)
            cs = int(m.group(5), 16)
            exp_cs = int(m.group(6), 16)

            analysis = self.analyze_corruption(f1, f2, f3, f4, cs, exp_cs)
            self.corruption_events.append(
                (elapsed, self.current_phase, f1, f2, f3, f4, cs, exp_cs))
            self.all_corruptions.append(
                (elapsed, self.current_phase, f1, f2, f3, f4))

            self.log_csv('corruption',
                         f'0x{f1:08X}', f'0x{f2:08X}',
                         f'0x{f3:08X}', f'0x{f4:08X}',
                         f'0x{cs:08X}', f'0x{exp_cs:08X}')

            # Only print first few corruptions to avoid flooding
            if len(self.corruption_events) <= 10:
                print(f"  [{elapsed:8.2f}s] !!! CORRUPTION #{len(self.corruption_events)} "
                      f"(Phase {self.current_phase}) !!!")
                print(f"           f1=0x{f1:08X} f2=0x{f2:08X} "
                      f"f3=0x{f3:08X} f4=0x{f4:08X}")
                print(f"           cs=0x{cs:08X} expected=0x{exp_cs:08X}")
                print(f"           Analysis: {analysis}")
            elif len(self.corruption_events) == 11:
                print(f"           ... (suppressing further corruption details)")
            return

        # Result parsing
        m = self.re_result_write_a.search(line)
        if m:
            self.write_a_count = int(m.group(1))

        m = self.re_result_write_b.search(line)
        if m:
            self.write_b_count = int(m.group(1))

        m = self.re_result_verify.search(line)
        if m:
            self.verify_count = int(m.group(1))

        m = self.re_result_corrupt.search(line)
        if m:
            self.corruption_count = int(m.group(1))

        m = self.re_result_rate.search(line)
        if m:
            whole = int(m.group(1))
            frac = int(m.group(2))
            self.corruption_rate = whole + frac / 100.0

        m = self.re_result_status.search(line)
        if m:
            status = m.group(1).strip()
            result = {
                'phase': self.current_phase,
                'write_a': self.write_a_count,
                'write_b': self.write_b_count,
                'verifies': self.verify_count,
                'corruptions': self.corruption_count,
                'rate': self.corruption_rate,
                'status': status
            }
            self.phase_results.append(result)
            self.phase_boundaries.append(
                (elapsed, self.current_phase, 'end'))

            print(f"\n  [{elapsed:8.2f}s] --- Phase {self.current_phase} Results ---")
            print(f"           Writes A  : {self.write_a_count:,}")
            print(f"           Writes B  : {self.write_b_count:,}")
            print(f"           Verifies  : {self.verify_count:,}")
            print(f"           Corrupted : {self.corruption_count:,}")
            print(f"           Rate      : {self.corruption_rate:.2f}%")
            print(f"           Status    : {status}")

            self.log_csv('result', status)

    def print_summary(self):
        elapsed = time.time() - self.start_time

        print(f"\n{'='*60}")
        print(f"  CRITICAL SECTION MONITOR - FINAL SUMMARY")
        print(f"{'='*60}")
        print(f"  Duration: {elapsed:.1f} seconds")
        print(f"  Phases completed: {len(self.phase_results)}")

        for r in self.phase_results:
            phase_name = "NO PROTECTION" if r['phase'] == 1 else "CRITICAL SECTION"
            print(f"\n  Phase {r['phase']} ({phase_name}):")
            print(f"    Write A ops    : {r['write_a']:,}")
            print(f"    Write B ops    : {r['write_b']:,}")
            print(f"    Verify ops     : {r['verifies']:,}")
            print(f"    Corruptions    : {r['corruptions']:,}")
            print(f"    Corruption rate: {r['rate']:.2f}%")
            print(f"    Status         : {r['status']}")

        if len(self.phase_results) >= 2:
            p1 = self.phase_results[0]
            p2 = self.phase_results[1]
            print(f"\n  === COMPARISON ===")
            print(f"  Phase 1 corruptions: {p1['corruptions']:,} "
                  f"({p1['rate']:.2f}%)")
            print(f"  Phase 2 corruptions: {p2['corruptions']:,} "
                  f"({p2['rate']:.2f}%)")
            if p1['corruptions'] > 0 and p2['corruptions'] == 0:
                print(f"  CONCLUSION: Critical sections PREVENT data corruption!")
            elif p1['corruptions'] == 0:
                print(f"  NOTE: No corruption in Phase 1 either "
                      f"(timing may need adjustment)")
        print(f"{'='*60}\n")

    def run_realtime(self):
        self.init_csv()
        print(f"\n[*] Connecting to {self.port} @ {self.baud} baud...")

        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
        except serial.SerialException as e:
            print(f"[ERROR] Cannot open {self.port}: {e}")
            sys.exit(1)

        print(f"[*] Connected. Monitoring critical section test...")
        print(f"[*] Press Ctrl+C to stop\n")

        try:
            while True:
                raw = ser.readline()
                if raw:
                    try:
                        line = raw.decode('utf-8', errors='replace').strip()
                    except Exception:
                        continue
                    if line:
                        self.parse_line(line)
        except KeyboardInterrupt:
            print("\n[*] Stopped by user")
        finally:
            self.print_summary()
            ser.close()
            if self.csv_fh:
                self.csv_fh.close()
                print(f"[*] CSV saved to: {self.csv_file}")

    def plot_results(self):
        if not HAS_MATPLOTLIB:
            print("[WARN] Cannot plot - matplotlib not installed")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 9))
        fig.suptitle('STM32 Critical Section Analysis', fontsize=14,
                     fontweight='bold')

        # Phase comparison bar chart
        if len(self.phase_results) >= 2:
            p1 = self.phase_results[0]
            p2 = self.phase_results[1]

            categories = ['Write A', 'Write B', 'Verifies', 'Corruptions']
            p1_vals = [p1['write_a'], p1['write_b'], p1['verifies'],
                       p1['corruptions']]
            p2_vals = [p2['write_a'], p2['write_b'], p2['verifies'],
                       p2['corruptions']]

            x = range(len(categories))
            width = 0.35

            bars1 = axes[0, 0].bar([i - width / 2 for i in x], p1_vals,
                                   width, label='No Protection',
                                   color='#ff6b6b', alpha=0.8)
            bars2 = axes[0, 0].bar([i + width / 2 for i in x], p2_vals,
                                   width, label='Critical Section',
                                   color='#51cf66', alpha=0.8)
            axes[0, 0].set_xticks(x)
            axes[0, 0].set_xticklabels(categories, fontsize=9)
            axes[0, 0].set_ylabel('Count')
            axes[0, 0].set_title('Phase Comparison')
            axes[0, 0].legend()
            axes[0, 0].set_yscale('log')
            axes[0, 0].grid(True, alpha=0.3, axis='y')

            # Corruption rate comparison
            rates = [p1['rate'], p2['rate']]
            colors = ['#ff6b6b' if r > 0 else '#51cf66' for r in rates]
            axes[0, 1].bar(['No Protection', 'Critical Section'],
                          rates, color=colors, alpha=0.8, edgecolor='black')
            axes[0, 1].set_ylabel('Corruption Rate (%)')
            axes[0, 1].set_title('Corruption Rate Comparison')
            axes[0, 1].grid(True, alpha=0.3, axis='y')

            for i, v in enumerate(rates):
                axes[0, 1].text(i, v + 0.1, f'{v:.2f}%', ha='center',
                               fontweight='bold')

        # Corruption timeline
        if self.all_corruptions:
            p1_corr = [(c[0], 1) for c in self.all_corruptions
                       if c[1] == 1]
            p2_corr = [(c[0], 1) for c in self.all_corruptions
                       if c[1] == 2]

            if p1_corr:
                t1 = [c[0] for c in p1_corr]
                axes[1, 0].scatter(t1, [1] * len(t1), c='red', s=10,
                                   alpha=0.5, label=f'Phase 1 ({len(t1)})')

            if p2_corr:
                t2 = [c[0] for c in p2_corr]
                axes[1, 0].scatter(t2, [2] * len(t2), c='blue', s=10,
                                   alpha=0.5, label=f'Phase 2 ({len(t2)})')

            # Mark phase boundaries
            for pb in self.phase_boundaries:
                color = 'red' if pb[1] == 1 else 'green'
                axes[1, 0].axvline(x=pb[0], color=color, linestyle='--',
                                  alpha=0.3)

            axes[1, 0].set_yticks([1, 2])
            axes[1, 0].set_yticklabels(['Phase 1\n(No Protection)',
                                        'Phase 2\n(Critical Section)'])
            axes[1, 0].set_xlabel('Time (seconds)')
            axes[1, 0].set_title('Corruption Events Timeline')
            axes[1, 0].legend(loc='upper right')
            axes[1, 0].grid(True, alpha=0.3)
        else:
            axes[1, 0].text(0.5, 0.5, 'No corruption data recorded',
                           ha='center', va='center', fontsize=12)

        # Field value analysis for corrupted reads
        if self.all_corruptions:
            # Show how many fields came from writer A vs B
            torn_data = []
            for c in self.all_corruptions:
                f1, f2, f3, f4 = c[2], c[3], c[4], c[5]
                from_a = sum(1 for f in [f1, f2, f3, f4]
                            if (f & 0x80000000) == 0)
                from_b = 4 - from_a
                torn_data.append((c[0], from_a, from_b))

            if torn_data:
                t = [d[0] for d in torn_data]
                fa = [d[1] for d in torn_data]
                fb = [d[2] for d in torn_data]
                axes[1, 1].scatter(t, fa, c='blue', s=15, alpha=0.5,
                                   label='Fields from Writer A')
                axes[1, 1].scatter(t, fb, c='orange', s=15, alpha=0.5,
                                   label='Fields from Writer B')
                axes[1, 1].set_xlabel('Time (seconds)')
                axes[1, 1].set_ylabel('Field Count')
                axes[1, 1].set_title('Torn Write Analysis')
                axes[1, 1].set_yticks([0, 1, 2, 3, 4])
                axes[1, 1].legend()
                axes[1, 1].grid(True, alpha=0.3)
        else:
            axes[1, 1].text(0.5, 0.5, 'No corruption data',
                           ha='center', va='center', fontsize=12)

        plt.tight_layout()
        plt.savefig('critical_section_analysis.png', dpi=150,
                    bbox_inches='tight')
        print("[*] Plot saved to: critical_section_analysis.png")
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='STM32 Critical Section Debug Monitor')
    parser.add_argument('--port', '-p', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--csv', '-c', default='critical_section_log.csv',
                        help='CSV output file')
    parser.add_argument('--plot', action='store_true',
                        help='Show plot after monitoring')
    args = parser.parse_args()

    monitor = CriticalSectionMonitor(args.port, args.baud, args.csv)
    monitor.run_realtime()

    if args.plot:
        monitor.plot_results()


if __name__ == '__main__':
    main()
