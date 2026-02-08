#!/usr/bin/env python3
"""Debug script for STM32_11_Event_Group_Sync - barrier sync analysis."""
import serial, time, csv, sys, re
from collections import defaultdict
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'event_group_sync_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    barriers, arrivals = [], defaultdict(list)
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event', 'worker', 'round', 'tick'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m_arr = re.search(r'\[WORKER-(\d)\] Round (\d+): Reached.*Tick=(\d+)', line)
                m_sync = re.search(r'\[WORKER-(\d)\] Round (\d+):.*SYNC OK.*Tick=(\d+)', line)
                m_bar = re.search(r'\[SYNC\].*Barrier #(\d+)', line)

                if m_arr:
                    w, r, t = m_arr.group(1), m_arr.group(2), int(m_arr.group(3))
                    arrivals[r].append((w, t))
                    writer.writerow([ts, 'ARRIVE', w, r, t])
                    f.flush()
                elif m_sync:
                    writer.writerow([ts, 'SYNC', m_sync.group(1), m_sync.group(2), m_sync.group(3)])
                    f.flush()
                elif m_bar:
                    barriers.append(int(m_bar.group(1)))

    except KeyboardInterrupt:
        print(f"\n=== Sync Barrier Stats ===")
        print(f"Barriers completed: {len(barriers)}")
        for rnd, arrs in sorted(arrivals.items()):
            if len(arrs) >= 2:
                ticks = [t for _, t in arrs]
                spread = max(ticks) - min(ticks)
                print(f"  Round {rnd}: spread={spread}ms")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
