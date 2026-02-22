/*
 * ESP32_07_Message_Buffer
 * xMessageBufferCreate() for discrete messages.
 * Multiple producers send different message types.
 * Consumer receives complete messages with length prefix.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "MSG_BUF";

#define MSG_BUF_SIZE  512

static MessageBufferHandle_t xMsgBuf;

/* ------------------------------------------------------------------ */
/*  Message types                                                     */
/* ------------------------------------------------------------------ */
typedef enum {
    MSG_TYPE_SENSOR = 0x01,
    MSG_TYPE_EVENT  = 0x02,
    MSG_TYPE_CMD    = 0x03,
} msg_type_t;

typedef struct {
    msg_type_t type;
    uint32_t   timestamp;
    uint16_t   value;
    char       text[32];
} message_t;

/* ------------------------------------------------------------------ */
/*  Producer: Sensor data                                             */
/* ------------------------------------------------------------------ */
static void sensor_producer(void *pv)
{
    uint32_t seq = 0;
    while (1) {
        message_t msg;
        msg.type      = MSG_TYPE_SENSOR;
        msg.timestamp = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        msg.value     = 200 + (seq * 7) % 800;  /* simulated sensor */
        snprintf(msg.text, sizeof(msg.text), "SENSOR-%lu", (unsigned long)seq);

        size_t sent = xMessageBufferSend(xMsgBuf, &msg, sizeof(msg), pdMS_TO_TICKS(200));
        printf("[SENSOR]  Sent msg #%lu  val=%u  bytes=%u\n",
               (unsigned long)seq, msg.value, (unsigned)sent);
        seq++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ------------------------------------------------------------------ */
/*  Producer: Event messages                                          */
/* ------------------------------------------------------------------ */
static void event_producer(void *pv)
{
    uint32_t seq = 0;
    const char *events[] = {"BOOT", "WIFI_OK", "TICK", "ALARM", "RESET"};

    while (1) {
        message_t msg;
        msg.type      = MSG_TYPE_EVENT;
        msg.timestamp = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        msg.value     = seq;
        snprintf(msg.text, sizeof(msg.text), "%s", events[seq % 5]);

        size_t sent = xMessageBufferSend(xMsgBuf, &msg, sizeof(msg), pdMS_TO_TICKS(200));
        printf("[EVENT]   Sent event \"%s\"  bytes=%u\n", msg.text, (unsigned)sent);
        seq++;
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

/* ------------------------------------------------------------------ */
/*  Producer: Command messages (short)                                */
/* ------------------------------------------------------------------ */
static void cmd_producer(void *pv)
{
    uint32_t seq = 0;
    while (1) {
        /* Send only partial struct — demonstrates variable-length messages */
        struct {
            msg_type_t type;
            uint32_t   cmd_id;
        } cmd;
        cmd.type   = MSG_TYPE_CMD;
        cmd.cmd_id = seq;

        size_t sent = xMessageBufferSend(xMsgBuf, &cmd, sizeof(cmd), pdMS_TO_TICKS(200));
        printf("[CMD]     Sent cmd_id=%lu  bytes=%u (short msg)\n",
               (unsigned long)seq, (unsigned)sent);
        seq++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ------------------------------------------------------------------ */
/*  Consumer — receives complete discrete messages                    */
/* ------------------------------------------------------------------ */
static void consumer_task(void *pv)
{
    uint8_t buf[sizeof(message_t) + 16];
    uint32_t msg_count = 0;

    while (1) {
        size_t received = xMessageBufferReceive(xMsgBuf, buf, sizeof(buf),
                                                 pdMS_TO_TICKS(3000));
        if (received == 0) {
            printf("[CONS]    Timeout — no message for 3 s\n");
            continue;
        }

        msg_count++;
        msg_type_t type = *(msg_type_t *)buf;

        switch (type) {
        case MSG_TYPE_SENSOR: {
            message_t *m = (message_t *)buf;
            printf("[CONS]    #%lu SENSOR  val=%u  text=\"%s\"  t=%lu ms  (%u bytes)\n",
                   (unsigned long)msg_count, m->value, m->text,
                   (unsigned long)m->timestamp, (unsigned)received);
            break;
        }
        case MSG_TYPE_EVENT: {
            message_t *m = (message_t *)buf;
            printf("[CONS]    #%lu EVENT   \"%s\"  t=%lu ms  (%u bytes)\n",
                   (unsigned long)msg_count, m->text,
                   (unsigned long)m->timestamp, (unsigned)received);
            break;
        }
        case MSG_TYPE_CMD: {
            uint32_t cmd_id;
            memcpy(&cmd_id, buf + sizeof(msg_type_t), sizeof(uint32_t));
            printf("[CONS]    #%lu CMD     id=%lu  (%u bytes — short)\n",
                   (unsigned long)msg_count, (unsigned long)cmd_id,
                   (unsigned)received);
            break;
        }
        default:
            printf("[CONS]    #%lu UNKNOWN type=0x%02X  (%u bytes)\n",
                   (unsigned long)msg_count, type, (unsigned)received);
        }

        /* Buffer space info */
        size_t space = xMessageBufferSpacesAvailable(xMsgBuf);
        printf("[CONS]    Buffer space available: %u bytes\n", (unsigned)space);
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Message Buffer Demo ===");
    ESP_LOGI(TAG, "Buffer size=%d  Message struct=%u bytes",
             MSG_BUF_SIZE, (unsigned)sizeof(message_t));

    xMsgBuf = xMessageBufferCreate(MSG_BUF_SIZE);
    if (!xMsgBuf) {
        ESP_LOGE(TAG, "Failed to create message buffer!");
        return;
    }

    xTaskCreate(sensor_producer, "sensor_prod", 2048, NULL, 5, NULL);
    xTaskCreate(event_producer,  "event_prod",  2048, NULL, 5, NULL);
    xTaskCreate(cmd_producer,    "cmd_prod",    2048, NULL, 5, NULL);
    xTaskCreate(consumer_task,   "consumer",    3072, NULL, 4, NULL);
}
