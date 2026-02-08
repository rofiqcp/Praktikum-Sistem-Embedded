#!/usr/bin/env python3
"""
============================================================================
Debug Analysis Script - STM32_13: I2C DMA Transfer
============================================================================
Script ini menganalisis output serial dari program I2C DMA Transfer
untuk memverifikasi transfer polling vs interrupt vs DMA dan
membandingkan waktu serta CPU usage.

Penggunaan:
    python debug_analysis.py /dev/ttyUSB0
    python debug_analysis.py COM3
    python debug_analysis.py output.log
============================================================================
"""

import sys
import re
from collections import defaultdict

def parse_line(line):
    """Parse satu baris output serial."""
    result = {}

    # Parse transfer result: [POLLING] Transfer 32 bytes...
    m = re.match(r'\[(POLLING|INTERRUPT|DMA)\]\s+Transfer\s+(\d+)\s+bytes', line)
    if m:
        result['type'] = 'transfer_start'
        result['mode'] = m.group(1)
        result['size'] = int(m.group(2))
        return result

    # Parse write time:   Write: 12345 us (...)
    m = re.match(r'\s+Write:\s+(\d+)\s+us\s+\((.*?)\)', line)
    if m:
        result['type'] = 'write'
        result['time_us'] = int(m.group(1))
        result['detail'] = m.group(2)
        return result

    # Parse read time:   Read : 12345 us (...)
    m = re.match(r'\s+Read\s*:\s+(\d+)\s+us\s+\((.*?)\)', line)
    if m:
        result['type'] = 'read'
        result['time_us'] = int(m.group(1))
        result['detail'] = m.group(2)
        return result

    # Parse verification:   Verifikasi: OK
    m = re.match(r'\s+Verifikasi:\s+(\w+)', line)
    if m:
        result['type'] = 'verify'
        result['status'] = m.group(1)
        return result

    # Parse DMA callback time
    m = re.match(r'\s+DMA\s+(TX|RX)\s+callback\s+time:\s+(\d+)\s+us', line)
    if m:
        result['type'] = 'dma_callback'
        result['direction'] = m.group(1)
        result['time_us'] = int(m.group(2))
        return result

    # Parse CPU free time
    m = re.match(r'.*CPU berhasil melakukan\s+(\d+)\s+iterasi', line)
    if m:
        result['type'] = 'cpu_free'
        result['iterations'] = int(m.group(1))
        return result

    # Parse prime count
    m = re.match(r'.*Ditemukan\s+(\d+)\s+bilangan prima', line)
    if m:
        result['type'] = 'primes'
        result['count'] = int(m.group(1))
        return result

    # Parse I2C device found
    m = re.match(r'\s+\[FOUND\]\s+Device di alamat\s+0x(\w+)', line)
    if m:
        result['type'] = 'device_found'
        result['address'] = int(m.group(1), 16)
        return result

    # Parse DMA read continuous
    m = re.match(r'\[DMA-READ #(\d+)\].*time=(\d+)us', line)
    if m:
        result['type'] = 'dma_continuous'
        result['cycle'] = int(m.group(1))
        result['time_us'] = int(m.group(2))
        return result

    return None

