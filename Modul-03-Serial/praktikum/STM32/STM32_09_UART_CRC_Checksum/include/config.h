#ifndef CONFIG_H
#define CONFIG_H

/* CRC-8 Parameters */
#define CRC_POLY        0x07
#define CRC_INIT        0x00

/* UART Configuration */
#define UART_BAUD       115200

/* Buffer Sizes */
#define MAX_DATA        128

/* Protocol Format: [LENGTH][DATA...][CRC8] */
/* LENGTH = number of data bytes (not including LENGTH or CRC) */

#endif /* CONFIG_H */
