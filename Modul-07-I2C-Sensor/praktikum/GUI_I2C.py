"""
Program Monitor I2C - UI untuk menampilkan data sensor I2C dan testing fungsionalitas

Program ini menggunakan tkinter untuk antarmuka GUI dan mendukung:
- Koneksi serial ke ESP32/STM32
- Display data sensor real-time
- Grafik real-time untuk berbagai sensor
- Simulasi I2C device scanner
- Panel testing untuk simulasi skenario
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
import serial
import serial.tools.list_ports
import threading
import time
import csv
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import re
from collections import deque
import datetime
import random

# Konfigurasi warna dan style
BG_COLOR = "#f0f0f0"
BUTTON_COLOR = "#4CAF50"
BUTTON_DANGER = "#f44336"
TEXT_COLOR = "#333333"

class I2CMonitorUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Monitor I2C - Praktikum Sistem Embedded")
        self.root.geometry("1300x750")
        self.root.configure(bg=BG_COLOR)
        
        # Variabel serial
        self.serial_port = None
        self.is_connected = False
        self.read_thread = None
        self.stop_thread = False
        
        # Data storage untuk grafik
        self.max_data_points = 100
        self.temp_data = deque(maxlen=self.max_data_points)
        self.hum_data = deque(maxlen=self.max_data_points)
        self.press_data = deque(maxlen=self.max_data_points)
        self.lux_data = deque(maxlen=self.max_data_points)
        self.time_data = deque(maxlen=self.max_data_points)
        
        # Data CSV
        self.csv_data = []
        
        # Define variables dari kode C (dapat dikonfigurasi)
        self.define_vars = {
            "#define I2C_SDA_PIN": "21",
            "#define I2C_SCL_PIN": "22",
            "#define I2C_FREQ_HZ": "100000",
            "#define BME280_ADDR": "0x76",
            "#define BH1750_ADDR": "0x23",
            "#define MAX_RETRY": "3",
            "#define SCAN_INTERVAL_MS": "5000",
            "#define DATA_FORMAT": "CSV",
            "#define ENABLE_TEMP": "1",
            "#define ENABLE_HUM": "1",
            "#define ENABLE_PRESS": "1",
            "#define ENABLE_LUX": "1",
        }
        
        self.setup_ui()
        
    def setup_ui(self):
        # Header
        header_frame = tk.Frame(self.root, bg="#2196F3", height=60)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        title_label = tk.Label(
            header_frame, 
            text="Monitor I2C - Praktikum Sistem Embedded",
            font=("Arial", 16, "bold"),
            bg="#2196F3",
            fg="white"
        )
        title_label.pack(pady=15)
        
        # Main container
        main_container = tk.Frame(self.root, bg=BG_COLOR)
        main_container.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Left panel - Controls
        left_panel = tk.Frame(main_container, bg=BG_COLOR, width=320)
        left_panel.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))
        left_panel.pack_propagate(False)
        
        # Connection frame
        conn_frame = tk.LabelFrame(left_panel, text="Koneksi Serial", bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 10, "bold"))
        conn_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Port Serial
        tk.Label(conn_frame, text="Port Serial:", bg=BG_COLOR, fg=TEXT_COLOR).pack(anchor=tk.W, padx=5, pady=(5, 0))
        port_frame = tk.Frame(conn_frame, bg=BG_COLOR)
        port_frame.pack(fill=tk.X, padx=5, pady=(0, 5))
        
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(port_frame, textvariable=self.port_var, state="readonly", width=22)
        self.port_combo.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        tk.Button(port_frame, text="Refresh", command=self.refresh_ports, bg="#2196F3", fg="white", padx=10).pack(side=tk.RIGHT, padx=(5, 0))
        
        # Baud Rate
        tk.Label(conn_frame, text="Baud Rate:", bg=BG_COLOR, fg=TEXT_COLOR).pack(anchor=tk.W, padx=5, pady=(5, 0))
        self.baud_var = tk.StringVar(value="115200")
        baud_combo = ttk.Combobox(conn_frame, textvariable=self.baud_var, state="readonly", width=25,
                                   values=["9600", "115200", "256000", "921600"])
        baud_combo.pack(fill=tk.X, padx=5, pady=(0, 5))
        
        # Connect/Disconnect button
        self.connect_btn = tk.Button(conn_frame, text="Sambung", command=self.toggle_connection, 
                                      bg=BUTTON_COLOR, fg="white", font=("Arial", 10, "bold"),
                                      padx=20, pady=5)
        self.connect_btn.pack(pady=5)
        
        # I2C Device Scanner
        scan_frame = tk.LabelFrame(left_panel, text="I2C Device Scanner", bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 10, "bold"))
        scan_frame.pack(fill=tk.X, pady=(0, 10))
        
        tk.Button(scan_frame, text="Pindai Perangkat I2C", command=self.scan_i2c_devices, 
                   bg="#FF9800", fg="white", font=("Arial", 10), padx=10, pady=5).pack(pady=5, padx=5, fill=tk.X)
        
        # Define Variables display
        define_frame = tk.LabelFrame(left_panel, text="Define Variables (Konfigurasi)", bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 10, "bold"))
        define_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.define_text = scrolledtext.ScrolledText(define_frame, height=8, font=("Courier", 8))
        self.define_text.pack(fill=tk.X, padx=5, pady=5)
        self.update_define_display()
        
        # Testing Panel
        test_frame = tk.LabelFrame(left_panel, text="Panel Testing", bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 10, "bold"))
        test_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Project selector
        tk.Label(test_frame, text="Pilih Project:", bg=BG_COLOR, fg=TEXT_COLOR).pack(anchor=tk.W, padx=5, pady=(5, 0))
        self.project_var = tk.StringVar(value="ESP32-BME280")
        project_combo = ttk.Combobox(test_frame, textvariable=self.project_var, state="readonly", width=25,
                                      values=["ESP32-BME280", "ESP32-BH1750", "STM32-MultiSensor"])
        project_combo.pack(fill=tk.X, padx=5, pady=(0, 5))
        
        # Test buttons
        tk.Button(test_frame, text="Simulasi Data Sensor", command=self.simulate_sensor_data,
                   bg="#9C27B0", fg="white", padx=10, pady=3).pack(fill=tk.X, padx=5, pady=(0, 3))
        tk.Button(test_frame, text="Simulasi Device Found", command=self.simulate_device_found,
                   bg="#9C27B0", fg="white", padx=10, pady=3).pack(fill=tk.X, padx=5, pady=(0, 3))
        tk.Button(test_frame, text="Simulasi Scan Complete", command=self.simulate_scan_complete,
                   bg="#9C27B0", fg="white", padx=10, pady=3).pack(fill=tk.X, padx=5, pady=(0, 3))
        tk.Button(test_frame, text="Simulasi Error", command=self.simulate_error,
                   bg="#f44336", fg="white", padx=10, pady=3).pack(fill=tk.X, padx=5, pady=(0, 5))
        
        # Right panel - Display
        right_panel = tk.Frame(main_container, bg=BG_COLOR)
        right_panel.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Data display
        display_frame = tk.LabelFrame(right_panel, text="Tampilkan Data", bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 10, "bold"))
        display_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        self.data_display = scrolledtext.ScrolledText(display_frame, height=12, font=("Courier", 9))
        self.data_display.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Button frame untuk data display
        btn_frame = tk.Frame(display_frame, bg=BG_COLOR)
        btn_frame.pack(fill=tk.X, padx=5, pady=(0, 5))
        
        tk.Button(btn_frame, text="Simpan ke CSV", command=self.save_to_csv,
                   bg="#2196F3", fg="white", padx=15, pady=3).pack(side=tk.LEFT, padx=(0, 5))
        tk.Button(btn_frame, text="Bersihkan", command=self.clear_display,
                   bg="#757575", fg="white", padx=15, pady=3).pack(side=tk.LEFT)
        
        # Graph frame
        graph_frame = tk.LabelFrame(right_panel, text="Grafik Real-Time", bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 10, "bold"))
        graph_frame.pack(fill=tk.BOTH, expand=True)
        
        # Setup matplotlib figure
        self.fig = Figure(figsize=(6, 3.5), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_xlabel("Waktu (sample)")
        self.ax.set_ylabel("Nilai")
        self.ax.grid(True, alpha=0.3)
        
        self.canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Status bar
        self.status_var = tk.StringVar(value="Siap - Pilih port serial dan klik Sambung")
        status_bar = tk.Label(self.root, textvariable=self.status_var, bd=1, relief=tk.SUNKEN, anchor=tk.W,
                               bg="#E0E0E0", fg=TEXT_COLOR)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)
        
        # Refresh ports saat startup
        self.refresh_ports()
        
    def refresh_ports(self):
        """Refresh daftar port serial yang tersedia"""
        try:
            ports = serial.tools.list_ports.comports()
            port_list = [port.device for port in ports]
            self.port_combo['values'] = port_list
            if port_list:
                self.port_combo.set(port_list[0])
            else:
                self.port_combo.set("")
            self.log_message("INFO: Port serial di-refresh")
        except Exception as e:
            self.log_message(f"ERROR: Gagal refresh port - {str(e)}")
        
    def toggle_connection(self):
        """Toggle koneksi serial"""
        if not self.is_connected:
            self.connect_serial()
        else:
            self.disconnect_serial()
            
    def connect_serial(self):
        """Sambung ke port serial"""
        port = self.port_var.get()
        baud = int(self.baud_var.get())
        
        if not port:
            messagebox.showerror("Error", "Pilih port serial terlebih dahulu!")
            return
            
        try:
            self.serial_port = serial.Serial(port, baud, timeout=1)
            self.is_connected = True
            self.connect_btn.config(text="Putus", bg=BUTTON_DANGER)
            self.status_var.set(f"Terhubung ke {port} @ {baud} baud")
            self.log_message(f"INFO: Terhubung ke {port} @ {baud} baud")
            
            # Start reading thread
            self.stop_thread = False
            self.read_thread = threading.Thread(target=self.read_serial_data, daemon=True)
            self.read_thread.start()
            
        except Exception as e:
            messagebox.showerror("Error", f"Gagal terhubung ke {port}: {str(e)}")
            self.log_message(f"ERROR: Gagal terhubung - {str(e)}")
            
    def disconnect_serial(self):
        """Putus koneksi serial"""
        self.stop_thread = True
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.is_connected = False
        self.connect_btn.config(text="Sambung", bg=BUTTON_COLOR)
        self.status_var.set("Terputus")
        self.log_message("INFO: Koneksi diputus")
        
    def read_serial_data(self):
        """Baca data dari serial port secara threaded"""
        while not self.stop_thread and self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting > 0:
                    line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        self.root.after(0, self.process_data, line)
            except Exception as e:
                self.root.after(0, self.log_message, f"ERROR: {str(e)}")
                break
            time.sleep(0.01)
            
    def process_data(self, line):
        """Proses dan parse data yang diterima"""
        self.log_message(f"RAW: {line}")
        
        # Parse DATA,temp,25.5,hum,60.0,...
        if line.startswith("DATA,"):
            self.parse_sensor_data(line)
        # Parse DEVICE_FOUND: addr=0x76 name=BME280
        elif "DEVICE_FOUND" in line:
            self.parse_device_found(line)
        # Parse SCAN_COMPLETE: total=3
        elif "SCAN_COMPLETE" in line:
            self.parse_scan_complete(line)
        # Parse ERROR,...
        elif line.startswith("ERROR"):
            self.log_message(f"ERROR dari device: {line}")
        else:
            self.log_message(f"UNKNOWN: {line}")
            
    def parse_sensor_data(self, line):
        """Parse data sensor dan update grafik"""
        try:
            parts = line.split(',')
            data_dict = {}
            for i in range(1, len(parts), 2):
                if i + 1 < len(parts):
                    try:
                        key = parts[i]
                        value = float(parts[i+1])
                        data_dict[key] = value
                    except ValueError:
                        pass
            
            timestamp = datetime.datetime.now().isoformat()
            csv_row = {"timestamp": timestamp}
            csv_row.update(data_dict)
            self.csv_data.append(csv_row)
            
            current_time = len(self.time_data)
            self.time_data.append(current_time)
            
            if 'temp' in data_dict:
                self.temp_data.append(data_dict['temp'])
            if 'hum' in data_dict:
                self.hum_data.append(data_dict['hum'])
            if 'press' in data_dict:
                self.press_data.append(data_dict['press'])
            if 'lux' in data_dict:
                self.lux_data.append(data_dict['lux'])
                
            self.update_graph()
            
        except Exception as e:
            self.log_message(f"ERROR parsing sensor data: {str(e)}")
            
    def parse_device_found(self, line):
        """Parse device found message"""
        self.log_message(f"DEVICE: {line}")
        
    def parse_scan_complete(self, line):
        """Parse scan complete message"""
        self.log_message(f"SCAN: {line}")
        
    def update_graph(self):
        """Update grafik real-time"""
        self.ax.clear()
        
        if self.temp_data:
            self.ax.plot(self.time_data, self.temp_data, label='Temp (C)', color='red', linewidth=2)
        if self.hum_data:
            self.ax.plot(self.time_data, self.hum_data, label='Hum (%)', color='blue', linewidth=2)
        if self.press_data:
            self.ax.plot(self.time_data, self.press_data, label='Press (hPa)', color='green', linewidth=2)
        if self.lux_data:
            self.ax.plot(self.time_data, self.lux_data, label='Lux', color='orange', linewidth=2)
            
        self.ax.set_xlabel("Waktu (sample)")
        self.ax.set_ylabel("Nilai")
        self.ax.legend(loc='upper left', fontsize=8)
        self.ax.grid(True, alpha=0.3)
        
        self.canvas.draw()
        
    def scan_i2c_devices(self):
        """Simulasi scan I2C devices"""
        if self.is_connected and self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.write(b"SCAN\n")
                self.log_message("INFO: Mengirim perintah SCAN ke device")
            except Exception as e:
                self.log_message(f"ERROR: Gagal mengirim SCAN - {str(e)}")
        else:
            self.log_message("SCAN: Memulai pemindaian I2C...")
            self.log_message("DEVICE_FOUND: addr=0x76 name=BME280")
            self.log_message("DEVICE_FOUND: addr=0x23 name=BH1750")
            self.log_message("SCAN_COMPLETE: total=2")
            
    def update_define_display(self):
        """Update tampilan define variables"""
        self.define_text.delete(1.0, tk.END)
        for key, value in self.define_vars.items():
            self.define_text.insert(tk.END, f"{key} {value}\n")
            
    def simulate_sensor_data(self):
        """Simulasi data sensor"""
        temp = round(random.uniform(20.0, 35.0), 2)
        hum = round(random.uniform(40.0, 80.0), 2)
        press = round(random.uniform(1000.0, 1020.0), 2)
        lux = round(random.uniform(100.0, 1000.0), 2)
        
        line = f"DATA,temp,{temp},hum,{hum},press,{press},lux,{lux}"
        self.process_data(line)
        self.log_message(f"SIMULASI: {line}")
        
    def simulate_device_found(self):
        """Simulasi device found"""
        devices = [
            "DEVICE_FOUND: addr=0x76 name=BME280",
            "DEVICE_FOUND: addr=0x23 name=BH1750",
            "DEVICE_FOUND: addr=0x40 name=SI7021"
        ]
        line = random.choice(devices)
        self.process_data(line)
        
    def simulate_scan_complete(self):
        """Simulasi scan complete"""
        line = "SCAN_COMPLETE: total=2"
        self.process_data(line)
        
    def simulate_error(self):
        """Simulasi error"""
        errors = [
            "ERROR,Failed to initialize BME280",
            "ERROR,I2C timeout on address 0x76",
            "ERROR,CRC mismatch in sensor data"
        ]
        line = random.choice(errors)
        self.process_data(line)
        
    def log_message(self, message):
        """Log message ke display area"""
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        formatted_msg = f"[{timestamp}] {message}\n"
        
        self.data_display.insert(tk.END, formatted_msg)
        self.data_display.see(tk.END)
        
    def save_to_csv(self):
        """Simpan data ke file CSV"""
        if not self.csv_data:
            messagebox.showwarning("Peringatan", "Tidak ada data untuk disimpan!")
            return
            
        filename = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
            title="Simpan Data CSV"
        )
        
        if filename:
            try:
                with open(filename, 'w', newline='', encoding='utf-8') as f:
                    if self.csv_data:
                        fieldnames = self.csv_data[0].keys()
                        writer = csv.DictWriter(f, fieldnames=fieldnames)
                        writer.writeheader()
                        writer.writerows(self.csv_data)
                messagebox.showinfo("Sukses", f"Data disimpan ke {filename}")
                self.log_message(f"INFO: Data disimpan ke {filename}")
            except Exception as e:
                messagebox.showerror("Error", f"Gagal menyimpan file: {str(e)}")
                
    def clear_display(self):
        """Bersihkan tampilan data"""
        self.data_display.delete(1.0, tk.END)
        self.log_message("INFO: Tampilan dibersihkan")
        
    def on_closing(self):
        """Handle window closing event"""
        if self.is_connected:
            self.disconnect_serial()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = I2CMonitorUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()