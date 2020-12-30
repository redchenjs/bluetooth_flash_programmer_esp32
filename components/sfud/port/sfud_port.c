/*
 * sfud_port.c
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "sfud.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board/flash.h"

static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf, size_t read_size)
{
    sfud_err result = SFUD_SUCCESS;

    if (write_size) {
        SFUD_ASSERT(write_buf);
    }
    if (read_size) {
        SFUD_ASSERT(read_buf);
    }

    flash_setpin_cs(0);
    flash_write_read(write_buf, write_size, read_buf, read_size);
    flash_setpin_cs(1);

    return result;
}

static void retry_delay_1ms(void)
{
    vTaskDelay(1 / portTICK_RATE_MS);
}

sfud_err sfud_spi_port_init(sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;

    switch (flash->index) {
        case SFUD_FLASH_DEVICE_INDEX: {
            flash->spi.wr = spi_write_read;
            flash->retry.delay = retry_delay_1ms;
            flash->retry.times = 300 * 1000;

            break;
        }
    }

    return result;
}
