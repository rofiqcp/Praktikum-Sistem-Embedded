#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GUI_RTOS_Advanced.py - Antarmuka Pengguna untuk Pengujian Program FreeRTOS Advanced
Modul 10 - Praktikum Sistem Embedded
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
import serial
import serial.tools.list_ports
import threading
import time
import json
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import queue
import re
from datetime import datetime
from collections import deque

class FreeRTOS_GUI:
    def __init__(self, root):
        self.root = root
        self.root.title("FreeRTOS Advanced - Pengujian Modul 10")
        self.root.geometry("1600x900")
        
        self.serial_port = None
        self.is_connected = False
        self.test_running = False
        self.data_queue = queue.Queue()
        
        self.program_data = {
            "STM32": [
                {"id": "STM32_01", "name": "Event Group Basic", "category": "Event Groups", 
                 "desc": "Demonstrasi penggunaan Event Group untuk sinkronisasi antar task"},
                {"id": "STM32_02", "name": "Software Timer Basic", "category": "Software Timers",
                 "desc": "Implementasi software timer dengan callback function"},
                {"id": "STM32_03", "name": "Task Notification Basic", "category": "Task Notifications",
                 "desc": "Komunikasi antar task menggunakan Task Notification"},
                {"id": "STM32_04", "name": "Counting Semaphore", "category": "Semaphores",
                 "desc": "Penggunaan Counting Semaphore untuk manajemen resource"},
                {"id": "STM32_05", "name": "Mutex Priority Inversion", "category": "Mutex",
                 "desc": "Demonstrasi Priority Inversion dan solusi dengan Mutex"},
                {"id": "STM32_06", "name": "Memory Allocation Dynamic", "category": "Memory",
                 "desc": "Alokasi memori dinamis pada FreeRTOS"},
                {"id": "STM32_07", "name": "Static Allocation", "category": "Static Allocation",
                 "desc": "Pembuatan task dengan alokasi memori statis"},
                {"id": "STM32_08", "name": "Critical Section", "category": "Critical Section",
                 "desc": "Penggunaan Critical Section untuk proteksi resource bersama"},
                {"id": "STM32_09", "name": "Message Buffer", "category": "Message Buffer",
                 "desc": "Komunikasi antar task menggunakan Message Buffer"},
                {"id": "STM32_10", "name": "Queue Set Multiplex", "category": "Queue Set",
                 "desc": "Multiplexing multiple queue dengan Queue Set"},
            ],
            "ESP32": [
                {"id": "ESP32_01", "name": "Event Group Basic", "category": "Event Groups",
                 "desc": "Demonstrasi penggunaan Event Group untuk sinkronisasi antar task"},
                {"id": "ESP32_02", "name": "Software Timer Basic", "category": "Software Timers",
                 "desc": "Implementasi software timer dengan callback function"},
                {"id": "ESP32_03", "name": "Task Notification Basic", "category": "Task Notifications",
                 "desc": "Komunikasi antar task menggunakan Task Notification"},
                {"id": "ESP32_04", "name": "Counting Semaphore", "category": "Semaphores",
                 "desc": "Penggunaan Counting Semaphore untuk manajemen resource"},
                {"id": "ESP32_05", "name": "Mutex Priority Inversion", "category": "Mutex",
                 "desc": "Demonstrasi Priority Inversion dan solusi dengan Mutex"},
                {"id": "ESP32_06", "name": "Memory Allocation Dynamic", "category": "Memory",
                 "desc": "Alokasi memori dinamis pada FreeRTOS"},
                {"id": "ESP32_07", "name": "Static Allocation", "category": "Static Allocation",
                 "desc": "Pembuatan task dengan alokasi memori statis"},
                {"id": "ESP32_08", "name": "Critical Section", "category": "Critical Section",
                 "desc": "Penggunaan Critical Section untuk proteksi resource bersama"},
                {"id": "ESP32_09", "name": "Message Buffer", "category": "Message Buffer",
                 "desc": "Komunikasi antar task menggunakan Message Buffer"},
                {"id": "ESP32_10", "name": "Queue Set Multiplex", "category": "Queue Set",
                 "desc": "Multiplexing multiple queue dengan Queue Set"},
            ]
        }
        
        self.rtos_state = {
            "event_group_bits": 0,
            "timer_status": {},
            "notification_values": {},
            "semaphore_counts": {},
            "mutex_owners": {},
            "heap_free": 0,
            "heap_used": 0,
            "heap_fragmentation": 0,
            "msg_buffer_pending": 0,
            "msg_buffer_space": 0,
            "queue_set_ready": [],
            "context_switches": 0,
            "isr_time": 0,
        }
        
        self.task_timing_data = deque(maxlen=100)
        self.memory_timeline = deque(maxlen=100)
        
        self.setup_ui()
        self.start_serial_monitor()
        
    def setup_ui(self):
        main_container = ttk.Frame(self.root)
        main_container.pack(fill='both', expand=True, padx=5, pady=5)
        
        top_frame = ttk.Frame(main_container)
        top_frame.pack(fill='x', pady=(0, 5))
        self.setup_connection_panel(top_frame)
        
        content_frame = ttk.Frame(main_container)
        content_frame.pack(fill='both', expand=True)
        
        left_frame = ttk.Frame(content_frame, width=300)
        left_frame.pack(side='left', fill='y', padx=(0, 5))
        left_frame.pack_propagate(False)
        self.setup_program_selection(left_frame)
        
        center_frame = ttk.Frame(content_frame)
        center_frame.pack(side='left', fill='both', expand=True, padx=5)
        
        self.setup_rtos_monitor(center_frame)
        self.setup_test_control(center_frame)
        
        right_frame = ttk.Frame(content_frame, width=350)
        right_frame.pack(side='right', fill='y', padx=(5, 0))
        right_frame.pack_propagate(False)
        self.setup_performance_panel(right_frame)
        
        bottom_frame = ttk.Frame(main_container)
        bottom_frame.pack(fill='x', pady=(5, 0))
        self.setup_results_log(bottom_frame)
        
    def setup_connection_panel(self, parent):
        conn_frame = ttk.LabelFrame(parent, text="Koneksi Serial", padding=10)
        conn_frame.pack(fill='x')
        
        ttk.Label(conn_frame, text="Port:").pack(side='left', padx=(0, 5))
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.port_var, width=30, state='readonly')
        self.port_combo.pack(side='left', padx=(0, 10))
        self.refresh_ports()
        
        ttk.Button(conn_frame, text="Refresh", command=self.refresh_ports).pack(side='left', padx=(0, 10))
        
        ttk.Label(conn_frame, text="Baud Rate:").pack(side='left', padx=(0, 5))
        self.baud_var = tk.StringVar(value="115200")
        baud_combo = ttk.Combobox(conn_frame, textvariable=self.baud_var, width=10, 
                                   values=["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"],
                                   state='readonly')
        baud_combo.pack(side='left', padx=(0, 10))
        
        self.connect_btn = ttk.Button(conn_frame, text="Hubungkan", command=self.toggle_connection)
        self.connect_btn.pack(side='left', padx=(0, 10))
        
        self.connection_status = ttk.Label(conn_frame, text="Terputus", foreground="red")
        self.connection_status.pack(side='left')
        
    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        port_list = [f"{p.device} - {p.description}" for p in ports]
        self.port_combo['values'] = port_list
        if port_list:
            self.port_combo.set(port_list[0])
            
    def toggle_connection(self):
        if not self.is_connected:
            self.connect_serial()
        else:
            self.disconnect_serial()
            
    def connect_serial(self):
        port_str = self.port_var.get()
        if not port_str:
            messagebox.showerror("Error", "Pilih port serial terlebih dahulu")
            return
        port = port_str.split(' - ')[0]
        try:
            self.serial_port = serial.Serial(port, int(self.baud_var.get()), timeout=0.1)
            self.is_connected = True
            self.connect_btn.config(text="Putuskan")
            self.connection_status.config(text="Tersambung", foreground="green")
            self.log_message(f"Terhubung ke {port} pada baud rate {self.baud_var.get()}")
        except Exception as e:
            messagebox.showerror("Error", f"Gagal menghubungkan: {str(e)}")
            
    def disconnect_serial(self):
        if self.serial_port:
            self.serial_port.close()
            self.serial_port = None
        self.is_connected = False
        self.connect_btn.config(text="Hubungkan")
        self.connection_status.config(text="Terputus", foreground="red")
        self.log_message("Koneksi diputus")
        
    def setup_program_selection(self, parent):
        prog_frame = ttk.LabelFrame(parent, text="Daftar Program", padding=10)
        prog_frame.pack(fill='both', expand=True)
        
        self.tree = ttk.Treeview(prog_frame, show='tree', height=28)
        self.tree.pack(fill='both', expand=True, side='left')
        
        tree_scroll = ttk.Scrollbar(prog_frame, orient='vertical', command=self.tree.yview)
        tree_scroll.pack(side='right', fill='y')
        self.tree.configure(yscrollcommand=tree_scroll.set)
        
        for platform in ["STM32", "ESP32"]:
            platform_id = self.tree.insert('', 'end', text=platform, open=True)
            categories = {}
            for prog in self.program_data[platform]:
                cat = prog['category']
                if cat not in categories:
                    categories[cat] = self.tree.insert(platform_id, 'end', text=cat, open=True)
                prog_id = self.tree.insert(categories[cat], 'end', text=f"{prog['id']} - {prog['name']}", 
                                           tags=(prog['id'],))
                self.tree.item(prog_id, values=(json.dumps(prog),))
        
        self.tree.bind('<<TreeviewSelect>>', self.on_program_select)
        
        desc_frame = ttk.LabelFrame(parent, text="Deskripsi Program", padding=10)
        desc_frame.pack(fill='x', pady=(5, 0))
        self.desc_text = tk.Text(desc_frame, height=4, wrap='word', state='disabled')
        self.desc_text.pack(fill='both', expand=True)
        
    def on_program_select(self, event):
        selection = self.tree.selection()
        if selection:
            item = selection[0]
            values = self.tree.item(item, 'values')
            if values:
                prog_data = json.loads(values[0])
                self.desc_text.config(state='normal')
                self.desc_text.delete('1.0', 'end')
                self.desc_text.insert('1.0', f"ID: {prog_data['id']}\nKategori: {prog_data['category']}\n\n{prog_data['desc']}")
                self.desc_text.config(state='disabled')
                
    def setup_rtos_monitor(self, parent):
        monitor_frame = ttk.LabelFrame(parent, text="Monitor RTOS Advanced", padding=10)
        monitor_frame.pack(fill='both', expand=True, pady=(0, 5))
        
        nb = ttk.Notebook(monitor_frame)
        nb.pack(fill='both', expand=True)
        
        self.setup_event_group_tab(nb)
        self.setup_timer_tab(nb)
        self.setup_notification_tab(nb)
        self.setup_semaphore_tab(nb)
        self.setup_mutex_tab(nb)
        self.setup_memory_tab(nb)
        self.setup_msg_buffer_tab(nb)
        self.setup_queue_set_tab(nb)
        
    def setup_event_group_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Event Group")
        
        ttk.Label(tab, text="Status Bit Event Group", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        bits_frame = ttk.Frame(tab)
        bits_frame.pack(fill='x')
        
        self.event_checkboxes = []
        for i in range(8):
            cb_var = tk.BooleanVar()
            cb = ttk.Checkbutton(bits_frame, text=f"Bit {i}", variable=cb_var, 
                                  command=lambda i=i, v=cb_var: self.on_event_bit_change(i, v))
            cb.pack(side='left', padx=5)
            self.event_checkboxes.append((cb, cb_var))
            
        ttk.Separator(tab, orient='horizontal').pack(fill='x', pady=10)
        
        info_frame = ttk.Frame(tab)
        info_frame.pack(fill='x')
        
        self.event_value_label = ttk.Label(info_frame, text="Nilai Event Group: 0x00")
        self.event_value_label.pack(anchor='w')
        
        self.event_dec_label = ttk.Label(info_frame, text="Nilai Desimal: 0")
        self.event_dec_label.pack(anchor='w')
        
        ttk.Button(tab, text="Set Semua Bit", command=self.set_all_bits).pack(side='left', padx=(0, 5), pady=10)
        ttk.Button(tab, text="Clear Semua Bit", command=self.clear_all_bits).pack(side='left', pady=10)
        
    def on_event_bit_change(self, bit, var):
        if var.get():
            self.rtos_state['event_group_bits'] |= (1 << bit)
        else:
            self.rtos_state['event_group_bits'] &= ~(1 << bit)
        self.update_event_display()
        
    def update_event_display(self):
        val = self.rtos_state['event_group_bits']
        self.event_value_label.config(text=f"Nilai Event Group: 0x{val:02X}")
        self.event_dec_label.config(text=f"Nilai Desimal: {val}")
        
    def set_all_bits(self):
        self.rtos_state['event_group_bits'] = 0xFF
        for cb, var in self.event_checkboxes:
            var.set(True)
        self.update_event_display()
        
    def clear_all_bits(self):
        self.rtos_state['event_group_bits'] = 0
        for cb, var in self.event_checkboxes:
            var.set(False)
        self.update_event_display()
        
    def setup_timer_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Software Timer")
        
        ttk.Label(tab, text="Status Timer", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        columns = ('Timer', 'Status', 'Periode (ms)', 'Hitungan')
        self.timer_tree = ttk.Treeview(tab, columns=columns, show='headings', height=6)
        for col in columns:
            self.timer_tree.heading(col, text=col)
            self.timer_tree.column(col, width=100)
        self.timer_tree.pack(fill='x')
        
        for i in range(3):
            self.timer_tree.insert('', 'end', values=(f'Timer {i+1}', 'Stop', '1000', '0'))
            
    def setup_notification_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Task Notification")
        
        ttk.Label(tab, text="Nilai Notifikasi Task", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        columns = ('Task', 'Nilai', 'Notifikasi Pending')
        self.notif_tree = ttk.Treeview(tab, columns=columns, show='headings', height=6)
        for col in columns:
            self.notif_tree.heading(col, text=col)
            self.notif_tree.column(col, width=120)
        self.notif_tree.pack(fill='x')
        
        for i in range(4):
            self.notif_tree.insert('', 'end', values=(f'Task {i+1}', '0', '0'))
            
    def setup_semaphore_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Semaphores")
        
        ttk.Label(tab, text="Status Semaphore", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        columns = ('Semaphore', 'Tipe', 'Hitungan', 'Max')
        self.sem_tree = ttk.Treeview(tab, columns=columns, show='headings', height=4)
        for col in columns:
            self.sem_tree.heading(col, text=col)
            self.sem_tree.column(col, width=100)
        self.sem_tree.pack(fill='x')
        
        self.sem_tree.insert('', 'end', values=('Binary Sem', 'Binary', '1', '1'))
        self.sem_tree.insert('', 'end', values=('Counting Sem', 'Counting', '3', '5'))
        
        btn_frame = ttk.Frame(tab)
        btn_frame.pack(fill='x', pady=10)
        ttk.Button(btn_frame, text="Take", command=lambda: self.log_message("Semaphore: Take")).pack(side='left', padx=5)
        ttk.Button(btn_frame, text="Give", command=lambda: self.log_message("Semaphore: Give")).pack(side='left', padx=5)
        ttk.Button(btn_frame, text="Take From ISR", command=lambda: self.log_message("Semaphore: Take From ISR")).pack(side='left', padx=5)
        
    def setup_mutex_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Mutex")
        
        ttk.Label(tab, text="Status Mutex", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        info_frame = ttk.Frame(tab)
        info_frame.pack(fill='x', pady=5)
        
        ttk.Label(info_frame, text="Owner Task:").grid(row=0, column=0, sticky='w', padx=5)
        self.mutex_owner = ttk.Label(info_frame, text="None")
        self.mutex_owner.grid(row=0, column=1, sticky='w', padx=5)
        
        ttk.Label(info_frame, text="Recursion Count:").grid(row=1, column=0, sticky='w', padx=5)
        self.mutex_recursion = ttk.Label(info_frame, text="0")
        self.mutex_recursion.grid(row=1, column=1, sticky='w', padx=5)
        
        ttk.Label(tab, text="Task Terblokir:", font=('Arial', 9, 'bold')).pack(anchor='w', pady=(10, 5))
        
        self.blocked_list = tk.Listbox(tab, height=4)
        self.blocked_list.pack(fill='x')
        self.blocked_list.insert('end', "Tidak ada task terblokir")
        
    def setup_memory_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Memory")
        
        ttk.Label(tab, text="Statistik Heap", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        info_frame = ttk.Frame(tab)
        info_frame.pack(fill='x', pady=5)
        
        ttk.Label(info_frame, text="Heap Bebas:").grid(row=0, column=0, sticky='w', padx=5)
        self.heap_free_label = ttk.Label(info_frame, text="0 bytes")
        self.heap_free_label.grid(row=0, column=1, sticky='w', padx=5)
        
        ttk.Label(info_frame, text="Heap Terpakai:").grid(row=1, column=0, sticky='w', padx=5)
        self.heap_used_label = ttk.Label(info_frame, text="0 bytes")
        self.heap_used_label.grid(row=1, column=1, sticky='w', padx=5)
        
        ttk.Label(info_frame, text="Total Heap:").grid(row=2, column=0, sticky='w', padx=5)
        self.heap_total_label = ttk.Label(info_frame, text="0 bytes")
        self.heap_total_label.grid(row=2, column=1, sticky='w', padx=5)
        
        ttk.Label(info_frame, text="Fragmentasi:").grid(row=3, column=0, sticky='w', padx=5)
        self.frag_label = ttk.Label(info_frame, text="0%")
        self.frag_label.grid(row=3, column=1, sticky='w', padx=5)
        
        ttk.Separator(tab, orient='horizontal').pack(fill='x', pady=10)
        
        alloc_frame = ttk.Frame(tab)
        alloc_frame.pack(fill='x')
        ttk.Label(alloc_frame, text="Alokasi Terbesar:").pack(side='left', padx=5)
        ttk.Label(alloc_frame, text="0 bytes").pack(side='left')
        
    def setup_msg_buffer_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Message Buffer")
        
        ttk.Label(tab, text="Status Message Buffer", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        info_frame = ttk.Frame(tab)
        info_frame.pack(fill='x', pady=5)
        
        ttk.Label(info_frame, text="Pesan Pending:").grid(row=0, column=0, sticky='w', padx=5)
        self.msg_pending = ttk.Label(info_frame, text="0")
        self.msg_pending.grid(row=0, column=1, sticky='w', padx=5)
        
        ttk.Label(info_frame, text="Ruang Tersedia:").grid(row=1, column=0, sticky='w', padx=5)
        self.msg_space = ttk.Label(info_frame, text="0 bytes")
        self.msg_space.grid(row=1, column=1, sticky='w', padx=5)
        
        ttk.Label(info_frame, text="Ukuran Buffer:").grid(row=2, column=0, sticky='w', padx=5)
        ttk.Label(info_frame, text="1024 bytes").grid(row=2, column=1, sticky='w', padx=5)
        
        ttk.Label(tab, text="Pesan Terakhir:", font=('Arial', 9, 'bold')).pack(anchor='w', pady=(10, 5))
        self.last_msg = tk.Text(tab, height=3, wrap='word', state='disabled')
        self.last_msg.pack(fill='x')
        
    def setup_queue_set_tab(self, notebook):
        tab = ttk.Frame(notebook, padding=10)
        notebook.add(tab, text="Queue Set")
        
        ttk.Label(tab, text="Status Queue Set", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(0, 10))
        
        columns = ('Queue', 'Tipe', 'Jumlah', 'Status')
        self.queue_tree = ttk.Treeview(tab, columns=columns, show='headings', height=4)
        for col in columns:
            self.queue_tree.heading(col, text=col)
            self.queue_tree.column(col, width=100)
        self.queue_tree.pack(fill='x')
        
        self.queue_tree.insert('', 'end', values=('Queue 1', 'Int', '0/10', 'Empty'))
        self.queue_tree.insert('', 'end', values=('Queue 2', 'Float', '0/10', 'Empty'))
        self.queue_tree.insert('', 'end', values=('Queue 3', 'String', '0/10', 'Empty'))
        
        ttk.Label(tab, text="Queue Siap:", font=('Arial', 9, 'bold')).pack(anchor='w', pady=(10, 5))
        self.ready_list = tk.Listbox(tab, height=3)
        self.ready_list.pack(fill='x')
        self.ready_list.insert('end', "Tidak ada queue siap")
        
    def setup_test_control(self, parent):
        control_frame = ttk.LabelFrame(parent, text="Kontrol Pengujian", padding=10)
        control_frame.pack(fill='x', pady=5)
        
        btn_frame = ttk.Frame(control_frame)
        btn_frame.pack(fill='x', pady=(0, 10))
        
        self.run_selected_btn = ttk.Button(btn_frame, text="Jalankan Terpilih", command=self.run_selected_test)
        self.run_selected_btn.pack(side='left', padx=5)
        
        self.run_all_btn = ttk.Button(btn_frame, text="Jalankan Semua", command=self.run_all_tests)
        self.run_all_btn.pack(side='left', padx=5)
        
        self.stop_btn = ttk.Button(btn_frame, text="Hentikan", command=self.stop_test, state='disabled')
        self.stop_btn.pack(side='left', padx=5)
        
        ttk.Separator(control_frame, orient='horizontal').pack(fill='x', pady=5)
        
        progress_frame = ttk.Frame(control_frame)
        progress_frame.pack(fill='x')
        
        ttk.Label(progress_frame, text="Progress:").pack(side='left', padx=(0, 5))
        self.progress = ttk.Progressbar(progress_frame, mode='determinate', maximum=20)
        self.progress.pack(side='left', fill='x', expand=True, padx=5)
        self.progress_label = ttk.Label(progress_frame, text="0/20")
        self.progress_label.pack(side='left', padx=5)
        
        self.current_test_label = ttk.Label(control_frame, text="Test saat ini: -")
        self.current_test_label.pack(anchor='w', pady=(5, 0))
        
    def run_selected_test(self):
        selection = self.tree.selection()
        if not selection:
            messagebox.showwarning("Peringatan", "Pilih program terlebih dahulu")
            return
        self.test_running = True
        self.stop_btn.config(state='normal')
        self.log_message("Memulai pengujian program terpilih...")
        
    def run_all_tests(self):
        self.test_running = True
        self.stop_btn.config(state='normal')
        self.progress['value'] = 0
        self.log_message("Memulai pengujian semua program (20 program)...")
        threading.Thread(target=self.run_all_tests_thread, daemon=True).start()
        
    def run_all_tests_thread(self):
        for i, platform in enumerate(["STM32", "ESP32"]):
            for j, prog in enumerate(self.program_data[platform]):
                if not self.test_running:
                    break
                self.current_test_label.config(text=f"Test saat ini: {prog['id']} - {prog['name']}")
                self.log_message(f"Menjalankan: {prog['id']} - {prog['name']}")
                self.progress['value'] = i * 10 + j + 1
                self.progress_label.config(text=f"{i * 10 + j + 1}/20")
                self.parse_rtos_output(prog['id'])
                time.sleep(0.5)
        self.test_running = False
        self.stop_btn.config(state='disabled')
        self.current_test_label.config(text="Test saat ini: -")
        self.log_message("Pengujian selesai!")
        
    def stop_test(self):
        self.test_running = False
        self.stop_btn.config(state='disabled')
        self.log_message("Pengujian dihentikan")
        
    def setup_performance_panel(self, parent):
        perf_frame = ttk.LabelFrame(parent, text="Panel Performa", padding=10)
        perf_frame.pack(fill='both', expand=True)
        
        nb = ttk.Notebook(perf_frame)
        nb.pack(fill='both', expand=True)
        
        timing_tab = ttk.Frame(nb, padding=5)
        nb.add(timing_tab, text="Timing Task")
        
        self.fig1 = Figure(figsize=(3.5, 2.5), dpi=100)
        self.ax1 = self.fig1.add_subplot(111)
        self.ax1.set_title("Waktu Eksekusi Task")
        self.ax1.set_xlabel("Waktu")
        self.ax1.set_ylabel("ms")
        self.canvas1 = FigureCanvasTkAgg(self.fig1, timing_tab)
        self.canvas1.get_tk_widget().pack(fill='both', expand=True)
        
        stats_tab = ttk.Frame(nb, padding=5)
        nb.add(stats_tab, text="Statistik")
        
        stats_frame = ttk.Frame(stats_tab)
        stats_frame.pack(fill='both', expand=True)
        
        ttk.Label(stats_frame, text="Context Switch:", font=('Arial', 9, 'bold')).grid(row=0, column=0, sticky='w', padx=5, pady=2)
        self.ctx_switch_label = ttk.Label(stats_frame, text="0")
        self.ctx_switch_label.grid(row=0, column=1, sticky='w', padx=5, pady=2)
        
        ttk.Label(stats_frame, text="ISR Execution Time:", font=('Arial', 9, 'bold')).grid(row=1, column=0, sticky='w', padx=5, pady=2)
        self.isr_time_label = ttk.Label(stats_frame, text="0 µs")
        self.isr_time_label.grid(row=1, column=1, sticky='w', padx=5, pady=2)
        
        ttk.Label(stats_frame, text="Task Switches:", font=('Arial', 9, 'bold')).grid(row=2, column=0, sticky='w', padx=5, pady=2)
        ttk.Label(stats_frame, text="0").grid(row=2, column=1, sticky='w', padx=5, pady=2)
        
        ttk.Label(stats_frame, text="CPU Usage:", font=('Arial', 9, 'bold')).grid(row=3, column=0, sticky='w', padx=5, pady=2)
        ttk.Label(stats_frame, text="0%").grid(row=3, column=1, sticky='w', padx=5, pady=2)
        
        ttk.Separator(stats_frame, orient='horizontal').grid(row=4, column=0, columnspan=2, sticky='ew', pady=10)
        
        ttk.Label(stats_frame, text="Task Aktif:", font=('Arial', 9, 'bold')).grid(row=5, column=0, sticky='w', padx=5, pady=2)
        ttk.Label(stats_frame, text="4").grid(row=5, column=1, sticky='w', padx=5, pady=2)
        
        mem_tab = ttk.Frame(nb, padding=5)
        nb.add(mem_tab, text="Memory Timeline")
        
        self.fig2 = Figure(figsize=(3.5, 2.5), dpi=100)
        self.ax2 = self.fig2.add_subplot(111)
        self.ax2.set_title("Alokasi Memori")
        self.ax2.set_xlabel("Waktu")
        self.ax2.set_ylabel("Bytes")
        self.canvas2 = FigureCanvasTkAgg(self.fig2, mem_tab)
        self.canvas2.get_tk_widget().pack(fill='both', expand=True)
        
    def setup_results_log(self, parent):
        log_frame = ttk.LabelFrame(parent, text="Hasil & Log", padding=10)
        log_frame.pack(fill='both', expand=True)
        
        nb = ttk.Notebook(log_frame)
        nb.pack(fill='both', expand=True)
        
        results_tab = ttk.Frame(nb, padding=5)
        nb.add(results_tab, text="Hasil Test")
        
        columns = ('Program', 'Status', 'Waktu', 'Keterangan')
        self.result_tree = ttk.Treeview(results_tab, columns=columns, show='headings', height=6)
        for col in columns:
            self.result_tree.heading(col, text=col)
            self.result_tree.column(col, width=150)
        self.result_tree.pack(fill='both', expand=True, side='left')
        
        result_scroll = ttk.Scrollbar(results_tab, orient='vertical', command=self.result_tree.yview)
        result_scroll.pack(side='right', fill='y')
        self.result_tree.configure(yscrollcommand=result_scroll.set)
        
        log_tab = ttk.Frame(nb, padding=5)
        nb.add(log_tab, text="Log Detail")
        
        self.log_text = scrolledtext.ScrolledText(log_tab, height=8, wrap='word', state='disabled')
        self.log_text.pack(fill='both', expand=True)
        
        export_frame = ttk.Frame(log_frame)
        export_frame.pack(fill='x', pady=(5, 0))
        
        ttk.Button(export_frame, text="Export Hasil (CSV)", command=self.export_csv).pack(side='left', padx=5)
        ttk.Button(export_frame, text="Export Log (TXT)", command=self.export_log).pack(side='left', padx=5)
        ttk.Button(export_frame, text="Clear Log", command=self.clear_log).pack(side='right', padx=5)
        
    def log_message(self, msg):
        timestamp = datetime.now().strftime("%H:%M:%S")
        full_msg = f"[{timestamp}] {msg}\n"
        self.log_text.config(state='normal')
        self.log_text.insert('end', full_msg)
        self.log_text.see('end')
        self.log_text.config(state='disabled')
        
    def export_csv(self):
        filename = filedialog.asksaveasfilename(defaultextension=".csv", filetypes=[("CSV files", "*.csv")])
        if filename:
            with open(filename, 'w') as f:
                f.write("Program,Status,Waktu,Keterangan\n")
                for item in self.result_tree.get_children():
                    values = self.result_tree.item(item)['values']
                    f.write(",".join(map(str, values)) + "\n")
            self.log_message(f"Hasil diekspor ke {filename}")
            
    def export_log(self):
        filename = filedialog.asksaveasfilename(defaultextension=".txt", filetypes=[("Text files", "*.txt")])
        if filename:
            with open(filename, 'w') as f:
                f.write(self.log_text.get('1.0', 'end'))
            self.log_message(f"Log diekspor ke {filename}")
            
    def clear_log(self):
        self.log_text.config(state='normal')
        self.log_text.delete('1.0', 'end')
        self.log_text.config(state='disabled')
        
    def start_serial_monitor(self):
        threading.Thread(target=self.read_serial, daemon=True).start()
        self.root.after(100, self.process_serial_data)
        
    def read_serial(self):
        while True:
            if self.serial_port and self.serial_port.is_open:
                try:
                    data = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                    if data:
                        self.data_queue.put(data)
                except:
                    pass
            time.sleep(0.01)
            
    def process_serial_data(self):
        try:
            while True:
                data = self.data_queue.get_nowait()
                self.parse_rtos_output(data)
                self.log_message(f"RX: {data}")
        except queue.Empty:
            pass
        self.root.after(100, self.process_serial_data)
        
    def parse_rtos_output(self, data):
        if "EventGroup" in data or "event" in data.lower():
            match = re.search(r'0x([0-9A-Fa-f]{2})', data)
            if match:
                val = int(match.group(1), 16)
                self.rtos_state['event_group_bits'] = val
                for i, (cb, var) in enumerate(self.event_checkboxes):
                    var.set(bool(val & (1 << i)))
                self.update_event_display()
                
        elif "Timer" in data:
            match = re.search(r'Timer.*?(\d+).*?fired.*?(\d+)', data)
            if match:
                self.log_message(f"Timer {match.group(1)} fired, count: {match.group(2)}")
                
        elif "Notification" in data or "notify" in data.lower():
            match = re.search(r'value[:\s]+(\d+)', data)
            if match:
                self.log_message(f"Notification value: {match.group(1)}")
                
        elif "Semaphore" in data:
            if "take" in data.lower():
                self.log_message("Semaphore di-take")
            elif "give" in data.lower():
                self.log_message("Semaphore di-give")
                
        elif "Heap" in data or "memory" in data.lower():
            match_free = re.search(r'free[:\s]+(\d+)', data)
            match_used = re.search(r'used[:\s]+(\d+)', data)
            if match_free:
                self.heap_free_label.config(text=f"{match_free.group(1)} bytes")
            if match_used:
                self.heap_used_label.config(text=f"{match_used.group(1)} bytes")
                
        elif "Message" in data or "msg" in data.lower():
            self.log_message(f"Message: {data}")
            
        elif "Context switch" in data.lower():
            match = re.search(r'(\d+)', data)
            if match:
                self.rtos_state['context_switches'] = int(match.group(1))
                self.ctx_switch_label.config(text=str(self.rtos_state['context_switches']))
                
    def update_graphs(self):
        if len(self.task_timing_data) > 0:
            self.ax1.clear()
            self.ax1.plot(list(self.task_timing_data))
            self.ax1.set_title("Waktu Eksekusi Task")
            self.ax1.set_xlabel("Waktu")
            self.ax1.set_ylabel("ms")
            self.canvas1.draw()
            
        if len(self.memory_timeline) > 0:
            self.ax2.clear()
            self.ax2.plot(list(self.memory_timeline))
            self.ax2.set_title("Alokasi Memori")
            self.ax2.set_xlabel("Waktu")
            self.ax2.set_ylabel("Bytes")
            self.canvas2.draw()
            
    def on_closing(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = FreeRTOS_GUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()
