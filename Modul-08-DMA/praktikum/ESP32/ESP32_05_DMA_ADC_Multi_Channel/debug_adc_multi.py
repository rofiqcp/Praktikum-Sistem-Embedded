#!/usr/bin/env python3
"""
===========================================================================
 Debug Script: ESP32 Multi-Channel ADC — Visualization & Analysis
===========================================================================
 Parses multi-channel ADC data, creates per-channel plots and
 correlation analysis.

 Usage:
   python debug_adc_multi.py [--port /dev/ttyUSB0] [--baud 115200]
   python debug_adc_multi.py --file output.log
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
    import numpy as np
    HAS_PLOT = True
except ImportError:
    HAS_PLOT = False


def parse_adc_multi_line(line):
    """Parse [ADC_MULTI] line for per-channel stats."""
    pattern = (
        r'\[ADC_MULTI\]\s+CH(\d+):\s+'
        r'avg=([\d.]+)\s+min=(\d+)\s+max=(\d+)\s+'
        r'rms=([\d.]+)\s+voltage=([\d.]+)V\s+samples=(\d+)'
    )
    m = re.search(pattern, line)
    if m:
        return {
            'channel': int(m.group(1)),
            'avg': float(m.group(2)),
            'min': int(m.group(3)),
            'max': int(m.group(4)),
            'rms': float(m.group(5)),
            'voltage': float(m.group(6)),
            'samples': int(m.group(7)),
        }
    return None


def parse_correlation_line(line):
    """Parse [CORR] line for correlation coefficients."""
    pattern = (
        r'\[CORR\]\s+CH0_CH1=([\d.+-]+)\s+'
        r'CH0_CH2=([\d.+-]+)\s+CH1_CH2=([\d.+-]+)'
    )
    m = re.search(pattern, line)
    if m:
        return {
            'ch0_ch1': float(m.group(1)),
            'ch0_ch2': float(m.group(2)),
            'ch1_ch2': float(m.group(3)),
        }
    return None


class MultiChannelAnalyzer:
    """Analyze and visualize multi-channel ADC data."""

    def __init__(self):
        self.channel_data = {i: [] for i in range(3)}
        self.correlations = []

    def add_channel_data(self, data):
        ch = data['channel']
        if ch in self.channel_data:
            self.channel_data[ch].append(data)

    def add_correlation(self, corr):
        self.correlations.append(corr)

    def create_charts(self):
        """Create multi-channel analysis charts."""
        if not HAS_PLOT:
            print("matplotlib not installed, skipping charts")
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('ESP32 Multi-Channel ADC (DMA) — Analysis', fontsize=14)
        colors = ['#e74c3c', '#3498db', '#2ecc71']
        ch_names = ['CH0 (GPIO36)', 'CH1 (GPIO39)', 'CH2 (GPIO34)']

        # Plot 1: Average values over time
        ax = axes[0][0]
        for ch in range(3):
            if self.channel_data[ch]:
                avgs = [d['avg'] for d in self.channel_data[ch]]
                ax.plot(avgs, '-o', label=ch_names[ch], color=colors[ch],
                        markersize=4, linewidth=1.5)
        ax.set_xlabel('Sample Interval')
        ax.set_ylabel('Average ADC Value')
        ax.set_title('Per-Channel Average Over Time')
        ax.legend()
        ax.grid(True, alpha=0.3)

        # Plot 2: Min-Max range (error bars)
        ax2 = axes[0][1]
        if any(self.channel_data[ch] for ch in range(3)):
            x_positions = np.arange(3)
            for ch in range(3):
                if self.channel_data[ch]:
                    last = self.channel_data[ch][-1]
                    avg = last['avg']
                    err_low = avg - last['min']
                    err_high = last['max'] - avg
                    ax2.bar(x_positions[ch], avg, width=0.6,
                            color=colors[ch], alpha=0.7, label=ch_names[ch])
                    ax2.errorbar(x_positions[ch], avg,
                                 yerr=[[err_low], [err_high]],
                                 fmt='none', color='black', capsize=5)
            ax2.set_xticks(x_positions)
            ax2.set_xticklabels([f'CH{i}' for i in range(3)])
            ax2.set_ylabel('ADC Value')
            ax2.set_title('Latest Per-Channel Range (Min/Avg/Max)')
            ax2.legend()
            ax2.grid(axis='y', alpha=0.3)

        # Plot 3: Voltage comparison
        ax3 = axes[1][0]
        for ch in range(3):
            if self.channel_data[ch]:
                volts = [d['voltage'] for d in self.channel_data[ch]]
                ax3.plot(volts, '-s', label=ch_names[ch], color=colors[ch],
                         markersize=4, linewidth=1.5)
        ax3.set_xlabel('Sample Interval')
        ax3.set_ylabel('Voltage (V)')
        ax3.set_title('Per-Channel Voltage Over Time')
        ax3.set_ylim(0, 3.5)
        ax3.axhline(y=3.3, color='gray', linestyle='--', alpha=0.5, label='3.3V Max')
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # Plot 4: Correlation heatmap
        ax4 = axes[1][1]
        if self.correlations:
            last_corr = self.correlations[-1]
            corr_matrix = np.array([
                [1.0, last_corr['ch0_ch1'], last_corr['ch0_ch2']],
                [last_corr['ch0_ch1'], 1.0, last_corr['ch1_ch2']],
                [last_corr['ch0_ch2'], last_corr['ch1_ch2'], 1.0],
            ])
            im = ax4.imshow(corr_matrix, cmap='RdBu_r', vmin=-1, vmax=1)
            ax4.set_xticks(range(3))
            ax4.set_yticks(range(3))
            ax4.set_xticklabels(['CH0', 'CH1', 'CH2'])
            ax4.set_yticklabels(['CH0', 'CH1', 'CH2'])
            for i in range(3):
                for j in range(3):
                    ax4.text(j, i, f'{corr_matrix[i, j]:.3f}',
                             ha='center', va='center', fontsize=12,
                             color='white' if abs(corr_matrix[i, j]) > 0.5 else 'black')
            plt.colorbar(im, ax=ax4, label='Pearson r')
            ax4.set_title('Cross-Channel Correlation')
        else:
            ax4.text(0.5, 0.5, 'No correlation data', transform=ax4.transAxes,
                     ha='center', va='center', fontsize=14)
            ax4.set_title('Cross-Channel Correlation')

        plt.tight_layout()
        plt.savefig('adc_multi_analysis.png', dpi=150)
        print("Chart saved: adc_multi_analysis.png")
        plt.show()


def read_and_analyze(port=None, baud=115200, filepath=None, duration=60):
    """Read from serial/file and analyze data."""
    analyzer = MultiChannelAnalyzer()

    if filepath:
        with open(filepath, 'r') as f:
            lines = [l.strip() for l in f.readlines()]
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

    # Parse lines
    ch_count = 0
    corr_count = 0
    for line in lines:
        data = parse_adc_multi_line(line)
        if data:
            analyzer.add_channel_data(data)
            ch_count += 1

        corr = parse_correlation_line(line)
        if corr:
            analyzer.add_correlation(corr)
            corr_count += 1

    print(f"\nParsed {ch_count} channel entries, {corr_count} correlations.")

    if ch_count > 0:
        analyzer.create_charts()
    else:
        print("No multi-channel ADC data found!")


def main():
    parser = argparse.ArgumentParser(description='ESP32 Multi-Channel ADC Debug Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', help='Read from log file')
    parser.add_argument('--duration', type=int, default=60, help='Read duration (s)')
    args = parser.parse_args()

    read_and_analyze(port=args.port, baud=args.baud,
                     filepath=args.file, duration=args.duration)


if __name__ == '__main__':
    main()
