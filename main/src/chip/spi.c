/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "driver/spi_master.h"

#define HSPI_TAG "hspi"

spi_device_handle_t hspi;

void hspi_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_SPI_MISO_PIN,
        .mosi_io_num = CONFIG_SPI_MOSI_PIN,
        .sclk_io_num = CONFIG_SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 1));

    spi_device_interface_config_t devcfg = {
        .mode = 0,                                // SPI mode 0
        .spics_io_num = -1,                       // CS pin
        .clock_speed_hz = 20000000,               // clock out at 20 MHz
        .queue_size = 2                           // we want to be able to queue 2 transactions at a time
    };
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &hspi));

    ESP_LOGI(HSPI_TAG, "initialized, sclk: %d, mosi: %d, miso: %d, cs: %d",
             buscfg.sclk_io_num,
             buscfg.mosi_io_num,
             buscfg.miso_io_num,
             devcfg.spics_io_num
    );
}
