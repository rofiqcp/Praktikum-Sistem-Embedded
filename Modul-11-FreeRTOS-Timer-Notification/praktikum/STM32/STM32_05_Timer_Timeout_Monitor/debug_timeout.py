#!/usr/bin/env python3
"""Debug script for STM32_05_Timer_Timeout_Monitor - heartbeat/timeout tracking."""
import serial, time, csv, sys, re
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'timeout_monitor_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    heartbeats, timeouts = [], []
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event', 'count', 'tick'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m_hb = re.search(r'\[HEARTBEAT\] #(\d+).*Tick=(\d+)', line)
                m_to = re.search(r'\[TIMEOUT\].*#(\d+).*Tick=(\d+)', line)

                if m_hb:
                    heartbeats.append(int(m_hb.group(2)))
                    writer.writerow([ts, 'HEARTBEAT', m_hb.group(1), m_hb.group(2)])
                    f.flush()
                elif m_to:
                    timeouts.append(int(m_to.group(2)))
                    writer.writerow([ts, 'TIMEOUT', m_to.group(1), m_to.group(2)])
                    f.flush()
                    print(f"    *** TIMEOUT DETECTED ***")

    except KeyboardInterrupt:
        print(f"\n=== Timeout Monitor Stats ===")
        print(f"Heartbeats: {len(heartbeats)}")
        print(f"Timeouts: {len(timeouts)}")
        if heartbeats and len(heartbeats) >= 2:
            intervals = [heartbeats[i]-heartbeats[i-1] for i in range(1, len(heartbeats))]
            print(f"HB intervals - Avg: {sum(intervals)/len(intervals):.0f}ms")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
