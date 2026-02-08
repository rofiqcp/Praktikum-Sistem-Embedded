#!/usr/bin/env python3
"""
Debug & Visualization Tool for STM32 DMA ADC Continuous Conversion

Parses serial output from STM32 firmware that reads ADC values
continuously via DMA and reports statistics.

Expected [DATA] tagged lines:
    [DATA] raw=NNN,min=NNN,max=NNN,avg=NNN,mv=NNN

Features:
    - Serial port reading (pyserial) or log file parsing
    - Real-time voltage plot over sample index
    - Histogram of raw ADC distribution
    - Min/Max/Avg envelope chart
    - Demo mode with simulated ADC waveform data
    - Graceful fallback when matplotlib/pyserial not installed

Usage:
    python debug_adc_continuous.py --port /dev/ttyACM0
    python debug_adc_continuous.py --file adc_log.txt
    python debug_adc_continuous.py  (demo mode)
"""

import re
import sys
import time
import math
import argparse
import statistics
from datetime import datetime

# ---------------------------------------------------------------------------
# Optional dependency imports
# ---------------------------------------------------------------------------
try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    from matplotlib.ticker import MaxNLocator
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# ---------------------------------------------------------------------------
# Constants & regex
# ---------------------------------------------------------------------------
DATA_PATTERN = re.compile(
    r"\[DATA\]\s*raw=(\d+),min=(\d+),max=(\d+),avg=(\d+),mv=(\d+)",
    re.IGNORECASE,
)
BAUD_RATE = 115200
READ_TIMEOUT = 0.5
DEFAULT_DURATION = 30
ADC_MAX = 4095           # 12-bit ADC
VREF_MV = 3300           # 3.3 V reference


# ---------------------------------------------------------------------------
# Data container
# ---------------------------------------------------------------------------
class AdcRecord:
    """Single ADC sampling report."""

    __slots__ = ("raw", "min_val", "max_val", "avg", "mv", "timestamp")

    def __init__(self, raw, min_val, max_val, avg, mv, timestamp=None):
        self.raw = int(raw)
        self.min_val = int(min_val)
        self.max_val = int(max_val)
        self.avg = int(avg)
        self.mv = int(mv)
        self.timestamp = timestamp or time.time()

    @property
    def voltage(self):
        """Convert mV to V."""
        return self.mv / 1000.0

    def __repr__(self):
        return (f"ADC(raw={self.raw}, min={self.min_val}, max={self.max_val}, "
                f"avg={self.avg}, {self.mv}mV)")


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------
def parse_line(line: str):
    """Return AdcRecord if the line matches, else None."""
    m = DATA_PATTERN.search(line)
    if m:
        return AdcRecord(*m.groups())
    return None


def read_serial(port: str, baudrate: int = BAUD_RATE,
                duration: int = DEFAULT_DURATION):
    """Capture lines from serial port."""
    if not HAS_SERIAL:
        print("[WARN] pyserial not installed. pip install pyserial")
        return []
    records = []
    print(f"[INFO] Opening {port} @ {baudrate} baud for {duration}s …")
    try:
        ser = serial.Serial(port, baudrate, timeout=READ_TIMEOUT)
        t_start = time.time()
        t_end = t_start + duration
        while time.time() < t_end:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode(errors="replace").strip()
            if line:
                print(f"  >> {line}")
            rec = parse_line(line)
            if rec:
                rec.timestamp = time.time() - t_start
                records.append(rec)
        ser.close()
    except serial.SerialException as exc:
        print(f"[ERROR] Serial: {exc}")
    print(f"[INFO] Captured {len(records)} ADC record(s).")
    return records


def read_file(path: str):
    """Parse saved log file."""
    records = []
    print(f"[INFO] Reading log file: {path}")
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            for idx, line in enumerate(fh):
                rec = parse_line(line)
                if rec:
                    rec.timestamp = idx * 0.1
                    records.append(rec)
    except OSError as exc:
        print(f"[ERROR] File: {exc}")
    print(f"[INFO] Parsed {len(records)} record(s) from file.")
    return records