def analyze_data(lines):
    """Analisis data output serial."""
    # Kumpulkan data per mode dan size
    current_mode = None
    current_size = None
    benchmarks = defaultdict(lambda: defaultdict(dict))
    verifications = []
    dma_callbacks = []
    cpu_free_iters = 0
    prime_count = 0
    devices_found = []
    dma_continuous = []

    for line in lines:
        parsed = parse_line(line.strip())
        if parsed is None:
            continue

        if parsed['type'] == 'transfer_start':
            current_mode = parsed['mode']
            current_size = parsed['size']
        elif parsed['type'] == 'write' and current_mode and current_size:
            benchmarks[current_size][current_mode]['write_us'] = parsed['time_us']
        elif parsed['type'] == 'read' and current_mode and current_size:
            benchmarks[current_size][current_mode]['read_us'] = parsed['time_us']
        elif parsed['type'] == 'verify':
            verifications.append({'mode': current_mode, 'size': current_size,
                                  'status': parsed['status']})
        elif parsed['type'] == 'dma_callback':
            dma_callbacks.append(parsed)
        elif parsed['type'] == 'cpu_free':
            cpu_free_iters = parsed['iterations']
        elif parsed['type'] == 'primes':
            prime_count = parsed['count']
        elif parsed['type'] == 'device_found':
            devices_found.append(parsed['address'])
        elif parsed['type'] == 'dma_continuous':
            dma_continuous.append(parsed)

    print("\n" + "=" * 70)
    print("  ANALISIS DEBUG - I2C DMA Transfer")
    print("=" * 70)

    # I2C Device Detection
    print(f"\n--- Device Detection ---")
    if devices_found:
        for addr in devices_found:
            label = " (AT24C32 EEPROM)" if addr == 0x50 else ""
            print(f"  Device ditemukan: 0x{addr:02X}{label}")
    else:
        print(f"  Tidak ada device terdeteksi")

    # Benchmark Results
    print(f"\n--- Benchmark Transfer ---")
    sizes = sorted(benchmarks.keys())
    if sizes:
        print(f"\n  WRITE (us):")
        print(f"  {'Size':>6} | {'Polling':>10} | {'Interrupt':>10} | {'DMA':>10} | {'DMA vs Poll':>12}")
        print(f"  {'-'*6}-+-{'-'*10}-+-{'-'*10}-+-{'-'*10}-+-{'-'*12}")
        for size in sizes:
            modes = benchmarks[size]
            poll_w = modes.get('POLLING', {}).get('write_us', 0)
            it_w = modes.get('INTERRUPT', {}).get('write_us', 0)
            dma_w = modes.get('DMA', {}).get('write_us', 0)
            speedup = f"{poll_w/dma_w:.1f}x" if dma_w > 0 else "N/A"
            print(f"  {size:>5}B | {poll_w:>10} | {it_w:>10} | {dma_w:>10} | {speedup:>12}")

        print(f"\n  READ (us):")
        print(f"  {'Size':>6} | {'Polling':>10} | {'Interrupt':>10} | {'DMA':>10} | {'DMA vs Poll':>12}")
        print(f"  {'-'*6}-+-{'-'*10}-+-{'-'*10}-+-{'-'*10}-+-{'-'*12}")
        for size in sizes:
            modes = benchmarks[size]
            poll_r = modes.get('POLLING', {}).get('read_us', 0)
            it_r = modes.get('INTERRUPT', {}).get('read_us', 0)
            dma_r = modes.get('DMA', {}).get('read_us', 0)
            speedup = f"{poll_r/dma_r:.1f}x" if dma_r > 0 else "N/A"
            print(f"  {size:>5}B | {poll_r:>10} | {it_r:>10} | {dma_r:>10} | {speedup:>12}")
    else:
        print(f"  Tidak ada data benchmark.")

    # DMA Callbacks
    if dma_callbacks:
        print(f"\n--- DMA Callbacks ---")
        for cb in dma_callbacks:
            print(f"  DMA {cb['direction']} selesai: {cb['time_us']} us")

    # CPU Free Time
    if cpu_free_iters > 0:
        print(f"\n--- CPU Free Time (DMA) ---")
        print(f"  Iterasi selama DMA transfer : {cpu_free_iters}")
        print(f"  Bilangan prima ditemukan    : {prime_count}")
        print(f"  -> CPU 100% produktif selama DMA!")

    # DMA Continuous
    if dma_continuous:
        print(f"\n--- DMA Continuous Reads ---")
        times = [d['time_us'] for d in dma_continuous]
        print(f"  Jumlah reads     : {len(dma_continuous)}")
        print(f"  Waktu min/max    : {min(times)} / {max(times)} us")
        print(f"  Waktu rata-rata  : {sum(times)//len(times)} us")

    # Verifikasi
    print(f"\n" + "=" * 70)
    print(f"  VERIFIKASI")
    print(f"=" * 70)

    errors = 0
    ok_count = sum(1 for v in verifications if v['status'] == 'OK')
    fail_count = sum(1 for v in verifications if v['status'] != 'OK')

    if ok_count > 0:
        print(f"  [OK] {ok_count} transfer terverifikasi benar")
    if fail_count > 0:
        print(f"  [GAGAL] {fail_count} transfer gagal verifikasi!")
        errors += fail_count

    if not benchmarks:
        print(f"  [GAGAL] Tidak ada data benchmark")
        errors += 1
    else:
        # Cek apakah DMA ada
        has_dma = any('DMA' in benchmarks[s] for s in benchmarks)
        if has_dma:
            print(f"  [OK] DMA transfer berfungsi")
        else:
            print(f"  [GAGAL] Tidak ada data DMA")
            errors += 1

    if cpu_free_iters > 0:
        print(f"  [OK] CPU free time terdemonstrasikan ({cpu_free_iters} iterasi)")
    else:
        print(f"  [INFO] CPU free time tidak terdeteksi")

    if errors == 0:
        print(f"\n  >>> SEMUA TES BERHASIL <<<")
    else:
        print(f"\n  >>> {errors} TES GAGAL <<<")
    print("=" * 70)

def main():
    if len(sys.argv) < 2:
        print("Penggunaan: python debug_analysis.py <serial_port_or_logfile>")
        print("Contoh    : python debug_analysis.py /dev/ttyUSB0")
        print("            python debug_analysis.py output.log")
        sys.exit(1)

    source = sys.argv[1]

    try:
        with open(source, 'r') as f:
            lines = f.readlines()
        print(f"Membaca dari file: {source}")
        analyze_data(lines)
    except FileNotFoundError:
        try:
            import serial
            print(f"Membuka serial port: {source} (115200 baud)")
            print("Tekan Ctrl+C untuk berhenti dan melihat analisis...\n")

            ser = serial.Serial(source, 115200, timeout=1)
            lines = []
            try:
                while True:
                    line = ser.readline().decode('utf-8', errors='ignore')
                    if line:
                        print(line, end='')
                        lines.append(line)
            except KeyboardInterrupt:
                pass
            finally:
                ser.close()

            analyze_data(lines)
        except ImportError:
            print("Error: pyserial belum terinstall.")
            print("Install: pip install pyserial")
            sys.exit(1)
        except Exception as e:
            print(f"Error membuka serial port: {e}")
            sys.exit(1)

if __name__ == '__main__':
    main()
