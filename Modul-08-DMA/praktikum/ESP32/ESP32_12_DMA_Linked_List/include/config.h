/*
 * ==========================================================================
 *  ESP32 DMA Scatter-Gather / Linked List Simulation - Configuration
 * ==========================================================================
 *  Modul 08 - Program 12: Scatter-Gather Pattern
 *
 *  CATATAN:
 *  - STM32 memiliki DMA linked-list (scatter-gather) mode di beberapa seri
 *  - ESP32 SPI DMA mendukung linked-list transfer secara internal
 *  - Program ini mensimulasikan scatter-gather pattern menggunakan
 *    SPI queued transactions sebagai analogi
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Scatter-Gather Configuration ---- */
#define NUM_SEGMENTS            4       /* Number of scatter-gather segments */

/* Segment sizes (variable, non-uniform) */
#define SEG_HEADER_SIZE         16      /* Packet header */
#define SEG_PAYLOAD1_SIZE       128     /* First payload */
#define SEG_PAYLOAD2_SIZE       256     /* Second payload */
#define SEG_CHECKSUM_SIZE       4       /* CRC/checksum */

/* Total packet size */
#define TOTAL_PACKET_SIZE       (SEG_HEADER_SIZE + SEG_PAYLOAD1_SIZE + \
                                 SEG_PAYLOAD2_SIZE + SEG_CHECKSUM_SIZE)

/* ---- SPI Configuration ---- */
#define SPI_HOST_ID             SPI2_HOST
#define SPI_MOSI_PIN            23
#define SPI_MISO_PIN            19
#define SPI_CLK_PIN             18
#define SPI_CS_PIN              5
#define SPI_CLOCK_HZ            10000000    /* 10 MHz */

/* ---- Test Configuration ---- */
#define TEST_ITERATIONS         100     /* Iterations per method */
#define NUM_PACKETS             10      /* Number of packets to build and send */

/* ---- Protocol Packet Configuration ---- */
#define PACKET_MAGIC            0xABCD  /* Packet magic number */
#define PACKET_VERSION          0x01    /* Protocol version */
#define CRC_POLYNOMIAL          0xEDB88320  /* CRC32 polynomial */

/* ---- Task Configuration ---- */
#define MAIN_TASK_STACK         8192
#define MAIN_TASK_PRIO          5

#endif /* CONFIG_H */
