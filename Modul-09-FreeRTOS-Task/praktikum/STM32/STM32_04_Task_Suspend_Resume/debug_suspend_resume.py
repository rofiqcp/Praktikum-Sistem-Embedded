#!/usr/bin/env python3
"""
==========================================================================
 Debug Serial Parser - STM32_04_Task_Suspend_Resume
==========================================================================
 Mem-parse output serial dari program Task Suspend/Resume FreeRTOS.
 Mengekstrak event suspend/resume, state task, dan button press.

 Penggunaan:
   python debug_suspend_resume.py --port /dev/ttyUSB0
   python debug_suspend_resume.py --file capture.log
   python debug_suspend_resume.py --demo

 Output:
   - Timeline state task (Running/Suspended)
   - Event suspend/resume/button press
   - Statistik cycle supervisor
==========================================================================
"""

import argparse
import sys
import re
import time
import os
from datetime import datetime
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class SuspendResumeParser:
    """Parser dan visualizer untuk data Task Suspend/Resume."""

    def __init__(self):
        self.events = []           # [(time, event_type, detail)]
        self.task_states = []      # [(time, state)]  state: 'Running','Suspended'
        self.cycle_data = []       # [(time, cycle, state_str)]
        self.data_counts = []      # [(time, packet_num)]
        self.total_lines = 0
        self.data_lines = 0
        self.start_time = time.time()
        self.current_state = 'Running'

    def parse_line(self, line):
        self.total_lines += 1
        line = line.strip()
        if not line:
            return
        t = time.time() - self.start_time
        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

        # Detect suspend events
        if 'SUSPEND' in line and ('ERROR' in line or 'error' in line.lower()):
            self.current_state = 'Suspended'
            self.events.append((t, 'suspend', line))
            self.task_states.append((t, 0))
            print(f"[{ts}] 🔴 SUSPEND: {line}")
            self.data_lines += 1
            return

        # Detect resume events
        if 'RESUME' in line and ('CLEAR' in line or 'resume' in line.lower()):
            self.current_state = 'Running'
            self.events.append((t, 'resume', line))
            self.task_states.append((t, 1))
            print(f"[{ts}] 🟢 RESUME: {line}")
            self.data_lines += 1
            return

        # Detect button press (toggle via ISR)
        if 'BUTTON' in line.upper() or 'TOGGLE' in line.upper():
            self.events.append((t, 'button', line))
            print(f"[{ts}] 🔘 BUTTON: {line}")
            self.data_lines += 1
            return

        # Detect supervisor cycle with state
        m = re.search(r'\[SUPERVISOR\]\s*Cycle:\s*(\d+).*Processor:\s*(\w+)', line)
        if m:
            cycle = int(m.group(1))
            state_str = m.group(2)
            is_running = 1 if state_str.lower() != 'suspended' else 0
            self.cycle_data.append((t, cycle, state_str))
            self.task_states.append((t, is_running))
            self.data_lines += 1
            emoji = '✅' if is_running else '⏸️'
            print(f"[{ts}] {emoji} Cycle {cycle}: {state_str}")
            return

        # Detect processor data packet
        m2 = re.search(r'\[PROCESSOR\].*paket\s*#(\d+)', line)
        if m2:
            pkt = int(m2.group(1))
            self.data_counts.append((t, pkt))
            if pkt % 5 == 0:
                print(f"[{ts}]    Processor paket #{pkt}")
            return

        # Generic output
        if '!!!!' in line:
            print(f"[{ts}] 🚨 {line}")
        elif '>>>' in line:
            print(f"[{ts}] 💚 {line}")
        else:
            print(f"[{ts}] {line}")

    def print_summary(self):
        suspend_count = sum(1 for _, e, _ in self.events if e == 'suspend')
        resume_count = sum(1 for _, e, _ in self.events if e == 'resume')
        button_count = sum(1 for _, e, _ in self.events if e == 'button')
        print(f"\n{'='*55}")
        print(f" RINGKASAN SUSPEND/RESUME")
        print(f"{'='*55}")
        print(f"  Total lines parsed : {self.total_lines}")
        print(f"  Event lines        : {self.data_lines}")
        print(f"  Suspend events     : {suspend_count}")
        print(f"  Resume events      : {resume_count}")
        print(f"  Button events      : {button_count}")
        print(f"  Supervisor cycles  : {len(self.cycle_data)}")
        print(f"  Data packets       : {len(self.data_counts)}")
        print(f"{'='*55}")

    def plot(self):
        if not HAS_MATPLOTLIB:
            print("[WARN] matplotlib tidak tersedia, skip plot.")
            return
        if not self.task_states and not self.events:
            print("[WARN] Tidak ada data untuk di-plot.")
            return

        fig, axes = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
        fig.suptitle('STM32_04 Task Suspend/Resume Analysis', fontsize=14)

        # Plot 1: Task state timeline
        ax1 = axes[0]
        if self.task_states:
            times = [s[0] for s in self.task_states]
            states = [s[1] for s in self.task_states]
            ax1.step(times, states, where='post', linewidth=2, color='#2196F3')
            ax1.fill_between(times, states, step='post', alpha=0.3, color='#2196F3')
        # Mark suspend/resume events
        for t, etype, _ in self.events:
            if etype == 'suspend':
                ax1.axvline(x=t, color='red', linestyle='--', alpha=0.7, linewidth=1)
            elif etype == 'resume':
                ax1.axvline(x=t, color='green', linestyle='--', alpha=0.7, linewidth=1)
            elif etype == 'button':
                ax1.axvline(x=t, color='orange', linestyle=':', alpha=0.7, linewidth=1)
        ax1.set_ylabel('Task State')
        ax1.set_yticks([0, 1])
        ax1.set_yticklabels(['Suspended', 'Running'])
        ax1.set_title('Processor Task State Timeline')
        ax1.grid(True, alpha=0.3)
        p_sus = mpatches.Patch(color='red', alpha=0.5, label='Suspend')
        p_res = mpatches.Patch(color='green', alpha=0.5, label='Resume')
        p_btn = mpatches.Patch(color='orange', alpha=0.5, label='Button')
        ax1.legend(handles=[p_sus, p_res, p_btn], loc='upper right', fontsize=8)

        # Plot 2: Data packets over time
        ax2 = axes[1]
        if self.data_counts:
            times = [d[0] for d in self.data_counts]
            pkts = [d[1] for d in self.data_counts]
            ax2.plot(times, pkts, 'o-', markersize=2, color='#4CAF50', linewidth=1)
        ax2.set_xlabel('Time (s)')
        ax2.set_ylabel('Packet #')
        ax2.set_title('Data Packets Processed (gaps = suspended)')
        ax2.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig('suspend_resume_analysis.png', dpi=150)
        print("[INFO] Plot disimpan: suspend_resume_analysis.png")
        plt.show()


