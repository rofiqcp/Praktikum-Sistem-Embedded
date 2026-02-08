#!/usr/bin/env python3
"""
Debug script for STM32_05_SPI_MCP3208_ADC
Real-time 8-channel voltage plot with matplotlib.

Parses serial output from MCP3208 ADC and displays:
- Real-time voltage bar chart for all 8 channels
- Voltage history line plot
- Statistics panel

Serial format expected:
  " 0  | 1234 | 1.23V  | [###..."  (table rows from read_all_channels)

Usage:
  python debug_mcp3208.py [PORT] [BAUD]
  python debug_mcp3208.py COM3 115200
  python debug_mcp3208.py /dev/ttyUSB0
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
NUM_CHANNELS = 8
VREF = 3.3
HISTORY_LEN = 100  # Number of data points to keep per channel

# ==================== Parse Arguments ====================
port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

# ==================== Data Storage ====================
voltages = [0.0] * NUM_CHANNELS
raw_values = [0] * NUM_CHANNELS
voltage_history = [deque(maxlen=HISTORY_LEN) for _ in range(NUM_CHANNELS)]
timestamps = deque(maxlen=HISTORY_LEN)
lock = threading.Lock()
running = True
cycle_count = 0

# ==================== Serial Parser ====================
# Match lines like: " 0  | 1234 | 1.23V  | [###..."
channel_pattern = re.compile(r'^\s*(\d)\s*\|\s*(\d+)\s*\|\s*([\d.]+)V')
cycle_pattern = re.compile(r'Cycle #(\d+)')

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
                    
                # Print raw serial data
                print(f"  {line}")
                
                # Parse cycle number
                m = cycle_pattern.search(line)
                if m:
                    cycle_count = int(m.group(1))
                
                # Parse channel data
                m = channel_pattern.match(line)
                if m:
                    ch = int(m.group(1))
                    raw = int(m.group(2))
                    voltage = float(m.group(3))
                    
                    if 0 <= ch < NUM_CHANNELS:
                        with lock:
                            voltages[ch] = voltage
                            raw_values[ch] = raw
                            voltage_history[ch].append(voltage)
                        
                        # Add timestamp for channel 0
                        if ch == 0:
                            with lock:
                                timestamps.append(time.time())
                                
            except (UnicodeDecodeError, ValueError):
                continue
                
    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")
        running = False

# ==================== Matplotlib Setup ====================
fig, axes = plt.subplots(2, 1, figsize=(14, 9))
fig.suptitle('MCP3208 8-Channel ADC Monitor', fontsize=14, fontweight='bold')

colors = plt.cm.tab10(np.linspace(0, 1, NUM_CHANNELS))

# Top: Bar chart of current voltages
ax_bar = axes[0]
bars = ax_bar.bar(range(NUM_CHANNELS), [0]*NUM_CHANNELS, color=colors, edgecolor='black')
ax_bar.set_xlim(-0.5, NUM_CHANNELS - 0.5)
ax_bar.set_ylim(0, VREF * 1.1)
ax_bar.set_xlabel('Channel')
ax_bar.set_ylabel('Voltage (V)')
ax_bar.set_title('Current Channel Voltages')
ax_bar.set_xticks(range(NUM_CHANNELS))
ax_bar.axhline(y=VREF, color='r', linestyle='--', alpha=0.5, label=f'VREF={VREF}V')
ax_bar.legend(loc='upper right')
ax_bar.grid(axis='y', alpha=0.3)

# Voltage labels on bars
bar_labels = []
for i in range(NUM_CHANNELS):
    label = ax_bar.text(i, 0, '', ha='center', va='bottom', fontsize=8, fontweight='bold')
    bar_labels.append(label)

# Bottom: Voltage history line plot
ax_line = axes[1]
lines = []
for ch in range(NUM_CHANNELS):
    line, = ax_line.plot([], [], color=colors[ch], linewidth=1.2, label=f'CH{ch}')
    lines.append(line)
ax_line.set_ylim(0, VREF * 1.1)
ax_line.set_xlabel('Sample')
ax_line.set_ylabel('Voltage (V)')
ax_line.set_title('Voltage History')
ax_line.legend(loc='upper right', ncol=4, fontsize=7)
ax_line.grid(alpha=0.3)

plt.tight_layout()

# ==================== Animation Update ====================
def update(frame):
    with lock:
        # Update bar chart
        for i, bar in enumerate(bars):
            bar.set_height(voltages[i])
            bar_labels[i].set_position((i, voltages[i]))
            bar_labels[i].set_text(f'{voltages[i]:.2f}V\n({raw_values[i]})')
        
        # Update line chart
        for ch in range(NUM_CHANNELS):
            hist = list(voltage_history[ch])
            if hist:
                lines[ch].set_data(range(len(hist)), hist)
        
        # Adjust x-axis for history
        max_len = max(len(voltage_history[ch]) for ch in range(NUM_CHANNELS))
        if max_len > 0:
            ax_line.set_xlim(0, max(max_len, 10))
    
    fig.suptitle(f'MCP3208 8-Channel ADC Monitor | Cycle: {cycle_count}', 
                 fontsize=14, fontweight='bold')
    
    return list(bars) + bar_labels + lines

# ==================== Main ====================
if __name__ == '__main__':
    print(f"[MCP3208 Debug] Starting on {port} @ {baud} baud")
    print(f"[MCP3208 Debug] Channels: {NUM_CHANNELS}, VREF: {VREF}V")
    print(f"[MCP3208 Debug] Close the plot window to exit.\n")
    
    # Start serial reader thread
    reader_thread = threading.Thread(target=serial_reader, daemon=True)
    reader_thread.start()
    
    # Start animation
    ani = animation.FuncAnimation(fig, update, interval=200, blit=False, cache_frame_data=False)
    
    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        print("\n[MCP3208 Debug] Stopped.")
