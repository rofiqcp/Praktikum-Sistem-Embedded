/**
 * ==========================================================================
 *  ESP32_08 - DIP Switch Reader (Parallel Input with Bit Masking)
 * ==========================================================================
 *  Pin definitions for 4-bit / 8-bit DIP switch
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ===== Board-specific pin mapping ===== */
#if defined(CONFIG_IDF_TARGET_ESP32)
    /*
     * ESP32 classic – use input-only pins GPIO32-35
     * Note: GPIO34-35 have NO internal pull-up → external pull-up required
     *       GPIO32-33 have internal pull-up available
     */
    #define SW_BIT0_PIN     GPIO_NUM_32     /* LSB  */
    #define SW_BIT1_PIN     GPIO_NUM_33
    #define SW_BIT2_PIN     GPIO_NUM_34     /* input-only, no internal pull-up */
    #define SW_BIT3_PIN     GPIO_NUM_35     /* input-only, no internal pull-up */

    /* For 8-bit expansion (optional) */
    #define SW_BIT4_PIN     GPIO_NUM_25
    #define SW_BIT5_PIN     GPIO_NUM_26
    #define SW_BIT6_PIN     GPIO_NUM_27
    #define SW_BIT7_PIN     GPIO_NUM_14

#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    #define SW_BIT0_PIN     GPIO_NUM_1
    #define SW_BIT1_PIN     GPIO_NUM_2
    #define SW_BIT2_PIN     GPIO_NUM_3
    #define SW_BIT3_PIN     GPIO_NUM_4
    #define SW_BIT4_PIN     GPIO_NUM_5
    #define SW_BIT5_PIN     GPIO_NUM_6
    #define SW_BIT6_PIN     GPIO_NUM_7
    #define SW_BIT7_PIN     GPIO_NUM_8

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    #define SW_BIT0_PIN     GPIO_NUM_1
    #define SW_BIT1_PIN     GPIO_NUM_2
    #define SW_BIT2_PIN     GPIO_NUM_3
    #define SW_BIT3_PIN     GPIO_NUM_4
    #define SW_BIT4_PIN     GPIO_NUM_5
    #define SW_BIT5_PIN     GPIO_NUM_6
    #define SW_BIT6_PIN     GPIO_NUM_7
    #define SW_BIT7_PIN     GPIO_NUM_8

#else
    #error "Unsupported target – add pin mapping for your chip"
#endif

/* ---------- Number of DIP switch bits to use ---------- */
#define DIP_SWITCH_BITS     4       /* Set to 8 for 8-bit DIP switch */

/* ---------- Timing ---------- */
#define READ_INTERVAL_MS    500     /* Read switches every 500 ms */

#endif /* CONFIG_H */
