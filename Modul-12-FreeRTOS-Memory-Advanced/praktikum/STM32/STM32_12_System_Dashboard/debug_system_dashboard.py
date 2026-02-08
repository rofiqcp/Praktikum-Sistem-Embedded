#!/usr/bin/env python3
"""
debug_system_dashboard.py
Full dashboard parser with rich terminal display for STM32_12_System_Dashboard.

Features:
- Parses all DASH_* sections: SYS, TASKS, RUNTIME, HEAP, SYNC, STACK
- Real-time terminal display with colored sections
- Tracks metrics over time for trend analysis
- Matplotlib charts: heap usage, CPU runtime, stack watermarks
- CSV data logging
- Continuous monitoring mode

Usage:
    python debug_system_dashboard.py --port /dev/ttyUSB0 --baud 115200
    python debug_system_dashboard.py --file log.txt
    python debug_system_dashboard.py --port /dev/ttyUSB0 --continuous
"""

import argparse
import csv
import re
import sys
import time
import os
from datetime import datetime
from collections import defaultdict, OrderedDict

import serial
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np


# ANSI color codes for terminal
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    DIM = '\033[2m'
    RESET = '\033[0m'
    WHITE = '\033[97m'
    BG_BLUE = '\033[44m'


class DashboardAnalyzer:
    def __init__(self):
        self.dashboards = []
        self.current_dashboard = None
        self.heap_history = []
        self.task_runtime_history = []
        self.stack_history = []
        self.sys_info_history = []
        self.raw_lines = []
        self.sync_history = []

    def new_dashboard(self, number):
        self.current_dashboard = {
            'number': number,
            'time': time.time(),
            'sys': {},
            'tasks': [],
            'runtime': [],
            'heap': {},
            'sync': {},
            'stacks': {}
        }

    def finalize_dashboard(self):
        if self.current_dashboard:
            self.dashboards.append(self.current_dashboard)
            # Extract heap history
            if self.current_dashboard['heap']:
                self.heap_history.append({
                    'time': self.current_dashboard['time'],
                    'number': self.current_dashboard['number'],
                    **self.current_dashboard['heap']
                })
            # Extract stack history
            if self.current_dashboard['stacks']:
                self.stack_history.append({
                    'time': self.current_dashboard['time'],
                    'number': self.current_dashboard['number'],
                    **self.current_dashboard['stacks']
                })
            # Extract runtime history
            if self.current_dashboard['runtime']:
                entry = {
                    'time': self.current_dashboard['time'],
                    'number': self.current_dashboard['number']
                }
                for rt in self.current_dashboard['runtime']:
                    entry[rt['task']] = rt.get('percent', 0)
                self.task_runtime_history.append(entry)

    def parse_line(self, line):
        line = line.strip()
        if not line:
            return
        self.raw_lines.append(line)

        # Dashboard header
        m = re.match(r'\[DASHBOARD\].*Dashboard\s+#(\d+)', line)
        if m:
            if self.current_dashboard:
                self.finalize_dashboard()
            num = int(m.group(1))
            self.new_dashboard(num)
            print(f"\n{Colors.BG_BLUE}{Colors.WHITE}{Colors.BOLD}"
                  f" ══════ Dashboard #{num} ══════ {Colors.RESET}")
            return

        # Dashboard footer
        if '[DASHBOARD] ══' in line and self.current_dashboard:
            self.finalize_dashboard()
            print(f"{Colors.DIM}{'═' * 50}{Colors.RESET}")
            return

        # DASH_SYS entries
        m = re.match(r'\[DASH_SYS\]\s+(.+?)\s*:\s*(.+)', line)
        if m:
            key = m.group(1).strip()
            val = m.group(2).strip()
            if self.current_dashboard:
                self.current_dashboard['sys'][key] = val
            print(f"  {Colors.CYAN}[SYS]{Colors.RESET} {key:20s}: {Colors.WHITE}{val}{Colors.RESET}")
            return

        if '[DASH_SYS]' in line:
            m2 = re.match(r'\[DASH_SYS\]\s+(.+)', line)
            if m2:
                print(f"  {Colors.CYAN}[SYS]{Colors.RESET} {m2.group(1)}")
            return

        # DASH_TASKS entries
        m = re.match(r'\[DASH_TASKS\]\s+(\S+)\s+(\S)\s+(\d+)\s+(\d+)\s+(\d+)', line)
        if m:
            task = {
                'name': m.group(1),
                'state': m.group(2),
                'priority': int(m.group(3)),
                'stack': int(m.group(4)),
                'num': int(m.group(5))
            }
            if self.current_dashboard:
                self.current_dashboard['tasks'].append(task)
            state_color = Colors.GREEN if task['state'] == 'R' else \
                          Colors.YELLOW if task['state'] == 'B' else \
                          Colors.DIM if task['state'] == 'S' else Colors.WHITE
            print(f"  {Colors.BLUE}[TASK]{Colors.RESET} {task['name']:15s} "
                  f"state={state_color}{task['state']}{Colors.RESET} "
                  f"pri={task['priority']} stk={task['stack']}")
            return

        if '[DASH_TASKS]' in line:
            m2 = re.match(r'\[DASH_TASKS\]\s+(.+)', line)
            if m2:
                print(f"  {Colors.BLUE}[TASK]{Colors.RESET} {m2.group(1)}")
            return

        # DASH_RUNTIME entries
        m = re.match(r'\[DASH_RUNTIME\]\s+(\S+)\s+(\d+)\s+(\S+)', line)
        if m:
            task_name = m.group(1)
            abs_time = int(m.group(2))
            pct_str = m.group(3).replace('%', '').strip('<')
            try:
                pct = float(pct_str)
            except ValueError:
                pct = 0.0
            rt = {'task': task_name, 'abs_time': abs_time, 'percent': pct}
            if self.current_dashboard:
                self.current_dashboard['runtime'].append(rt)
            bar_len = int(pct / 2)
            bar = '█' * bar_len + '░' * (50 - bar_len)
            pct_color = Colors.RED if pct > 50 else \
                        Colors.YELLOW if pct > 20 else Colors.GREEN
            print(f"  {Colors.HEADER}[CPU]{Colors.RESET}  {task_name:15s} "
                  f"{pct_color}{pct:5.1f}%{Colors.RESET} |{bar}|")
            return

        if '[DASH_RUNTIME]' in line:
            m2 = re.match(r'\[DASH_RUNTIME\]\s+(.+)', line)
            if m2:
                print(f"  {Colors.HEADER}[CPU]{Colors.RESET}  {m2.group(1)}")
            return

        # DASH_HEAP entries
        m = re.match(r'\[DASH_HEAP\]\s+(.+?)\s*:\s*(\d+)(?:\s*(bytes|%))?', line)
        if m:
            key = m.group(1).strip()
            val = int(m.group(2))
            unit = m.group(3) if m.group(3) else ''
            if self.current_dashboard:
                self.current_dashboard['heap'][key] = val

            # Color code heap usage
            color = Colors.WHITE
            if 'Usage' in key:
                color = Colors.RED if val > 80 else Colors.YELLOW if val > 50 else Colors.GREEN
            elif 'Free' in key and 'block' not in key.lower() and 'count' not in key.lower():
                color = Colors.RED if val < 2000 else Colors.YELLOW if val < 5000 else Colors.GREEN

            print(f"  {Colors.GREEN}[HEAP]{Colors.RESET} {key:25s}: {color}{val}{Colors.RESET} {unit}")
            return

        if '[DASH_HEAP]' in line:
            m2 = re.match(r'\[DASH_HEAP\]\s+(.+)', line)
            if m2:
                print(f"  {Colors.GREEN}[HEAP]{Colors.RESET} {m2.group(1)}")
            return

        # DASH_SYNC entries
        m = re.match(r'\[DASH_SYNC\]\s+(\w+)\s*:\s*(.+)', line)
        if m:
            name = m.group(1)
            info = m.group(2)
            if self.current_dashboard:
                self.current_dashboard['sync'][name] = info
            print(f"  {Colors.YELLOW}[SYNC]{Colors.RESET} {name:15s}: {info}")
            return

        if '[DASH_SYNC]' in line:
            m2 = re.match(r'\[DASH_SYNC\]\s+(.+)', line)
            if m2:
                print(f"  {Colors.YELLOW}[SYNC]{Colors.RESET} {m2.group(1)}")
            return

        # DASH_STACK entries
        m = re.match(r'\[DASH_STACK\]\s+(\S+)\s*:\s*(\d+)\s+words', line)
        if m:
            task_name = m.group(1)
            words_free = int(m.group(2))
            if self.current_dashboard:
                self.current_dashboard['stacks'][task_name] = words_free
            color = Colors.RED if words_free < 20 else \
                    Colors.YELLOW if words_free < 50 else Colors.GREEN
            print(f"  {Colors.RED}[STK]{Colors.RESET}  {task_name:15s}: "
                  f"{color}{words_free} words{Colors.RESET} free (HWM)")
            return

        if '[DASH_STACK]' in line:
            m2 = re.match(r'\[DASH_STACK\]\s+(.+)', line)
            if m2:
                print(f"  {Colors.RED}[STK]{Colors.RESET}  {m2.group(1)}")
            return

        # Other lines
        if '[ERROR]' in line:
            print(f"  {Colors.RED}{Colors.BOLD}{line}{Colors.RESET}")

    def save_csv(self, filename):
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)

            # Heap history
            writer.writerow(['Dashboard#', 'Timestamp', 'Heap_Free', 'Heap_Used',
                              'Heap_Usage%', 'Largest_Block', 'Min_Ever_Free',
                              'Alloc_Count', 'Free_Count'])
            for h in self.heap_history:
                writer.writerow([
                    h.get('number', ''),
                    h.get('time', ''),
                    h.get('Free', ''),
                    h.get('Used', ''),
                    h.get('Usage', ''),
                    h.get('Largest free block', ''),
                    h.get('Min ever free', ''),
                    h.get('Successful allocs', ''),
                    h.get('Successful frees', '')
                ])

            writer.writerow([])
            writer.writerow(['Dashboard#', 'Task', 'CPU_Percent'])
            for rt in self.task_runtime_history:
                num = rt.get('number', '')
                for key, val in rt.items():
                    if key not in ('time', 'number'):
                        writer.writerow([num, key, val])

            writer.writerow([])
            writer.writerow(['Dashboard#', 'Task', 'Stack_HWM_Words'])
            for sh in self.stack_history:
                num = sh.get('number', '')
                for key, val in sh.items():
                    if key not in ('time', 'number'):
                        writer.writerow([num, key, val])

        print(f"\nData saved to {filename}")

    def plot_dashboard(self):
        fig, axes = plt.subplots(2, 2, figsize=(18, 14))
        fig.suptitle('STM32_12: FreeRTOS System Dashboard Analysis',
                     fontsize=14, fontweight='bold')

        # Plot 1: Heap usage over time
        ax1 = axes[0][0]
        if self.heap_history:
            nums = [h.get('number', 0) for h in self.heap_history]
            free = [h.get('Free', 0) for h in self.heap_history]
            used = [h.get('Used', 0) for h in self.heap_history]
            min_ever = [h.get('Min ever free', 0) for h in self.heap_history]

            ax1.stackplot(nums, used, free,
                         labels=['Used', 'Free'],
                         colors=['#e74c3c', '#2ecc71'], alpha=0.7)
            ax1.plot(nums, min_ever, 'k--', label='Min ever free', linewidth=2)
            ax1.set_xlabel('Dashboard Report #')
            ax1.set_ylabel('Bytes')
            ax1.set_title('Heap Usage Over Time')
            ax1.legend(loc='upper right')
            ax1.grid(alpha=0.3)

        # Plot 2: CPU runtime % per task (last report)
        ax2 = axes[0][1]
        if self.task_runtime_history:
            last_rt = self.task_runtime_history[-1]
            tasks = [k for k in last_rt if k not in ('time', 'number')]
            pcts = [last_rt[k] for k in tasks]
            colors = plt.cm.Set3(np.linspace(0, 1, len(tasks)))
            bars = ax2.bar(range(len(tasks)), pcts, color=colors, edgecolor='black')
            ax2.set_xticks(range(len(tasks)))
            ax2.set_xticklabels(tasks, rotation=30, ha='right', fontsize=9)
            ax2.set_ylabel('CPU %')
            ax2.set_title(f'CPU Runtime Distribution (Report #{last_rt.get("number", "?")})')
            ax2.grid(axis='y', alpha=0.3)
            for bar, pct in zip(bars, pcts):
                ax2.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.5,
                         f'{pct:.1f}%', ha='center', va='bottom', fontsize=8)

        # Plot 3: Stack high water marks (last report)
        ax3 = axes[1][0]
        if self.stack_history:
            last_stk = self.stack_history[-1]
            tasks = [k for k in last_stk if k not in ('time', 'number')]
            hwms = [last_stk[k] for k in tasks]
            colors = ['#e74c3c' if h < 20 else '#f39c12' if h < 50 else '#2ecc71'
                      for h in hwms]
            bars = ax3.barh(range(len(tasks)), hwms, color=colors, edgecolor='black')
            ax3.set_yticks(range(len(tasks)))
            ax3.set_yticklabels(tasks, fontsize=9)
            ax3.set_xlabel('Words Free (HWM)')
            ax3.set_title('Stack High Water Marks')
            ax3.axvline(x=20, color='red', linestyle='--', alpha=0.5, label='Danger (<20)')
            ax3.axvline(x=50, color='orange', linestyle='--', alpha=0.5, label='Warning (<50)')
            ax3.legend(fontsize=8)
            ax3.grid(axis='x', alpha=0.3)

        # Plot 4: Task CPU % over time
        ax4 = axes[1][1]
        if len(self.task_runtime_history) > 1:
            all_tasks = set()
            for rt in self.task_runtime_history:
                for k in rt:
                    if k not in ('time', 'number'):
                        all_tasks.add(k)
            colors_map = plt.cm.tab10(np.linspace(0, 1, len(all_tasks)))
            for i, task in enumerate(sorted(all_tasks)):
                nums = [rt.get('number', 0) for rt in self.task_runtime_history]
                vals = [rt.get(task, 0) for rt in self.task_runtime_history]
                ax4.plot(nums, vals, '-o', label=task, markersize=4,
                         color=colors_map[i])
            ax4.set_xlabel('Dashboard Report #')
            ax4.set_ylabel('CPU %')
            ax4.set_title('CPU Usage Trend Per Task')
            ax4.legend(fontsize=7, loc='upper right')
            ax4.grid(alpha=0.3)
        else:
            ax4.text(0.5, 0.5, 'Need multiple reports\nfor trend analysis',
                     transform=ax4.transAxes, ha='center', va='center',
                     fontsize=12, color='gray')
            ax4.set_title('CPU Usage Trend (insufficient data)')

        plt.tight_layout()
        plt.savefig('system_dashboard_analysis.png', dpi=150, bbox_inches='tight')
        print("\nPlot saved to system_dashboard_analysis.png")
        plt.show()


