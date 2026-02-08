#!/usr/bin/env python3
"""
===========================================================================
 Debug Script: ESP32 ADC Continuous (DMA) — Waveform & FFT Analysis
===========================================================================
 Parses ADC statistics from serial, plots waveform and FFT.

 Usage:
   python debug_adc_continuous.py [--port /dev/ttyUSB0] [--baud 115200]
   python debug_adc_continuous.py --file output.log
===========================================================================
"""

import re
import sys
import argparse
import time
from collections import deque

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    import numpy as np
    HAS_PLOT = True
except ImportError:
    HAS_PLOT = False


def parse_adc_stats(line):
    """Parse [ADC_STATS] lines."""
    result = {}

    # Sample count and rate
    m = re.search(r'Samples=(\d+)\s*\|\s*Rate=([\d.]+)', line)
    if m:
        result['samples'] = int(m.group(1))
        result['rate'] = float(m.group(2))

    # Raw stats
    m = re.search(r'Avg=([\d.]+)\s+Min=(\d+)\s+Max=(\d+)\s+RMS=([\d.]+)', line)
    if m:
        result['avg'] = float(m.group(1))
        result['min'] = int(m.group(2))
        result['max'] = int(m.group(3))
        result['rms'] = float(m.group(4))

    # Voltage stats
    m = re.search(r'Avg=([\d.]+)V\s+Min=([\d.]+)V\s+Max=([\d.]+)V', line)
    if m:
        result['avg_v'] = float(m.group(1))
        result['min_v'] = float(m.group(2))
        result['max_v'] = float(m.group(3))

    # Zero crossings and frequency
    m = re.search(r'ZeroCrossings=(\d+)\s+EstFreq=([\d.]+)', line)
    if m:
        result['zero_crossings'] = int(m.group(1))
        result['est_freq'] = float(m.group(2))

    return result if result else None


class ADCDashboard:
    """Live dashboard for ADC statistics."""

    def __init__(self):
        self.history = {
            'time': deque(maxlen=100),
            'avg': deque(maxlen=100),
            'min': deque(maxlen=100),
            'max': deque(maxlen=100),
            'rms': deque(maxlen=100),
            'rate': deque(maxlen=100),
            'freq': deque(maxlen=100),
        }
        self.sample_idx = 0

    def add_stats(self, stats):
        """Add parsed stats to history."""
        self.sample_idx += 1
        self.history['time'].append(self.sample_idx)

        if 'avg' in stats:
            self.history['avg'].append(stats['avg'])
        if 'min' in stats:
            self.history['min'].append(stats['min'])
        if 'max' in stats:
            self.history['max'].append(stats['max'])
        if 'rms' in stats:
            self.history['rms'].append(stats['rms'])
        if 'rate' in stats:
            self.history['rate'].append(stats['rate'])
        if 'est_freq' in stats:
            self.history['freq'].append(stats['est_freq'])

    def create_dashboard(self):
        """Create static dashboard from collected data."""
        if not HAS_PLOT:
            print("matplotlib not installed, skipping charts")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('ESP32 ADC Continuous (DMA) — Statistics Dashboard', fontsize=14)

        t = list(self.history['time'])

        # Plot 1: ADC Values over time
        ax = axes[0][0]
        if self.history['avg']:
            ax.plot(t[-len(self.history['avg']):], list(self.history['avg']),
                    'b-', label='Average', linewidth=2)
        if self.history['min']:
            ax.plot(t[-len(self.history['min']):], list(self.history['min']),
                    'g--', label='Min', alpha=0.7)
        if self.history['max']:
            ax.plot(t[-len(self.history['max']):], list(self.history['max']),
                    'r--', label='Max', alpha=0.7)
        ax.set_xlabel('Sample Interval')
        ax.set_ylabel('ADC Raw Value')
        ax.set_title('ADC Values Over Time')
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_ylim(0, 4095)

        # Plot 2: RMS over time
        ax2 = axes[0][1]
        if self.history['rms']:
            ax2.plot(t[-len(self.history['rms']):], list(self.history['rms']),
                     'purple', linewidth=2)
            ax2.fill_between(t[-len(self.history['rms']):],
                             list(self.history['rms']), alpha=0.2, color='purple')
        ax2.set_xlabel('Sample Interval')
        ax2.set_ylabel('RMS Value')
        ax2.set_title('ADC RMS Over Time')
        ax2.grid(True, alpha=0.3)

        # Plot 3: Sampling Rate
        ax3 = axes[1][0]
        if self.history['rate']:
            ax3.plot(t[-len(self.history['rate']):], list(self.history['rate']),
                     'green', linewidth=2, marker='o', markersize=3)
            target = 20000
            ax3.axhline(y=target, color='red', linestyle='--',
                        label=f'Target: {target} Hz', alpha=0.7)
        ax3.set_xlabel('Sample Interval')
        ax3.set_ylabel('Sample Rate (Hz)')
        ax3.set_title('Actual Sampling Rate')
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # Plot 4: Estimated Signal Frequency
        ax4 = axes[1][1]
        if self.history['freq']:
            ax4.plot(t[-len(self.history['freq']):], list(self.history['freq']),
                     'orange', linewidth=2, marker='s', markersize=3)
        ax4.set_xlabel('Sample Interval')
        ax4.set_ylabel('Frequency (Hz)')
        ax4.set_title('Estimated Signal Frequency (Zero-Crossing)')
        ax4.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig('adc_continuous_dashboard.png', dpi=150)
        print("Dashboard saved: adc_continuous_dashboard.png")
        plt.show()


def read_and_parse(port=None, baud=115200, filepath=None, duration=60):
    """Read from serial or file and parse ADC stats."""
    dashboard = ADCDashboard()

    if filepath:
        with open(filepath, 'r') as f:
            lines = f.readlines()
    else:
        if not HAS_SERIAL:
            print("ERROR: pyserial not installed")
            sys.exit(1)
        lines = []
        try:
            ser = serial.Serial(port, baud, timeout=1)
            start = time.time()
            print(f"Reading from {port} for {duration}s...")
            while (time.time() - start) < duration:
                raw = ser.readline()
                if raw:
                    line = raw.decode('utf-8', errors='replace').strip()
                    print(line)
                    lines.append(line)
            ser.close()
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            sys.exit(1)

    # Parse all lines
    stat_count = 0
    for line in lines:
        if isinstance(line, bytes):
            line = line.decode('utf-8', errors='replace').strip()
        else:
            line = line.strip()

        if '[ADC_STATS]' in line:
            stats = parse_adc_stats(line)
            if stats:
                dashboard.add_stats(stats)
                stat_count += 1

    print(f"\nParsed {stat_count} ADC stat entries.")

    if stat_count > 0:
        dashboard.create_dashboard()
    else:
        print("No ADC statistics found in output!")


def main():
    parser = argparse.ArgumentParser(description='ESP32 ADC Continuous Debug Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', help='Read from log file')
    parser.add_argument('--duration', type=int, default=60, help='Read duration (s)')
    args = parser.parse_args()

    read_and_parse(port=args.port, baud=args.baud,
                   filepath=args.file, duration=args.duration)


if __name__ == '__main__':
    main()
