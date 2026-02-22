#!/usr/bin/env python3
"""
============================================================================
BLE Advertising Scanner & RSSI Analysis Tool
============================================================================
Deskripsi:
  Script Python untuk melakukan scanning BLE devices di sekitar,
  khususnya mencari ESP32 BLE Advertiser. Menganalisis RSSI
  (Received Signal Strength Indicator) untuk estimasi jarak.

Dependensi:
  pip install bleak matplotlib numpy

Penggunaan:
  python debug_analysis.py              # Scan mode
  python debug_analysis.py --monitor    # Continuous RSSI monitoring
  python debug_analysis.py --plot       # Plot RSSI over time

Author: Praktikum Sistem Embedded
============================================================================
"""

import asyncio
import argparse
import time
import sys
from datetime import datetime

try:
    from bleak import BleakScanner
    HAS_BLEAK = True
except ImportError:
    HAS_BLEAK = False
    print("[WARNING] 'bleak' library not installed. Install with: pip install bleak")

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARNING] 'matplotlib/numpy' not installed. Plot features disabled.")

# ============================================================================
# Konfigurasi
# ============================================================================
TARGET_DEVICE_NAME = "ESP32_BLE_ADV"
SCAN_DURATION = 5.0  # Durasi scan dalam detik
RSSI_LOG_FILE = "ble_rssi_log.csv"

# Path loss model constants untuk estimasi jarak
# RSSI = -10 * n * log10(d) + A
# A = RSSI at 1 meter, n = path loss exponent
RSSI_AT_1M = -59       # Typical RSSI at 1 meter (dBm)
PATH_LOSS_EXPONENT = 2.0  # Free space = 2.0, indoor = 2.5-3.5


def estimate_distance(rssi):
    """Estimasi jarak berdasarkan RSSI menggunakan path loss model."""
    if rssi == 0:
        return -1.0
    ratio = (RSSI_AT_1M - rssi) / (10 * PATH_LOSS_EXPONENT)
    distance = 10 ** ratio
    return round(distance, 2)


def rssi_to_quality(rssi):
    """Konversi RSSI ke kualitas sinyal (persentase)."""
    if rssi >= -50:
        return 100
    elif rssi >= -60:
        return 80
    elif rssi >= -70:
        return 60
    elif rssi >= -80:
        return 40
    elif rssi >= -90:
        return 20
    else:
        return 0


async def scan_ble_devices():
    """Scan semua BLE devices yang ada di sekitar."""
    print(f"\n{'='*60}")
    print(f"  BLE Device Scanner")
    print(f"  Scanning for {SCAN_DURATION} seconds...")
    print(f"{'='*60}\n")

    devices = await BleakScanner.discover(timeout=SCAN_DURATION)

    if not devices:
        print("[!] No BLE devices found!")
        return []

    # Sort berdasarkan RSSI (sinyal terkuat dulu)
    devices_sorted = sorted(devices, key=lambda d: d.rssi, reverse=True)

    print(f"{'No':<4} {'Name':<25} {'Address':<20} {'RSSI':<8} {'Quality':<10} {'Distance':<10}")
    print("-" * 77)

    target_found = False
    results = []

    for i, device in enumerate(devices_sorted, 1):
        name = device.name or "Unknown"
        quality = rssi_to_quality(device.rssi)
        distance = estimate_distance(device.rssi)

        marker = ""
        if TARGET_DEVICE_NAME in (name or ""):
            marker = " <<<< TARGET"
            target_found = True

        print(f"{i:<4} {name:<25} {device.address:<20} {device.rssi:<8} {quality}%{'':<7} ~{distance}m{marker}")

        results.append({
            'name': name,
            'address': device.address,
            'rssi': device.rssi,
            'quality': quality,
            'distance': distance,
            'timestamp': datetime.now().isoformat()
        })

    print(f"\nTotal devices found: {len(devices)}")
    if target_found:
        print(f"[OK] Target device '{TARGET_DEVICE_NAME}' FOUND!")
    else:
        print(f"[!!] Target device '{TARGET_DEVICE_NAME}' NOT found.")
        print(f"     Make sure ESP32 is advertising.")

    return results


