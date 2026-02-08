#!/usr/bin/env python3
"""Debug script for STM32_12_Timer_vs_Notification_Benchmark - results capture."""
import serial, time, csv, sys, re
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'benchmark_results_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    results = {}
    in_results = False
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'method', 'min_ticks', 'max_ticks', 'avg_ticks', 'count'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                if 'BENCHMARK RESULTS' in line:
                    in_results = True
                    continue

                if in_results:
                    m = re.search(r'(\w+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)', line)
                    if m:
                        method = m.group(1)
                        mn, mx, avg, cnt = m.group(2), m.group(3), m.group(4), m.group(5)
                        results[method] = {'min': mn, 'max': mx, 'avg': avg, 'count': cnt}
                        writer.writerow([ts, method, mn, mx, avg, cnt])
                        f.flush()

                # Progress tracking
                m_prog = re.search(r'(\w+): (\d+)/(\d+) done', line)
                if m_prog:
                    method, done, total = m_prog.group(1), m_prog.group(2), m_prog.group(3)
                    pct = int(done) * 100 // int(total)
                    print(f"    -> {method} progress: {pct}%")

    except KeyboardInterrupt:
        print(f"\n=== Benchmark Results ===")
        if results:
            print(f"{'Method':<15} {'Min':>8} {'Max':>8} {'Avg':>8} {'Count':>8}")
            print(f"{'-'*15} {'-'*8} {'-'*8} {'-'*8} {'-'*8}")
            for m, r in results.items():
                print(f"{m:<15} {r['min']:>8} {r['max']:>8} {r['avg']:>8} {r['count']:>8}")
        else:
            print("No results captured yet. Wait for benchmark to complete.")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
