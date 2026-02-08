#!/usr/bin/env python3
"""
Debug script for STM32_07_SPI_Multi_Slave
Per-slave statistics and timing comparison plot.

Parses serial output and displays:
- Per-slave transfer timing bar chart
- Transfer timing history for each slave
- Error rate comparison
- Bus utilization statistics

Serial formats expected:
  Individual: "Slave-1 (PA4): TX=[0xA1,...] RX=[...] 12us OK"
  Back-to-back: "4 transfers completed in 48 us"
  Stats table lines

Usage:
  python debug_multi_slave.py [PORT] [BAUD]
  python debug_multi_slave.py /dev/ttyUSB0 115200
"""

import sys
import re
import time
import threading
from collections import deque

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np

# ==================== Configuration ====================
DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_BAUD = 115200
HISTORY_LEN = 200

# ==================== Parse Arguments ====================
port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

# ==================== Data Storage ====================
class SlaveData:
    def __init__(self, name):
        self.name = name
        self.timing_history = deque(maxlen=HISTORY_LEN)
        self.tx_count = 0
        self.rx_count = 0
        self.error_count = 0
        self.last_time_us = 0
        self.min_time_us = float('inf')
        self.max_time_us = 0
        self.total_bytes = 0

slave1 = SlaveData("Slave-1")
slave2 = SlaveData("Slave-2")
back_to_back_times = deque(maxlen=HISTORY_LEN)
cycle_count = 0
lock = threading.Lock()
running = True

# ==================== Serial Parser ====================
# Match transfer results: "Slave-1 (PA4): TX=[...] RX=[...] 12us OK"
transfer_pattern = re.compile(
    r'(Slave-\d)\s*\(.*?\):\s*TX=\[.*?\]\s*RX=\[.*?\]\s*(\d+)us\s*(OK|ERR)'
)
# Match back-to-back: "4 transfers completed in 48 us"
b2b_pattern = re.compile(r'(\d+)\s*transfers completed in\s*(\d+)\s*us')
# Match cycle
cycle_pattern = re.compile(r'Cycle #(\d+)')
# Match stats lines
stats_pattern = re.compile(r'(TX Count|RX Count|Errors|Total Bytes|Last Time|Min Time|Max Time|Avg Time)\s*\|\s*(\d+)')

def serial_reader():
    """Background thread to read and parse serial data."""
    global running, cycle_count
    
    try:
        ser = serial.Serial(port, baud, timeout=1)
        print(f"[DEBUG] Connected to {port} @ {baud} baud")
        ser.reset_input_buffer()
        
        while running:
            try:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                
                print(f"  {line}")
                
                # Parse cycle
                m = cycle_pattern.search(line)
                if m:
                    cycle_count = int(m.group(1))
                    continue
                
                # Parse individual transfer
                m = transfer_pattern.search(line)
                if m:
                    slave_name = m.group(1)
                    time_us = int(m.group(2))
                    status = m.group(3)
                    
                    with lock:
                        if 'Slave-1' in slave_name:
                            s = slave1
                        else:
                            s = slave2
                        
                        s.last_time_us = time_us
                        s.timing_history.append(time_us)
                        s.tx_count += 1
                        
                        if status == 'OK':
                            s.rx_count += 1
                        else:
                            s.error_count += 1
                        
                        if time_us < s.min_time_us:
                            s.min_time_us = time_us
                        if time_us > s.max_time_us:
                            s.max_time_us = time_us
                    continue
                
                # Parse back-to-back
                m = b2b_pattern.search(line)
                if m:
                    total_us = int(m.group(2))
                    with lock:
                        back_to_back_times.append(total_us)
                    continue
                    
            except (UnicodeDecodeError, ValueError):
                continue
                
    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")
        running = False

# ==================== Matplotlib Setup ====================
fig, axes = plt.subplots(2, 2, figsize=(15, 9))
fig.suptitle('SPI Multi-Slave Communication Monitor', fontsize=14, fontweight='bold')

# Top-left: Transfer timing comparison (bar)
ax_bar = axes[0][0]
bar_labels = ['Slave-1\nLast', 'Slave-2\nLast', 'Slave-1\nMin', 'Slave-2\nMin',
              'Slave-1\nMax', 'Slave-2\nMax']
bar_colors = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12', '#9b59b6', '#e67e22']
bars = ax_bar.bar(range(6), [0]*6, color=bar_colors, edgecolor='black')
ax_bar.set_xticks(range(6))
ax_bar.set_xticklabels(bar_labels, fontsize=7)
ax_bar.set_ylabel('Time (μs)')
ax_bar.set_title('Transfer Timing Comparison')
ax_bar.grid(axis='y', alpha=0.3)

# Top-right: Timing history line plot
ax_hist = axes[0][1]
line_s1, = ax_hist.plot([], [], 'b-', linewidth=1.2, label='Slave-1', marker='.', markersize=3)
line_s2, = ax_hist.plot([], [], 'r-', linewidth=1.2, label='Slave-2', marker='.', markersize=3)
ax_hist.set_xlabel('Transfer #')
ax_hist.set_ylabel('Time (μs)')
ax_hist.set_title('Transfer Timing History')
ax_hist.legend(loc='upper right')
ax_hist.grid(alpha=0.3)

