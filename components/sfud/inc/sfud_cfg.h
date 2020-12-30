/*
 * sfud_cfg.h
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef _SFUD_CFG_H_
#define _SFUD_CFG_H_

#include "esp_log.h"

#define SFUD_INFO(...)  ESP_LOGD("[SFUD]", __VA_ARGS__)

#define SFUD_USING_SFDP
#define SFUD_USING_FLASH_INFO_TABLE

enum {
    SFUD_FLASH_DEVICE_INDEX = 0
};

#define SFUD_FLASH_DEVICE_TABLE                                                \
{                                                                              \
    [SFUD_FLASH_DEVICE_INDEX] = {.name = "FLASH", .spi.name = "HSPI"}          \
}

#endif /* _SFUD_CFG_H_ */
