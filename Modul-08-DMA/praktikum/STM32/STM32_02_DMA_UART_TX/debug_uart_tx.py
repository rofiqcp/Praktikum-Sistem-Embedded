#!/usr/bin/env python3
"""
Debug & Performance Analyzer for STM32 DMA UART TX

Parses serial output from STM32 DMA UART TX firmware comparing
DMA-based transmission versus polling (blocking) transmission.

Expected [DATA] tagged lines:
    [DATA] method=DMA,size=NNN,time_us=NNN,throughput=NNN
    [DATA] method=POLL,size=NNN,time_us=NNN,throughput=NNN

Features:
    - Serial port reading (pyserial) or log file parsing
    - Grouped bar chart: DMA vs Polling transfer time per size
    - Throughput comparison line chart
    - Demo mode with realistic simulated timing data
    - Graceful fallback when matplotlib/pyserial not installed

Usage:
    python debug_uart_tx.py --port /dev/ttyACM0
    python debug_uart_tx.py --file capture.log
    python debug_uart_tx.py  (demo mode)
"""

import re
import sys
import time
import argparse
import statistics
from collections import defaultdict
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
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# ---------------------------------------------------------------------------
# Constants & regex
# ---------------------------------------------------------------------------
DATA_PATTERN = re.compile(
    r"\[DATA\]\s*method=(\w+),size=(\d+),time_us=(\d+),throughput=(\d+)",
    re.IGNORECASE,
)
BAUD_RATE = 115200
READ_TIMEOUT = 0.5
DEFAULT_DURATION = 30


# ---------------------------------------------------------------------------
# Data container
# ---------------------------------------------------------------------------
class TxRecord:
    """One UART TX measurement."""

    __slots__ = ("method", "size", "time_us", "throughput")

    def __init__(self, method, size, time_us, throughput):
        self.method = method.upper()
        self.size = int(size)
        self.time_us = int(time_us)
        self.throughput = int(throughput)

    def __repr__(self):
        return (f"TxRec({self.method}, size={self.size}, "
                f"t={self.time_us}µs, tp={self.throughput})")


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------
def parse_line(line: str):
    """Return TxRecord or None."""
    m = DATA_PATTERN.search(line)
    if m:
        return TxRecord(*m.groups())
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
    print(f"[INFO] Captured {len(records)} TX record(s).")
    return records


def read_file(path: str):
    """Parse saved log file."""
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
# Demo data
# ---------------------------------------------------------------------------
def generate_demo_data():
    """Simulate DMA vs Polling TX benchmark data."""
    import random
    random.seed(99)
    sizes = [32, 64, 128, 256, 512, 1024, 2048]
    records = []
    for sz in sizes:
        # Polling: roughly proportional to size
        poll_us = int(sz * 8.68 + random.gauss(0, sz * 0.3))
        poll_tp = int(sz * 1_000_000 / max(poll_us, 1))
        records.append(TxRecord("POLL", sz, poll_us, poll_tp))
        # DMA: fixed setup + small proportional part
        dma_us = int(45 + sz * 0.9 + random.gauss(0, 5))
        dma_tp = int(sz * 1_000_000 / max(dma_us, 1))
        records.append(TxRecord("DMA", sz, dma_us, dma_tp))
    print(f"[INFO] Generated demo data: {len(records)} records "
          f"({len(sizes)} sizes × 2 methods).")
    return records


