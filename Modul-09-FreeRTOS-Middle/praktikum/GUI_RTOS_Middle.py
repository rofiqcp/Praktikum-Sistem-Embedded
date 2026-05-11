#!/usr/bin/env python3
"""
GUI_RTOS_Middle.py - Antarmuka Pengguna Tkinter untuk Pengujian Program Modul 09 FreeRTOS Middle
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
import serial
import serial.tools.list_ports
import threading
import time
import json
import csv
from datetime import datetime
from collections import deque
import sys
import os
import math

# Definisi program dan peripheral yang digunakan
PROGRAMS = {
    "STM32": [
        {"name": "STM32_01_GPIO_RTOS_Task", "peripherals": ["GPIO"], "desc": "Blink LED dengan RTOS Task"},
        {"name": "STM32_02_External_Interrupt_RTOS", "peripherals": ["Interrupt"], "desc": "External Interrupt dengan RTOS"},
        {"name": "STM32_03_Encoder_2Pin_Interrupt", "peripherals": ["Encoder", "Interrupt"], "desc": "Encoder 2 Pin dengan Interrupt"},
        {"name": "STM32_04_Serial_RTOS_Queue", "peripherals": ["Serial"], "desc": "Komunikasi Serial dengan Queue RTOS"},
        {"name": "STM32_05_DAC_RTOS_Output", "peripherals": ["DAC"], "desc": "Output DAC dengan RTOS"},
        {"name": "STM32_06_ADC_RTOS_Input", "peripherals": ["ADC"], "desc": "Input ADC dengan RTOS"},
        {"name": "STM32_07_I2C_RTOS_Sensor", "peripherals": ["I2C"], "desc": "Sensor I2C dengan RTOS"},
        {"name": "STM32_08_SPI_RTOS_OLED", "peripherals": ["SPI"], "desc": "OLED SPI dengan RTOS"},
        {"name": "STM32_09_GPIO_Interrupt_Combined", "peripherals": ["GPIO", "Interrupt"], "desc": "GPIO dan Interrupt Combined"},
        {"name": "STM32_10_Multi_Peripheral_RTOS", "peripherals": ["GPIO", "ADC", "Serial"], "desc": "Multi Peripheral RTOS"},
    ],
    "ESP32": [
        {"name": "ESP32_01_GPIO_RTOS_Task", "peripherals": ["GPIO"], "desc": "Blink LED dengan RTOS Task"},
        {"name": "ESP32_02_External_Interrupt_RTOS", "peripherals": ["Interrupt"], "desc": "External Interrupt dengan RTOS"},
        {"name": "ESP32_03_Encoder_2Pin_Interrupt", "peripherals": ["Encoder", "Interrupt"], "desc": "Encoder 2 Pin dengan Interrupt"},
        {"name": "ESP32_04_Serial_RTOS_Queue", "peripherals": ["Serial"], "desc": "Komunikasi Serial dengan Queue RTOS"},
        {"name": "ESP32_05_DAC_RTOS_Output", "peripherals": ["DAC"], "desc": "Output DAC dengan RTOS"},
        {"name": "ESP32_06_ADC_RTOS_Input", "peripherals": ["ADC"], "desc": "Input ADC dengan RTOS"},
        {"name": "ESP32_07_I2C_RTOS_Sensor", "peripherals": ["I2C"], "desc": "Sensor I2C dengan RTOS"},
        {"name": "ESP32_08_SPI_RTOS_OLED", "peripherals": ["SPI"], "desc": "OLED SPI dengan RTOS"},
        {"name": "ESP32_09_GPIO_Interrupt_Combined", "peripherals": ["GPIO", "Interrupt"], "desc": "GPIO dan Interrupt Combined"},
        {"name": "ESP32_10_Multi_Peripheral_RTOS", "peripherals": ["GPIO", "ADC", "Serial"], "desc": "Multi Peripheral RTOS"},
    ],
    "Multi": [
        {"name": "Multi_01", "peripherals": ["GPIO", "Serial"], "desc": "Komunikasi STM32-ESP32 GPIO"},
        {"name": "Multi_02", "peripherals": ["ADC", "Serial"], "desc": "Komunikasi STM32-ESP32 ADC"},
        {"name": "Multi_03", "peripherals": ["I2C", "Serial"], "desc": "Komunikasi STM32-ESP32 I2C"},
        {"name": "Multi_04", "peripherals": ["SPI", "Serial"], "desc": "Komunikasi STM32-ESP32 SPI"},
        {"name": "Multi_05", "peripherals": ["Encoder", "Serial"], "desc": "Komunikasi STM32-ESP32 Encoder"},
    ]
}

PERIPHERAL_COLORS = {
    "GPIO": "#4CAF50",
    "Interrupt": "#F44336",
    "Encoder": "#2196F3",
    "Serial": "#FF9800",
    "DAC": "#9C27B0",
    "ADC": "#00BCD4",
    "I2C": "#795548",
    "SPI": "#607D8B"
}

TASK_STATE_COLORS = {
    "Ready": "#FFC107",
    "Running": "#4CAF50",
    "Blocked": "#F44336",
    "Suspended": "#9E9E9E"
}

class SerialConnection:
    def __init__(self, port=None, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.connected = False
        self.read_thread = None
        self.running = False
        self.callback = None
        self.buffer = deque(maxlen=10000)

    def connect(self):
        try:
            if self.port:
                self.serial = serial.Serial(self.port, self.baudrate, timeout=0.1)
                self.connected = True
                self.running = True
                self.read_thread = threading.Thread(target=self._read_loop, daemon=True)
                self.read_thread.start()
                return True, f"Terhubung ke {self.port}"
            return False, "Port tidak dipilih"
        except Exception as e:
            return False, f"Gagal terhubung: {str(e)}"

    def disconnect(self):
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()
        self.connected = False
        return "Terputus dari " + (self.port or "port")

    def _read_loop(self):
        while self.running and self.serial and self.serial.is_open:
            try:
                if self.serial.in_waiting:
                    data = self.serial.read(self.serial.in_waiting)
                    text = data.decode('utf-8', errors='ignore')
                    self.buffer.append(text)
                    if self.callback:
                        self.callback(text, self.port)
                time.sleep(0.01)
            except Exception as e:
                if self.callback:
                    self.callback(f"[ERROR] {str(e)}", self.port)
                break

    def write(self, data):
        if self.serial and self.serial.is_open:
            if isinstance(data, str):
                data = data.encode('utf-8')
            self.serial.write(data)
            return True
        return False

    def get_buffer_text(self):
        return ''.join(list(self.buffer))


class TaskMonitor:
    def __init__(self):
        self.tasks = {}
        self.running = False

    def parse_rtos_output(self, text):
        """Parse FreeRTOS output untuk task info"""
        lines = text.split('\n')
        for line in lines:
            if "Task:" in line or "task" in line.lower():
                parts = line.split()
                if len(parts) >= 3:
                    task_name = parts[1] if parts[0].lower() == "task:" else parts[0]
                    state = "Ready"
                    for s in TASK_STATE_COLORS.keys():
                        if s.lower() in line.lower():
                            state = s
                            break
                    stack = 0
                    cpu = 0
                    for i, p in enumerate(parts):
                        if "stack" in p.lower() or "hwm" in p.lower():
                            try:
                                stack = int(parts[i+1])
                            except:
                                pass
                        if "cpu" in p.lower() or "%" in p:
                            try:
                                cpu = float(p.replace("%", ""))
                            except:
                                pass
                    self.tasks[task_name] = {
                        "state": state,
                        "stack": stack,
                        "cpu": cpu
                    }


class GUI_RTOS_Middle:
    def __init__(self, root):
        self.root = root
        self.root.title("GUI Pengujian RTOS Middle - Modul 09")
        self.root.geometry("1600x900")

        self.board1_conn = SerialConnection()
        self.board2_conn = SerialConnection()
        self.task_monitor = TaskMonitor()
        self.selected_programs = []
        self.test_results = []
        self.test_running = False

        self.setup_ui()

    def setup_ui(self):
        # Main container
        main_container = ttk.Frame(self.root)
        main_container.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Top panel - Connection
        self.setup_connection_panel(main_container)

        # Middle panel
        middle_panel = ttk.Frame(main_container)
        middle_panel.pack(fill=tk.BOTH, expand=True, pady=5)

        # Left panel - Program Selection
        self.setup_program_panel(middle_panel)

        # Center panel
        center_panel = ttk.Frame(middle_panel)
        center_panel.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)

        # Center-Left - RTOS Monitor
        self.setup_rtos_monitor(center_panel)

        # Center - Test Control
        self.setup_test_control(center_panel)

        # Right panel - Peripheral Test
        self.setup_peripheral_panel(middle_panel)

        # Bottom panel - Results/Log
        self.setup_results_panel(main_container)

    def setup_connection_panel(self, parent):
        conn_frame = ttk.LabelFrame(parent, text="Koneksi Board", padding=10)
        conn_frame.pack(fill=tk.X, pady=(0, 5))

        # Board 1
        board1_frame = ttk.Frame(conn_frame)
        board1_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)

        ttk.Label(board1_frame, text="Board 1 (STM32):").grid(row=0, column=0, sticky=tk.W, padx=5)
        self.board1_port = ttk.Combobox(board1_frame, width=30, state="readonly")
        self.board1_port.grid(row=0, column=1, padx=5)
        self.refresh_ports()

        ttk.Label(board1_frame, text="Baud Rate:").grid(row=0, column=2, padx=5)
        self.board1_baud = ttk.Entry(board1_frame, width=10)
        self.board1_baud.insert(0, "115200")
        self.board1_baud.grid(row=0, column=3, padx=5)

        self.board1_connect_btn = ttk.Button(board1_frame, text="Hubungkan", command=self.toggle_board1)
        self.board1_connect_btn.grid(row=0, column=4, padx=5)

        self.board1_status = ttk.Label(board1_frame, text="Terputus", foreground="red")
        self.board1_status.grid(row=0, column=5, padx=5)

        # Board 2
        board2_frame = ttk.Frame(conn_frame)
        board2_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)

        ttk.Label(board2_frame, text="Board 2 (ESP32):").grid(row=0, column=0, sticky=tk.W, padx=5)
        self.board2_port = ttk.Combobox(board2_frame, width=30, state="readonly")
        self.board2_port.grid(row=0, column=1, padx=5)
        self.board2_port['values'] = self.board1_port['values']

        ttk.Label(board2_frame, text="Baud Rate:").grid(row=0, column=2, padx=5)
        self.board2_baud = ttk.Entry(board2_frame, width=10)
        self.board2_baud.insert(0, "115200")
        self.board2_baud.grid(row=0, column=3, padx=5)

        self.board2_connect_btn = ttk.Button(board2_frame, text="Hubungkan", command=self.toggle_board2)
        self.board2_connect_btn.grid(row=0, column=4, padx=5)

        self.board2_status = ttk.Label(board2_frame, text="Terputus", foreground="red")
        self.board2_status.grid(row=0, column=5, padx=5)

        # Refresh ports button
        ttk.Button(conn_frame, text="Refresh Port", command=self.refresh_ports).pack(side=tk.RIGHT, padx=5)

    def setup_program_panel(self, parent):
        prog_frame = ttk.LabelFrame(parent, text="Daftar Program", padding=5)
        prog_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 5))

        # Treeview with checkboxes
        columns = ("Nama", "Peripheral", "Deskripsi")
        self.prog_tree = ttk.Treeview(prog_frame, columns=columns, show="tree headings", height=25)
        self.prog_tree.heading("#0", text="Pilih")
        self.prog_tree.heading("Nama", text="Nama Program")
        self.prog_tree.heading("Peripheral", text="Peripheral")
        self.prog_tree.heading("Deskripsi", text="Deskripsi")

        self.prog_tree.column("#0", width=50, minwidth=50)
        self.prog_tree.column("Nama", width=200, minwidth=150)
        self.prog_tree.column("Peripheral", width=150, minwidth=100)
        self.prog_tree.column("Deskripsi", width=200, minwidth=150)

        # Add programs to tree
        self.check_vars = {}
        for category, programs in PROGRAMS.items():
            cat_id = self.prog_tree.insert("", tk.END, text=category, open=True, values=(category, "", ""))
            for prog in programs:
                peripherals_text = ", ".join(prog["peripherals"])
                prog_id = self.prog_tree.insert(cat_id, tk.END, text="☐", values=(prog["name"], peripherals_text, prog["desc"]))
                self.check_vars[prog_id] = {"checked": False, "prog": prog}

        self.prog_tree.pack(fill=tk.Y, expand=True)
        self.prog_tree.bind("<Button-1>", self.on_tree_click)

        # Buttons
        btn_frame = ttk.Frame(prog_frame)
        btn_frame.pack(fill=tk.X, pady=5)
        ttk.Button(btn_frame, text="Pilih Semua", command=self.select_all).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Batal Pilih", command=self.deselect_all).pack(side=tk.LEFT, padx=2)

    def setup_rtos_monitor(self, parent):
        rtos_frame = ttk.LabelFrame(parent, text="Monitor RTOS", padding=5)
        rtos_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))

        # Task list
        columns = ("Task", "State", "Stack", "CPU %")
        self.task_tree = ttk.Treeview(rtos_frame, columns=columns, show="headings", height=10)
        for col in columns:
            self.task_tree.heading(col, text=col)
            self.task_tree.column(col, width=80, minwidth=60)
        self.task_tree.pack(fill=tk.BOTH, expand=True)

        # Stack visualization
        ttk.Label(rtos_frame, text="Stack High-Water Mark:").pack(anchor=tk.W, pady=(5, 0))
        self.stack_canvas = tk.Canvas(rtos_frame, height=100, bg="white")
        self.stack_canvas.pack(fill=tk.X, pady=5)

        # CPU usage
        ttk.Label(rtos_frame, text="Penggunaan CPU per Task:").pack(anchor=tk.W, pady=(5, 0))
        self.cpu_canvas = tk.Canvas(rtos_frame, height=100, bg="white")
        self.cpu_canvas.pack(fill=tk.X, pady=5)

    def setup_test_control(self, parent):
        test_frame = ttk.LabelFrame(parent, text="Kontrol Pengujian", padding=5)
        test_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)

        # Buttons
        btn_frame = ttk.Frame(test_frame)
        btn_frame.pack(fill=tk.X, pady=5)

        self.test_selected_btn = ttk.Button(btn_frame, text="Uji Terpilih", command=self.test_selected)
        self.test_selected_btn.pack(side=tk.LEFT, padx=2)

        self.test_all_btn = ttk.Button(btn_frame, text="Uji Semua", command=self.test_all)
        self.test_all_btn.pack(side=tk.LEFT, padx=2)

        self.stop_test_btn = ttk.Button(btn_frame, text="Berhenti", command=self.stop_test, state=tk.DISABLED)
        self.stop_test_btn.pack(side=tk.LEFT, padx=2)

        # Progress bar
        ttk.Label(test_frame, text="Progress:").pack(anchor=tk.W, pady=(5, 0))
        self.progress = ttk.Progressbar(test_frame, mode='determinate')
        self.progress.pack(fill=tk.X, pady=2)

        # Current program info
        self.current_test_label = ttk.Label(test_frame, text="Siap untuk pengujian")
        self.current_test_label.pack(anchor=tk.W, pady=5)

        # Output display for multi-board
        output_frame = ttk.Frame(test_frame)
        output_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        # Board 1 output
        ttk.Label(output_frame, text="Output Board 1 (STM32):").grid(row=0, column=0, sticky=tk.W)
        self.board1_output = scrolledtext.ScrolledText(output_frame, height=8, width=40)
        self.board1_output.grid(row=1, column=0, padx=(0, 5))

        # Board 2 output
        ttk.Label(output_frame, text="Output Board 2 (ESP32):").grid(row=0, column=1, sticky=tk.W)
        self.board2_output = scrolledtext.ScrolledText(output_frame, height=8, width=40)
        self.board2_output.grid(row=1, column=1, padx=(5, 0))

        # Configure grid weights
        output_frame.columnconfigure(0, weight=1)
        output_frame.columnconfigure(1, weight=1)

    def setup_peripheral_panel(self, parent):
        periph_frame = ttk.LabelFrame(parent, text="Tes Peripheral", padding=5)
        periph_frame.pack(side=tk.LEFT, fill=tk.BOTH, padx=(5, 0))

        # Notebook for different peripherals
        self.periph_notebook = ttk.Notebook(periph_frame)
        self.periph_notebook.pack(fill=tk.BOTH, expand=True)

        # GPIO Tab
        self.gpio_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.gpio_frame, text="GPIO")
        self.setup_gpio_tab()

        # Interrupt Tab
        self.interrupt_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.interrupt_frame, text="Interrupt")
        self.setup_interrupt_tab()

        # Encoder Tab
        self.encoder_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.encoder_frame, text="Encoder")
        self.setup_encoder_tab()

        # Serial Tab
        self.serial_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.serial_frame, text="Serial")
        self.setup_serial_tab()

        # ADC Tab
        self.adc_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.adc_frame, text="ADC")
        self.setup_adc_tab()

        # DAC Tab
        self.dac_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.dac_frame, text="DAC")
        self.setup_dac_tab()

        # I2C Tab
        self.i2c_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.i2c_frame, text="I2C")
        self.setup_i2c_tab()

        # SPI Tab
        self.spi_frame = ttk.Frame(self.periph_notebook)
        self.periph_notebook.add(self.spi_frame, text="SPI")
        self.setup_spi_tab()

    def setup_gpio_tab(self):
        # Pin states
        ttk.Label(self.gpio_frame, text="Status Pin GPIO:").pack(anchor=tk.W, pady=5)

        self.gpio_pins_frame = ttk.Frame(self.gpio_frame)
        self.gpio_pins_frame.pack(fill=tk.X)

        # Create LED indicators for common pins
        self.gpio_leds = {}
        for i, pin in enumerate(range(0, 16)):
            frame = ttk.Frame(self.gpio_pins_frame)
            frame.grid(row=i//4, column=i%4, padx=5, pady=5)

            canvas = tk.Canvas(frame, width=30, height=30, bg="white")
            canvas.create_oval(5, 5, 25, 25, fill="gray", tags=f"led_{pin}")
            canvas.pack()
            ttk.Label(frame, text=f"P{pin}").pack()

            self.gpio_leds[pin] = canvas

        # Control buttons
        ctrl_frame = ttk.Frame(self.gpio_frame)
        ctrl_frame.pack(fill=tk.X, pady=5)

        ttk.Button(ctrl_frame, text="Refresh GPIO", command=self.refresh_gpio).pack(side=tk.LEFT, padx=2)
        ttk.Button(ctrl_frame, text="Set High", command=lambda: self.set_gpio("high")).pack(side=tk.LEFT, padx=2)
        ttk.Button(ctrl_frame, text="Set Low", command=lambda: self.set_gpio("low")).pack(side=tk.LEFT, padx=2)

    def setup_interrupt_tab(self):
        ttk.Label(self.interrupt_frame, text="Monitor Interrupt:").pack(anchor=tk.W, pady=5)

        # Interrupt count
        count_frame = ttk.Frame(self.interrupt_frame)
        count_frame.pack(fill=tk.X, pady=5)

        ttk.Label(count_frame, text="Jumlah Interrupt:").grid(row=0, column=0, sticky=tk.W, padx=5)
        self.interrupt_count = tk.StringVar(value="0")
        ttk.Label(count_frame, textvariable=self.interrupt_count, font=("Arial", 16, "bold")).grid(row=0, column=1, padx=5)

        # Timing
        ttk.Label(count_frame, text="Waktu Terakhir:").grid(row=1, column=0, sticky=tk.W, padx=5)
        self.interrupt_time = tk.StringVar(value="-")
        ttk.Label(count_frame, textvariable=self.interrupt_time).grid(row=1, column=1, sticky=tk.W, padx=5)

        # Interrupt log
        ttk.Label(self.interrupt_frame, text="Log Interrupt:").pack(anchor=tk.W, pady=(10, 5))
        self.interrupt_log = scrolledtext.ScrolledText(self.interrupt_frame, height=8)
        self.interrupt_log.pack(fill=tk.BOTH, expand=True)

        ttk.Button(self.interrupt_frame, text="Clear Log", command=lambda: self.interrupt_log.delete(1.0, tk.END)).pack(pady=5)

    def setup_encoder_tab(self):
        ttk.Label(self.encoder_frame, text="Monitor Encoder:").pack(anchor=tk.W, pady=5)

        # Rotation count
        count_frame = ttk.Frame(self.encoder_frame)
        count_frame.pack(fill=tk.X, pady=5)

        ttk.Label(count_frame, text="Jumlah Putaran:").grid(row=0, column=0, sticky=tk.W, padx=5)
        self.encoder_count = tk.StringVar(value="0")
        ttk.Label(count_frame, textvariable=self.encoder_count, font=("Arial", 16, "bold")).grid(row=0, column=1, padx=5)

        # Direction
        ttk.Label(count_frame, text="Arah Putaran:").grid(row=1, column=0, sticky=tk.W, padx=5)
        self.encoder_dir = tk.StringVar(value="-")
        ttk.Label(count_frame, textvariable=self.encoder_dir, font=("Arial", 12)).grid(row=1, column=1, sticky=tk.W, padx=5)

        # Speed
        ttk.Label(count_frame, text="Kecepatan (RPM):").grid(row=2, column=0, sticky=tk.W, padx=5)
        self.encoder_speed = tk.StringVar(value="0")
        ttk.Label(count_frame, textvariable=self.encoder_speed).grid(row=2, column=1, sticky=tk.W, padx=5)

        # Visual representation
        self.encoder_canvas = tk.Canvas(self.encoder_frame, height=100, bg="white")
        self.encoder_canvas.pack(fill=tk.X, pady=5)

        ttk.Button(self.encoder_frame, text="Reset Counter", command=self.reset_encoder).pack(pady=5)

    def setup_serial_tab(self):
        # Send frame
        send_frame = ttk.LabelFrame(self.serial_frame, text="Kirim Data")
        send_frame.pack(fill=tk.X, pady=5)

        self.serial_send_entry = ttk.Entry(send_frame)
        self.serial_send_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5, pady=5)

        ttk.Button(send_frame, text="Kirim", command=self.send_serial).pack(side=tk.LEFT, padx=5, pady=5)
        ttk.Button(send_frame, text="Clear", command=self.clear_serial).pack(side=tk.LEFT, padx=5, pady=5)

        # Receive terminal
        ttk.Label(self.serial_frame, text="Terminal Serial:").pack(anchor=tk.W, pady=5)
        self.serial_terminal = scrolledtext.ScrolledText(self.serial_frame, height=12)
        self.serial_terminal.pack(fill=tk.BOTH, expand=True)

        # Board selection for send
        board_select_frame = ttk.Frame(self.serial_frame)
        board_select_frame.pack(fill=tk.X, pady=5)

        self.serial_board = tk.StringVar(value="board1")
        ttk.Radiobutton(board_select_frame, text="Board 1", variable=self.serial_board, value="board1").pack(side=tk.LEFT, padx=5)
        ttk.Radiobutton(board_select_frame, text="Board 2", variable=self.serial_board, value="board2").pack(side=tk.LEFT, padx=5)
        ttk.Radiobutton(board_select_frame, text="Keduanya", variable=self.serial_board, value="both").pack(side=tk.LEFT, padx=5)

    def setup_adc_tab(self):
        ttk.Label(self.adc_frame, text="Monitor ADC:").pack(anchor=tk.W, pady=5)

        # ADC channels
        self.adc_values = {}
        adc_display = ttk.Frame(self.adc_frame)
        adc_display.pack(fill=tk.X, pady=5)

        for i in range(4):
            frame = ttk.Frame(adc_display)
            frame.grid(row=0, column=i, padx=10)

            ttk.Label(frame, text=f"Channel {i}").pack()

            canvas = tk.Canvas(frame, width=50, height=150, bg="white")
            canvas.create_rectangle(10, 140, 40, 140, fill="blue", tags="bar")
            canvas.pack(pady=5)

            value_var = tk.StringVar(value="0")
            ttk.Label(frame, textvariable=value_var).pack()

            self.adc_values[i] = {"canvas": canvas, "value": value_var}

        ttk.Button(self.adc_frame, text="Refresh ADC", command=self.refresh_adc).pack(pady=5)

    def setup_dac_tab(self):
        ttk.Label(self.dac_frame, text="Kontrol DAC:").pack(anchor=tk.W, pady=5)

        # DAC output slider
        for i in range(2):
            frame = ttk.Frame(self.dac_frame)
            frame.pack(fill=tk.X, pady=5)

            ttk.Label(frame, text=f"DAC Channel {i}:").pack(side=tk.LEFT, padx=5)

            slider = ttk.Scale(frame, from_=0, to=4095, orient=tk.HORIZONTAL, length=200)
            slider.pack(side=tk.LEFT, padx=5)
            slider.bind("<ButtonRelease-1>", lambda e, ch=i: self.set_dac(ch, e))

            value_label = ttk.Label(frame, text="0")
            value_label.pack(side=tk.LEFT, padx=5)

            # Update label on slider change
            def update_label(val, lbl=value_label, ch=i, sld=slider):
                lbl.config(text=str(int(float(val))))
                return val
            slider.config(command=update_label)

    def setup_i2c_tab(self):
        ttk.Label(self.i2c_frame, text="Pemindaian Perangkat I2C:").pack(anchor=tk.W, pady=5)

        # Detected devices list
        columns = ("Alamat", "Nama", "Status")
        self.i2c_tree = ttk.Treeview(self.i2c_frame, columns=columns, show="headings", height=8)
        for col in columns:
            self.i2c_tree.heading(col, text=col)
            self.i2c_tree.column(col, width=100)
        self.i2c_tree.pack(fill=tk.BOTH, expand=True, pady=5)

        btn_frame = ttk.Frame(self.i2c_frame)
        btn_frame.pack(fill=tk.X, pady=5)

        ttk.Button(btn_frame, text="Scan I2C", command=self.scan_i2c).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Clear", command=lambda: self.i2c_tree.delete(*self.i2c_tree.get_children())).pack(side=tk.LEFT, padx=2)

    def setup_spi_tab(self):
        ttk.Label(self.spi_frame, text="Statistik Transfer SPI:").pack(anchor=tk.W, pady=5)

        # Statistics
        stats_frame = ttk.Frame(self.spi_frame)
        stats_frame.pack(fill=tk.X, pady=5)

        self.spi_stats = {
            "sent": tk.StringVar(value="0"),
            "received": tk.StringVar(value="0"),
            "errors": tk.StringVar(value="0"),
            "speed": tk.StringVar(value="0 bps")
        }

        labels = [("Data Terkirim:", "sent"), ("Data Diterima:", "received"),
                  ("Error:", "errors"), ("Kecepatan:", "speed")]

        for i, (label, key) in enumerate(labels):
            ttk.Label(stats_frame, text=label).grid(row=i, column=0, sticky=tk.W, padx=5, pady=2)
            ttk.Label(stats_frame, textvariable=self.spi_stats[key]).grid(row=i, column=1, sticky=tk.W, padx=5, pady=2)

        # Transfer test
        ttk.Label(self.spi_frame, text="Tes Transfer:").pack(anchor=tk.W, pady=(10, 5))

        send_frame = ttk.Frame(self.spi_frame)
        send_frame.pack(fill=tk.X, pady=5)

        self.spi_send_entry = ttk.Entry(send_frame)
        self.spi_send_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.spi_send_entry.insert(0, "0x00, 0x01, 0x02")

        ttk.Button(send_frame, text="Kirim", command=self.send_spi).pack(side=tk.LEFT, padx=5)

        # Receive display
        self.spi_receive = scrolledtext.ScrolledText(self.spi_frame, height=6)
        self.spi_receive.pack(fill=tk.BOTH, expand=True, pady=5)

    def setup_results_panel(self, parent):
        results_frame = ttk.LabelFrame(parent, text="Hasil dan Log", padding=5)
        results_frame.pack(fill=tk.BOTH, expand=True, pady=(5, 0))

        # Results table
        ttk.Label(results_frame, text="Tabel Hasil Pengujian:").pack(anchor=tk.W, pady=(0, 5))

        result_columns = ("Program", "Status", "Waktu", "Board", "Keterangan")
        self.result_tree = ttk.Treeview(results_frame, columns=result_columns, show="headings", height=6)
        for col in result_columns:
            self.result_tree.heading(col, text=col)
            self.result_tree.column(col, width=120)
        self.result_tree.pack(fill=tk.X, pady=5)

        # Log output
        ttk.Label(results_frame, text="Log Output:").pack(anchor=tk.W, pady=(5, 0))

        log_frame = ttk.Frame(results_frame)
        log_frame.pack(fill=tk.BOTH, expand=True)

        self.log_text = scrolledtext.ScrolledText(log_frame, height=8, bg="black", fg="white")
        self.log_text.pack(fill=tk.BOTH, expand=True)

        # Log control buttons
        log_btn_frame = ttk.Frame(results_frame)
        log_btn_frame.pack(fill=tk.X, pady=5)

        ttk.Button(log_btn_frame, text="Clear Log", command=self.clear_log).pack(side=tk.LEFT, padx=2)
        ttk.Button(log_btn_frame, text="Simpan Log", command=self.save_log).pack(side=tk.LEFT, padx=2)
        ttk.Button(log_btn_frame, text="Export Hasil", command=self.export_results).pack(side=tk.LEFT, padx=2)

    def refresh_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.board1_port['values'] = ports
        self.board2_port['values'] = ports
        if ports:
            self.board1_port.set(ports[0])
            if len(ports) > 1:
                self.board2_port.set(ports[1])

    def toggle_board1(self):
        if not self.board1_conn.connected:
            port = self.board1_port.get()
            baud = int(self.board1_baud.get())
            self.board1_conn = SerialConnection(port, baud)
            self.board1_conn.callback = self.on_serial_data
            success, msg = self.board1_conn.connect()
            if success:
                self.board1_status.config(text="Terhubung", foreground="green")
                self.board1_connect_btn.config(text="Putuskan")
                self.log(f"Board 1: {msg}")
            else:
                messagebox.showerror("Error", msg)
        else:
            msg = self.board1_conn.disconnect()
            self.board1_status.config(text="Terputus", foreground="red")
            self.board1_connect_btn.config(text="Hubungkan")
            self.log(f"Board 1: {msg}")

    def toggle_board2(self):
        if not self.board2_conn.connected:
            port = self.board2_port.get()
            baud = int(self.board2_baud.get())
            self.board2_conn = SerialConnection(port, baud)
            self.board2_conn.callback = self.on_serial_data
            success, msg = self.board2_conn.connect()
            if success:
                self.board2_status.config(text="Terhubung", foreground="green")
                self.board2_connect_btn.config(text="Putuskan")
                self.log(f"Board 2: {msg}")
            else:
                messagebox.showerror("Error", msg)
        else:
            msg = self.board2_conn.disconnect()
            self.board2_status.config(text="Terputus", foreground="red")
            self.board2_connect_btn.config(text="Hubungkan")
            self.log(f"Board 2: {msg}")

    def on_serial_data(self, data, port):
        # Update appropriate output window
        if port == self.board1_conn.port:
            self.board1_output.insert(tk.END, data)
            self.board1_output.see(tk.END)
        elif port == self.board2_conn.port:
            self.board2_output.insert(tk.END, data)
            self.board2_output.see(tk.END)

        # Parse RTOS output
        self.task_monitor.parse_rtos_output(data)
        self.update_rtos_display()

        # Log with color coding
        self.log(data, "data" if "ERROR" not in data else "error")

    def on_tree_click(self, event):
        item = self.prog_tree.identify('item', event.x, event.y)
        column = self.prog_tree.identify('column', event.x, event.y)

        if column == '#0' and item in self.check_vars:
            var = self.check_vars[item]
            var["checked"] = not var["checked"]
            new_text = "☑" if var["checked"] else "☐"
            self.prog_tree.item(item, text=new_text)

    def select_all(self):
        for item, var in self.check_vars.items():
            var["checked"] = True
            self.prog_tree.item(item, text="☑")

    def deselect_all(self):
        for item, var in self.check_vars.items():
            var["checked"] = False
            self.prog_tree.item(item, text="☐")

    def get_selected_programs(self):
        selected = []
        for item, var in self.check_vars.items():
            if var["checked"]:
                selected.append(var["prog"])
        return selected

    def test_selected(self):
        selected = self.get_selected_programs()
        if not selected:
            messagebox.showwarning("Peringatan", "Pilih program yang akan diuji!")
            return
        self.run_tests(selected)

    def test_all(self):
        all_programs = []
        for category in PROGRAMS.values():
            all_programs.extend(category)
        self.run_tests(all_programs)

    def run_tests(self, programs):
        if self.test_running:
            return

        self.test_running = True
        self.test_selected_btn.config(state=tk.DISABLED)
        self.test_all_btn.config(state=tk.DISABLED)
        self.stop_test_btn.config(state=tk.NORMAL)

        self.progress.config(maximum=len(programs), value=0)
        self.test_results = []

        def test_thread():
            for i, prog in enumerate(programs):
                if not self.test_running:
                    break

                self.current_test_label.config(text=f"Menguji: {prog['name']}")

                # Simulate test
                result = self.run_single_test(prog)
                self.test_results.append(result)

                # Update UI
                self.root.after(0, self.update_test_result, result)
                self.root.after(0, lambda v=i+1: self.progress.config(value=v))

            self.test_running = False
            self.root.after(0, self.test_complete)

        threading.Thread(target=test_thread, daemon=True).start()

    def run_single_test(self, prog):
        start_time = time.time()
        status = "LULUS"
        keterangan = "Pengujian berhasil"

        # Check if boards connected for multi programs
        if prog["name"].startswith("Multi") and not (self.board1_conn.connected and self.board2_conn.connected):
            status = "GAGAL"
            keterangan = "Board tidak terhubung"
        elif not self.board1_conn.connected and not self.board2_conn.connected:
            status = "GAGAL"
            keterangan = "Tidak ada board yang terhubung"
        else:
            # Simulate test based on peripherals
            for peripheral in prog["peripherals"]:
                if peripheral == "GPIO":
                    self.test_gpio()
                elif peripheral == "Interrupt":
                    self.test_interrupt()
                elif peripheral == "Encoder":
                    self.test_encoder()
                elif peripheral == "Serial":
                    self.test_serial_peripheral()
                elif peripheral == "ADC":
                    self.test_adc()
                elif peripheral == "DAC":
                    self.test_dac()
                elif peripheral == "I2C":
                    self.test_i2c()
                elif peripheral == "SPI":
                    self.test_spi()

            time.sleep(0.5)  # Simulate test time

        elapsed = time.time() - start_time
        return {
            "program": prog["name"],
            "status": status,
            "time": f"{elapsed:.2f}s",
            "board": "STM32" if "STM32" in prog["name"] else ("ESP32" if "ESP32" in prog["name"] else "Multi"),
            "keterangan": keterangan
        }

    def update_test_result(self, result):
        self.result_tree.insert("", tk.END, values=(
            result["program"],
            result["status"],
            result["time"],
            result["board"],
            result["keterangan"]
        ))

    def test_complete(self):
        self.test_running = False
        self.test_selected_btn.config(state=tk.NORMAL)
        self.test_all_btn.config(state=tk.NORMAL)
        self.stop_test_btn.config(state=tk.DISABLED)
        self.current_test_label.config(text="Pengujian selesai")
        messagebox.showinfo("Selesai", "Pengujian selesai dilakukan!")

    def stop_test(self):
        self.test_running = False
        self.current_test_label.config(text="Pengujian dihentikan")

    def update_rtos_display(self):
        self.task_tree.delete(*self.task_tree.get_children())
        for task_name, info in self.task_monitor.tasks.items():
            self.task_tree.insert("", tk.END, values=(
                task_name,
                info["state"],
                info["stack"],
                f"{info['cpu']:.1f}"
            ))

        # Update visual displays
        self.update_stack_display()
        self.update_cpu_display()

    def update_stack_display(self):
        self.stack_canvas.delete("all")
        if not self.task_monitor.tasks:
            return

        x = 10
        width = 40
        max_height = 80

        for task_name, info in list(self.task_monitor.tasks.items())[:10]:
            height = min(info["stack"] / 10, max_height)
            color = TASK_STATE_COLORS.get(info["state"], "gray")

            self.stack_canvas.create_rectangle(x, max_height - height + 20, x + width, max_height + 20,
                                               fill=color, outline="black")
            self.stack_canvas.create_text(x + width/2, max_height + 30, text=task_name[:6], font=("Arial", 6))
            x += width + 10

    def update_cpu_display(self):
        self.cpu_canvas.delete("all")
        if not self.task_monitor.tasks:
            return

        x = 10
        width = 40
        max_height = 80

        for task_name, info in list(self.task_monitor.tasks.items())[:10]:
            height = (info["cpu"] / 100) * max_height if info["cpu"] > 0 else 5
            color = TASK_STATE_COLORS.get(info["state"], "gray")

            self.cpu_canvas.create_rectangle(x, max_height - height + 20, x + width, max_height + 20,
                                             fill=color, outline="black")
            self.cpu_canvas.create_text(x + width/2, max_height + 30, text=task_name[:6], font=("Arial", 6))
            x += width + 10

    def test_gpio(self):
        pass  # Simulated

    def test_interrupt(self):
        count = int(self.interrupt_count.get()) + 1
        self.interrupt_count.set(str(count))
        self.interrupt_time.set(datetime.now().strftime("%H:%M:%S.%f")[:-3])
        self.interrupt_log.insert(tk.END, f"[{self.interrupt_time.get()}] Interrupt #{count}\n")
        self.interrupt_log.see(tk.END)

    def test_encoder(self):
        count = int(self.encoder_count.get()) + 1
        self.encoder_count.set(str(count))
        self.encoder_dir.set("CW" if count % 2 == 0 else "CCW")
        self.draw_encoder_visual()

    def draw_encoder_visual(self):
        self.encoder_canvas.delete("all")
        cx, cy = 100, 50
        r = 30
        count = int(self.encoder_count.get())

        # Draw circle
        self.encoder_canvas.create_oval(cx-r, cy-r, cx+r, cy+r, outline="black", width=2)

        # Draw indicator
        angle = (count * 36) % 360
        import math
        x = cx + r * math.cos(math.radians(angle - 90))
        y = cy + r * math.sin(math.radians(angle - 90))
        self.encoder_canvas.create_line(cx, cy, x, y, fill="red", width=2)

        self.encoder_canvas.create_text(cx, cy, text=str(count), font=("Arial", 12, "bold"))

    def test_serial_peripheral(self):
        self.serial_terminal.insert(tk.END, f"[{datetime.now().strftime('%H:%M:%S')}] Tes serial berhasil\n")
        self.serial_terminal.see(tk.END)

    def test_adc(self):
        import random
        for i in range(4):
            value = random.randint(0, 4095)
            self.adc_values[i]["value"].set(str(value))
            canvas = self.adc_values[i]["canvas"]
            canvas.delete("bar")
            height = (value / 4095) * 130
            canvas.create_rectangle(10, 140 - height, 40, 140, fill="blue", tags="bar")

    def test_dac(self):
        pass

    def test_i2c(self):
        pass

    def test_spi(self):
        current = int(self.spi_stats["sent"].get())
        self.spi_stats["sent"].set(str(current + 1))
        self.spi_receive.insert(tk.END, f"Received: 0x{hex(0x55)[2:].zfill(2).upper()}\n")
        self.spi_receive.see(tk.END)

    def refresh_gpio(self):
        pass

    def set_gpio(self, state):
        pass

    def reset_encoder(self):
        self.encoder_count.set("0")
        self.encoder_dir.set("-")
        self.encoder_speed.set("0")
        self.encoder_canvas.delete("all")

    def send_serial(self):
        data = self.serial_send_entry.get()
        if self.serial_board.get() in ["board1", "both"] and self.board1_conn.connected:
            self.board1_conn.write(data + "\n")
        if self.serial_board.get() in ["board2", "both"] and self.board2_conn.connected:
            self.board2_conn.write(data + "\n")
        self.serial_send_entry.delete(0, tk.END)

    def clear_serial(self):
        self.serial_terminal.delete(1.0, tk.END)

    def refresh_adc(self):
        self.test_adc()

    def set_dac(self, channel, event=None):
        pass

    def scan_i2c(self):
        # Simulate I2C scan
        devices = [
            ("0x48", "ADS1115", "Ditemukan"),
            ("0x76", "BME280", "Ditemukan"),
            ("0x3C", "SSD1306", "Ditemukan")
        ]
        for addr, name, status in devices:
            self.i2c_tree.insert("", tk.END, values=(addr, name, status))

    def send_spi(self):
        data = self.spi_send_entry.get()
        self.spi_receive.insert(tk.END, f"Sent: {data}\n")
        self.spi_receive.see(tk.END)

    def log(self, message, msg_type="info"):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{timestamp}] {message}")

        # Color coding
        if msg_type == "error":
            self.log_text.tag_add("error", "end-2l", "end-1l")
            self.log_text.tag_config("error", foreground="red")
        elif msg_type == "data":
            self.log_text.tag_add("data", "end-2l", "end-1l")
            self.log_text.tag_config("data", foreground="green")
        else:
            self.log_text.tag_add("info", "end-2l", "end-1l")
            self.log_text.tag_config("info", foreground="white")

        self.log_text.see(tk.END)

    def clear_log(self):
        self.log_text.delete(1.0, tk.END)

    def save_log(self):
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        if filename:
            with open(filename, 'w') as f:
                f.write(self.log_text.get(1.0, tk.END))
            messagebox.showinfo("Simpan", f"Log disimpan ke {filename}")

    def export_results(self):
        filename = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")]
        )
        if filename:
            with open(filename, 'w', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(["Program", "Status", "Waktu", "Board", "Keterangan"])
                for item in self.result_tree.get_children():
                    writer.writerow(self.result_tree.item(item)['values'])
            messagebox.showinfo("Export", f"Hasil diekspor ke {filename}")


def main():
    root = tk.Tk()
    app = GUI_RTOS_Middle(root)
    root.mainloop()


if __name__ == "__main__":
    main()
