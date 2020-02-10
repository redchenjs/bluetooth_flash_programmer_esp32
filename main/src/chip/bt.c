/*
 * bt.c
 *
 *  Created on: 2018-03-09 10:36
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_bt.h"
#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#define TAG "bt"

char bt_mac_str[18] = {0};

void bt_init(void)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    snprintf(bt_mac_str, sizeof(bt_mac_str), MACSTR, MAC2STR(esp_bt_dev_get_address()));

    ESP_LOGI(TAG, "initialized, addr: [%s]", bt_mac_str);
}