# ---------------------------------------------------------------------------
# Text report
# ---------------------------------------------------------------------------
def print_report(records):
    """Formatted comparison table."""
    # Group by size
    by_size = defaultdict(dict)
    for r in records:
        by_size[r.size][r.method] = r

    print("\n" + "=" * 78)
    print("  STM32 DMA UART TX – Performance Report")
    print(f"  Generated: {datetime.now():%Y-%m-%d %H:%M:%S}")
    print("=" * 78)
    hdr = (f"{'Size':>6} │ {'POLL µs':>9} {'POLL B/s':>10} │"
           f" {'DMA µs':>9} {'DMA B/s':>10} │ {'Saving':>8}")
    print(hdr)
    print("─" * 78)

    savings_list = []
    for sz in sorted(by_size.keys()):
        group = by_size[sz]
        poll = group.get("POLL")
        dma = group.get("DMA")
        p_us = poll.time_us if poll else "—"
        p_tp = poll.throughput if poll else "—"
        d_us = dma.time_us if dma else "—"
        d_tp = dma.throughput if dma else "—"
        if poll and dma and poll.time_us > 0:
            saving = (1 - dma.time_us / poll.time_us) * 100
            savings_list.append(saving)
            sav_str = f"{saving:+.1f}%"
        else:
            sav_str = "—"
        print(f"{sz:>6} │ {str(p_us):>9} {str(p_tp):>10} │"
              f" {str(d_us):>9} {str(d_tp):>10} │ {sav_str:>8}")

    print("─" * 78)
    if savings_list:
        print(f"  Average time saving with DMA: "
              f"{statistics.mean(savings_list):.1f}%")
    print("=" * 78 + "\n")


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------
def plot_results(records):
    """Bar chart for time comparison, line chart for throughput."""
    if not HAS_MATPLOTLIB:
        print("[WARN] matplotlib not installed – skipping charts.")
        print("       Install with: pip install matplotlib")
        return

    by_size = defaultdict(dict)
    for r in records:
        by_size[r.size][r.method] = r
    sizes_sorted = sorted(by_size.keys())

    poll_times = [by_size[s].get("POLL", TxRecord("POLL", s, 0, 0)).time_us
                  for s in sizes_sorted]
    dma_times = [by_size[s].get("DMA", TxRecord("DMA", s, 0, 0)).time_us
                 for s in sizes_sorted]
    poll_tp = [by_size[s].get("POLL", TxRecord("POLL", s, 0, 0)).throughput
               for s in sizes_sorted]
    dma_tp = [by_size[s].get("DMA", TxRecord("DMA", s, 0, 0)).throughput
              for s in sizes_sorted]
    labels = [str(s) for s in sizes_sorted]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle("STM32 DMA UART TX – DMA vs Polling", fontsize=14,
                 fontweight="bold")

    # --- Transfer time bar chart ---
    x = range(len(sizes_sorted))
    w = 0.35
    ax1.bar([i - w / 2 for i in x], poll_times, w, label="Polling",
            color="#e74c3c", edgecolor="black", linewidth=0.5)
    ax1.bar([i + w / 2 for i in x], dma_times, w, label="DMA",
            color="#2ecc71", edgecolor="black", linewidth=0.5)
    ax1.set_xlabel("Payload Size (bytes)")
    ax1.set_ylabel("Transfer Time (µs)")
    ax1.set_title("Transfer Time Comparison")
    ax1.set_xticks(list(x))
    ax1.set_xticklabels(labels, rotation=45, ha="right")
    ax1.legend()
    ax1.grid(axis="y", alpha=0.3)

    # --- Throughput line chart ---
    ax2.plot(labels, poll_tp, "s--", color="#e74c3c", linewidth=1.5,
             markersize=6, label="Polling")
    ax2.plot(labels, dma_tp, "o-", color="#2ecc71", linewidth=2,
             markersize=7, label="DMA")
    ax2.set_xlabel("Payload Size (bytes)")
    ax2.set_ylabel("Throughput (bytes/s)")
    ax2.set_title("Throughput Comparison")
    ax2.legend()
    ax2.grid(alpha=0.3)
    for i, (pt, dt) in enumerate(zip(poll_tp, dma_tp)):
        if dt > pt:
            ax2.annotate(f"+{(dt-pt)*100//max(pt,1)}%",
                         (i, dt), textcoords="offset points",
                         xytext=(0, 8), ha="center", fontsize=7,
                         color="#27ae60")

    plt.tight_layout()
    out_path = "uart_tx_dma_results.png"
    plt.savefig(out_path, dpi=150)
    print(f"[INFO] Chart saved to {out_path}")
    plt.show()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_parser():
    p = argparse.ArgumentParser(
        description="STM32 DMA UART TX Performance Analyzer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Examples:\n"
               "  %(prog)s --port /dev/ttyACM0\n"
               "  %(prog)s --file uart_log.txt\n"
               "  %(prog)s                       (demo mode)\n",
    )
    p.add_argument("--port", "-p", help="Serial port (e.g. /dev/ttyACM0)")
    p.add_argument("--baud", "-b", type=int, default=BAUD_RATE,
                   help=f"Baud rate (default {BAUD_RATE})")
    p.add_argument("--file", "-f", help="Read from log file")
    p.add_argument("--duration", "-d", type=int, default=DEFAULT_DURATION,
                   help=f"Serial capture duration in seconds (default {DEFAULT_DURATION})")
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
        print("[WARN] No TX records captured. Exiting.")
        sys.exit(1)

    print_report(records)

    if not args.no_plot:
        plot_results(records)


if __name__ == "__main__":
    main()
