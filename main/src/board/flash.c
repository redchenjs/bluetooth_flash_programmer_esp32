/*
 * flash.c
 *
 *  Created on: 2018-03-16 12:30
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "chip/spi.h"

#define TAG "flash"

static spi_transaction_t hspi_trans[2];

void flash_init(void)
{
    memset(hspi_trans, 0x00, sizeof(hspi_trans));

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
    hspi_trans[0].length = write_size * 8,
    hspi_trans[0].rxlength = 0;
    hspi_trans[0].tx_buffer = write_buf;
    hspi_trans[0].rx_buffer = NULL;

    spi_device_queue_trans(hspi, &hspi_trans[0], portMAX_DELAY);

    hspi_trans[1].length = read_size * 8;
    hspi_trans[1].rxlength = read_size * 8;
    hspi_trans[1].tx_buffer = NULL;
    hspi_trans[1].rx_buffer = read_buf;

    spi_device_queue_trans(hspi, &hspi_trans[1], portMAX_DELAY);
}
