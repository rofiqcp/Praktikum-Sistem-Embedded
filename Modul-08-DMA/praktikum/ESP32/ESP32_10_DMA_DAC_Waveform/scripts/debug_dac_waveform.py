"""
==========================================================================
 ESP32 DAC Waveform - Debug & Visualization
==========================================================================
 Modul 08 - Program 10: Expected vs actual waveform, frequency analysis
 
 Usage: python debug_dac_waveform.py [COM_PORT]
==========================================================================
"""

import serial
import re
import time
import sys
import math
from collections import deque
from datetime import datetime

# ---- Configuration ----
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
BAUD_RATE = 115200
MAX_POINTS = 200
WAVEFORM_BUF_SIZE = 256

# ---- Data Storage ----
actual_freqs = deque(maxlen=MAX_POINTS)
sample_rates = deque(maxlen=MAX_POINTS)
waveform_types = deque(maxlen=MAX_POINTS)
timestamps = deque(maxlen=MAX_POINTS)

# ---- Regex Patterns ----
re_waveform = re.compile(r'Waveform:\s+(\w+)')
re_target_freq = re.compile(r'Target Freq:\s+(\d+)\s+Hz')
re_actual_freq = re.compile(r'Actual Freq:\s+([\d.]+)\s+Hz')
re_sample_rate = re.compile(r'Sample Rate:\s+([\d.]+)\s+Hz')
re_vpp = re.compile(r'Vpp:\s+([\d.]+)\s+V')
re_samples = re.compile(r'Total Samples:\s+(\d+)')

def generate_waveform(wtype, n_points=80):
    """Generate expected waveform for display."""
    wave = []
    for i in range(n_points):
        phase = i / n_points
        if wtype == 'SINE':
            wave.append(0.5 + 0.5 * math.sin(2 * math.pi * phase))
        elif wtype == 'SQUARE':
            wave.append(1.0 if phase < 0.5 else 0.0)
        elif wtype == 'TRIANGLE':
            wave.append(phase * 2 if phase < 0.5 else (1 - phase) * 2)
        elif wtype == 'SAWTOOTH':
            wave.append(phase)
        else:
            wave.append(0.5)
    return wave

