/**
 * config.h - Konfigurasi Pin untuk Multi-Board Support
 * ESP32 DevKit V1 / Lolin S2 Mini / ESP32-S3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- LED Pin ---- */
#if defined(CONFIG_IDF_TARGET_ESP32)
    #define LED_PIN         GPIO_NUM_2      /* ESP32 DevKit V1 built-in LED */
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    #define LED_PIN         GPIO_NUM_15     /* Lolin S2 Mini built-in LED */
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    #define LED_PIN         GPIO_NUM_48     /* ESP32-S3 DevKitC built-in LED */
#else
    #define LED_PIN         GPIO_NUM_2      /* Default */
#endif

#endif /* CONFIG_H */
