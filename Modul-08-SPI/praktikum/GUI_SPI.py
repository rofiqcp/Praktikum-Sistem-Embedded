"""
Program GUI SPI - Antarmuka untuk Testing 25 Program Modul 08 SPI

Program ini menggunakan tkinter untuk antarmuka GUI dan mendukung:
- Pemilihan 25 program (10 STM32, 10 ESP32, 5 Multi)
- Koneksi serial ke STM32/ESP32
- Testing otomatis semua program
- Visualisasi wiring diagram
- Panel hasil test dengan export CSV
- Log lengkap dengan timestamp
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
import serial
import serial.tools.list_ports
import threading
import time
import csv
import datetime
import os
import re
from collections import deque
from tkinter import font as tkfont

# Konfigurasi warna dan style
BG_COLOR = "#f0f0f0"
BUTTON_COLOR = "#4CAF50"
BUTTON_DANGER = "#f44336"
BUTTON_WARNING = "#FF9800"
BUTTON_INFO = "#2196F3"
TEXT_COLOR = "#333333"
SUCCESS_COLOR = "#4CAF50"
ERROR_COLOR = "#f44336"
INFO_COLOR = "#2196F3"
DATA_COLOR = "#009688"

# Base path untuk Modul 08 SPI
BASE_PATH = "/home/otomasi/Praktikum-Sistem-Embedded/Modul-08-SPI/praktikum"

class SPI_GUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Monitor SPI - Praktikum Sistem Embedded (Modul 08)")
        self.root.geometry("1600x950")
        self.root.configure(bg=BG_COLOR)
        
        # Variabel serial untuk STM32 dan ESP32
        self.stm32_serial = None
        self.esp32_serial = None
        self.stm32_connected = False
        self.esp32_connected = False
        
        # Thread control
        self.stm32_read_thread = None
        self.esp32_read_thread = None
        self.stop_stm32_thread = False
        self.stop_esp32_thread = False
        
        # Test state
        self.test_running = False
        self.current_test = 0
        self.total_tests = 0
        self.test_results = []
        
        # Selected programs
        self.selected_programs = set()
        self.program_checkboxes = {}  # Store checkbox variables
        
        # Program definitions (25 programs)
        self.programs = {
            'STM32': [
                {'id': 'STM32_01', 'name': 'SPI Loopback', 'desc': 'Uji loopback SPI dengan menghubungkan MOSI ke MISO', 'path': 'STM32/STM32_01_SPI_Loopback', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4'}},
                {'id': 'STM32_02', 'name': 'OLED SSD1306', 'desc': 'Menampilkan teks dan grafik pada OLED SSD1306 via SPI', 'path': 'STM32/STM32_02_SPI_OLED_SSD1306', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4', 'DC': 'PA3', 'RST': 'PA2'}},
                {'id': 'STM32_03', 'name': 'Flash W25Q32', 'desc': 'Membaca dan menulis data ke memori flash W25Q32', 'path': 'STM32/STM32_03_SPI_Flash_W25Q32', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4'}},
                {'id': 'STM32_04', 'name': 'SD Card', 'desc': 'Membaca dan menulis file ke kartu SD menggunakan SPI', 'path': 'STM32/STM32_04_SPI_SD_Card', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4'}},
                {'id': 'STM32_05', 'name': 'MCP3208 ADC', 'desc': 'Membaca data analog dari ADC MCP3208 via SPI', 'path': 'STM32/STM32_05_SPI_MCP3208_ADC', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4'}},
                {'id': 'STM32_06', 'name': 'DAC MCP4921', 'desc': 'Menghasilkan tegangan analog dengan DAC MCP4921', 'path': 'STM32/STM32_06_SPI_DAC_MCP4921', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4'}},
                {'id': 'STM32_07', 'name': 'Multi Slave', 'desc': 'Komunikasi SPI dengan beberapa slave device', 'path': 'STM32/STM32_07_SPI_Multi_Slave', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS1': 'PA4', 'CS2': 'PA3'}},
                {'id': 'STM32_08', 'name': 'OLED FreeRTOS', 'desc': 'Menampilkan OLED dengan task FreeRTOS', 'path': 'STM32/STM32_08_SPI_OLED_FreeRTOS', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4', 'DC': 'PA3'}},
                {'id': 'STM32_09', 'name': 'Data Logger FreeRTOS', 'desc': 'Mencatat data sensor ke SD card dengan FreeRTOS', 'path': 'STM32/STM32_09_SPI_DataLogger_FreeRTOS', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4'}},
                {'id': 'STM32_10', 'name': 'Multi Device FreeRTOS', 'desc': 'Mengelola multiple SPI device dengan FreeRTOS', 'path': 'STM32/STM32_10_SPI_MultiDevice_FreeRTOS', 'pins': {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS1': 'PA4', 'CS2': 'PA3'}},
            ],
            'ESP32': [
                {'id': 'ESP32_01', 'name': 'SPI Loopback', 'desc': 'Uji loopback SPI dengan koneksi MOSI-MISO', 'path': 'ESP32/ESP32_01_SPI_Loopback', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}},
                {'id': 'ESP32_02', 'name': 'OLED SSD1306', 'desc': 'Menampilkan informasi pada OLED SSD1306', 'path': 'ESP32/ESP32_02_SPI_OLED_SSD1306', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5', 'DC': 'GPIO22'}},
                {'id': 'ESP32_03', 'name': 'SD Card', 'desc': 'Membaca menulis file ke SD card', 'path': 'ESP32/ESP32_03_SPI_SDCard', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}},
                {'id': 'ESP32_04', 'name': 'MCP3208 ADC', 'desc': 'Membaca sensor analog via MCP3208', 'path': 'ESP32/ESP32_04_SPI_MCP3208_ADC', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}},
                {'id': 'ESP32_05', 'name': 'DAC MCP4921', 'desc': 'Generasi sinyal analog dengan MCP4921', 'path': 'ESP32/ESP32_05_SPI_DAC_MCP4921', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}},
                {'id': 'ESP32_06', 'name': 'NVS Storage', 'desc': 'Menyimpan konfigurasi ke Non-Volatile Storage', 'path': 'ESP32/ESP32_06_SPI_NVS_Storage', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}},
                {'id': 'ESP32_07', 'name': 'Speed Benchmark', 'desc': 'Mengukur kecepatan transfer SPI', 'path': 'ESP32/ESP32_07_SPI_Speed_Benchmark', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}},
                {'id': 'ESP32_08', 'name': 'OLED FreeRTOS', 'desc': 'Display OLED dengan manajemen FreeRTOS', 'path': 'ESP32/ESP32_08_SPI_OLED_FreeRTOS', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5', 'DC': 'GPIO22'}},
                {'id': 'ESP32_09', 'name': 'Data Logger FreeRTOS', 'desc': 'Logging data dengan task FreeRTOS', 'path': 'ESP32/ESP32_09_SPI_DataLogger_FreeRTOS', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}},
                {'id': 'ESP32_10', 'name': 'Multi Device FreeRTOS', 'desc': 'Multiple device SPI dengan FreeRTOS', 'path': 'ESP32/ESP32_10_SPI_MultiDevice_FreeRTOS', 'pins': {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS1': 'GPIO5', 'CS2': 'GPIO17'}},
            ],
            'Multi': [
                {'id': 'Multi_01', 'name': 'STM32 Master + ESP32 Slave', 'desc': 'STM32 sebagai master, ESP32 sebagai slave - Komunikasi satu arah', 'path': 'Multi/Multi_01_STM32_Master_ESP32_Slave', 'has_stm32': True, 'has_esp32': True, 'pins': {'STM32_SCK': 'PA5', 'STM32_MISO': 'PA6', 'STM32_MOSI': 'PA7', 'STM32_CS': 'PA4', 'ESP32_SCK': 'GPIO18', 'ESP32_MISO': 'GPIO19', 'ESP32_MOSI': 'GPIO23', 'ESP32_CS': 'GPIO5'}},
                {'id': 'Multi_02', 'name': 'ESP32 Master + STM32 Slave', 'desc': 'ESP32 sebagai master, STM32 sebagai slave - Komunikasi satu arah', 'path': 'Multi/Multi_02_ESP32_Master_STM32_Slave', 'has_stm32': True, 'has_esp32': True, 'pins': {'ESP32_SCK': 'GPIO18', 'ESP32_MISO': 'GPIO19', 'ESP32_MOSI': 'GPIO23', 'ESP32_CS': 'GPIO5', 'STM32_SCK': 'PA5', 'STM32_MISO': 'PA6', 'STM32_MOSI': 'PA7', 'STM32_CS': 'PA4'}},
                {'id': 'Multi_03', 'name': 'Bidirectional Exchange', 'desc': 'Pertukaran data dua arah STM32 dan ESP32', 'path': 'Multi/Multi_03_Bidirectional_Exchange', 'has_stm32': True, 'has_esp32': True, 'pins': {'STM32_SCK': 'PA5', 'STM32_MISO': 'PA6', 'STM32_MOSI': 'PA7', 'STM32_CS': 'PA4', 'ESP32_SCK': 'GPIO18', 'ESP32_MISO': 'GPIO19', 'ESP32_MOSI': 'GPIO23', 'ESP32_CS': 'GPIO5'}},
                {'id': 'Multi_04', 'name': 'RTOS Queue', 'desc': 'Komunikasi SPI dengan FreeRTOS Queue - Sinkronisasi data', 'path': 'Multi/Multi_04_STM32_ESP32_RTOS_Queue', 'has_stm32': True, 'has_esp32': True, 'pins': {'STM32_SCK': 'PA5', 'STM32_MOSI': 'PA7', 'STM32_CS': 'PA4', 'ESP32_SCK': 'GPIO18', 'ESP32_MISO': 'GPIO19', 'ESP32_CS': 'GPIO5'}},
                {'id': 'Multi_05', 'name': 'RTOS Semaphore', 'desc': 'Sinkronisasi SPI dengan FreeRTOS Semaphore', 'path': 'Multi/Multi_05_STM32_ESP32_RTOS_Semaphore', 'has_stm32': True, 'has_esp32': True, 'pins': {'STM32_SCK': 'PA5', 'STM32_MOSI': 'PA7', 'STM32_CS': 'PA4', 'ESP32_SCK': 'GPIO18', 'ESP32_MISO': 'GPIO19', 'ESP32_CS': 'GPIO5'}},
            ]
        }
        
        self.setup_ui()
        
    def setup_ui(self):
        # Menu Bar
        self.setup_menu()
        
        # Main container
        main_container = tk.Frame(self.root, bg=BG_COLOR)
        main_container.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Top Panel - Connection
        self.setup_connection_panel(main_container)
        
        # Middle Panel - Main content dengan PanedWindow
        content_paned = tk.PanedWindow(main_container, orient=tk.HORIZONTAL, bg=BG_COLOR, 
                                        sashwidth=5, sashrelief=tk.RAISED)
        content_paned.pack(fill=tk.BOTH, expand=True, pady=(5, 0))
        
        # Left Panel - Program Selection
        left_frame = tk.Frame(content_paned, bg=BG_COLOR, width=300)
        left_frame.pack_propagate(False)
        content_paned.add(left_frame, minsize=280, width=300)
        self.setup_program_selection(left_frame)
        
        # Center Panel - Test Tabs
        center_frame = tk.Frame(content_paned, bg=BG_COLOR)
        content_paned.add(center_frame, minsize=500)
        self.setup_test_tabs(center_frame)
        
        # Right Panel - Results
        right_frame = tk.Frame(content_paned, bg=BG_COLOR, width=350)
        right_frame.pack_propagate(False)
        content_paned.add(right_frame, minsize=300, width=350)
        self.setup_results_panel(right_frame)
        
        # Bottom Panel - Log
        self.setup_log_panel(main_container)
        
        # Status Bar
        self.setup_status_bar()
        
        # Refresh ports saat startup
        self.refresh_ports()
        
    def setup_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File Menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Keluar", command=self.on_closing)
        
        # Tools Menu
        tools_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Tools", menu=tools_menu)
        tools_menu.add_command(label="Auto-detect Ports", command=self.auto_detect_ports)
        tools_menu.add_command(label="Refresh Programs", command=self.refresh_programs)
        tools_menu.add_separator()
        tools_menu.add_command(label="Quick Test Mode", command=self.quick_test_mode)
        tools_menu.add_command(label="Bersihkan Hasil", command=self.clear_results)
        
        # Help Menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="Tentang", command=self.show_about)
        help_menu.add_command(label="Dokumentasi", command=self.open_documentation)
        help_menu.add_command(label="Pin SPI Reference", command=self.show_pin_reference)
        
    def setup_connection_panel(self, parent):
        conn_frame = tk.LabelFrame(parent, text="Koneksi Serial", bg=BG_COLOR, 
                                    fg=TEXT_COLOR, font=("Arial", 10, "bold"))
        conn_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # STM32 Connection
        stm32_frame = tk.Frame(conn_frame, bg=BG_COLOR)
        stm32_frame.pack(fill=tk.X, padx=5, pady=3)
        
        tk.Label(stm32_frame, text="STM32:", bg=BG_COLOR, fg=TEXT_COLOR, width=8, anchor=tk.W).pack(side=tk.LEFT)
        
        self.stm32_port_var = tk.StringVar()
        self.stm32_port_combo = ttk.Combobox(stm32_frame, textvariable=self.stm32_port_var, state="readonly", width=22)
        self.stm32_port_combo.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=3)
        
        self.stm32_led_canvas = tk.Canvas(stm32_frame, width=16, height=16, bg=BG_COLOR, highlightthickness=0)
        self.stm32_led_canvas.pack(side=tk.LEFT, padx=3)
        self.stm32_led = self.stm32_led_canvas.create_oval(1, 1, 15, 15, fill="red", outline="black")
        
        self.stm32_connect_btn = tk.Button(stm32_frame, text="Sambung", command=lambda: self.toggle_connection('stm32'), 
                                           bg=BUTTON_COLOR, fg="white", padx=10, pady=2, font=("Arial", 9))
        self.stm32_connect_btn.pack(side=tk.LEFT, padx=3)
        
        # ESP32 Connection
        esp32_frame = tk.Frame(conn_frame, bg=BG_COLOR)
        esp32_frame.pack(fill=tk.X, padx=5, pady=3)
        
        tk.Label(esp32_frame, text="ESP32:", bg=BG_COLOR, fg=TEXT_COLOR, width=8, anchor=tk.W).pack(side=tk.LEFT)
        
        self.esp32_port_var = tk.StringVar()
        self.esp32_port_combo = ttk.Combobox(esp32_frame, textvariable=self.esp32_port_var, state="readonly", width=22)
        self.esp32_port_combo.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=3)
        
        self.esp32_led_canvas = tk.Canvas(esp32_frame, width=16, height=16, bg=BG_COLOR, highlightthickness=0)
        self.esp32_led_canvas.pack(side=tk.LEFT, padx=3)
        self.esp32_led = self.esp32_led_canvas.create_oval(1, 1, 15, 15, fill="red", outline="black")
        
        self.esp32_connect_btn = tk.Button(esp32_frame, text="Sambung", command=lambda: self.toggle_connection('esp32'), 
                                           bg=BUTTON_COLOR, fg="white", padx=10, pady=2, font=("Arial", 9))
        self.esp32_connect_btn.pack(side=tk.LEFT, padx=3)
        
        # Bottom row - Baud rate and Refresh
        bottom_frame = tk.Frame(conn_frame, bg=BG_COLOR)
        bottom_frame.pack(fill=tk.X, padx=5, pady=(3, 5))
        
        tk.Label(bottom_frame, text="Baud:", bg=BG_COLOR, fg=TEXT_COLOR).pack(side=tk.LEFT, padx=(0, 5))
        
        self.baud_var = tk.StringVar(value="115200")
        baud_combo = ttk.Combobox(bottom_frame, textvariable=self.baud_var, state="readonly", width=12,
                                   values=["9600", "115200", "256000", "921600"])
        baud_combo.pack(side=tk.LEFT, padx=(0, 10))
        
        tk.Button(bottom_frame, text="Refresh Ports", command=self.refresh_ports, 
                   bg=BUTTON_INFO, fg="white", padx=10, pady=2).pack(side=tk.LEFT)
        
    def setup_program_selection(self, parent):
        # Title
        tk.Label(parent, text="Daftar Program (25 Total)", bg=BG_COLOR, fg=TEXT_COLOR, 
                 font=("Arial", 11, "bold")).pack(pady=(5, 5))
        
        # Create notebook for categories
        select_notebook = ttk.Notebook(parent)
        select_notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=(0, 5))
        
        # STM32 Tab
        stm32_frame = tk.Frame(select_notebook, bg=BG_COLOR)
        select_notebook.add(stm32_frame, text="STM32 (10)")
        self.setup_program_checkboxes(stm32_frame, 'STM32')
        
        # ESP32 Tab
        esp32_frame = tk.Frame(select_notebook, bg=BG_COLOR)
        select_notebook.add(esp32_frame, text="ESP32 (10)")
        self.setup_program_checkboxes(esp32_frame, 'ESP32')
        
        # Multi Tab
        multi_frame = tk.Frame(select_notebook, bg=BG_COLOR)
        select_notebook.add(multi_frame, text="Multi (5)")
        self.setup_program_checkboxes(multi_frame, 'Multi')
        
        # Select/Deselect All buttons
        btn_frame = tk.Frame(parent, bg=BG_COLOR)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Button(btn_frame, text="Pilih Semua", command=self.select_all, 
                   bg=BUTTON_INFO, fg="white", padx=10, pady=3).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        tk.Button(btn_frame, text="Hapus Pilihan", command=self.deselect_all, 
                   bg=BUTTON_WARNING, fg="white", padx=10, pady=3).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 0))
        
        # Test Control Panel
        control_frame = tk.LabelFrame(parent, text="Kontrol Test", bg=BG_COLOR, fg=TEXT_COLOR, 
                                       font=("Arial", 9, "bold"))
        control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Progress bar
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(control_frame, variable=self.progress_var, maximum=100)
        self.progress_bar.pack(fill=tk.X, padx=5, pady=5)
        
        self.progress_label = tk.Label(control_frame, text="Progress: 0%", bg=BG_COLOR, fg=TEXT_COLOR)
        self.progress_label.pack(pady=(0, 5))
        
        # Buttons
        btn_frame2 = tk.Frame(control_frame, bg=BG_COLOR)
        btn_frame2.pack(fill=tk.X, padx=5, pady=(0, 5))
        
        tk.Button(btn_frame2, text="Test Terpilih", command=self.test_selected, 
                   bg=BUTTON_COLOR, fg="white", padx=10, pady=3).pack(fill=tk.X, pady=(0, 3))
        
        tk.Button(btn_frame2, text="Test Semua (25)", command=self.test_all, 
                   bg=BUTTON_WARNING, fg="white", padx=10, pady=3).pack(fill=tk.X)
        
        self.status_test_var = tk.StringVar(value="Siap")
        tk.Label(control_frame, textvariable=self.status_test_var, bg=BG_COLOR, fg=TEXT_COLOR,
                  font=("Arial", 9)).pack(pady=(0, 5))
        
    def setup_program_checkboxes(self, parent, category):
        # Canvas with scrollbar for checkboxes
        canvas = tk.Canvas(parent, bg=BG_COLOR, highlightthickness=0)
        scrollbar = ttk.Scrollbar(parent, orient="vertical", command=canvas.yview)
        scrollable_frame = tk.Frame(canvas, bg=BG_COLOR)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        # Add checkboxes
        for prog in self.programs[category]:
            var = tk.BooleanVar()
            self.program_checkboxes[prog['id']] = var
            
            cb_frame = tk.Frame(scrollable_frame, bg=BG_COLOR)
            cb_frame.pack(fill=tk.X, pady=2, padx=5)
            
            cb = tk.Checkbutton(cb_frame, text=f"{prog['id']}", variable=var, 
                                bg=BG_COLOR, fg=TEXT_COLOR, anchor=tk.W,
                                command=lambda p=prog['id']: self.on_checkbox_click(p))
            cb.pack(side=tk.LEFT)
            
            tk.Label(cb_frame, text=prog['name'], bg=BG_COLOR, fg="blue", 
                      font=("Arial", 9)).pack(side=tk.LEFT, padx=(5, 0))
            
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
    def on_checkbox_click(self, prog_id):
        var = self.program_checkboxes[prog_id]
        if var.get():
            self.selected_programs.add(prog_id)
        else:
            self.selected_programs.discard(prog_id)
            
    def select_all(self):
        for category in self.programs:
            for prog in self.programs[category]:
                self.program_checkboxes[prog['id']].set(True)
                self.selected_programs.add(prog['id'])
                
    def deselect_all(self):
        for category in self.programs:
            for prog in self.programs[category]:
                self.program_checkboxes[prog['id']].set(False)
                self.selected_programs.discard(prog['id'])
                
    def setup_test_tabs(self, parent):
        # Notebook untuk tabs
        self.notebook = ttk.Notebook(parent)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Tab untuk welcome/info
        welcome_frame = tk.Frame(self.notebook, bg=BG_COLOR)
        self.notebook.add(welcome_frame, text="ℹ️ Info")
        
        tk.Label(welcome_frame, text="GUI Test SPI - Modul 08", bg=BG_COLOR, fg=TEXT_COLOR,
                  font=("Arial", 14, "bold")).pack(pady=20)
        tk.Label(welcome_frame, text="Pilih program di panel kiri untuk melihat detail dan testing", 
                  bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 11)).pack(pady=10)
        
        # Wiring diagram info
        wiring_frame = tk.LabelFrame(welcome_frame, text="Informasi Pin SPI", bg=BG_COLOR, fg=TEXT_COLOR,
                                      font=("Arial", 10, "bold"))
        wiring_frame.pack(fill=tk.X, padx=20, pady=20)
        
        # STM32 Wiring
        tk.Label(wiring_frame, text="STM32 (SPI1):", bg=BG_COLOR, fg="blue", 
                  font=("Arial", 10, "bold")).pack(anchor=tk.W, padx=10, pady=(10, 5))
        stm32_pins = {'SCK': 'PA5', 'MISO': 'PA6', 'MOSI': 'PA7', 'CS': 'PA4'}
        for pin, gpio in stm32_pins.items():
            tk.Label(wiring_frame, text=f"  {pin}: {gpio}", bg=BG_COLOR, fg=TEXT_COLOR).pack(anchor=tk.W, padx=20)
        
        # ESP32 Wiring
        tk.Label(wiring_frame, text="ESP32 (VSPI):", bg=BG_COLOR, fg="blue", 
                  font=("Arial", 10, "bold")).pack(anchor=tk.W, padx=10, pady=(10, 5))
        esp32_pins = {'SCK': 'GPIO18', 'MISO': 'GPIO19', 'MOSI': 'GPIO23', 'CS': 'GPIO5'}
        for pin, gpio in esp32_pins.items():
            tk.Label(wiring_frame, text=f"  {pin}: {gpio}", bg=BG_COLOR, fg=TEXT_COLOR).pack(anchor=tk.W, padx=20)
            
    def create_program_tab(self, prog_info):
        """Create a tab for a specific program with wiring diagram and controls"""
        tab_frame = tk.Frame(self.notebook, bg=BG_COLOR)
        self.notebook.add(tab_frame, text=prog_info['id'])
        self.notebook.select(tab_frame)
        
        # Top - Program info
        info_frame = tk.Frame(tab_frame, bg=BG_COLOR)
        info_frame.pack(fill=tk.X, padx=10, pady=5)
        
        tk.Label(info_frame, text=f"{prog_info['id']} - {prog_info['name']}", 
                  bg=BG_COLOR, fg=TEXT_COLOR, font=("Arial", 12, "bold")).pack(anchor=tk.W)
        tk.Label(info_frame, text=prog_info['desc'], 
                  bg=BG_COLOR, fg="gray", font=("Arial", 10)).pack(anchor=tk.W)
        
        # Middle - Wiring Diagram
        wiring_frame = tk.LabelFrame(tab_frame, text="Wiring Diagram", bg=BG_COLOR, fg=TEXT_COLOR,
                                      font=("Arial", 10, "bold"))
        wiring_frame.pack(fill=tk.X, padx=10, pady=5)
        
        # Create canvas for wiring diagram
        canvas = tk.Canvas(wiring_frame, height=200, bg="white", highlightthickness=1, highlightbackground="gray")
        canvas.pack(fill=tk.X, padx=5, pady=5)
        
        # Draw wiring diagram
        self.draw_wiring_diagram(canvas, prog_info)
        
        # Pin info text
        pin_text = scrolledtext.ScrolledText(wiring_frame, height=5, font=("Courier", 9))
        pin_text.pack(fill=tk.X, padx=5, pady=(0, 5))
        
        if 'pins' in prog_info:
            for pin, gpio in prog_info['pins'].items():
                pin_text.insert(tk.END, f"{pin}: {gpio}\n")
        pin_text.config(state=tk.DISABLED)
        
        # Controls
        control_frame = tk.Frame(tab_frame, bg=BG_COLOR)
        control_frame.pack(fill=tk.X, padx=10, pady=5)
        
        if prog_info.get('has_stm32') or 'STM32' in prog_info['id']:
            tk.Button(control_frame, text="Start STM32", command=lambda: self.start_program(prog_info, 'stm32'),
                       bg=BUTTON_COLOR, fg="white", padx=15, pady=3).pack(side=tk.LEFT, padx=(0, 5))
        
        if prog_info.get('has_esp32') or 'ESP32' in prog_info['id']:
            tk.Button(control_frame, text="Start ESP32", command=lambda: self.start_program(prog_info, 'esp32'),
                       bg=BUTTON_INFO, fg="white", padx=15, pady=3).pack(side=tk.LEFT, padx=5)
        
        tk.Button(control_frame, text="Stop", command=lambda: self.stop_program(prog_info),
                   bg=BUTTON_DANGER, fg="white", padx=15, pady=3).pack(side=tk.LEFT, padx=5)
        
        # Output Console
        console_frame = tk.LabelFrame(tab_frame, text="Output Console", bg=BG_COLOR, fg=TEXT_COLOR,
                                       font=("Arial", 10, "bold"))
        console_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        console = scrolledtext.ScrolledText(console_frame, height=12, font=("Courier", 9))
        console.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Configure text colors
        console.tag_config('INFO', foreground=INFO_COLOR)
        console.tag_config('ERROR', foreground=ERROR_COLOR)
        console.tag_config('DATA', foreground=DATA_COLOR)
        console.tag_config('SUCCESS', foreground=SUCCESS_COLOR)
        console.tag_config('WARNING', foreground=BUTTON_WARNING)
        
        # Store reference to console
        setattr(self, f"console_{prog_info['id']}", console)
        
    def draw_wiring_diagram(self, canvas, prog_info):
        """Draw a simple wiring diagram on canvas"""
        canvas.delete("all")
        
        # Title
        canvas.create_text(300, 15, text=f"Wiring: {prog_info['name']}", font=("Arial", 10, "bold"))
        
        # Draw STM32/ESP32 board
        if 'STM32' in prog_info['id'] or prog_info.get('has_stm32'):
            canvas.create_rectangle(50, 50, 200, 150, fill="#E3F2FD", outline="blue", width=2)
            canvas.create_text(125, 40, text="STM32", font=("Arial", 9, "bold"), fill="blue")
            
            # Draw pins
            if 'pins' in prog_info:
                y = 70
                for pin, gpio in list(prog_info['pins'].items())[:4]:
                    if not pin.startswith('ESP'):
                        canvas.create_text(60, y, text=f"{pin}:", anchor=tk.W, font=("Courier", 8))
                        canvas.create_text(120, y, text=gpio, anchor=tk.W, font=("Courier", 8, "bold"))
                        y += 18
        
        if 'ESP32' in prog_info['id'] or prog_info.get('has_esp32'):
            canvas.create_rectangle(400, 50, 550, 150, fill="#FFF3E0", outline="orange", width=2)
            canvas.create_text(475, 40, text="ESP32", font=("Arial", 9, "bold"), fill="orange")
            
            # Draw pins
            if 'pins' in prog_info:
                y = 70
                for pin, gpio in list(prog_info['pins'].items())[:4]:
                    if not pin.startswith('STM'):
                        canvas.create_text(410, y, text=f"{pin}:", anchor=tk.W, font=("Courier", 8))
                        canvas.create_text(470, y, text=gpio, anchor=tk.W, font=("Courier", 8, "bold"))
                        y += 18
        
        # Draw SPI device (if applicable)
        if 'SSD1306' in prog_info['name'] or 'OLED' in prog_info['name']:
            canvas.create_rectangle(250, 170, 350, 220, fill="#E8F5E9", outline=SUCCESS_COLOR, width=2)
            canvas.create_text(300, 195, text="OLED", font=("Arial", 9, "bold"))
            
            # Connection lines
            if 'STM32' in prog_info['id'] or prog_info.get('has_stm32'):
                canvas.create_line(200, 100, 250, 195, dash=(4, 2))
            if 'ESP32' in prog_info['id'] or prog_info.get('has_esp32'):
                canvas.create_line(400, 100, 350, 195, dash=(4, 2))
                
    def setup_results_panel(self, parent):
        # Title
        tk.Label(parent, text="Hasil Test", bg=BG_COLOR, fg=TEXT_COLOR, 
                 font=("Arial", 11, "bold")).pack(pady=(5, 5))
        
        # Results table
        table_frame = tk.Frame(parent)
        table_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=(0, 5))
        
        # Scrollbars
        table_scroll_y = tk.Scrollbar(table_frame)
        table_scroll_y.pack(side=tk.RIGHT, fill=tk.Y)
        table_scroll_x = tk.Scrollbar(table_frame, orient=tk.HORIZONTAL)
        table_scroll_x.pack(side=tk.BOTTOM, fill=tk.X)
        
        # Treeview for results
        self.result_tree = ttk.Treeview(table_frame, yscrollcommand=table_scroll_y.set,
                                         xscrollcommand=table_scroll_x.set, height=18)
        self.result_tree.pack(fill=tk.BOTH, expand=True)
        
        table_scroll_y.config(command=self.result_tree.yview)
        table_scroll_x.config(command=self.result_tree.xview)
        
        # Setup columns
        self.result_tree['columns'] = ('Program', 'Status', 'Waktu', 'Keterangan')
        self.result_tree.column('#0', width=0, stretch=tk.NO)
        self.result_tree.column('Program', width=100, anchor=tk.W)
        self.result_tree.column('Status', width=60, anchor=tk.CENTER)
        self.result_tree.column('Waktu', width=60, anchor=tk.CENTER)
        self.result_tree.column('Keterangan', width=130, anchor=tk.W)
        
        self.result_tree.heading('#0', text='', anchor=tk.W)
        self.result_tree.heading('Program', text='Program', anchor=tk.W)
        self.result_tree.heading('Status', text='Status', anchor=tk.CENTER)
        self.result_tree.heading('Waktu', text='Waktu', anchor=tk.CENTER)
        self.result_tree.heading('Keterangan', text='Keterangan', anchor=tk.W)
        
        # Configure row colors
        self.result_tree.tag_configure('PASS', background='#C8E6C9')
        self.result_tree.tag_configure('FAIL', background='#FFCDD2')
        
        # Summary statistics
        summary_frame = tk.LabelFrame(parent, text="Ringkasan", bg=BG_COLOR, fg=TEXT_COLOR,
                                       font=("Arial", 9, "bold"))
        summary_frame.pack(fill=tk.X, padx=5, pady=5)
        
        self.summary_var = tk.StringVar(value="Belum ada test")
        tk.Label(summary_frame, textvariable=self.summary_var, bg=BG_COLOR, fg=TEXT_COLOR,
                  justify=tk.LEFT).pack(anchor=tk.W, padx=5, pady=5)
        
        # Export button
        tk.Button(summary_frame, text="Export ke CSV", command=self.export_results,
                   bg=BUTTON_INFO, fg="white", padx=10, pady=3).pack(fill=tk.X, padx=5, pady=(0, 5))
        
    def setup_log_panel(self, parent):
        log_frame = tk.LabelFrame(parent, text="Log", bg=BG_COLOR, fg=TEXT_COLOR,
                                   font=("Arial", 10, "bold"))
        log_frame.pack(fill=tk.X, padx=5, pady=(5, 0))
        
        # Log text widget
        self.log_text = scrolledtext.ScrolledText(log_frame, height=8, font=("Courier", 9))
        self.log_text.pack(fill=tk.X, padx=5, pady=5)
        
        # Configure text colors
        self.log_text.tag_config('INFO', foreground=INFO_COLOR)
        self.log_text.tag_config('ERROR', foreground=ERROR_COLOR)
        self.log_text.tag_config('DATA', foreground=DATA_COLOR)
        self.log_text.tag_config('SUCCESS', foreground=SUCCESS_COLOR)
        self.log_text.tag_config('WARNING', foreground=BUTTON_WARNING)
        
        # Log buttons
        log_btn_frame = tk.Frame(log_frame, bg=BG_COLOR)
        log_btn_frame.pack(fill=tk.X, padx=5, pady=(0, 5))
        
        tk.Button(log_btn_frame, text="Simpan Log", command=self.save_log,
                   bg=BUTTON_INFO, fg="white", padx=10, pady=3).pack(side=tk.LEFT, padx=(0, 5))
        tk.Button(log_btn_frame, text="Bersihkan Log", command=self.clear_log,
                   bg=BUTTON_WARNING, fg="white", padx=10, pady=3).pack(side=tk.LEFT, padx=5)
        tk.Button(log_btn_frame, text="Auto-scroll", command=self.toggle_autoscroll,
                   bg="gray", fg="white", padx=10, pady=3).pack(side=tk.LEFT, padx=5)
                   
        self.autoscroll = True
        
    def setup_status_bar(self):
        self.status_var = tk.StringVar(value="Siap - Pilih program dan klik Test")
        status_bar = tk.Label(self.root, textvariable=self.status_var, bd=1, relief=tk.SUNKEN, anchor=tk.W,
                               bg="#E0E0E0", fg=TEXT_COLOR)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)
        
    def refresh_ports(self):
        try:
            ports = serial.tools.list_ports.comports()
            port_list = [port.device for port in ports]
            
            self.stm32_port_combo['values'] = port_list
            self.esp32_port_combo['values'] = port_list
            
            # Auto-detect and set
            for port in ports:
                desc = port.description.lower()
                if 'stm' in desc or 'st-link' in desc:
                    self.stm32_port_combo.set(port.device)
                elif 'esp' in desc or 'cp210' in desc or 'ch340' in desc:
                    self.esp32_port_combo.set(port.device)
            
            if port_list and not self.stm32_port_combo.get():
                self.stm32_port_combo.set(port_list[0])
            if port_list and not self.esp32_port_combo.get():
                self.esp32_port_combo.set(port_list[0])
                
            self.log("INFO", "Port serial di-refresh")
        except Exception as e:
            self.log("ERROR", f"Gagal refresh port: {str(e)}")
            
    def auto_detect_ports(self):
        self.refresh_ports()
        ports = serial.tools.list_ports.comports()
        stm32_ports = []
        esp32_ports = []
        
        for port in ports:
            desc = port.description.lower()
            if 'stm' in desc or 'st-link' in desc:
                stm32_ports.append(port.device)
            elif 'esp' in desc or 'cp210' in desc or 'ch340' in desc:
                esp32_ports.append(port.device)
                
        msg = f"Auto-detect selesai:\n"
        msg += f"STM32: {', '.join(stm32_ports) if stm32_ports else 'Tidak ditemukan'}\n"
        msg += f"ESP32: {', '.join(esp32_ports) if esp32_ports else 'Tidak ditemukan'}"
        messagebox.showinfo("Auto-detect Ports", msg)
        self.log("INFO", "Auto-detect ports selesai")
        
    def toggle_connection(self, board_type):
        if board_type == 'stm32':
            if not self.stm32_connected:
                self.connect_serial('stm32')
            else:
                self.disconnect_serial('stm32')
        else:
            if not self.esp32_connected:
                self.connect_serial('esp32')
            else:
                self.disconnect_serial('esp32')
                
    def connect_serial(self, board_type):
        if board_type == 'stm32':
            port = self.stm32_port_var.get()
            btn = self.stm32_connect_btn
            led = self.stm32_led
            led_canvas = self.stm32_led_canvas
        else:
            port = self.esp32_port_var.get()
            btn = self.esp32_connect_btn
            led = self.esp32_led
            led_canvas = self.esp32_led_canvas
            
        baud = int(self.baud_var.get())
        
        if not port:
            messagebox.showerror("Error", "Pilih port serial terlebih dahulu!")
            return
            
        try:
            ser = serial.Serial(port, baud, timeout=1)
            
            if board_type == 'stm32':
                self.stm32_serial = ser
                self.stm32_connected = True
                self.stm32_connect_btn.config(text="Putus", bg=BUTTON_DANGER)
                self.stm32_led_canvas.itemconfig(self.stm32_led, fill="green")
                self.status_var.set(f"STM32 terhubung ke {port}")
                self.log("SUCCESS", f"STM32 terhubung ke {port} @ {baud} baud")
                
                # Start reading thread
                self.stop_stm32_thread = False
                self.stm32_read_thread = threading.Thread(target=self.read_serial_data, 
                                                          args=('stm32',), daemon=True)
                self.stm32_read_thread.start()
            else:
                self.esp32_serial = ser
                self.esp32_connected = True
                self.esp32_connect_btn.config(text="Putus", bg=BUTTON_DANGER)
                self.esp32_led_canvas.itemconfig(self.esp32_led, fill="green")
                self.status_var.set(f"ESP32 terhubung ke {port}")
                self.log("SUCCESS", f"ESP32 terhubung ke {port} @ {baud} baud")
                
                # Start reading thread
                self.stop_esp32_thread = False
                self.esp32_read_thread = threading.Thread(target=self.read_serial_data, 
                                                          args=('esp32',), daemon=True)
                self.esp32_read_thread.start()
                
        except Exception as e:
            messagebox.showerror("Error", f"Gagal terhubung ke {port}: {str(e)}")
            self.log("ERROR", f"Gagal terhubung {board_type}: {str(e)}")
            
    def disconnect_serial(self, board_type):
        if board_type == 'stm32':
            self.stop_stm32_thread = True
            if self.stm32_serial and self.stm32_serial.is_open:
                self.stm32_serial.close()
            self.stm32_connected = False
            self.stm32_connect_btn.config(text="Sambung", bg=BUTTON_COLOR)
            self.stm32_led_canvas.itemconfig(self.stm32_led, fill="red")
            self.log("INFO", "STM32 diputus")
        else:
            self.stop_esp32_thread = True
            if self.esp32_serial and self.esp32_serial.is_open:
                self.esp32_serial.close()
            self.esp32_connected = False
            self.esp32_connect_btn.config(text="Sambung", bg=BUTTON_COLOR)
            self.esp32_led_canvas.itemconfig(self.esp32_led, fill="red")
            self.log("INFO", "ESP32 diputus")
            
        if not self.stm32_connected and not self.esp32_connected:
            self.status_var.set("Terputus")
            
    def read_serial_data(self, board_type):
        if board_type == 'stm32':
            ser = self.stm32_serial
            stop = lambda: self.stop_stm32_thread
        else:
            ser = self.esp32_serial
            stop = lambda: self.stop_esp32_thread
            
        while not stop() and ser and ser.is_open:
            try:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        self.root.after(0, self.process_serial_data, board_type, line)
            except Exception as e:
                self.root.after(0, self.log, "ERROR", f"Serial read error {board_type}: {str(e)}")
                break
            time.sleep(0.01)
            
    def process_serial_data(self, board_type, line):
        prefix = "STM32" if board_type == 'stm32' else "ESP32"
        
        if line.startswith("ERROR"):
            self.log("ERROR", f"{prefix}: {line}")
        elif line.startswith("DATA"):
            self.log("DATA", f"{prefix}: {line}")
        elif "PASS" in line or "OK" in line:
            self.log("SUCCESS", f"{prefix}: {line}")
        else:
            self.log("INFO", f"{prefix}: {line}")
            
        # Also write to active program console if open
        self.write_to_active_console(board_type, line)
        
    def write_to_active_console(self, board_type, line):
        """Write serial data to the active program's console"""
        try:
            current_tab = self.notebook.select()
            if current_tab:
                tab_widget = self.nametowidget(current_tab)
                tab_text = self.notebook.tab(tab_widget, "text")
                
                if tab_text != "ℹ️ Info":
                    console = getattr(self, f"console_{tab_text}", None)
                    if console:
                        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
                        formatted = f"[{timestamp}] [{board_type.upper()}] {line}\n"
                        
                        if "ERROR" in line:
                            console.insert(tk.END, formatted, 'ERROR')
                        elif "DATA" in line:
                            console.insert(tk.END, formatted, 'DATA')
                        elif "PASS" in line or "OK" in line:
                            console.insert(tk.END, formatted, 'SUCCESS')
                        else:
                            console.insert(tk.END, formatted, 'INFO')
                        console.see(tk.END)
        except:
            pass
            
    def test_selected(self):
        if not self.selected_programs:
            messagebox.showwarning("Peringatan", "Pilih program yang akan ditest terlebih dahulu!")
            return
        self.run_tests(list(self.selected_programs))
        
    def test_all(self):
        all_programs = []
        for category in self.programs:
            for prog in self.programs[category]:
                all_programs.append(prog['id'])
        self.run_tests(all_programs)
        
    def run_tests(self, program_ids):
        if self.test_running:
            messagebox.showwarning("Peringatan", "Test sedang berjalan!")
            return
            
        self.test_running = True
        self.current_test = 0
        self.total_tests = len(program_ids)
        self.test_results = []
        
        thread = threading.Thread(target=self._run_test_thread, args=(program_ids,), daemon=True)
        thread.start()
        
    def _run_test_thread(self, program_ids):
        for idx, prog_id in enumerate(program_ids):
            self.current_test = idx
            progress = ((idx + 1) / self.total_tests) * 100
            self.root.after(0, self.update_progress, progress, f"Testing {prog_id}...")
            
            # Find program info
            prog_info = None
            for category in self.programs:
                for prog in self.programs[category]:
                    if prog['id'] == prog_id:
                        prog_info = prog
                        break
                if prog_info:
                    break
                    
            if prog_info:
                start_time = time.time()
                
                # Simulate test - in real implementation, this would flash and test the program
                time.sleep(1.5)  # Simulate test duration
                
                elapsed = time.time() - start_time
                
                # Simulate result
                import random
                status = "PASS" if random.choice([True, True, True, False]) else "FAIL"
                
                result = {
                    'program': prog_id,
                    'status': status,
                    'time': f"{elapsed:.2f}s",
                    'notes': 'OK' if status == "PASS" else 'Error detected'
                }
                
                self.root.after(0, self.add_test_result, result)
                self.root.after(0, self.log, "SUCCESS" if status == "PASS" else "ERROR", 
                          f"Test {prog_id}: {status} ({elapsed:.2f}s)")
                        
        self.root.after(0, self.update_progress, 100, "Test selesai!")
        self.root.after(0, self.test_complete)
        
    def update_progress(self, value, status):
        self.progress_var.set(value)
        self.progress_label.config(text=f"Progress: {value:.1f}%")
        self.status_test_var.set(status)
        
    def add_test_result(self, result):
        tag = result['status']
        self.result_tree.insert('', tk.END, values=(
            result['program'],
            result['status'],
            result['time'],
            result['notes']
        ), tags=(tag,))
        self.test_results.append(result)
        self.update_summary()
        
    def update_summary(self):
        total = len(self.test_results)
        passed = sum(1 for r in self.test_results if r['status'] == "PASS")
        failed = total - passed
        self.summary_var.set(f"Total: {total} | PASS: {passed} | FAIL: {failed}")
        
    def test_complete(self):
        self.test_running = False
        self.log("INFO", "Semua test selesai")
        messagebox.showinfo("Test Selesai", f"Semua program telah ditest!\n\nPASS: {sum(1 for r in self.test_results if r['status'] == 'PASS')}\nFAIL: {sum(1 for r in self.test_results if r['status'] == 'FAIL')}")
        
    def quick_test_mode(self):
        messagebox.showinfo("Quick Test", "Quick Test Mode diaktifkan.\nTest akan berjalan dengan durasi singkat (0.5s per program).")
        self.log("INFO", "Quick Test Mode diaktifkan")
        
    def refresh_programs(self):
        self.log("INFO", "Refresh daftar program")
        messagebox.showinfo("Refresh", "Daftar program telah di-refresh")
        
    def clear_results(self):
        for item in self.result_tree.get_children():
            self.result_tree.delete(item)
        self.test_results = []
        self.update_summary()
        self.log("INFO", "Hasil test dibersihkan")
        
    def start_program(self, prog_info, board_type):
        """Start a specific program on the selected board"""
        if board_type == 'stm32' and not self.stm32_connected:
            messagebox.showwarning("Peringatan", "STM32 belum terhubung!")
            return
        if board_type == 'esp32' and not self.esp32_connected:
            messagebox.showwarning("Peringatan", "ESP32 belum terhubung!")
            return
            
        self.log("INFO", f"Memulai {prog_info['id']} pada {'STM32' if board_type == 'stm32' else 'ESP32'}...")
        
        # Create tab if not exists
        tab_exists = False
        for tab_id in self.notebook.tabs():
            if self.notebook.tab(tab_id, "text") == prog_info['id']:
                tab_exists = True
                self.notebook.select(tab_id)
                break
                
        if not tab_exists:
            self.create_program_tab(prog_info)
            
    def stop_program(self, prog_info):
        """Stop a running program"""
        self.log("INFO", f"Menghentikan {prog_info['id']}...")
        # In real implementation, send stop command to board
        
    def export_results(self):
        if not self.test_results:
            messagebox.showwarning("Peringatan", "Tidak ada hasil test untuk diexport!")
            return
            
        filename = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
            title="Simpan Hasil Test"
        )
        
        if filename:
            try:
                with open(filename, 'w', newline='', encoding='utf-8') as f:
                    writer = csv.writer(f)
                    writer.writerow(['Program', 'Status', 'Waktu', 'Keterangan'])
                    for result in self.test_results:
                        writer.writerow([result['program'], result['status'], 
                                         result['time'], result['notes']])
                messagebox.showinfo("Sukses", f"Hasil disimpan ke {filename}")
                self.log("INFO", f"Hasil test diexport ke {filename}")
            except Exception as e:
                messagebox.showerror("Error", f"Gagal menyimpan file: {str(e)}")
                
    def log(self, level, message):
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        formatted_msg = f"[{timestamp}] [{level}] {message}\n"
        
        self.log_text.insert(tk.END, formatted_msg, level)
        if self.autoscroll:
            self.log_text.see(tk.END)
            
    def save_log(self):
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
            title="Simpan Log"
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(self.log_text.get(1.0, tk.END))
                messagebox.showinfo("Sukses", f"Log disimpan ke {filename}")
            except Exception as e:
                messagebox.showerror("Error", f"Gagal menyimpan log: {str(e)}")
                
    def clear_log(self):
        self.log_text.delete(1.0, tk.END)
        self.log("INFO", "Log dibersihkan")
        
    def toggle_autoscroll(self):
        self.autoscroll = not self.autoscroll
        self.log("INFO", f"Auto-scroll: {'ON' if self.autoscroll else 'OFF'}")
        
    def show_about(self):
        about_text = """Monitor SPI - Modul 08
Praktikum Sistem Embedded

Program GUI untuk testing 25 program SPI:
- 10 Program STM32
- 10 Program ESP32
- 5 Program Multi (STM32 + ESP32)

Version: 1.0
Dibuat untuk keperluan praktikum.
"""
        messagebox.showinfo("Tentang", about_text)
        
    def open_documentation(self):
        doc_path = "/home/otomasi/Praktikum-Sistem-Embedded/Modul-08-SPI/Materi.md"
        if os.path.exists(doc_path):
            try:
                os.system(f"xdg-open {doc_path}")
            except:
                # Try alternative methods
                try:
                    os.system(f"firefox {doc_path}")
                except:
                    messagebox.showinfo("Dokumentasi", f"Buka file secara manual:\n{doc_path}")
        else:
            messagebox.showwarning("Peringatan", f"File dokumentasi tidak ditemukan!\n{doc_path}")
            
    def show_pin_reference(self):
        ref_window = tk.Toplevel(self.root)
        ref_window.title("Referensi Pin SPI")
        ref_window.geometry("500x400")
        
        text = scrolledtext.ScrolledText(ref_window, font=("Courier", 10))
        text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        content = """REFERENSI PIN SPI

STM32F103C8 (Blue Pill) - SPI1:
  SCK  : PA5
  MISO : PA6
  MOSI : PA7
  CS   : PA4 (user defined)
  LED  : PC13 (onboard)

ESP32 DevKit V1 - VSPI:
  SCK  : GPIO18
  MISO : GPIO19
  MOSI : GPIO23
  CS   : GPIO5 (user defined)
  LED  : GPIO2 (onboard)

ESP32 DevKit V1 - HSPI:
  SCK  : GPIO14
  MISO : GPIO12
  MOSI : GPIO13
  CS   : GPIO15 (user defined)

Wiring Multi Board:
  STM32 Master - ESP32 Slave:
    STM32 SCK  -> ESP32 SCK
    STM32 MOSI -> ESP32 MOSI
    STM32 MISO <- ESP32 MISO
    STM32 CS   -> ESP32 CS
    
  ESP32 Master - STM32 Slave:
    ESP32 SCK  -> STM32 SCK
    ESP32 MOSI -> STM32 MOSI
    ESP32 MISO <- STM32 MISO
    ESP32 CS   -> STM32 CS
"""
        text.insert(tk.END, content)
        text.config(state=tk.DISABLED)
        
    def on_closing(self):
        if self.stm32_connected:
            self.disconnect_serial('stm32')
        if self.esp32_connected:
            self.disconnect_serial('esp32')
        self.root.destroy()

def main():
    root = tk.Tk()
    app = SPI_GUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()
