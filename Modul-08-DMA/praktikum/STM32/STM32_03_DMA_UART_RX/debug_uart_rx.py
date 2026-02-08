#!/usr/bin/env python3
"""
Debug & Monitoring Tool for STM32 DMA UART RX

Parses serial output from STM32 DMA UART RX firmware that uses
DMA + IDLE-line detection for efficient variable-length reception.

Expected [DATA] tagged lines:
    [DATA] received=NNN,buffer_usage=NNN,overruns=NNN,idle_count=NNN

Features:
    - Serial port reading (pyserial) or log file parsing
    - Real-time style chart of cumulative bytes received over samples
    - Buffer usage & overrun tracking chart
    - Demo mode with simulated RX traffic patterns
    - Graceful fallback when matplotlib/pyserial not installed

Usage:
    python debug_uart_rx.py --port /dev/ttyACM0
    python debug_uart_rx.py --file capture.log
    python debug_uart_rx.py  (demo mode)
"""

import re
import sys
import time
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
    r"\[DATA\]\s*received=(\d+),buffer_usage=(\d+),"
    r"overruns=(\d+),idle_count=(\d+)",
    re.IGNORECASE,
)
BAUD_RATE = 115200
READ_TIMEOUT = 0.5
DEFAULT_DURATION = 30


# ---------------------------------------------------------------------------
# Data container
# ---------------------------------------------------------------------------
class RxRecord:
    """Single UART RX status snapshot."""

    __slots__ = ("received", "buffer_usage", "overruns", "idle_count",
                 "timestamp")

    def __init__(self, received, buffer_usage, overruns, idle_count,
                 timestamp=None):
        self.received = int(received)
        self.buffer_usage = int(buffer_usage)
        self.overruns = int(overruns)
        self.idle_count = int(idle_count)
        self.timestamp = timestamp or time.time()

    def __repr__(self):
        return (f"RxRec(rx={self.received}, buf={self.buffer_usage}%, "
                f"ovr={self.overruns}, idle={self.idle_count})")


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------
def parse_line(line: str):
    """Return an RxRecord if the line matches, else None."""
    m = DATA_PATTERN.search(line)
    if m:
        return RxRecord(*m.groups())
    return None


def read_serial(port: str, baudrate: int = BAUD_RATE,
                duration: int = DEFAULT_DURATION):
    """Capture lines from serial port for *duration* seconds."""
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
    print(f"[INFO] Captured {len(records)} RX record(s).")
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
                    rec.timestamp = idx * 0.5  # synthetic timestamps
                    records.append(rec)
    except OSError as exc:
        print(f"[ERROR] File: {exc}")
    print(f"[INFO] Parsed {len(records)} record(s) from file.")
    return records


# ---------------------------------------------------------------------------
# Demo data
# ---------------------------------------------------------------------------
def generate_demo_data():
    """Simulate RX traffic with occasional bursts and overruns."""
    import random
    random.seed(7)
    records = []
    cumulative_rx = 0
    total_overruns = 0
    idle_count = 0
    for i in range(40):
        # Simulate variable-length incoming packets
        if i % 10 < 7:
            chunk = random.randint(10, 120)
        else:
            chunk = random.randint(200, 512)  # burst
        cumulative_rx += chunk
        buf_usage = min(100, int(chunk / 5.12 * 10 + random.gauss(0, 5)))
        if buf_usage > 90:
            total_overruns += 1
        idle_count += 1
        rec = RxRecord(cumulative_rx, buf_usage, total_overruns, idle_count,
                       timestamp=i * 0.5)
        records.append(rec)
    print(f"[INFO] Generated demo data: {len(records)} samples over "
          f"{records[-1].timestamp:.1f}s.")
    return records