def monitor_serial(port, baud, duration=60, continuous=False):
    analyzer = DashboardAnalyzer()
    print(f"{Colors.BOLD}Connecting to {port} at {baud} baud...{Colors.RESET}")
    if continuous:
        print(f"{Colors.YELLOW}Continuous mode: Press Ctrl+C to stop{Colors.RESET}")
    else:
        print(f"Monitoring for {duration} seconds...")
    print()

    try:
        ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        start_time = time.time()
        try:
            while continuous or (time.time() - start_time) < duration:
                if ser.in_waiting:
                    try:
                        line = ser.readline().decode('utf-8', errors='replace').strip()
                        if line:
                            analyzer.parse_line(line)
                    except Exception as e:
                        print(f"Parse error: {e}")
        except KeyboardInterrupt:
            print(f"\n{Colors.YELLOW}Monitoring stopped by user{Colors.RESET}")

        ser.close()
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return None

    # Finalize any pending dashboard
    if analyzer.current_dashboard:
        analyzer.finalize_dashboard()

    return analyzer


def process_file(filename):
    analyzer = DashboardAnalyzer()
    print(f"Processing log file: {filename}\n")
    with open(filename, 'r') as f:
        for line in f:
            analyzer.parse_line(line)
    if analyzer.current_dashboard:
        analyzer.finalize_dashboard()
    return analyzer


