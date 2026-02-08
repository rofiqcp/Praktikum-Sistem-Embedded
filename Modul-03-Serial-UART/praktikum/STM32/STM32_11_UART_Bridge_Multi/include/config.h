#ifndef CONFIG_H
#define CONFIG_H

/* UART Baud Rates */
#define UART1_BAUD      115200   /* PC/terminal side */
#define UART2_BAUD      9600     /* External device side */

/* Buffer Size */
#define BRIDGE_BUF_SIZE 256

/* Status Report Interval */
#define REPORT_INTERVAL_MS  10000  /* Print stats every 10 seconds */

/* USART1: PA9(TX), PA10(RX) — connected to PC/terminal */
/* USART2: PA2(TX), PA3(RX)  — connected to other device */

#endif /* CONFIG_H */
