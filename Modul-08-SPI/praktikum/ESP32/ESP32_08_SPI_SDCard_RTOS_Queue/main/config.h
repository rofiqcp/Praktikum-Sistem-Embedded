#ifndef CONFIG_H
#define CONFIG_H

// SD Card SPI Configuration
#define SD_MOSI_PIN     23
#define SD_MISO_PIN     19
#define SD_SCLK_PIN     18
#define SD_CS_PIN       5

// FreeRTOS Configuration
#define WRITER_TASK_PRIORITY    5
#define READER_TASK_PRIORITY    4
#define LOGGER_TASK_PRIORITY    3
#define MONITOR_TASK_PRIORITY   2

#define TASK_STACK_SIZE         4096

// Queue Configuration
#define WRITE_QUEUE_SIZE        20
#define READ_QUEUE_SIZE         10
#define LOG_QUEUE_SIZE          50

// Operation types
#define OP_WRITE                1
#define OP_READ                 2
#define OP_DELETE               3
#define OP_LIST                 4

#endif // CONFIG_H
