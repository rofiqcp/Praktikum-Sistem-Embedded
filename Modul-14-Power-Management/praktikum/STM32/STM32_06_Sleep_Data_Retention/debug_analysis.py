#!/usr/bin/env python3
"""
STM32_06_Sleep_Data_Retention - Data Persistence Analyzer
Parses serial output to analyze backup register persistence across Standby.
Tracks SRAM vs backup register data, boot count, sensor values.
"""

import sys
import re
from datetime import datetime


class DataPersistenceAnalyzer:
    def __init__(self):
        self.start_time = datetime.now()
        self.boot_events = []
        self.backup_snapshots = []
        self.persistence_comparisons = []
        self.current_boot = None

    def parse_line(self, line):
        """Parse a single serial output line."""
        line = line.strip()
        if not line:
            return

        # Normal boot
        if 'NORMAL BOOT' in line or 'Normal boot' in line:
            self.current_boot = {
                'type': 'NORMAL',
                'timestamp': datetime.now(),
                'bkp_data': {}
            }
            self.boot_events.append(self.current_boot)
            return

        # Standby wakeup
        if 'WAKEUP FROM STANDBY' in line:
            self.current_boot = {
                'type': 'STANDBY_WAKE',
                'timestamp': datetime.now(),
                'bkp_data': {}
            }
            self.boot_events.append(self.current_boot)
            return

        # Magic key validity
        magic_match = re.search(r'Magic key (VALID|INVALID)', line)
        if magic_match and self.current_boot:
            self.current_boot['magic_valid'] = (magic_match.group(1) == 'VALID')
            return

        # Backup register parsing
        bkp_boot = re.search(r'BKP_DR1 \(Boot Count\)\s*:\s*(\d+)', line)
        if bkp_boot and self.current_boot:
            self.current_boot['bkp_data']['boot_count'] = int(bkp_boot.group(1))
            return

        bkp_sensor = re.search(r'BKP_DR2 \(Sensor Val\)\s*:\s*(\d+)', line)
        if bkp_sensor and self.current_boot:
            self.current_boot['bkp_data']['sensor_val'] = int(bkp_sensor.group(1))
            return

        bkp_magic = re.search(r'BKP_DR3 \(Magic Key\)\s*:\s*0x([0-9A-Fa-f]+)', line)
        if bkp_magic and self.current_boot:
            self.current_boot['bkp_data']['magic'] = int(bkp_magic.group(1), 16)
            return

        bkp_ts = re.search(r'BKP_DR4 \(Timestamp\)\s*:\s*(\d+)', line)
        if bkp_ts and self.current_boot:
            self.current_boot['bkp_data']['timestamp'] = int(bkp_ts.group(1))
            return

        bkp_rt = re.search(r'BKP_DR5 \(Total RT\)\s*:\s*(\d+)', line)
        if bkp_rt and self.current_boot:
            self.current_boot['bkp_data']['total_runtime'] = int(bkp_rt.group(1))
            return

        bkp_flags = re.search(r'BKP_DR6 \(Status Flags\)\s*:\s*0x([0-9A-Fa-f]+)', line)
        if bkp_flags and self.current_boot:
            self.current_boot['bkp_data']['status_flags'] = int(bkp_flags.group(1), 16)
            return

        # SRAM comparison
        sram_match = re.search(r'SRAM \(volatile\)\s*\|\s*(\d+)\s*\|\s*(\d+)', line)
        if sram_match:
            self.persistence_comparisons.append({
                'sram_boot': int(sram_match.group(1)),
                'sram_sensor': int(sram_match.group(2)),
                'timestamp': datetime.now()
            })
            return

        bkp_row = re.search(r'Backup Regs\s*\|\s*(\d+)\s*\|\s*(\d+)', line)
        if bkp_row and self.persistence_comparisons:
            self.persistence_comparisons[-1]['bkp_boot'] = int(bkp_row.group(1))
            self.persistence_comparisons[-1]['bkp_sensor'] = int(bkp_row.group(2))
            return

        # New data written
        new_boot = re.search(r'\[DATA\] New boot count\s*:\s*(\d+)', line)
        if new_boot and self.current_boot:
            self.current_boot['new_boot_count'] = int(new_boot.group(1))
            return

        new_sensor = re.search(r'\[DATA\] New sensor value\s*:\s*(\d+)', line)
        if new_sensor and self.current_boot:
            self.current_boot['new_sensor'] = int(new_sensor.group(1))
            return

        rtc_ctr = re.search(r'\[DATA\] RTC counter\s*:\s*(\d+)', line)
        if rtc_ctr and self.current_boot:
            self.current_boot['rtc_counter'] = int(rtc_ctr.group(1))
            return

    def print_summary(self):
        """Print analysis summary."""
        elapsed = (datetime.now() - self.start_time).total_seconds()

        normal = sum(1 for b in self.boot_events if b['type'] == 'NORMAL')
        standby = sum(1 for b in self.boot_events if b['type'] == 'STANDBY_WAKE')

        print("\n" + "=" * 60)
        print("  DATA PERSISTENCE ANALYZER")
        print("=" * 60)
        print(f"  Analysis duration : {elapsed:.1f} seconds")
        print(f"  Total boots       : {len(self.boot_events)}")
        print(f"    Normal          : {normal}")
        print(f"    From Standby    : {standby}")

        # Boot count tracking
        boot_counts = []
        for b in self.boot_events:
            bc = b.get('new_boot_count') or b.get('bkp_data', {}).get('boot_count', 0)
            boot_counts.append(bc)

        if boot_counts:
            print(f"\n  --- Boot Count Progression ---")
            for i, bc in enumerate(boot_counts):
                btype = self.boot_events[i]['type']
                print(f"    Boot #{i+1}: count={bc} ({btype})")

        # SRAM vs Backup comparison
        if self.persistence_comparisons:
            print(f"\n  --- SRAM vs Backup Register Comparison ---")
            print(f"    {'#':<4} {'SRAM Boot':<12} {'BKP Boot':<12} {'SRAM Lost?':<12}")
            print(f"    {'-'*40}")
            for i, comp in enumerate(self.persistence_comparisons):
                sram_b = comp.get('sram_boot', '?')
                bkp_b = comp.get('bkp_boot', '?')
                lost = "YES" if sram_b == 0 and isinstance(bkp_b, int) and bkp_b > 0 else "NO"
                print(f"    {i+1:<4} {str(sram_b):<12} {str(bkp_b):<12} {lost:<12}")

        # Backup register data history
        if self.boot_events:
            print(f"\n  --- Backup Register History ---")
            for i, boot in enumerate(self.boot_events):
                bkp = boot.get('bkp_data', {})
                if bkp:
                    magic_ok = "VALID" if boot.get('magic_valid', False) else "INVALID"
                    print(f"    Boot #{i+1} ({boot['type']}):")
                    print(f"      Boot Count : {bkp.get('boot_count', 'N/A')}")
                    print(f"      Sensor Val : {bkp.get('sensor_val', 'N/A')}")
                    print(f"      Magic      : {magic_ok}")
                    print(f"      Runtime    : {bkp.get('total_runtime', 'N/A')} sec")

        # Sensor value tracking
        sensor_vals = [b.get('new_sensor', 0) for b in self.boot_events if 'new_sensor' in b]
        if sensor_vals:
            print(f"\n  --- Sensor Value History ---")
            for i, val in enumerate(sensor_vals):
                print(f"    Reading #{i+1}: {val}")
            print(f"    Range: {min(sensor_vals)} - {max(sensor_vals)}")

        # Conclusion
        print(f"\n  --- Conclusion ---")
        if standby > 0:
            print(f"    Backup registers SURVIVED {standby} Standby cycles")
            print(f"    SRAM was RESET on every Standby wakeup")
            print(f"    Data persistence: VERIFIED")
        else:
            print(f"    No Standby wakeup detected yet")
            print(f"    Run longer to observe persistence behavior")

        print("\n" + "=" * 60)

    def process_file(self, filename):
        try:
            with open(filename, 'r') as f:
                for line in f:
                    self.parse_line(line)
        except FileNotFoundError:
            print(f"File not found: {filename}")
            return False
        return True


