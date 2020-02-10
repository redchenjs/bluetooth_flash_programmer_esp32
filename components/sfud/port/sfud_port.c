/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
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

static void retry_delay_10ms(void)
{
    vTaskDelay(10 / portTICK_RATE_MS);
}

sfud_err sfud_spi_port_init(sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;

    switch (flash->index) {
        case SFUD_TARGET_DEVICE_INDEX: {
            flash->spi.wr = spi_write_read;
            flash->retry.delay = retry_delay_10ms;
            flash->retry.times = 60 * 100;

            break;
        }
    }

    return result;
}
