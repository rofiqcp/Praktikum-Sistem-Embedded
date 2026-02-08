#!/usr/bin/env python3
"""
Debug & Benchmark Analyzer for STM32 DMA Memory-to-Memory Transfer

Parses serial output from STM32 DMA mem-to-mem benchmark firmware.
Expects [DATA] tagged lines with format:
    [DATA] size=NNN,dma_cycles=NNN,cpu_cycles=NNN,speedup=N.NN,verify=PASS

Features:
    - Serial port reading (pyserial) or log file parsing
    - Bar chart comparing DMA vs CPU cycles at different transfer sizes
    - Line chart for speedup ratio across sizes
    - Demo mode with simulated benchmark data
    - Graceful fallback when matplotlib/pyserial not installed

Usage:
    python debug_dma_benchmark.py --port /dev/ttyACM0
    python debug_dma_benchmark.py --file capture.log
    python debug_dma_benchmark.py  (demo mode)
"""

import re
import sys
import time
import argparse
import statistics
from collections import OrderedDict
from datetime import datetime

# ---------------------------------------------------------------------------
# Optional dependency imports with graceful fallback
# ---------------------------------------------------------------------------
try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
DATA_PATTERN = re.compile(
    r"\[DATA\]\s*size=(\d+),dma_cycles=(\d+),cpu_cycles=(\d+),"
    r"speedup=([\d.]+),verify=(\w+)",
    re.IGNORECASE,
)
BAUD_RATE = 115200
READ_TIMEOUT = 0.5
DEFAULT_DURATION = 30  # seconds to read from serial


# ---------------------------------------------------------------------------
# Data container
# ---------------------------------------------------------------------------
class BenchmarkRecord:
    """Single benchmark measurement."""

    __slots__ = ("size", "dma_cycles", "cpu_cycles", "speedup", "verify")

    def __init__(self, size, dma_cycles, cpu_cycles, speedup, verify):
        self.size = int(size)
        self.dma_cycles = int(dma_cycles)
        self.cpu_cycles = int(cpu_cycles)
        self.speedup = float(speedup)
        self.verify = verify.upper()

    def __repr__(self):
        return (
            f"Bench(size={self.size}, dma={self.dma_cycles}, "
            f"cpu={self.cpu_cycles}, x{self.speedup:.2f}, {self.verify})"
        )


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------
def parse_line(line: str):
    """Return a BenchmarkRecord if the line matches, else None."""
    m = DATA_PATTERN.search(line)
    if m:
        return BenchmarkRecord(*m.groups())
    return None


def read_serial(port: str, baudrate: int = BAUD_RATE,
                duration: int = DEFAULT_DURATION):
    """Read lines from a serial port for *duration* seconds."""
    if not HAS_SERIAL:
        print("[WARN] pyserial not installed. pip install pyserial")
        return []
    records = []
    print(f"[INFO] Opening {port} @ {baudrate} baud for {duration}s …")
    try:
        ser = serial.Serial(port, baudrate, timeout=READ_TIMEOUT)
        t_end = time.time() + duration
        while time.time() < t_end:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode(errors="replace").strip()
            if line:
                print(f"  >> {line}")
            rec = parse_line(line)
            if rec:
                records.append(rec)
        ser.close()
    except serial.SerialException as exc:
        print(f"[ERROR] Serial: {exc}")
    print(f"[INFO] Captured {len(records)} benchmark record(s).")
    return records


def read_file(path: str):
    """Parse a saved log file."""
    records = []
    print(f"[INFO] Reading log file: {path}")
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            for line in fh:
                rec = parse_line(line)
                if rec:
                    records.append(rec)
    except OSError as exc:
        print(f"[ERROR] File: {exc}")
    print(f"[INFO] Parsed {len(records)} record(s) from file.")
    return records


# ---------------------------------------------------------------------------
# Demo / simulated data
# ---------------------------------------------------------------------------
def generate_demo_data():
    """Create simulated benchmark records for demonstration."""
    import random
    random.seed(42)
    sizes = [64, 128, 256, 512, 1024, 2048, 4096]
    records = []
    for sz in sizes:
        cpu = int(sz * 3.2 + random.gauss(0, sz * 0.1))
        dma = int(sz * 0.9 + 80 + random.gauss(0, sz * 0.05))
        spd = round(cpu / max(dma, 1), 2)
        records.append(BenchmarkRecord(sz, dma, cpu, spd, "PASS"))
    print("[INFO] Generated demo data with 7 transfer sizes.")
    return records


