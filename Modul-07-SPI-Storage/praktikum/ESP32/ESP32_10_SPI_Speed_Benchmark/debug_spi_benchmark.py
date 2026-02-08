#!/usr/bin/env python3
"""
Debug Script: SPI Speed Benchmark Analyzer
Modul 07 - SPI & Storage | Program 10

Fitur:
- Parse hasil benchmark dari serial output
- Generate heatmap (speed vs buffer size)
- Bar chart throughput per konfigurasi
- Efficiency line plot
- Perbandingan DMA vs non-DMA

Penggunaan:
    python debug_spi_benchmark.py [PORT] [BAUDRATE]
    python debug_spi_benchmark.py /dev/ttyUSB0 115200
"""

import sys
import re
import time
import numpy as np
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import matplotlib.colors as mcolors
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class SPIBenchmarkDebugger:
    """Parser dan visualizer untuk SPI benchmark."""

    def __init__(self):
        self.results = []  # List of dicts with benchmark results
        self.current_speed = None
        self.current_speed_label = None

    def parse_line(self, line):
        """Parse satu baris output serial."""

        # Detect current speed being tested
        speed_match = re.search(r'Testing speed:\s*(\S+ \S+)\s*\((\d+)\s*Hz\)', line)
        if speed_match:
            self.current_speed_label = speed_match.group(1).strip()
            self.current_speed = int(speed_match.group(2))
            print(f"  [SPEED] Testing {self.current_speed_label}")
            return

        # Parse result line: "-> XX.X KB/s (X.XX Mbps), efficiency: XX.X%, time: XXXX us"
        result_match = re.search(
            r'->\s*([\d.]+)\s*KB/s\s*\(([\d.]+)\s*Mbps\),\s*efficiency:\s*([\d.]+)%,\s*time:\s*(\d+)\s*us',
            line
        )
        if result_match and self.current_speed is not None:
            throughput_kbps = float(result_match.group(1))
            throughput_mbps = float(result_match.group(2))
            efficiency = float(result_match.group(3))
            time_us = int(result_match.group(4))

            # Get buffer size from preceding line context
            buf_match = re.search(r'Buffer:\s*(\d+)\s*bytes', line)
            buf_size = int(buf_match.group(1)) if buf_match else None

            self.results.append({
                'speed_hz': self.current_speed,
                'speed_label': self.current_speed_label,
                'buffer_size': buf_size,
                'throughput_kbps': throughput_kbps,
                'throughput_mbps': throughput_mbps,
                'efficiency': efficiency,
                'time_us': time_us,
                'dma': buf_size > 32 if buf_size else False,
            })
            print(f"  [RESULT] {self.current_speed_label}, buf={buf_size}B: "
                  f"{throughput_kbps:.1f} KB/s, eff={efficiency:.1f}%")
            return

        # Parse buffer + result combined
        buf_result = re.search(r'Buffer:\s*(\d+)\s*bytes.*?DMA:\s*(\w+)', line)
        if buf_result:
            # Store buffer context for next result line
            self._pending_buffer = int(buf_result.group(1))
            self._pending_dma = buf_result.group(2).lower() == 'yes'

        # Try to match result without buffer in same line
        if result_match is None:
            result_match2 = re.search(
                r'->\s*([\d.]+)\s*KB/s\s*\(([\d.]+)\s*Mbps\)',
                line
            )
            if result_match2 and hasattr(self, '_pending_buffer'):
                eff_match = re.search(r'efficiency:\s*([\d.]+)%', line)
                time_match = re.search(r'time:\s*(\d+)\s*us', line)
                self.results.append({
                    'speed_hz': self.current_speed,
                    'speed_label': self.current_speed_label,
                    'buffer_size': self._pending_buffer,
                    'throughput_kbps': float(result_match2.group(1)),
                    'throughput_mbps': float(result_match2.group(2)),
                    'efficiency': float(eff_match.group(1)) if eff_match else 0,
                    'time_us': int(time_match.group(1)) if time_match else 0,
                    'dma': self._pending_dma,
                })

    def print_summary(self):
        """Print ringkasan hasil benchmark."""
        print("\n" + "=" * 70)
        print("          SPI SPEED BENCHMARK SUMMARY")
        print("=" * 70)

        if not self.results:
            print("  No results captured.")
            return

        print(f"\n  Total tests: {len(self.results)}")

        # Best throughput
        best = max(self.results, key=lambda r: r['throughput_kbps'])
        print(f"\n  BEST THROUGHPUT:")
        print(f"    Speed: {best['speed_label']}, Buffer: {best['buffer_size']}B")
        print(f"    Throughput: {best['throughput_kbps']:.1f} KB/s ({best['throughput_mbps']:.2f} Mbps)")
        print(f"    Efficiency: {best['efficiency']:.1f}%")

        # Best efficiency
        best_eff = max(self.results, key=lambda r: r['efficiency'])
        print(f"\n  BEST EFFICIENCY:")
        print(f"    Speed: {best_eff['speed_label']}, Buffer: {best_eff['buffer_size']}B")
        print(f"    Efficiency: {best_eff['efficiency']:.1f}%")

        print("=" * 70)

    def plot_results(self):
        """Generate visualisasi benchmark."""
        if not HAS_MATPLOTLIB or not self.results:
            print("[WARN] Cannot plot: matplotlib or data missing")
            return

        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('SPI Speed Benchmark Analysis', fontsize=14, fontweight='bold')

        # Collect unique speeds and buffer sizes
        speeds = sorted(set(r['speed_hz'] for r in self.results))
        buf_sizes = sorted(set(r['buffer_size'] for r in self.results if r['buffer_size']))
        speed_labels = []
        for s in speeds:
            for r in self.results:
                if r['speed_hz'] == s and r.get('speed_label'):
                    speed_labels.append(r['speed_label'])
                    break
            else:
                speed_labels.append(f"{s/1e6:.0f}MHz")

        # Build 2D arrays for heatmap
        throughput_map = np.zeros((len(speeds), len(buf_sizes)))
        efficiency_map = np.zeros((len(speeds), len(buf_sizes)))

        for r in self.results:
            if r['buffer_size'] in buf_sizes and r['speed_hz'] in speeds:
                si = speeds.index(r['speed_hz'])
                bi = buf_sizes.index(r['buffer_size'])
                throughput_map[si][bi] = r['throughput_kbps']
                efficiency_map[si][bi] = r['efficiency']

        # Plot 1: Throughput Heatmap
        ax1 = axes[0][0]
        im1 = ax1.imshow(throughput_map, cmap='YlOrRd', aspect='auto')
        ax1.set_xticks(range(len(buf_sizes)))
        ax1.set_xticklabels([str(b) for b in buf_sizes])
        ax1.set_yticks(range(len(speeds)))
        ax1.set_yticklabels(speed_labels)
        ax1.set_xlabel('Buffer Size (bytes)')
        ax1.set_ylabel('SPI Speed')
        ax1.set_title('Throughput Heatmap (KB/s)')
        for i in range(len(speeds)):
            for j in range(len(buf_sizes)):
                val = throughput_map[i][j]
                ax1.text(j, i, f'{val:.0f}', ha='center', va='center',
                         fontsize=7, color='white' if val > throughput_map.max() * 0.6 else 'black')
        fig.colorbar(im1, ax=ax1)

        # Plot 2: Throughput Bar Chart
        ax2 = axes[0][1]
        x = np.arange(len(buf_sizes))
        width = 0.8 / len(speeds)
        for i, (speed, label) in enumerate(zip(speeds, speed_labels)):
            vals = [throughput_map[i][j] for j in range(len(buf_sizes))]
            ax2.bar(x + i * width, vals, width, label=label)
        ax2.set_xticks(x + width * len(speeds) / 2)
        ax2.set_xticklabels([str(b) for b in buf_sizes])
        ax2.set_xlabel('Buffer Size (bytes)')
        ax2.set_ylabel('Throughput (KB/s)')
        ax2.set_title('Throughput by Configuration')
        ax2.legend(fontsize=7, ncol=2)
        ax2.grid(True, alpha=0.3, axis='y')

        # Plot 3: Efficiency Line Plot
        ax3 = axes[1][0]
        for i, (speed, label) in enumerate(zip(speeds, speed_labels)):
            vals = [efficiency_map[i][j] for j in range(len(buf_sizes))]
            ax3.plot([str(b) for b in buf_sizes], vals, 'o-', label=label, linewidth=2)
        ax3.set_xlabel('Buffer Size (bytes)')
        ax3.set_ylabel('Efficiency (%)')
        ax3.set_title('Efficiency vs Buffer Size')
        ax3.legend(fontsize=7, ncol=2)
        ax3.grid(True, alpha=0.3)
        ax3.set_ylim(0, 105)

        # Plot 4: Efficiency Heatmap
        ax4 = axes[1][1]
        im4 = ax4.imshow(efficiency_map, cmap='RdYlGn', aspect='auto', vmin=0, vmax=100)
        ax4.set_xticks(range(len(buf_sizes)))
        ax4.set_xticklabels([str(b) for b in buf_sizes])
        ax4.set_yticks(range(len(speeds)))
        ax4.set_yticklabels(speed_labels)
        ax4.set_xlabel('Buffer Size (bytes)')
        ax4.set_ylabel('SPI Speed')
        ax4.set_title('Efficiency Heatmap (%)')
        for i in range(len(speeds)):
            for j in range(len(buf_sizes)):
                val = efficiency_map[i][j]
                ax4.text(j, i, f'{val:.1f}', ha='center', va='center',
                         fontsize=7, color='white' if val < 40 else 'black')
        fig.colorbar(im4, ax=ax4)

        plt.tight_layout()
        plt.savefig('spi_benchmark_analysis.png', dpi=150, bbox_inches='tight')
        print("[INFO] Plot saved to spi_benchmark_analysis.png")
        plt.show()


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    debugger = SPIBenchmarkDebugger()

    if not HAS_SERIAL:
        print("[INFO] Running in demo mode with sample data...")
        demo_lines = [
            "Testing speed:  1 MHz (1000000 Hz)",
            "  Buffer: 16 bytes, DMA: No ...",
            "    -> 48.2 KB/s (0.39 Mbps), efficiency: 39.5%, time: 33200 us",
            "  Buffer: 64 bytes, DMA: Yes ...",
            "    -> 110.5 KB/s (0.88 Mbps), efficiency: 90.5%, time: 57900 us",
            "  Buffer: 256 bytes, DMA: Yes ...",
            "    -> 118.0 KB/s (0.94 Mbps), efficiency: 96.7%, time: 217000 us",
            "  Buffer: 1024 bytes, DMA: Yes ...",
            "    -> 121.5 KB/s (0.97 Mbps), efficiency: 99.5%, time: 842300 us",
            "  Buffer: 4096 bytes, DMA: Yes ...",
            "    -> 122.0 KB/s (0.98 Mbps), efficiency: 99.9%, time: 3356000 us",
            "Testing speed: 10 MHz (10000000 Hz)",
            "  Buffer: 16 bytes, DMA: No ...",
            "    -> 185.2 KB/s (1.48 Mbps), efficiency: 15.2%, time: 8640 us",
            "  Buffer: 64 bytes, DMA: Yes ...",
            "    -> 720.5 KB/s (5.76 Mbps), efficiency: 59.0%, time: 8880 us",
            "  Buffer: 256 bytes, DMA: Yes ...",
            "    -> 980.0 KB/s (7.84 Mbps), efficiency: 80.3%, time: 26120 us",
            "  Buffer: 1024 bytes, DMA: Yes ...",
            "    -> 1120.5 KB/s (8.96 Mbps), efficiency: 91.8%, time: 91400 us",
            "  Buffer: 4096 bytes, DMA: Yes ...",
            "    -> 1200.0 KB/s (9.60 Mbps), efficiency: 98.3%, time: 341300 us",
            "Testing speed: 40 MHz (40000000 Hz)",
            "  Buffer: 16 bytes, DMA: No ...",
            "    -> 220.0 KB/s (1.76 Mbps), efficiency: 4.5%, time: 7270 us",
            "  Buffer: 64 bytes, DMA: Yes ...",
            "    -> 1500.5 KB/s (12.00 Mbps), efficiency: 30.7%, time: 4260 us",
            "  Buffer: 256 bytes, DMA: Yes ...",
            "    -> 3200.0 KB/s (25.60 Mbps), efficiency: 65.5%, time: 8000 us",
            "  Buffer: 1024 bytes, DMA: Yes ...",
            "    -> 4200.5 KB/s (33.60 Mbps), efficiency: 85.9%, time: 24380 us",
            "  Buffer: 4096 bytes, DMA: Yes ...",
            "    -> 4700.0 KB/s (37.60 Mbps), efficiency: 96.1%, time: 87150 us",
        ]
        for line in demo_lines:
            debugger.parse_line(line)
            time.sleep(0.02)

        debugger.print_summary()
        debugger.plot_results()
        return

    print(f"[INFO] Connecting to {port} at {baud} baud...")
    print("[INFO] Press Ctrl+C to stop and show results\n")

    try:
        ser = serial.Serial(port, baud, timeout=1)
        while True:
            if ser.in_waiting > 0:
                raw = ser.readline()
                try:
                    line = raw.decode('utf-8', errors='replace').strip()
                except Exception:
                    continue
                if line:
                    print(f"[SERIAL] {line}")
                    debugger.parse_line(line)
    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
    finally:
        debugger.print_summary()
        debugger.plot_results()


if __name__ == '__main__':
    main()
