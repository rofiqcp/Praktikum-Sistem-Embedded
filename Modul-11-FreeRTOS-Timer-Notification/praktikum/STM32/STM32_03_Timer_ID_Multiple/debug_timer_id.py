#!/usr/bin/env python3
"""Debug script for STM32_03_Timer_ID_Multiple - tracks multi-timer events."""
import serial, time, csv, sys, re
from collections import defaultdict
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'timer_id_multiple_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    timer_data = {0: [], 1: [], 2: []}

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'timer_id', 'period_ms', 'count', 'tick'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m = re.search(r'\[TIMER-(\d)\].*Period=(\d+)ms.*Count=(\d+).*Tick=(\d+)', line)
                if m:
                    tid = int(m.group(1))
                    period, count, tick = int(m.group(2)), int(m.group(3)), int(m.group(4))
                    timer_data[tid].append(tick)
                    writer.writerow([ts, tid, period, count, tick])
                    f.flush()

                    if len(timer_data[tid]) >= 2:
                        interval = timer_data[tid][-1] - timer_data[tid][-2]
                        print(f"    -> Timer-{tid} interval: {interval}ms (expected {period}ms)")

    except KeyboardInterrupt:
        print(f"\n=== Multi-Timer Stats ===")
        for tid in range(3):
            ticks = timer_data[tid]
            print(f"Timer-{tid}: {len(ticks)} fires")
            if len(ticks) >= 2:
                intervals = [ticks[i]-ticks[i-1] for i in range(1, len(ticks))]
                avg = sum(intervals)/len(intervals)
                print(f"  Avg interval: {avg:.1f}ms, Min: {min(intervals)}, Max: {max(intervals)}")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