def print_ascii_waveform(wave, label, width=60, height=12):
    """Print ASCII waveform."""
    print(f"\n  {label}")
    print(f"  {'─' * (width + 4)}")
    
    step = max(1, len(wave) // width)
    sampled = [wave[i * step] for i in range(min(width, len(wave) // step))]
    
    for row in range(height):
        threshold = 1.0 - row / (height - 1)
        line = "  │"
        for val in sampled:
            if abs(val - threshold) < (0.5 / height):
                line += "●"
            elif val > threshold:
                line += "│" if row == height // 2 else " "
            else:
                line += " "
        line += "│"
        
        if row == 0:
            line += " 3.3V"
        elif row == height - 1:
            line += " 0.0V"
        elif row == height // 2:
            line += " 1.65V"
        
        print(line)
    
    print(f"  {'─' * (width + 4)}")

def print_dashboard(data_acc):
    """Print debug dashboard."""
    print("\033[2J\033[H")
    print("=" * 65)
    print("   ESP32 DAC Waveform Output - Debug Dashboard")
    print("=" * 65)
    
    wtype = data_acc.get('waveform', 'SINE')
    target_f = data_acc.get('target_freq', 1000)
    actual_f = data_acc.get('actual_freq', 0)
    sr = data_acc.get('sample_rate', 0)
    vpp = data_acc.get('vpp', 3.3)
    
    print(f"\n  Waveform:    {wtype}")
    print(f"  Target:      {target_f} Hz")
    print(f"  Actual:      {actual_f:.1f} Hz")
    print(f"  Sample Rate: {sr:.0f} Hz")
    print(f"  Vpp:         {vpp:.2f} V")
    
    # Frequency accuracy
    if target_f > 0 and actual_f > 0:
        error_pct = abs(actual_f - target_f) / target_f * 100
        color = '\033[92m' if error_pct < 5 else '\033[93m' if error_pct < 20 else '\033[91m'
        print(f"  Freq Error:  {color}{error_pct:.1f}%\033[0m")
    
    # Expected waveform
    wave = generate_waveform(wtype)
    print_ascii_waveform(wave, f"Expected {wtype} Waveform (GPIO25)")
    
    # Frequency history
    if len(actual_freqs) > 3:
        print(f"\n  Frequency History (last 10):")
        recent = list(zip(list(waveform_types)[-10:], list(actual_freqs)[-10:]))
        for i, (wt, f) in enumerate(recent):
            bar_len = int(f / 50)
            bar_len = min(bar_len, 30)
            print(f"    {wt:10s} │{'█' * bar_len}│ {f:.0f} Hz")
    
    # Frequency analysis
    if actual_f > 0:
        nyquist = sr / 2 if sr > 0 else 0
        print(f"\n  {'─' * 50}")
        print(f"  FREQUENCY ANALYSIS")
        print(f"  {'─' * 50}")
        print(f"  Nyquist freq:   {nyquist:.0f} Hz")
        print(f"  Points/cycle:   {WAVEFORM_BUF_SIZE}")
        print(f"  f = SR / N = {sr:.0f} / {WAVEFORM_BUF_SIZE} = {sr/WAVEFORM_BUF_SIZE:.1f} Hz")
        
        # Harmonics for square/triangle
        if wtype in ('SQUARE', 'TRIANGLE'):
            print(f"  Expected harmonics:")
            for h in range(1, 6, 2):
                harm_f = actual_f * h
                if harm_f < nyquist:
                    print(f"    {h}th: {harm_f:.0f} Hz (within Nyquist)")
                else:
                    print(f"    {h}th: {harm_f:.0f} Hz (aliased!)")
    
    print(f"\n{'=' * 65}")
    print(f"  {datetime.now().strftime('%H:%M:%S')} | Samples: {data_acc.get('total_samples', 0)}")

def main():
    """Main function."""
    print(f"Connecting to {SERIAL_PORT}...")
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Connected!\n")
    except serial.SerialException as e:
        print(f"Error: {e}\nRunning demo...\n")
        demo_mode()
        return
    
    data_acc = {}
    
    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    m = re_waveform.search(line)
                    if m:
                        data_acc['waveform'] = m.group(1)
                        waveform_types.append(m.group(1))
                    
                    m = re_target_freq.search(line)
                    if m:
                        data_acc['target_freq'] = int(m.group(1))
                    
                    m = re_actual_freq.search(line)
                    if m:
                        data_acc['actual_freq'] = float(m.group(1))
                        actual_freqs.append(float(m.group(1)))
                        timestamps.append(time.time())
                    
                    m = re_sample_rate.search(line)
                    if m:
                        data_acc['sample_rate'] = float(m.group(1))
                        sample_rates.append(float(m.group(1)))
                    
                    m = re_vpp.search(line)
                    if m:
                        data_acc['vpp'] = float(m.group(1))
                    
                    m = re_samples.search(line)
                    if m:
                        data_acc['total_samples'] = int(m.group(1))
                        print_dashboard(data_acc)
    
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()

def demo_mode():
    """Demo mode."""
    waves = ['SINE', 'SQUARE', 'TRIANGLE', 'SAWTOOTH']
    t = 0
    data_acc = {}
    
    try:
        while True:
            wtype = waves[int(t / 5) % 4]
            freq = 1000 + 50 * math.sin(t * 0.1)
            
            data_acc.update({
                'waveform': wtype,
                'target_freq': 1000,
                'actual_freq': freq,
                'sample_rate': freq * 256,
                'vpp': 3.3,
                'total_samples': int(t * 256000),
            })
            
            actual_freqs.append(freq)
            waveform_types.append(wtype)
            sample_rates.append(freq * 256)
            timestamps.append(time.time())
            
            print_dashboard(data_acc)
            time.sleep(1)
            t += 1
    except KeyboardInterrupt:
        print("\nDemo stopped.")

if __name__ == '__main__':
    main()