# Bottom-left: Statistics text
ax_stats = axes[1][0]
ax_stats.axis('off')
stats_text = ax_stats.text(0.05, 0.95, '', transform=ax_stats.transAxes,
                           fontsize=9, verticalalignment='top', fontfamily='monospace',
                           bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.9))

# Bottom-right: Back-to-back timing
ax_b2b = axes[1][1]
line_b2b, = ax_b2b.plot([], [], 'g-', linewidth=1.5, marker='o', markersize=4, label='B2B Total')
ax_b2b.set_xlabel('Transfer Set #')
ax_b2b.set_ylabel('Total Time (μs)')
ax_b2b.set_title('Back-to-Back Transfer Timing')
ax_b2b.legend(loc='upper right')
ax_b2b.grid(alpha=0.3)

plt.tight_layout()

# ==================== Animation Update ====================
def update(frame):
    with lock:
        s1_hist = list(slave1.timing_history)
        s2_hist = list(slave2.timing_history)
        b2b_list = list(back_to_back_times)
        
        s1_last = slave1.last_time_us
        s2_last = slave2.last_time_us
        s1_min = slave1.min_time_us if slave1.min_time_us != float('inf') else 0
        s2_min = slave2.min_time_us if slave2.min_time_us != float('inf') else 0
        s1_max = slave1.max_time_us
        s2_max = slave2.max_time_us
        
        s1_tx = slave1.tx_count
        s2_tx = slave2.tx_count
        s1_err = slave1.error_count
        s2_err = slave2.error_count
    
    # Update bar chart
    values = [s1_last, s2_last, s1_min, s2_min, s1_max, s2_max]
    for bar, val in zip(bars, values):
        bar.set_height(val)
    max_val = max(values) if any(v > 0 for v in values) else 10
    ax_bar.set_ylim(0, max_val * 1.3)
    
    # Add value labels on bars
    for bar, val in zip(bars, values):
        if val > 0:
            ax_bar.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                       f'{val}', ha='center', va='bottom', fontsize=7)
    
    # Update timing history
    if s1_hist:
        line_s1.set_data(range(len(s1_hist)), s1_hist)
    if s2_hist:
        line_s2.set_data(range(len(s2_hist)), s2_hist)
    max_len = max(len(s1_hist), len(s2_hist), 1)
    ax_hist.set_xlim(0, max_len)
    all_times = s1_hist + s2_hist
    if all_times:
        ax_hist.set_ylim(0, max(all_times) * 1.3)
    
    # Update stats text
    s1_avg = sum(s1_hist) / len(s1_hist) if s1_hist else 0
    s2_avg = sum(s2_hist) / len(s2_hist) if s2_hist else 0
    s1_err_rate = (s1_err / s1_tx * 100) if s1_tx > 0 else 0
    s2_err_rate = (s2_err / s2_tx * 100) if s2_tx > 0 else 0
    
    stats_str = (
        f"{'':>14} {'Slave-1':>12} {'Slave-2':>12}\n"
        f"{'─'*40}\n"
        f"{'TX Count':>14} {s1_tx:>12} {s2_tx:>12}\n"
        f"{'Errors':>14} {s1_err:>12} {s2_err:>12}\n"
        f"{'Error Rate':>14} {s1_err_rate:>10.1f}% {s2_err_rate:>10.1f}%\n"
        f"{'Last (μs)':>14} {s1_last:>12} {s2_last:>12}\n"
        f"{'Min (μs)':>14} {s1_min:>12} {s2_min:>12}\n"
        f"{'Max (μs)':>14} {s1_max:>12} {s2_max:>12}\n"
        f"{'Avg (μs)':>14} {s1_avg:>12.1f} {s2_avg:>12.1f}\n"
        f"{'─'*40}\n"
        f"Cycle: {cycle_count}"
    )
    stats_text.set_text(stats_str)
    
    # Update back-to-back plot
    if b2b_list:
        line_b2b.set_data(range(len(b2b_list)), b2b_list)
        ax_b2b.set_xlim(0, max(len(b2b_list), 1))
        ax_b2b.set_ylim(0, max(b2b_list) * 1.3)
    
    fig.suptitle(f'SPI Multi-Slave Monitor | Cycle: {cycle_count}',
                 fontsize=14, fontweight='bold')
    
    return [*bars, line_s1, line_s2, stats_text, line_b2b]

# ==================== Main ====================
if __name__ == '__main__':
    print(f"[Multi-Slave Debug] Starting on {port} @ {baud} baud")
    print(f"[Multi-Slave Debug] Close the plot window to exit.\n")
    
    reader_thread = threading.Thread(target=serial_reader, daemon=True)
    reader_thread.start()
    
    ani = animation.FuncAnimation(fig, update, interval=300, blit=False, cache_frame_data=False)
    
    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        print("\n[Multi-Slave Debug] Stopped.")
