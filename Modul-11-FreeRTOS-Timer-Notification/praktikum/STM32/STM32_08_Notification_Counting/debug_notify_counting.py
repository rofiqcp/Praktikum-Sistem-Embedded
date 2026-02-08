#!/usr/bin/env python3
"""Debug script for STM32_08_Notification_Counting - event counting analysis."""
import serial, time, csv, sys, re
from datetime import datetime

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILE = 'notification_counting_log.csv'

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    print(f"[DEBUG] Connecting to {port} @ {BAUD_RATE}")

    batches, processed = [], []
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        with open(CSV_FILE, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event', 'batch', 'count', 'pending', 'tick'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"  {line}")
                ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]

                m_gen = re.search(r'\[GEN\] Batch #(\d+).*Generating (\d+)', line)
                m_proc = re.search(r'\[HANDLER\].*#(\d+).*pending=(\d+).*Tick=(\d+)', line)

                if m_gen:
                    batches.append(int(m_gen.group(2)))
                    writer.writerow([ts, 'BATCH', m_gen.group(1), m_gen.group(2), '', ''])
                    f.flush()
                elif m_proc:
                    processed.append(int(m_proc.group(3)))
                    writer.writerow([ts, 'PROCESSED', '', m_proc.group(1), m_proc.group(2), m_proc.group(3)])
                    f.flush()

    except KeyboardInterrupt:
        print(f"\n=== Counting Stats ===")
        print(f"Batches: {len(batches)}, Total generated: {sum(batches)}")
        print(f"Total processed: {len(processed)}")
        if batches:
            print(f"Avg batch size: {sum(batches)/len(batches):.1f}")
        print(f"Log: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")

if __name__ == '__main__':
    main()