# ---------------------------------------------------------------------------
# Demo data
# ---------------------------------------------------------------------------
def generate_demo_data():
    """Simulate ADC readings: slow sine wave + noise (like potentiometer)."""
    import random
    random.seed(21)
    records = []
    n_samples = 80
    for i in range(n_samples):
        # Base: sine wave 0–3.3V with period ~40 samples
        base = (math.sin(2 * math.pi * i / 40) + 1) / 2  # 0..1
        noise = random.gauss(0, 0.02)
        value = max(0.0, min(1.0, base + noise))
        raw = int(value * ADC_MAX)
        # Simulate min/max/avg from a small DMA buffer window
        spread = random.randint(5, 30)
        min_v = max(0, raw - spread)
        max_v = min(ADC_MAX, raw + spread)
        avg_v = raw + random.randint(-3, 3)
        mv = int(raw * VREF_MV / ADC_MAX)
        records.append(AdcRecord(raw, min_v, max_v, avg_v, mv,
                                 timestamp=i * 0.1))
    print(f"[INFO] Generated demo data: {n_samples} ADC samples "
          f"(sine + noise).")
    return records


# ---------------------------------------------------------------------------
# Text report
# ---------------------------------------------------------------------------
def print_report(records):
    """Print ADC statistics table."""
    print("\n" + "=" * 72)
    print("  STM32 DMA ADC Continuous – Sampling Report")
    print(f"  Generated: {datetime.now():%Y-%m-%d %H:%M:%S}")
    print("=" * 72)

    if not records:
        print("  No data available.")
        return

    hdr = (f"{'#':>4} {'Time':>6} {'Raw':>6} {'Min':>6} {'Max':>6}"
           f" {'Avg':>6} {'mV':>6} {'Volts':>7}")
    print(hdr)
    print("-" * 72)
    for i, r in enumerate(records[:50]):  # cap display at 50
        print(f"{i:>4} {r.timestamp:>6.1f} {r.raw:>6} {r.min_val:>6}"
              f" {r.max_val:>6} {r.avg:>6} {r.mv:>6} {r.voltage:>7.3f}")
    if len(records) > 50:
        print(f"  … ({len(records) - 50} more rows omitted)")
    print("-" * 72)

    all_raw = [r.raw for r in records]
    all_mv = [r.mv for r in records]
    global_min = min(r.min_val for r in records)
    global_max = max(r.max_val for r in records)
    avg_raw = statistics.mean(all_raw)
    std_raw = statistics.stdev(all_raw) if len(all_raw) > 1 else 0
    avg_mv = statistics.mean(all_mv)

    print(f"  Samples      : {len(records)}")
    print(f"  Raw range    : {global_min} – {global_max}  "
          f"(of {ADC_MAX} max)")
    print(f"  Raw avg±std  : {avg_raw:.1f} ± {std_raw:.1f}")
    print(f"  Voltage range: {global_min * VREF_MV / ADC_MAX:.0f} – "
          f"{global_max * VREF_MV / ADC_MAX:.0f} mV")
    print(f"  Avg voltage  : {avg_mv:.0f} mV  ({avg_mv / 1000:.3f} V)")
    peak_to_peak = global_max - global_min
    print(f"  Peak-to-peak : {peak_to_peak} counts  "
          f"({peak_to_peak * VREF_MV / ADC_MAX:.0f} mV)")
    enob = math.log2(ADC_MAX / max(std_raw * 2, 1)) if std_raw > 0 else 12
    print(f"  Est. ENOB    : {enob:.1f} bits")
    print("=" * 72 + "\n")


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------
def plot_results(records):
    """Voltage plot with min/max envelope, and ADC histogram."""
    if not HAS_MATPLOTLIB:
        print("[WARN] matplotlib not installed – skipping charts.")
        print("       Install with: pip install matplotlib")
        return

    ts = [r.timestamp for r in records]
    voltages = [r.voltage for r in records]
    raw_vals = [r.raw for r in records]
    mins_v = [r.min_val * VREF_MV / ADC_MAX / 1000 for r in records]
    maxs_v = [r.max_val * VREF_MV / ADC_MAX / 1000 for r in records]
    avgs_v = [r.avg * VREF_MV / ADC_MAX / 1000 for r in records]

    fig, axes = plt.subplots(2, 2, figsize=(14, 9))
    fig.suptitle("STM32 DMA ADC Continuous Conversion", fontsize=14,
                 fontweight="bold")

    # --- 1. Voltage over time ---
    ax1 = axes[0][0]
    ax1.plot(ts, voltages, "-", color="#2980b9", linewidth=1.2,
             label="Instantaneous (mV→V)")
    ax1.plot(ts, avgs_v, "--", color="#e67e22", linewidth=1,
             alpha=0.7, label="Avg")
    ax1.fill_between(ts, mins_v, maxs_v, alpha=0.15, color="#2ecc71",
                     label="Min–Max envelope")
    ax1.set_ylabel("Voltage (V)")
    ax1.set_xlabel("Time (s)")
    ax1.set_title("Voltage vs Time")
    ax1.legend(fontsize=8)
    ax1.grid(alpha=0.3)
    ax1.set_ylim(-0.1, 3.5)

    # --- 2. Raw ADC values ---
    ax2 = axes[0][1]
    ax2.plot(ts, raw_vals, ".", color="#8e44ad", markersize=3, alpha=0.6)
    ax2.axhline(y=statistics.mean(raw_vals), color="#e74c3c",
                linestyle="--", linewidth=0.8,
                label=f"Mean = {statistics.mean(raw_vals):.0f}")
    ax2.set_ylabel("ADC Raw Count")
    ax2.set_xlabel("Time (s)")
    ax2.set_title("Raw ADC Values")
    ax2.legend(fontsize=8)
    ax2.grid(alpha=0.3)
    ax2.set_ylim(-100, ADC_MAX + 200)

    # --- 3. Histogram ---
    ax3 = axes[1][0]
    n_bins = min(50, max(10, len(set(raw_vals)) // 2))
    ax3.hist(raw_vals, bins=n_bins, color="#3498db", edgecolor="black",
             linewidth=0.5, alpha=0.8)
    ax3.axvline(x=statistics.mean(raw_vals), color="#e74c3c",
                linestyle="--", linewidth=1.2,
                label=f"Mean = {statistics.mean(raw_vals):.0f}")
    if len(raw_vals) > 1:
        std = statistics.stdev(raw_vals)
        mean = statistics.mean(raw_vals)
        ax3.axvspan(mean - std, mean + std, alpha=0.1, color="red",
                    label=f"±1σ ({std:.1f})")
    ax3.set_xlabel("ADC Raw Count")
    ax3.set_ylabel("Frequency")
    ax3.set_title("ADC Value Distribution")
    ax3.legend(fontsize=8)
    ax3.grid(alpha=0.3)

    # --- 4. Peak-to-peak / noise per sample ---
    ax4 = axes[1][1]
    pp = [r.max_val - r.min_val for r in records]
    ax4.bar(range(len(pp)), pp, color="#e67e22", alpha=0.7, width=1.0,
            edgecolor="none")
    ax4.axhline(y=statistics.mean(pp), color="#c0392b", linestyle="--",
                linewidth=1, label=f"Avg P-P = {statistics.mean(pp):.1f}")
    ax4.set_xlabel("Sample #")
    ax4.set_ylabel("Peak-to-Peak (counts)")
    ax4.set_title("Per-Sample Noise (Max − Min)")
    ax4.legend(fontsize=8)
    ax4.grid(alpha=0.3)
    ax4.xaxis.set_major_locator(MaxNLocator(integer=True))

    plt.tight_layout()
    out_path = "adc_continuous_results.png"
    plt.savefig(out_path, dpi=150)
    print(f"[INFO] Chart saved to {out_path}")
    plt.show()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_parser():
    p = argparse.ArgumentParser(
        description="STM32 DMA ADC Continuous Conversion Analyzer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Examples:\n"
               "  %(prog)s --port /dev/ttyACM0\n"
               "  %(prog)s --file adc_log.txt\n"
               "  %(prog)s                       (demo mode)\n",
    )
    p.add_argument("--port", "-p", help="Serial port (e.g. /dev/ttyACM0)")
    p.add_argument("--baud", "-b", type=int, default=BAUD_RATE,
                   help=f"Baud rate (default {BAUD_RATE})")
    p.add_argument("--file", "-f", help="Read from log file")
    p.add_argument("--duration", "-d", type=int, default=DEFAULT_DURATION,
                   help=f"Serial capture duration (default {DEFAULT_DURATION}s)")
    p.add_argument("--no-plot", action="store_true",
                   help="Skip chart generation")
    return p


def main():
    args = build_parser().parse_args()

    if args.file:
        records = read_file(args.file)
    elif args.port:
        records = read_serial(args.port, args.baud, args.duration)
    else:
        print("[INFO] No --port or --file given – running demo mode.")
        records = generate_demo_data()

    if not records:
        print("[WARN] No ADC records captured. Exiting.")
        sys.exit(1)

    print_report(records)

    if not args.no_plot:
        plot_results(records)


if __name__ == "__main__":
    main()
