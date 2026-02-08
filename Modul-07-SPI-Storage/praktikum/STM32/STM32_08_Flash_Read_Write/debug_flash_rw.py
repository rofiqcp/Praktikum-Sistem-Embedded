#!/usr/bin/env python3
"""
Debug script for STM32_08_Flash_Read_Write
Parse flash operations, track data integrity, boot counter history.

Parses serial output and displays:
- Flash operation timeline
- Data integrity status
- Boot counter history plot
- Flash memory map visualization

Serial formats expected:
  "[FLASH] Writing calibration data..."
  "[PASS] All 16 halfwords verified correctly!"
  "Boot count: 5"
  Flash hex dump lines

Usage:
  python debug_flash_rw.py [PORT] [BAUD]
  python debug_flash_rw.py /dev/ttyUSB0 115200
"""

import sys
import re
import time
import threading
from collections import deque
from datetime import datetime

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np

# ==================== Configuration ====================
DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_BAUD = 115200
MAX_EVENTS = 100
MAX_BOOT_HISTORY = 50

# ==================== Parse Arguments ====================
port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

# ==================== Data Storage ====================
class FlashMonitor:
    def __init__(self):
        self.boot_counts = deque(maxlen=MAX_BOOT_HISTORY)
        self.boot_timestamps = deque(maxlen=MAX_BOOT_HISTORY)
        self.events = deque(maxlen=MAX_EVENTS)
        self.pass_count = 0
        self.fail_count = 0
        self.erase_count = 0
        self.write_count = 0
        self.read_count = 0
        self.calib_offset = 0
        self.calib_gain = 0.0
        self.calib_name = ""
        self.calib_valid = False
        self.flash_size_kb = 64
        self.page_size = 1024
        self.data_start_page = 62

monitor = FlashMonitor()
lock = threading.Lock()
running = True

# ==================== Serial Parser ====================
boot_count_pattern = re.compile(r'Boot count:\s*(\d+)')
pass_pattern = re.compile(r'\[PASS\]\s*(.*)')
fail_pattern = re.compile(r'\[FAIL\]\s*(.*)')
erase_pattern = re.compile(r'(?:Page erased|Erasing page)')
write_pattern = re.compile(r'(?:Writing|halfwords written)')
read_pattern = re.compile(r'(?:Reading calibration|Reading back)')
calib_pattern = re.compile(r'Calibration:\s*offset=([-\d]+),\s*gain=([\d.]+),\s*name="([^"]*)"')
flash_info_pattern = re.compile(r'Flash Size\s*:\s*(\d+)\s*KB')

def add_event(event_type, message):
    """Add a timestamped event to the log."""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    monitor.events.append((timestamp, event_type, message))

def serial_reader():
    """Background thread to read and parse serial data."""
    global running
    
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
                
                with lock:
                    # Boot counter
                    m = boot_count_pattern.search(line)
                    if m:
                        count = int(m.group(1))
                        monitor.boot_counts.append(count)
                        monitor.boot_timestamps.append(time.time())
                        add_event('BOOT', f'Count = {count}')
                    
                    # Pass/Fail
                    m = pass_pattern.search(line)
                    if m:
                        monitor.pass_count += 1
                        add_event('PASS', m.group(1))
                    
                    m = fail_pattern.search(line)
                    if m:
                        monitor.fail_count += 1
                        add_event('FAIL', m.group(1))
                    
                    # Operations
                    if erase_pattern.search(line):
                        monitor.erase_count += 1
                        add_event('ERASE', line.strip())
                    
                    if write_pattern.search(line):
                        monitor.write_count += 1
                        add_event('WRITE', line.strip())
                    
                    if read_pattern.search(line):
                        monitor.read_count += 1
                        add_event('READ', line.strip())
                    
                    # Calibration data
                    m = calib_pattern.search(line)
                    if m:
                        monitor.calib_offset = int(m.group(1))
                        monitor.calib_gain = float(m.group(2))
                        monitor.calib_name = m.group(3)
                        monitor.calib_valid = True
                    
                    # Flash info
                    m = flash_info_pattern.search(line)
                    if m:
                        monitor.flash_size_kb = int(m.group(1))
                    
            except (UnicodeDecodeError, ValueError):
                continue
                
    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")
        running = False

# ==================== Matplotlib Setup ====================
fig, axes = plt.subplots(2, 2, figsize=(15, 10))
fig.suptitle('STM32F103 Flash Read/Write Monitor', fontsize=14, fontweight='bold')

# Top-left: Boot counter history
ax_boot = axes[0][0]
line_boot, = ax_boot.plot([], [], 'b-o', linewidth=2, markersize=6, label='Boot Count')
ax_boot.set_xlabel('Reading #')
ax_boot.set_ylabel('Boot Count')
ax_boot.set_title('Boot Counter History')
ax_boot.legend()
ax_boot.grid(alpha=0.3)

# Top-right: Operation counts (bar chart)
ax_ops = axes[0][1]
op_names = ['Erase', 'Write', 'Read', 'Pass', 'Fail']
op_colors = ['#e74c3c', '#3498db', '#2ecc71', '#27ae60', '#c0392b']
op_bars = ax_ops.bar(op_names, [0]*5, color=op_colors, edgecolor='black')
ax_ops.set_ylabel('Count')
ax_ops.set_title('Flash Operation Counts')
ax_ops.grid(axis='y', alpha=0.3)

