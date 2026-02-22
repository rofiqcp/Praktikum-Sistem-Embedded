# Referensi Modul 11: FreeRTOS Timer & Notification

## 📚 Dokumentasi Resmi FreeRTOS
1.  **FreeRTOS Software Timer API Reference**
    *   [https://www.freertos.org/FreeRTOS-Software-Timer-API-Functions.html](https://www.freertos.org/FreeRTOS-Software-Timer-API-Functions.html)
    *   Penjelasan detail mengenai `xTimerCreate`, `xTimerStart`, `xTimerStop`, dan callback prototype.

2.  **FreeRTOS Task Notifications**
    *   [https://www.freertos.org/RTOS-task-notifications.html](https://www.freertos.org/RTOS-task-notifications.html)
    *   Panduan lengkap tentang *Unblocking a task*, *Passing data*, dan *Updating state* menggunakan notifikasi.

3.  **Using FreeRTOS on STM32 (CubeIDE)**
    *   Manual STM32CubeIDE untuk integrasi FreeRTOS.
    *   [ST Microelectronics Wiki - FreeRTOS](https://wiki.st.com/stm32mcu/wiki/STM32CubeIDE:FreeRTOS)

## 📖 Buku & Artikel
1.  **"Mastering the FreeRTOS Real Time Kernel"** (Official Book)
    *   Chapter 5: Software Timer.
    *   Chapter 9: Task Notifications.
    *   *Sangat direkomendasikan untuk pemahaman mendalam.*

2.  **Artikel: "RTOS Task Notifications: The Better Binary Semaphore?"**
    *   Membahas keunggulan performa (RAM & Speed) notification dibandingkan semaphore untuk sinkronisasi ISR-to-Task.

## 🎥 Video Tutorial
1.  **Digi-Key Introduction to RTOS Part 10 - Software Timers**
    *   Tutorial visual cara kerja Software Timer dan Daemon Task.
2.  **Learn Embedded Systems - FreeRTOS Task Notifications**
    *   Contoh implementasi praktis notifikasi sebagai pengganti Queue dan Semaphore.
