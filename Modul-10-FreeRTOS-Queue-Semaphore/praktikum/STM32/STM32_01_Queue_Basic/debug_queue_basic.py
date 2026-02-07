#!/usr/bin/env python3
"""
Debug/Analysis Script for STM32_01_Queue_Basic
Monitors serial output, logs to CSV, and analyzes queue patterns.
"""

import serial
import sys
import os
import csv
import time
import signal
import argparse
import re
from datetime import datetime
from collections import defaultdict

# Global flag for graceful shutdown
running = True

def signal_handler(sig, frame):
    global running
    print("\n[INFO] Ctrl+C detected, stopping capture...")
    running = False

signal.signal(signal.SIGINT, signal_handler)

def parse_args():
    parser = argparse.ArgumentParser(description="Queue Basic Debug Monitor")
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0', help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60, help='Capture duration in seconds (default: 60)')
    parser.add_argument('-o', '--output', default='queue_basic_log.csv', help='Output CSV file')
    return parser.parse_args()

def main():
    args = parse_args()
    
    print(f"[CONFIG] Port: {args.port}, Baudrate: {args.baudrate}")
    print(f"[CONFIG] Duration: {args.duration}s, Output: {args.output}")
    
    # Statistics
    stats = {
        'total_lines': 0,
        'send_count': 0,
        'receive_count': 0,
        'send_fail_count': 0,
        'timeouts': 0,
        'values_sent': [],
        'values_received': [],
        'queue_fill_levels': [],
    }
    
    try:
        ser = serial.Serial(args.port, args.baudrate, timeout=1)
        print(f"[CONNECTED] {args.port} at {args.baudrate} baud")
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {args.port}: {e}")
        sys.exit(1)
    
    start_time = time.time()
    
    with open(args.output, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'elapsed_s', 'source', 'event', 'value', 'raw_line'])
        
        print(f"\n[CAPTURING] Started at {datetime.now().strftime('%H:%M:%S')}...")
        print("-" * 60)
        
        while running and (time.time() - start_time) < args.duration:
            try:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='replace').strip()
                    if not line:
                        continue
                    
                    elapsed = time.time() - start_time
                    timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                    stats['total_lines'] += 1
                    
                    print(f"[{timestamp}] {line}")
                    
                    # Parse producer messages
                    m = re.search(r'\[Producer\] Sent: (\d+)', line)
                    if m:
                        val = int(m.group(1))
                        stats['send_count'] += 1
                        stats['values_sent'].append(val)
                        writer.writerow([timestamp, f"{elapsed:.3f}", 'Producer', 'SEND', val, line])
                    
                    # Parse consumer messages
                    m = re.search(r'\[Consumer\] Received: (\d+)', line)
                    if m:
                        val = int(m.group(1))
                        stats['receive_count'] += 1
                        stats['values_received'].append(val)
                        writer.writerow([timestamp, f"{elapsed:.3f}", 'Consumer', 'RECEIVE', val, line])
                    
                    # Parse queue full
                    if 'Queue full' in line:
                        stats['send_fail_count'] += 1
                        writer.writerow([timestamp, f"{elapsed:.3f}", 'Producer', 'QUEUE_FULL', '', line])
                    
                    # Parse timeout
                    if 'timeout' in line:
                        stats['timeouts'] += 1
                        writer.writerow([timestamp, f"{elapsed:.3f}", 'Consumer', 'TIMEOUT', '', line])
                    
                    # Parse queue status
                    m = re.search(r'waiting: (\d+), spaces: (\d+)', line)
                    if m:
                        waiting = int(m.group(1))
                        stats['queue_fill_levels'].append(waiting)
                        writer.writerow([timestamp, f"{elapsed:.3f}", 'Queue', 'STATUS', waiting, line])
                        
            except Exception as e:
                print(f"[WARN] Read error: {e}")
    
    ser.close()
    
    # Print summary
    duration = time.time() - start_time
    print("\n" + "=" * 60)
    print("         QUEUE BASIC - ANALYSIS SUMMARY")
    print("=" * 60)
    print(f"  Duration:          {duration:.1f}s")
    print(f"  Total lines:       {stats['total_lines']}")
    print(f"  Items sent:        {stats['send_count']}")
    print(f"  Items received:    {stats['receive_count']}")
    print(f"  Send failures:     {stats['send_fail_count']}")
    print(f"  Receive timeouts:  {stats['timeouts']}")
    
    if stats['send_count'] > 0:
        success_rate = (stats['send_count'] - stats['send_fail_count']) / stats['send_count'] * 100
        print(f"  Send success rate: {success_rate:.1f}%")
    
    if stats['queue_fill_levels']:
        avg_fill = sum(stats['queue_fill_levels']) / len(stats['queue_fill_levels'])
        max_fill = max(stats['queue_fill_levels'])
        print(f"  Avg queue fill:    {avg_fill:.1f}")
        print(f"  Max queue fill:    {max_fill}")
    
    if stats['values_sent'] and stats['values_received']:
        sent_set = set(stats['values_sent'])
        recv_set = set(stats['values_received'])
        lost = sent_set - recv_set
        if lost:
            print(f"  Lost values:       {len(lost)} ({sorted(lost)[:5]}...)")
        else:
            print(f"  Lost values:       None (all delivered)")
    
    print(f"\n  Log saved to: {args.output}")
    print("=" * 60)

if __name__ == '__main__':
    main()
