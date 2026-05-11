#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"

/* W5500 Common Registers */
#define W5500_MR        0x0000  /* Mode Register */
#define W5500_GAR       0x0001  /* Gateway IP Address */
#define W5500_SUBR      0x0005  /* Subnet Mask */
#define W5500_SHAR      0x0009  /* Source Hardware (MAC) */
#define W5500_SIPR      0x000F  /* Source IP Address */
#define W5500_PHYCFGR   0x002E  /* PHY Config */
#define W5500_VERSIONR  0x0039  /* Chip Version (0x04 for W5500) */

/* W5500 Block Select Bits (BSB) */
#define W5500_BSB_COMMON    0x00
#define W5500_BSB_Sn(n)     ((n)*4 + 1)  /* Socket n Register */
#define W5500_BSB_Sn_TX(n)  ((n)*4 + 2)
#define W5500_BSB_Sn_RX(n)  ((n)*4 + 3)

/* W5500 SPI frame: 2-byte address + 1-byte control */
#define W5500_RD(bsb)  (((bsb) << 3) | 0x00)
#define W5500_WR(bsb)  (((bsb) << 3) | 0x04)

/* W5500 PHY Config bits */
#define W5500_PHYCFGR_LNK   (1 << 0)  /* Link status bit */

esp_err_t w5500_spi_init(spi_device_handle_t *spi, spi_host_device_t host,
                          int mosi, int miso, int sclk, int cs, int rst_pin, int int_pin);
uint8_t   w5500_read8(spi_device_handle_t spi, uint16_t addr, uint8_t bsb);
void      w5500_write8(spi_device_handle_t spi, uint16_t addr, uint8_t bsb, uint8_t val);
void      w5500_readbuf(spi_device_handle_t spi, uint16_t addr, uint8_t bsb, uint8_t *buf, size_t len);
void      w5500_writebuf(spi_device_handle_t spi, uint16_t addr, uint8_t bsb, const uint8_t *buf, size_t len);
bool      w5500_link_up(spi_device_handle_t spi);
