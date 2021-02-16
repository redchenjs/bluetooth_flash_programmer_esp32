/*
 * bt_app.c
 *
 *  Created on: 2018-03-09 13:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "user/bt_spp.h"
#include "user/bt_app.h"

#define BT_APP_TAG "bt_app"
#define BT_GAP_TAG "bt_gap"

static void bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BT_GAP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
        } else {
            ESP_LOGE(BT_GAP_TAG, "authentication failed: %d", param->auth_cmpl.stat);
        }
        break;
    default:
        break;
    }
}

void bt_app_init(void)
{
    esp_bt_dev_set_device_name(CONFIG_BT_NAME);
    esp_bt_gap_register_callback(bt_gap_event_handler);

    esp_spp_init(ESP_SPP_MODE_CB);
    esp_spp_register_callback(bt_spp_event_handler);

    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    ESP_LOGI(BT_APP_TAG, "started.");
}
