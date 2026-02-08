#!/usr/bin/env python3
"""Debug script for STM32_09_Notification_ISR - ISR response comparison."""
import serial, time, csv, sys, re
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'notification_isr_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    notify_times, sema_times = [], []
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'method', 'response_ticks', 'total'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m_n = re.search(r'\[NOTIFY\] Response: (\d+).*Total=(\d+)', line)
                m_s = re.search(r'\[SEMA\] Response: (\d+).*Total=(\d+)', line)

                if m_n:
                    notify_times.append(int(m_n.group(1)))
                    writer.writerow([ts, 'NOTIFY', m_n.group(1), m_n.group(2)])
                    f.flush()
                elif m_s:
                    sema_times.append(int(m_s.group(1)))
                    writer.writerow([ts, 'SEMAPHORE', m_s.group(1), m_s.group(2)])
                    f.flush()

    except KeyboardInterrupt:
        print(f"\n=== ISR Response Comparison ===")
        for name, times in [("Notification", notify_times), ("Semaphore", sema_times)]:
            if times:
                print(f"{name}: n={len(times)}, avg={sum(times)/len(times):.1f}, "
                      f"min={min(times)}, max={max(times)} ticks")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
