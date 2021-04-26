/*
 * app_main.c
 *
 *  Created on: 2018-03-11 15:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "core/app.h"

#include "chip/bt.h"
#include "chip/nvs.h"
#include "chip/spi.h"

#include "board/flash.h"

#include "user/bt_app.h"

static void core_init(void)
{
    app_print_info();
}

static void chip_init(void)
{
    nvs_init();

    bt_init();

    spi_host_init();
}

static void board_init(void)
{
    flash_init();
}

static void user_init(void)
{
    bt_app_init();
}

int app_main(void)
{
    core_init();

    chip_init();

    board_init();

    user_init();

    return 0;
}
