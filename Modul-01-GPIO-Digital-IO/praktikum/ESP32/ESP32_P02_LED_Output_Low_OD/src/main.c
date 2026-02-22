/**
 * @file    main.c
 * @brief   P02 — Shadow & Ghost: Active-LOW, Open-Drain, dan Logika Terbalik
 *
 * ================================================================
 * KONSEP 1: Active-HIGH vs Active-LOW
 * ================================================================
 * Active-HIGH (normal):   pin=1 → LED MENYALA, pin=0 → LED MATI
 * Active-LOW (terbalik):  pin=0 → LED MENYALA, pin=1 → LED MATI
 *
 * ESP32 DevKit built-in LED (GPIO2) = Active-HIGH
 *   (keduanya Active-HIGH pada ESP32!)
 *
 * Untuk demo Active-LOW, kita hubungkan LED secara terbalik:
 *   VCC ─[LED]─[220Ω]── GPIO → Active-LOW circuit
 *   GPIO=0 (GND) → ada beda potensial → LED MENYALA
 *   GPIO=1 (3.3V) → tidak ada beda potensial → LED MATI
 *
 * ================================================================
 * KONSEP 2: OPEN-DRAIN vs PUSH-PULL (ESP32)
 * ================================================================
 * PUSH-PULL  (GPIO_MODE_OUTPUT):
 *   gpio_set_level(pin, 1) → output aktif HIGH (3.3V, impedansi rendah)
 *   gpio_set_level(pin, 0) → output aktif LOW  (0V,   impedansi rendah)
 *
 * OPEN-DRAIN (GPIO_MODE_OUTPUT_OD):
 *   gpio_set_level(pin, 0) → N-MOS aktif → pin = GND (kuat)
 *   gpio_set_level(pin, 1) → N-MOS OFF   → pin = Hi-Z (mengambang)
 *   → Perlu pull-up untuk menjadi HIGH!
 *   Dengan internal pull-up ~45kΩ: Hi-Z → VCC via 45kΩ (lemah)
 *   → LED menyala sangat redup (Shadow!)
 *
 * ================================================================
 * RANGKAIAN:
 *   GPIO4  ─[220Ω]─ LED+G LED-  ─ GND   (PP Active-HIGH ref)
 *   GPIO5  ─[220Ω]─ LED+O LED-  ─ GND   (OD + internal 45kΩ PU)
 *   VCC    ─[LED+R]─[220Ω]─ GPIO2       (Active-LOW demo via OB LED)
 *
 * PLATFORM : ESP32 DevKit V1
 * FRAMEWORK: ESP-IDF (FreeRTOS)
 * ================================================================
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define LED_PP_PIN   GPIO_NUM_4    /* Push-Pull output (Active-HIGH ref) */
#define LED_OD_PIN   GPIO_NUM_5    /* Open-Drain + internal pull-up */
#define LED_OB_PIN   GPIO_NUM_2    /* Built-in LED Active-HIGH */

#define DELAY_MS 700

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* PA0 equivalent: GPIO4 → Push-Pull output */
    gpio_config_t pp_conf = {
        .pin_bit_mask = (1ULL << LED_PP_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&pp_conf);

    /* GPIO5 → Open-Drain output + internal pull-up ~45kΩ
     *
     * GPIO_MODE_OUTPUT_OD:
     *   level=0 → N-MOS ON  → pin = GND (aktif kuat)
     *   level=1 → N-MOS OFF → pin = Hi-Z → via 45kΩ = HIGH lemah
     *
     * Arus saat Hi-Z: (3.3-2.0)/(45000+220) ≈ 0.029 mA → LED "Ghost" redup
     */
    gpio_config_t od_conf = {
        .pin_bit_mask = (1ULL << LED_OD_PIN),
        .mode         = GPIO_MODE_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_ENABLE,    /* ~45kΩ internal pull-up */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&od_conf);

    /* GPIO2: Built-in LED Push-Pull */
    gpio_config_t ob_conf = {
        .pin_bit_mask = (1ULL << LED_OB_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&ob_conf);

    /* Semua mati di awal */
    gpio_set_level(LED_PP_PIN, 0);
    gpio_set_level(LED_OD_PIN, 1);  /* OD : 1 = Hi-Z = mati (tanpa pull, float) */
    gpio_set_level(LED_OB_PIN, 0);
}

/* ================================================================
 *  MAIN APPLICATION
 * ================================================================ */
void app_main(void)
{
    gpio_init_all();

    while (1)
    {
        /* ======================================================
         *  TAHAP 1: PP ON kuat, OD mengambang ke GND
         *  GPIO4 = HIGH → LED PP terang
         *  GPIO5 = LOW  → OD N-MOS mati → LED mati (LOW via OD)
         * ====================================================== */
        gpio_set_level(LED_OB_PIN, 1);   /* OB ON  */
        gpio_set_level(LED_PP_PIN, 1);   /* PP ON terang */
        gpio_set_level(LED_OD_PIN, 0);   /* OD LOW → LED MATI (N-MOS tarik GND) */
        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));

        /* ======================================================
         *  TAHAP 2: PP ON kuat vs OD Hi-Z (via pull-up 45kΩ)
         *  GPIO4 = HIGH → LED PP terang    (impedansi rendah)
         *  GPIO5 = HIGH → Hi-Z → pull-up → Ghost! LED redup
         *  PERBANDINGAN nyata: satu terang, satu "hantu" redup
         * ====================================================== */
        gpio_set_level(LED_OB_PIN, 0);   /* OB OFF */
        gpio_set_level(LED_PP_PIN, 1);   /* PP ON terang ~15mA */
        gpio_set_level(LED_OD_PIN, 1);   /* OD Hi-Z → via 45kΩ → Ghost 0.029mA */
        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));

        /* ======================================================
         *  TAHAP 3: Hanya OD LOW (satu-satunya "kuat" di OD)
         *  OD level=0 → N-MOS aktif → pin = GND kuat
         * ====================================================== */
        gpio_set_level(LED_OB_PIN, 1);   /* OB ON  */
        gpio_set_level(LED_PP_PIN, 0);   /* PP OFF */
        gpio_set_level(LED_OD_PIN, 0);   /* OD LOW aktif kuat */
        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));

        /* ======================================================
         *  TAHAP 4: Semua mati
         *  PP=0 → 0V → mati; OD=1 → Hi-Z via 45kΩ → ghost redup
         * ====================================================== */
        gpio_set_level(LED_OB_PIN, 0);   /* OB OFF */
        gpio_set_level(LED_PP_PIN, 0);   /* PP OFF */
        gpio_set_level(LED_OD_PIN, 1);   /* OD Hi-Z → redup */
        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
    }
}