# ---------------------------------------------------------------------------
# Text report
# ---------------------------------------------------------------------------
def print_report(records):
    """Print summary statistics."""
    print("\n" + "=" * 70)
    print("  STM32 DMA UART RX – Reception Monitor Report")
    print(f"  Generated: {datetime.now():%Y-%m-%d %H:%M:%S}")
    print("=" * 70)

    if not records:
        print("  No data available.")
        return

    hdr = f"{'#':>4} {'Time':>7} {'Rx Total':>10} {'Buf %':>7} {'Overruns':>9} {'IDLE':>6}"
    print(hdr)
    print("-" * 70)
    for i, r in enumerate(records):
        flag = " ⚠" if r.overruns > 0 and (i == 0 or
               r.overruns > records[i-1].overruns) else ""
        print(f"{i:>4} {r.timestamp:>7.1f}s {r.received:>10}"
              f" {r.buffer_usage:>6}% {r.overruns:>9} {r.idle_count:>6}{flag}")
    print("-" * 70)

    total_rx = records[-1].received
    duration = records[-1].timestamp - records[0].timestamp
    avg_buf = statistics.mean(r.buffer_usage for r in records)
    max_buf = max(r.buffer_usage for r in records)
    total_ovr = records[-1].overruns

    print(f"  Total received : {total_rx:,} bytes")
    if duration > 0:
        print(f"  Avg throughput : {total_rx / duration:,.0f} bytes/s")
    print(f"  Buffer usage   : avg {avg_buf:.1f}%  max {max_buf}%")
    print(f"  Total overruns : {total_ovr}")
    print(f"  IDLE events    : {records[-1].idle_count}")
    if total_ovr > 0:
        print("  ⚠  Overruns detected – consider larger DMA buffer or "
              "faster processing.")
    print("=" * 70 + "\n")


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------
def plot_results(records):
    """Create RX monitoring charts."""
    if not HAS_MATPLOTLIB:
        print("[WARN] matplotlib not installed – skipping charts.")
        print("       Install with: pip install matplotlib")
        return

    ts = [r.timestamp for r in records]
    rx_bytes = [r.received for r in records]
    buf_pct = [r.buffer_usage for r in records]
    overruns = [r.overruns for r in records]

    fig, axes = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
    fig.suptitle("STM32 DMA UART RX – Reception Monitor", fontsize=14,
                 fontweight="bold")

    # --- Bytes received (cumulative) ---
    ax1 = axes[0]
    ax1.plot(ts, rx_bytes, "-", color="#2980b9", linewidth=2,
             label="Cumulative Bytes Received")
    ax1.fill_between(ts, rx_bytes, alpha=0.15, color="#2980b9")
    ax1.set_ylabel("Bytes Received")
    ax1.set_title("Cumulative Data Reception")
    ax1.legend(loc="upper left")
    ax1.grid(alpha=0.3)

    # Annotate per-sample deltas as bar-style on secondary axis
    ax1b = ax1.twinx()
    deltas = [rx_bytes[0]] + [rx_bytes[i] - rx_bytes[i-1]
                               for i in range(1, len(rx_bytes))]
    ax1b.bar(ts, deltas, width=0.35, alpha=0.25, color="#e67e22",
             label="Per-sample Δ bytes")
    ax1b.set_ylabel("Δ bytes / sample")
    ax1b.legend(loc="upper right")

    # --- Buffer usage & overruns ---
    ax2 = axes[1]
    ax2.plot(ts, buf_pct, "o-", color="#27ae60", linewidth=1.5,
             markersize=4, label="Buffer Usage %")
    ax2.axhline(y=80, color="orange", linestyle="--", linewidth=0.8,
                label="Warning (80%)")
    ax2.axhline(y=100, color="red", linestyle="--", linewidth=0.8,
                label="Full (100%)")
    ax2.fill_between(ts, buf_pct, alpha=0.12, color="#27ae60")
    ax2.set_ylabel("Buffer Usage (%)")
    ax2.set_xlabel("Time (s)")
    ax2.set_title("Buffer Utilization & Overrun Events")
    ax2.set_ylim(-5, 115)
    ax2.legend(loc="upper left", fontsize=8)
    ax2.grid(alpha=0.3)

    # Mark overrun events
    ovr_ts = []
    ovr_val = []
    for i in range(1, len(records)):
        if records[i].overruns > records[i-1].overruns:
            ovr_ts.append(ts[i])
            ovr_val.append(buf_pct[i])
    if ovr_ts:
        ax2.scatter(ovr_ts, ovr_val, color="red", marker="x", s=100,
                    zorder=5, label=f"Overrun ({len(ovr_ts)})")
        ax2.legend(loc="upper left", fontsize=8)

    plt.tight_layout()
    out_path = "uart_rx_dma_monitor.png"
    plt.savefig(out_path, dpi=150)
    print(f"[INFO] Chart saved to {out_path}")
    plt.show()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_parser():
    p = argparse.ArgumentParser(
        description="STM32 DMA UART RX Reception Monitor",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Examples:\n"
               "  %(prog)s --port /dev/ttyACM0\n"
               "  %(prog)s --file rx_log.txt\n"
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
        print("[WARN] No RX records captured. Exiting.")
        sys.exit(1)

    print_report(records)

    if not args.no_plot:
        plot_results(records)


if __name__ == "__main__":
    main()