def main():
    parser = argparse.ArgumentParser(description='FreeRTOS System Dashboard Analyzer')
    parser.add_argument('--port', type=str, default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--duration', type=int, default=60,
                        help='Monitoring duration in seconds (default: 60)')
    parser.add_argument('--file', type=str, default=None,
                        help='Process log file instead of serial')
    parser.add_argument('--csv', type=str, default='system_dashboard_log.csv',
                        help='CSV output filename')
    parser.add_argument('--no-plot', action='store_true',
                        help='Skip plotting')
    parser.add_argument('--continuous', action='store_true',
                        help='Continuous monitoring (Ctrl+C to stop)')

    args = parser.parse_args()

    if args.file:
        analyzer = process_file(args.file)
    else:
        analyzer = monitor_serial(args.port, args.baud, args.duration, args.continuous)

    if analyzer:
        analyzer.save_csv(args.csv)

        # Print summary
        print(f"\n{Colors.BOLD}{'=' * 60}{Colors.RESET}")
        print(f"{Colors.BOLD}SYSTEM DASHBOARD ANALYSIS SUMMARY{Colors.RESET}")
        print(f"{'=' * 60}")
        print(f"  Dashboard reports captured : {len(analyzer.dashboards)}")
        print(f"  Heap snapshots             : {len(analyzer.heap_history)}")
        print(f"  Runtime snapshots          : {len(analyzer.task_runtime_history)}")

        if analyzer.heap_history:
            last = analyzer.heap_history[-1]
            print(f"\n  Last Heap Status:")
            print(f"    Free   : {last.get('Free', 'N/A')} bytes")
            print(f"    Used   : {last.get('Used', 'N/A')} bytes")
            print(f"    Usage  : {last.get('Usage', 'N/A')}%")

        if analyzer.stack_history:
            last_stk = analyzer.stack_history[-1]
            min_task = None
            min_hwm = 9999
            for k, v in last_stk.items():
                if k not in ('time', 'number') and isinstance(v, int) and v < min_hwm:
                    min_hwm = v
                    min_task = k
            if min_task:
                color = Colors.RED if min_hwm < 20 else Colors.YELLOW if min_hwm < 50 else Colors.GREEN
                print(f"\n  Lowest stack HWM: {color}{min_task} = {min_hwm} words{Colors.RESET}")

        if not args.no_plot and len(analyzer.dashboards) > 0:
            analyzer.plot_dashboard()


if __name__ == '__main__':
    main()
