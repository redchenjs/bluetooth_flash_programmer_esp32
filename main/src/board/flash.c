/*
 * flash.c
 *
 *  Created on: 2018-03-16 12:30
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "chip/spi.h"
#include "driver/gpio.h"

#define TAG "flash"

static spi_transaction_t spi_trans[2];

void flash_init(void)
{
    memset(spi_trans, 0x00, sizeof(spi_trans));

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(CONFIG_SPI_CS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(CONFIG_SPI_CS_PIN, 1);

    ESP_LOGI(TAG, "initialized, cs: %d", CONFIG_SPI_CS_PIN);
}

void flash_setpin_cs(uint8_t val)
{
    gpio_set_level(CONFIG_SPI_CS_PIN, val);
}

void flash_write_read(const uint8_t *write_buf, size_t write_size, uint8_t *read_buf, size_t read_size)
{
    spi_trans[0].length = write_size * 8,
    spi_trans[0].rxlength = 0;
    spi_trans[0].tx_buffer = write_buf;
    spi_trans[0].rx_buffer = NULL;

    spi_device_queue_trans(spi_host, &spi_trans[0], portMAX_DELAY);

    spi_trans[1].length = read_size * 8;
    spi_trans[1].rxlength = read_size * 8;
    spi_trans[1].tx_buffer = NULL;
    spi_trans[1].rx_buffer = read_buf;

    spi_device_queue_trans(spi_host, &spi_trans[1], portMAX_DELAY);
}
