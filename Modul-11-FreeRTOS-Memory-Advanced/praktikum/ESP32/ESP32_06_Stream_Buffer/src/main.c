/*
 * ESP32_06_Stream_Buffer
 * xStreamBufferCreate() for variable-length byte streams.
 * Producer writes sensor data bytes, consumer reads.
 * Trigger level demonstration.  LED on GPIO2 shows activity.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

static const char *TAG = "STREAM_BUF";

#define LED_PIN             GPIO_NUM_2
#define STREAM_BUF_SIZE     256
#define TRIGGER_LEVEL       16   /* consumer unblocks when ≥16 bytes available */

static StreamBufferHandle_t xStream;

/* ------------------------------------------------------------------ */
/*  Simulated sensor data producer                                    */
/* ------------------------------------------------------------------ */
static void producer_task(void *pv)
{
    uint8_t seq = 0;
    while (1) {
        /* Variable-length write: 1-8 bytes per iteration */
        uint8_t len = (seq % 8) + 1;
        uint8_t buf[8];
        for (int i = 0; i < len; i++) buf[i] = seq + i;

        size_t sent = xStreamBufferSend(xStream, buf, len, pdMS_TO_TICKS(100));

        size_t avail = xStreamBufferBytesAvailable(xStream);
        size_t space = xStreamBufferSpacesAvailable(xStream);

        printf("[PROD] Sent %u/%u bytes (seq=%u)  buf_used=%u  buf_free=%u\n",
               (unsigned)sent, len, seq,
               (unsigned)avail, (unsigned)space);

        seq++;
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* ------------------------------------------------------------------ */
/*  Consumer — blocks until trigger level bytes available             */
/* ------------------------------------------------------------------ */
static void consumer_task(void *pv)
{
    uint8_t buf[64];
    while (1) {
        /* Will block until at least TRIGGER_LEVEL bytes in buffer */
        size_t received = xStreamBufferReceive(xStream, buf, sizeof(buf),
                                                pdMS_TO_TICKS(5000));
        if (received > 0) {
            printf("[CONS] Received %u bytes: ", (unsigned)received);
            for (size_t i = 0; i < received && i < 16; i++) {
                printf("%02X ", buf[i]);
            }
            if (received > 16) printf("…");
            printf("\n");
        } else {
            printf("[CONS] Timeout — no data for 5 s\n");
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Stats task                                                        */
/* ------------------------------------------------------------------ */
static void stats_task(void *pv)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));

        size_t avail = xStreamBufferBytesAvailable(xStream);
        size_t space = xStreamBufferSpacesAvailable(xStream);
        bool   full  = xStreamBufferIsFull(xStream);
        bool   empty = xStreamBufferIsEmpty(xStream);

        printf("[STATS] Stream: used=%u  free=%u  full=%d  empty=%d  trigger=%d\n",
               (unsigned)avail, (unsigned)space, full, empty, TRIGGER_LEVEL);
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Stream Buffer Demo ===");
    ESP_LOGI(TAG, "Buffer size=%d  Trigger level=%d", STREAM_BUF_SIZE, TRIGGER_LEVEL);

    /* LED */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io);
    gpio_set_level(LED_PIN, 0);

    /* Create stream buffer with trigger level */
    xStream = xStreamBufferCreate(STREAM_BUF_SIZE, TRIGGER_LEVEL);
    if (!xStream) {
        ESP_LOGE(TAG, "Failed to create stream buffer!");
        return;
    }

    xTaskCreate(producer_task, "producer", 2048, NULL, 5, NULL);
    xTaskCreate(consumer_task, "consumer", 2048, NULL, 4, NULL);
    xTaskCreate(stats_task,    "stats",    2048, NULL, 3, NULL);
}
