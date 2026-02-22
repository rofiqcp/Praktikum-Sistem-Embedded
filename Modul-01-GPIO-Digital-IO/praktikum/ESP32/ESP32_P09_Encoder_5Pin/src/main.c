/**
 * @file    main.c
 * @brief   P09 — Twist & Count: Rotary Encoder Kuadratur & Counter
 *
 * ================================================================
 * KONSEP: ROTARY ENCODER QUADRATURE DECODING
 * ================================================================
 *
 * Encoder 5-pin (KY-040):
 *   GND → GND,  VCC → 3.3V
 *   CLK (A) → GPIO25  (INPUT, PULLUP)
 *   DT  (B) → GPIO26  (INPUT, PULLUP)
 *   SW      → GPIO27  (INPUT, PULLUP, active-low push-button)
 *
 * DECODING KUADRATUR:
 *   Deteksi FALLING EDGE pada CLK, baca DT:
 *   CLK ↓ saat DT=1 → Clockwise      → count++
 *   CLK ↓ saat DT=0 → Counter-CW     → count--
 *
 * OUTPUT: 8 LED menampilkan count (0-255) dalam biner
 *   LED[0]=GPIO4 = bit0 (LSB), LED[7]=GPIO18 = bit7 (MSB)
 *   Tekan SW → reset count ke 128
 *
 * ================================================================
 * RANGKAIAN:
 *   Encoder GND → GND
 *   Encoder VCC → 3.3V
 *   Encoder CLK → GPIO25 (INPUT PULLUP)
 *   Encoder DT  → GPIO26 (INPUT PULLUP)
 *   Encoder SW  → GPIO27 (INPUT PULLUP)
 *   GPIO4─[220Ω]─LED1─GND  ... GPIO18─[220Ω]─LED8─GND
 *
 * PLATFORM : ESP32 DevKit V1
 * FRAMEWORK: ESP-IDF (FreeRTOS)
 * ================================================================
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define ENC_CLK   GPIO_NUM_25   /* Channel A */
#define ENC_DT    GPIO_NUM_26   /* Channel B */
#define ENC_SW    GPIO_NUM_27   /* Push switch, active-low */

#define NUM_LEDS  8
static const gpio_num_t LED[NUM_LEDS] = {
    GPIO_NUM_4,  GPIO_NUM_5,  GPIO_NUM_13, GPIO_NUM_14,
    GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18
};

/* ================================================================
 *  ENCODER STATE
 * ================================================================ */
static int     enc_count     = 128;   /* Range 0-255, start di tengah */
static int     last_clk      = 1;
static int     last_sw       = 1;
static int64_t last_enc_time = 0;
static int64_t last_sw_time  = 0;

#define ENC_DEBOUNCE_US   5000LL    /* 5ms debounce encoder */
#define SW_DEBOUNCE_US   50000LL    /* 50ms debounce switch */

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* 8 LEDs: OUTPUT */
    uint64_t led_mask = 0;
    for (int i = 0; i < NUM_LEDS; i++)
        led_mask |= (1ULL << LED[i]);

    gpio_config_t out_conf = {
        .pin_bit_mask = led_mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);

    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], 0);

    /* CLK, DT, SW: INPUT + PULLUP */
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL<<ENC_CLK)|(1ULL<<ENC_DT)|(1ULL<<ENC_SW),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);
}

/* ================================================================
 *  LED COUNT DISPLAY (biner, atomik via bit manipulation)
 * ================================================================ */
static void led_show_count(void)
{
    uint8_t val = (uint8_t)(enc_count & 0xFF);

    /* ESP32 atomic GPIO set/clear via register:
     * GPIO.out_w1ts = set mask (bit HIGH)
     * GPIO.out_w1tc = clear mask (bit LOW)
     * Tapi untuk kompatibilitas, gunakan loop gpio_set_level */
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], (val >> i) & 1);
}

/* ================================================================
 *  ENCODER READ
 * ================================================================ */
static void encoder_read(void)
{
    int64_t now = esp_timer_get_time();

    /* ---- CLK: Deteksi Falling Edge ---- */
    int clk = gpio_get_level(ENC_CLK);
    int dt  = gpio_get_level(ENC_DT);

    /* Falling edge: sebelumnya HIGH (1), sekarang LOW (0) */
    if ((last_clk == 1) && (clk == 0))
    {
        if ((now - last_enc_time) >= ENC_DEBOUNCE_US)
        {
            last_enc_time = now;
            if (dt == 1)
            {
                /* DT HIGH saat CLK turun → Clockwise (CW) */
                enc_count++;
                if (enc_count > 255) enc_count = 255;
            }
            else
            {
                /* DT LOW saat CLK turun → Counter-Clockwise (CCW) */
                enc_count--;
                if (enc_count < 0) enc_count = 0;
            }
            led_show_count();
        }
    }
    last_clk = clk;

    /* ---- SW: Falling Edge (tombol reset) ---- */
    int sw = gpio_get_level(ENC_SW);
    if ((last_sw == 1) && (sw == 0))
    {
        if ((now - last_sw_time) >= SW_DEBOUNCE_US)
        {
            last_sw_time = now;
            enc_count    = 128;   /* Reset ke tengah */
            led_show_count();
        }
    }
    last_sw = sw;
}

/* ================================================================
 *  MAIN APPLICATION
 * ================================================================ */
void app_main(void)
{
    gpio_init_all();
    led_show_count();   /* Tampilkan nilai awal 128 */

    while (1)
    {
        encoder_read();
        vTaskDelay(pdMS_TO_TICKS(1));   /* Polling 1ms */
    }
}