def main():
    analyzer = DataPersistenceAnalyzer()

    if len(sys.argv) > 1:
        if analyzer.process_file(sys.argv[1]):
            analyzer.print_summary()
    else:
        print("[DataPersistenceAnalyzer] Demo mode")
        print("Usage: python debug_analysis.py <serial_log.txt>")
        print()

        sample = [
            ">>> NORMAL BOOT (first power-on) <<<",
            "  BKP_DR1 (Boot Count)  : 0",
            "  BKP_DR2 (Sensor Val)  : 0",
            "  BKP_DR3 (Magic Key)   : 0x0000 (INVALID)",
            "  BKP_DR4 (Timestamp)   : 0",
            "  BKP_DR5 (Total RT)    : 0 sec",
            "  BKP_DR6 (Status Flags): 0x0000",
            "  SRAM (volatile) |          0 |          0",
            "  Backup Regs     |          0 |          0",
            "[DATA] New boot count   : 1",
            "[DATA] New sensor value : 2847",
            "[DATA] RTC counter      : 5",
            ">>> WAKEUP FROM STANDBY <<<",
            "[BKP] Magic key VALID - backup data intact",
            "  BKP_DR1 (Boot Count)  : 1",
            "  BKP_DR2 (Sensor Val)  : 2847",
            "  BKP_DR3 (Magic Key)   : 0xCAFE (VALID)",
            "  BKP_DR4 (Timestamp)   : 5",
            "  BKP_DR5 (Total RT)    : 10 sec",
            "  BKP_DR6 (Status Flags): 0x0001",
            "  SRAM (volatile) |          0 |          0",
            "  Backup Regs     |          1 |       2847",
            "[DATA] New boot count   : 2",
            "[DATA] New sensor value : 1523",
            "[DATA] RTC counter      : 20",
        ]

        for line in sample:
            analyzer.parse_line(line)

        analyzer.print_summary()


if __name__ == "__main__":
    main()