# Bottom-left: Flash memory map
ax_map = axes[1][0]
ax_map.set_title('Flash Memory Map')
ax_map.axis('off')

# Bottom-right: Event log and data integrity
ax_log = axes[1][1]
ax_log.axis('off')
log_text = ax_log.text(0.02, 0.98, '', transform=ax_log.transAxes,
                       fontsize=8, verticalalignment='top', fontfamily='monospace',
                       bbox=dict(boxstyle='round', facecolor='lightyellow', alpha=0.9))

plt.tight_layout()

# ==================== Draw Flash Memory Map ====================
def draw_memory_map():
    ax_map.clear()
    ax_map.set_title('Flash Memory Map (64KB)', fontsize=10)
    ax_map.set_xlim(0, 10)
    ax_map.set_ylim(0, 10)
    ax_map.axis('off')
    
    total_pages = monitor.flash_size_kb
    data_start = monitor.data_start_page
    
    # Draw flash pages as colored blocks
    cols = 16
    rows = total_pages // cols
    cell_w = 10.0 / cols
    cell_h = 6.0 / max(rows, 1)
    
    for page in range(total_pages):
        row = page // cols
        col = page % cols
        x = col * cell_w
        y = 8 - row * cell_h
        
        if page >= data_start:
            if page == data_start:
                color = '#e74c3c'  # Calibration page (red)
            else:
                color = '#f39c12'  # Boot counter page (orange)
        else:
            color = '#3498db'  # Application code (blue)
        
        rect = plt.Rectangle((x, y - cell_h), cell_w * 0.9, cell_h * 0.85,
                             facecolor=color, edgecolor='white', linewidth=0.5)
        ax_map.add_patch(rect)
    
    # Legend
    ax_map.text(0, 1.5, '■ App Code', color='#3498db', fontsize=9, fontweight='bold')
    ax_map.text(3.5, 1.5, '■ Calibration', color='#e74c3c', fontsize=9, fontweight='bold')
    ax_map.text(7, 1.5, '■ Boot Counter', color='#f39c12', fontsize=9, fontweight='bold')
    
    # Calibration info
    if monitor.calib_valid:
        ax_map.text(0, 0.5, 
                   f'Calibration: offset={monitor.calib_offset}, '
                   f'gain={monitor.calib_gain:.2f}, name="{monitor.calib_name}"',
                   fontsize=8, style='italic')

# ==================== Animation Update ====================
def update(frame):
    with lock:
        boot_list = list(monitor.boot_counts)
        events = list(monitor.events)
        erase_c = monitor.erase_count
        write_c = monitor.write_count
        read_c = monitor.read_count
        pass_c = monitor.pass_count
        fail_c = monitor.fail_count
    
    # Update boot counter plot
    if boot_list:
        line_boot.set_data(range(len(boot_list)), boot_list)
        ax_boot.set_xlim(0, max(len(boot_list), 1))
        ax_boot.set_ylim(0, max(boot_list) * 1.2 + 1)
    
    # Update operation bars
    values = [erase_c, write_c, read_c, pass_c, fail_c]
    for bar, val in zip(op_bars, values):
        bar.set_height(val)
    max_val = max(values) if any(v > 0 for v in values) else 1
    ax_ops.set_ylim(0, max_val * 1.3)
    
    # Draw memory map
    draw_memory_map()
    
    # Update event log
    log_lines = ["RECENT EVENTS", "=" * 45]
    for ts, etype, msg in events[-15:]:
        marker = {'PASS': '✓', 'FAIL': '✗', 'ERASE': '⌫', 
                 'WRITE': '✎', 'READ': '⇣', 'BOOT': '⟳'}.get(etype, '•')
        log_lines.append(f"{ts} {marker} [{etype:5s}] {msg[:40]}")
    
    log_lines.append("")
    log_lines.append(f"Total: {erase_c}E {write_c}W {read_c}R | "
                    f"Pass:{pass_c} Fail:{fail_c}")
    
    integrity = pass_c / (pass_c + fail_c) * 100 if (pass_c + fail_c) > 0 else 0
    log_lines.append(f"Data Integrity: {integrity:.1f}%")
    
    log_text.set_text('\n'.join(log_lines))
    
    fig.suptitle(f'STM32F103 Flash Monitor | Boots: {boot_list[-1] if boot_list else 0}',
                 fontsize=14, fontweight='bold')
    
    return [line_boot, *op_bars, log_text]

# ==================== Main ====================
if __name__ == '__main__':
    print(f"[Flash Debug] Starting on {port} @ {baud} baud")
    print(f"[Flash Debug] Close the plot window to exit.\n")
    
    reader_thread = threading.Thread(target=serial_reader, daemon=True)
    reader_thread.start()
    
    ani = animation.FuncAnimation(fig, update, interval=500, blit=False, cache_frame_data=False)
    
    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        print("\n[Flash Debug] Stopped.")
