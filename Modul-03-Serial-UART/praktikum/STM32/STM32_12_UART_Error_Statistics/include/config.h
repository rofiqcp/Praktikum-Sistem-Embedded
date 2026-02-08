#ifndef CONFIG_H
#define CONFIG_H

/* UART Configuration */
#define UART_BAUD           115200

/* Report Interval */
#define REPORT_INTERVAL_MS  5000

/* Error Thresholds for Health Status */
#define WARN_THRESHOLD      1    /* >= 1 total errors = WARNING */
#define CRIT_THRESHOLD      5   /* > 5 total errors = CRITICAL */

/* UART with parity: WordLength=9B (8 data + 1 parity) */
/* Parity: EVEN */

#endif /* CONFIG_H */
