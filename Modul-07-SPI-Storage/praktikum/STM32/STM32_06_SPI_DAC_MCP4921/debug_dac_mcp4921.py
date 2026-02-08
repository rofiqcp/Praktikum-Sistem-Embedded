#!/usr/bin/env python3
"""
Debug script for STM32_06_SPI_DAC_MCP4921
Plot reconstructed waveform from serial DAC values.

Parses serial output and displays:
- Real-time waveform reconstruction plot
- DAC value vs expected waveform overlay
- Waveform type indicator
- Output voltage scale

Serial format expected:
  DAC:SINE,idx:123,val:2048,V:1.650,total:12345

Usage:
  python debug_dac_mcp4921.py [PORT] [BAUD]
  python debug_dac_mcp4921.py /dev/ttyUSB0 115200
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
VREF = 3.3
LUT_SIZE = 256
HISTORY_LEN = 512  # Show up to 2 full waveform cycles

# ==================== Parse Arguments ====================
port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

# ==================== Data Storage ====================
dac_values = deque(maxlen=HISTORY_LEN)
voltage_values = deque(maxlen=HISTORY_LEN)
sample_indices = deque(maxlen=HISTORY_LEN)
current_wave = "UNKNOWN"
total_samples = 0
lock = threading.Lock()
running = True

# ==================== Reference Waveforms ====================
def generate_reference_sine(n=LUT_SIZE):
    x = np.linspace(0, 2 * np.pi, n, endpoint=False)
    return (np.sin(x) + 1) / 2 * 4095

def generate_reference_sawtooth(n=LUT_SIZE):
    return np.linspace(0, 4095, n)

def generate_reference_triangle(n=LUT_SIZE):
    half = n // 2
    up = np.linspace(0, 4095, half)
    down = np.linspace(4095, 0, n - half)
    return np.concatenate([up, down])

def generate_reference_square(n=LUT_SIZE):
    wave = np.zeros(n)
    wave[n//2:] = 4095
    return wave

ref_waves = {
    'SINE': generate_reference_sine(),
    'SAWTOOTH': generate_reference_sawtooth(),
    'TRIANGLE': generate_reference_triangle(),
    'SQUARE': generate_reference_square(),
}

# ==================== Serial Parser ====================
# Match: DAC:SINE,idx:123,val:2048,V:1.650,total:12345
dac_pattern = re.compile(
    r'DAC:(\w+),idx:\s*(\d+),val:\s*(\d+),V:([\d.]+),total:(\d+)'
)
switch_pattern = re.compile(r'Waveform switched to:\s*(\w+)')

def serial_reader():
    """Background thread to read and parse serial data."""
    global running, current_wave, total_samples
    
    try:
        ser = serial.Serial(port, baud, timeout=1)
        print(f"[DEBUG] Connected to {port} @ {baud} baud")
        ser.reset_input_buffer()
        
        while running:
            try:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                
                # Parse DAC data
                m = dac_pattern.search(line)
                if m:
                    wave_name = m.group(1)
                    idx = int(m.group(2))
                    val = int(m.group(3))
                    voltage = float(m.group(4))
                    total = int(m.group(5))
                    
                    with lock:
                        current_wave = wave_name
                        total_samples = total
                        dac_values.append(val)
                        voltage_values.append(voltage)
                        sample_indices.append(idx)
                    continue
                
                # Parse waveform switch
                m = switch_pattern.search(line)
                if m:
                    with lock:
                        current_wave = m.group(1)
                        dac_values.clear()
                        voltage_values.clear()
                        sample_indices.clear()
                    print(f"  >>> Waveform: {current_wave}")
                    continue
                
                # Print other lines
                if line:
                    print(f"  {line}")
                    
            except (UnicodeDecodeError, ValueError):
                continue
                
    except serial.SerialException as e:
        print(f"[ERROR] Serial: {e}")
        running = False

# ==================== Matplotlib Setup ====================
fig, axes = plt.subplots(2, 1, figsize=(14, 9))
fig.suptitle('MCP4921 DAC Waveform Monitor', fontsize=14, fontweight='bold')

# Top: Reconstructed waveform (DAC values)
ax_wave = axes[0]
line_dac, = ax_wave.plot([], [], 'b-', linewidth=1.5, label='DAC Output')
line_ref, = ax_wave.plot([], [], 'r--', linewidth=1.0, alpha=0.5, label='Reference')
ax_wave.set_ylim(-100, 4200)
ax_wave.set_xlabel('Sample')
ax_wave.set_ylabel('DAC Value (0-4095)')
ax_wave.set_title('Reconstructed Waveform')
ax_wave.legend(loc='upper right')
ax_wave.grid(alpha=0.3)
ax_wave.axhline(y=2048, color='gray', linestyle=':', alpha=0.3)

# Bottom: Voltage output
ax_volt = axes[1]
line_volt, = ax_volt.plot([], [], 'g-', linewidth=1.5, label='Voltage')
ax_volt.set_ylim(-0.1, VREF * 1.1)
ax_volt.set_xlabel('Sample')
ax_volt.set_ylabel('Voltage (V)')
ax_volt.set_title('Output Voltage')
ax_volt.legend(loc='upper right')
ax_volt.grid(alpha=0.3)
ax_volt.axhline(y=VREF/2, color='gray', linestyle=':', alpha=0.3)
ax_volt.axhline(y=VREF, color='r', linestyle='--', alpha=0.3, label=f'VREF={VREF}V')

# Info text
info_text = ax_wave.text(0.02, 0.95, '', transform=ax_wave.transAxes,
                         fontsize=10, verticalalignment='top',
                         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))

plt.tight_layout()

# ==================== Animation Update ====================
def update(frame):
    with lock:
        dac_list = list(dac_values)
        volt_list = list(voltage_values)
        idx_list = list(sample_indices)
        wave = current_wave
        total = total_samples
    
    n = len(dac_list)
    if n > 0:
        x = list(range(n))
        
        # Update DAC waveform
        line_dac.set_data(x, dac_list)
        ax_wave.set_xlim(0, max(n, 10))
        
        # Update reference waveform
        if wave in ref_waves and n > 0:
            ref = ref_waves[wave]
            ref_x = np.linspace(0, n - 1, len(ref))
            line_ref.set_data(ref_x, ref)
        else:
            line_ref.set_data([], [])
        
        # Update voltage
        line_volt.set_data(x, volt_list)
        ax_volt.set_xlim(0, max(n, 10))
        
        # Update info text
        last_val = dac_list[-1] if dac_list else 0
        last_v = volt_list[-1] if volt_list else 0
        info_text.set_text(
            f'Waveform: {wave}\n'
            f'DAC: {last_val} | V: {last_v:.3f}V\n'
            f'Samples: {total}'
        )
    
    fig.suptitle(f'MCP4921 DAC Waveform Monitor | {wave}',
                 fontsize=14, fontweight='bold')
    
    return [line_dac, line_ref, line_volt, info_text]

# ==================== Main ====================
if __name__ == '__main__':
    print(f"[MCP4921 Debug] Starting on {port} @ {baud} baud")
    print(f"[MCP4921 Debug] VREF: {VREF}V, LUT Size: {LUT_SIZE}")
    print(f"[MCP4921 Debug] Close the plot window to exit.\n")
    
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
        print("\n[MCP4921 Debug] Stopped.")