async def monitor_rssi(duration=60):
    """Monitor RSSI dari target device secara kontinyu."""
    print(f"\n{'='*60}")
    print(f"  RSSI Continuous Monitor - Target: {TARGET_DEVICE_NAME}")
    print(f"  Duration: {duration}s | Press Ctrl+C to stop")
    print(f"{'='*60}\n")

    rssi_data = []
    timestamps = []
    start_time = time.time()

    # Buka file log
    with open(RSSI_LOG_FILE, 'w') as f:
        f.write("timestamp,rssi,quality,estimated_distance\n")

    try:
        while (time.time() - start_time) < duration:
            devices = await BleakScanner.discover(timeout=2.0)

            target = None
            for device in devices:
                if device.name and TARGET_DEVICE_NAME in device.name:
                    target = device
                    break

            elapsed = round(time.time() - start_time, 1)

            if target:
                rssi = target.rssi
                quality = rssi_to_quality(rssi)
                distance = estimate_distance(rssi)

                rssi_data.append(rssi)
                timestamps.append(elapsed)

                # Visual RSSI bar
                bar_len = max(0, rssi + 100)
                bar = "█" * (bar_len // 2)

                print(f"[{elapsed:>6.1f}s] RSSI: {rssi:>4d} dBm | "
                      f"Quality: {quality:>3d}% | "
                      f"Distance: ~{distance:>5.2f}m | {bar}")

                # Log ke file
                with open(RSSI_LOG_FILE, 'a') as f:
                    f.write(f"{elapsed},{rssi},{quality},{distance}\n")
            else:
                print(f"[{elapsed:>6.1f}s] Target not found in this scan")

    except KeyboardInterrupt:
        print("\n\n[INFO] Monitoring stopped by user.")

    # Statistik RSSI
    if rssi_data:
        print(f"\n{'='*60}")
        print(f"  RSSI Statistics")
        print(f"{'='*60}")
        print(f"  Samples     : {len(rssi_data)}")
        print(f"  Min RSSI    : {min(rssi_data)} dBm")
        print(f"  Max RSSI    : {max(rssi_data)} dBm")
        print(f"  Average RSSI: {sum(rssi_data)/len(rssi_data):.1f} dBm")
        print(f"  Std Dev     : {(sum((x - sum(rssi_data)/len(rssi_data))**2 for x in rssi_data) / len(rssi_data))**0.5:.1f} dBm")
        print(f"  Data saved  : {RSSI_LOG_FILE}")

    return timestamps, rssi_data


def plot_rssi_data(timestamps=None, rssi_data=None):
    """Plot RSSI data dari monitoring atau file log."""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib required for plotting. Install: pip install matplotlib numpy")
        return

    # Jika tidak ada data, coba baca dari file
    if timestamps is None or rssi_data is None:
        try:
            data = np.genfromtxt(RSSI_LOG_FILE, delimiter=',', skip_header=1)
            timestamps = data[:, 0]
            rssi_data = data[:, 1]
            print(f"[INFO] Loaded {len(timestamps)} samples from {RSSI_LOG_FILE}")
        except Exception as e:
            print(f"[ERROR] Cannot load data: {e}")
            return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f"BLE RSSI Analysis - {TARGET_DEVICE_NAME}", fontsize=14, fontweight='bold')

    # Plot 1: RSSI over time
    ax1 = axes[0][0]
    ax1.plot(timestamps, rssi_data, 'b-', linewidth=1, alpha=0.7, label='RSSI')
    ax1.axhline(y=np.mean(rssi_data), color='r', linestyle='--', label=f'Mean: {np.mean(rssi_data):.1f} dBm')
    ax1.fill_between(timestamps, rssi_data, alpha=0.3)
    ax1.set_xlabel('Time (seconds)')
    ax1.set_ylabel('RSSI (dBm)')
    ax1.set_title('RSSI Over Time')
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    # Plot 2: RSSI histogram
    ax2 = axes[0][1]
    ax2.hist(rssi_data, bins=20, color='green', alpha=0.7, edgecolor='black')
    ax2.axvline(x=np.mean(rssi_data), color='r', linestyle='--', label=f'Mean: {np.mean(rssi_data):.1f}')
    ax2.set_xlabel('RSSI (dBm)')
    ax2.set_ylabel('Frequency')
    ax2.set_title('RSSI Distribution')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # Plot 3: Estimated distance over time
    ax3 = axes[1][0]
    distances = [estimate_distance(r) for r in rssi_data]
    ax3.plot(timestamps, distances, 'orange', linewidth=1)
    ax3.set_xlabel('Time (seconds)')
    ax3.set_ylabel('Estimated Distance (m)')
    ax3.set_title('Estimated Distance Over Time')
    ax3.grid(True, alpha=0.3)

    # Plot 4: Signal quality over time
    ax4 = axes[1][1]
    qualities = [rssi_to_quality(r) for r in rssi_data]
    ax4.plot(timestamps, qualities, 'purple', linewidth=1)
    ax4.fill_between(timestamps, qualities, alpha=0.2, color='purple')
    ax4.set_xlabel('Time (seconds)')
    ax4.set_ylabel('Signal Quality (%)')
    ax4.set_title('Signal Quality Over Time')
    ax4.set_ylim(0, 105)
    ax4.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('ble_rssi_analysis.png', dpi=150, bbox_inches='tight')
    print("[INFO] Plot saved to ble_rssi_analysis.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(description="BLE Advertising Scanner & RSSI Analyzer")
    parser.add_argument('--monitor', action='store_true', help='Continuous RSSI monitoring mode')
    parser.add_argument('--plot', action='store_true', help='Plot RSSI data from log file')
    parser.add_argument('--duration', type=int, default=60, help='Monitor duration in seconds')
    parser.add_argument('--target', type=str, default=TARGET_DEVICE_NAME, help='Target device name')
    args = parser.parse_args()

    global TARGET_DEVICE_NAME
    TARGET_DEVICE_NAME = args.target

    if not HAS_BLEAK and not args.plot:
        print("[ERROR] 'bleak' library required for BLE scanning.")
        print("        Install with: pip install bleak")
        sys.exit(1)

    if args.plot:
        plot_rssi_data()
    elif args.monitor:
        timestamps, rssi_data = asyncio.run(monitor_rssi(args.duration))
        if rssi_data and HAS_MATPLOTLIB:
            plot_rssi_data(timestamps, rssi_data)
    else:
        asyncio.run(scan_ble_devices())


if __name__ == "__main__":
    main()
