/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "chip/spi.h"

spi_device_handle_t spi_host;

void spi_host_init(void)
{
    spi_bus_config_t bus_conf = {
        .miso_io_num = CONFIG_SPI_MISO_PIN,
        .mosi_io_num = CONFIG_SPI_MOSI_PIN,
        .sclk_io_num = CONFIG_SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_NUM, &bus_conf, 1));

    spi_device_interface_config_t dev_conf = {
        .mode = 0,                                // SPI mode 0
        .spics_io_num = -1,                       // CS pin
        .clock_speed_hz = 20000000,               // clock out at 20 MHz
        .queue_size = 2                           // we want to be able to queue 2 transactions at a time
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_NUM, &dev_conf, &spi_host));

    ESP_LOGI(SPI_HOST_TAG, "initialized, sclk: %d, mosi: %d, miso: %d, cs: %d",
             bus_conf.sclk_io_num,
             bus_conf.mosi_io_num,
             bus_conf.miso_io_num,
             dev_conf.spics_io_num
    );
}