# ---------------------------------------------------------------------------
# Text report
# ---------------------------------------------------------------------------
def print_report(records):
    """Print a formatted table to stdout."""
    print("\n" + "=" * 72)
    print("  DMA Memory-to-Memory Benchmark Report")
    print(f"  Generated: {datetime.now():%Y-%m-%d %H:%M:%S}")
    print("=" * 72)
    hdr = f"{'Size':>8} {'DMA cyc':>10} {'CPU cyc':>10} {'Speedup':>8} {'Verify':>8}"
    print(hdr)
    print("-" * 72)
    pass_count = fail_count = 0
    for r in records:
        flag = "✓" if r.verify == "PASS" else "✗"
        print(
            f"{r.size:>8} {r.dma_cycles:>10} {r.cpu_cycles:>10}"
            f" {r.speedup:>7.2f}x {flag:>7}"
        )
        if r.verify == "PASS":
            pass_count += 1
        else:
            fail_count += 1
    print("-" * 72)
    if records:
        avg_spd = statistics.mean(r.speedup for r in records)
        max_spd = max(r.speedup for r in records)
        print(f"  Avg speedup: {avg_spd:.2f}x  |  Max speedup: {max_spd:.2f}x")
    print(f"  Pass: {pass_count}  Fail: {fail_count}")
    print("=" * 72 + "\n")


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------
def plot_results(records):
    """Create bar chart (DMA vs CPU) and speedup line chart."""
    if not HAS_MATPLOTLIB:
        print("[WARN] matplotlib not installed – skipping charts.")
        print("       Install with: pip install matplotlib")
        return

    # Deduplicate by size (keep last)
    by_size = OrderedDict()
    for r in records:
        by_size[r.size] = r
    data = list(by_size.values())
    data.sort(key=lambda r: r.size)

    sizes = [r.size for r in data]
    dma_c = [r.dma_cycles for r in data]
    cpu_c = [r.cpu_cycles for r in data]
    speedups = [r.speedup for r in data]
    labels = [str(s) for s in sizes]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5))
    fig.suptitle("STM32 DMA Memory-to-Memory Benchmark", fontsize=14,
                 fontweight="bold")

    # --- Bar chart ---
    x_pos = range(len(sizes))
    w = 0.35
    bars1 = ax1.bar([p - w / 2 for p in x_pos], cpu_c, w, label="CPU memcpy",
                    color="#e74c3c", edgecolor="black", linewidth=0.5)
    bars2 = ax1.bar([p + w / 2 for p in x_pos], dma_c, w, label="DMA",
                    color="#2ecc71", edgecolor="black", linewidth=0.5)
    ax1.set_xlabel("Transfer Size (bytes)")
    ax1.set_ylabel("Clock Cycles")
    ax1.set_title("DMA vs CPU Transfer Cycles")
    ax1.set_xticks(list(x_pos))
    ax1.set_xticklabels(labels, rotation=45, ha="right")
    ax1.legend()
    ax1.grid(axis="y", alpha=0.3)

    for bar in bars1:
        ax1.annotate(f"{int(bar.get_height())}",
                     xy=(bar.get_x() + bar.get_width() / 2, bar.get_height()),
                     ha="center", va="bottom", fontsize=7)
    for bar in bars2:
        ax1.annotate(f"{int(bar.get_height())}",
                     xy=(bar.get_x() + bar.get_width() / 2, bar.get_height()),
                     ha="center", va="bottom", fontsize=7)

    # --- Speedup line chart ---
    ax2.plot(labels, speedups, "o-", color="#3498db", linewidth=2,
             markersize=7, label="Speedup")
    ax2.axhline(y=1.0, color="gray", linestyle="--", linewidth=0.8,
                label="Break-even (1x)")
    ax2.fill_between(range(len(labels)), speedups, 1.0, alpha=0.15,
                     color="#3498db")
    ax2.set_xlabel("Transfer Size (bytes)")
    ax2.set_ylabel("Speedup (x)")
    ax2.set_title("DMA Speedup over CPU memcpy")
    ax2.legend()
    ax2.grid(alpha=0.3)
    for i, sp in enumerate(speedups):
        ax2.annotate(f"{sp:.2f}x", (i, sp), textcoords="offset points",
                     xytext=(0, 8), ha="center", fontsize=8)

    plt.tight_layout()
    out_path = "dma_benchmark_results.png"
    plt.savefig(out_path, dpi=150)
    print(f"[INFO] Chart saved to {out_path}")
    plt.show()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_parser():
    p = argparse.ArgumentParser(
        description="STM32 DMA Memory-to-Memory Benchmark Analyzer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Examples:\n"
               "  %(prog)s --port /dev/ttyACM0\n"
               "  %(prog)s --file capture.log\n"
               "  %(prog)s                       (demo mode)\n",
    )
    p.add_argument("--port", "-p", help="Serial port (e.g. /dev/ttyACM0)")
    p.add_argument("--baud", "-b", type=int, default=BAUD_RATE,
                   help=f"Baud rate (default {BAUD_RATE})")
    p.add_argument("--file", "-f", help="Read from log file instead of serial")
    p.add_argument("--duration", "-d", type=int, default=DEFAULT_DURATION,
                   help=f"Serial capture duration in seconds (default {DEFAULT_DURATION})")
    p.add_argument("--no-plot", action="store_true",
                   help="Skip chart generation")
    return p


def main():
    args = build_parser().parse_args()

    # Acquire data
    if args.file:
        records = read_file(args.file)
    elif args.port:
        records = read_serial(args.port, args.baud, args.duration)
    else:
        print("[INFO] No --port or --file given – running demo mode.")
        records = generate_demo_data()

    if not records:
        print("[WARN] No benchmark records captured. Exiting.")
        sys.exit(1)

    print_report(records)

    if not args.no_plot:
        plot_results(records)


if __name__ == "__main__":
    main()