def generate_demo_data():
    """Generate simulated suspend/resume data."""
    lines = []
    lines.append("=== Task Suspend/Resume Demo ===")
    lines.append("[SUPERVISOR] Simulasi error setiap 8 cycle")
    lines.append("[SUPERVISOR] Auto-resume setelah 3 cycle dari error")
    pkt = 0
    for cycle in range(1, 41):
        if cycle % 8 == 0:
            state = 'Suspended'
            lines.append(f"[SUPERVISOR] Cycle: {cycle}, Processor: {state}")
            lines.append("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            lines.append("!!! ERROR TERDETEKSI - SUSPEND PROCESSOR !!!")
            lines.append("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        elif cycle % 8 == 3:
            lines.append(">>> ERROR CLEARED - RESUME PROCESSOR >>>")
            state = 'Running'
            lines.append(f"[SUPERVISOR] Cycle: {cycle}, Processor: {state}")
        else:
            state = 'Running' if (cycle % 8) < 8 and (cycle % 8) != 0 else 'Suspended'
            if (cycle % 8) > 0 and (cycle % 8) < 8 and not (1 <= (cycle % 8) <= 2):
                state = 'Running'
            else:
                state = 'Suspended'
            lines.append(f"[SUPERVISOR] Cycle: {cycle}, Processor: {state}")
        if state == 'Running':
            for _ in range(3):
                pkt += 1
                lines.append(f"[PROCESSOR] Memproses data paket #{pkt}")
        if cycle == 15:
            lines.append("[BUTTON] Toggle via tombol PA0")
    return lines


def main():
    parser = argparse.ArgumentParser(
        description='Debug parser untuk STM32_04 Task Suspend/Resume')
    parser.add_argument('--port', type=str, help='Serial port (e.g. /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', type=str, help='File log untuk di-parse')
    parser.add_argument('--demo', action='store_true', help='Jalankan dengan data demo')
    args = parser.parse_args()

    p = SuspendResumeParser()

    if args.demo:
        print("[INFO] Mode DEMO - menggunakan data simulasi\n")
        for line in generate_demo_data():
            p.parse_line(line)
            time.sleep(0.02)
        p.print_summary()
        p.plot()
    elif args.file:
        print(f"[INFO] Membaca file: {args.file}\n")
        with open(args.file, 'r', errors='replace') as f:
            for line in f:
                p.parse_line(line)
        p.print_summary()
        p.plot()
    elif args.port:
        if not HAS_SERIAL:
            print("[ERROR] pyserial belum terinstall. pip install pyserial")
            sys.exit(1)
        print(f"[INFO] Membuka {args.port} @ {args.baud} baud\n")
        try:
            ser = serial.Serial(args.port, args.baud, timeout=1)
            while True:
                raw = ser.readline()
                if raw:
                    p.parse_line(raw.decode('utf-8', errors='replace'))
        except KeyboardInterrupt:
            print("\n[INFO] Dihentikan oleh user.")
            ser.close()
            p.print_summary()
            p.plot()
        except serial.SerialException as e:
            print(f"[ERROR] Serial: {e}")
            sys.exit(1)
    else:
        parser.print_help()
        print("\nContoh: python debug_suspend_resume.py --demo")


if __name__ == '__main__':
    main()
