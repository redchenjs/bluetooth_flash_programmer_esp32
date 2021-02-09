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
#include "user/bt_app_core.h"

#define BT_APP_TAG "bt_app"
#define BT_GAP_TAG "bt_gap"

enum {
    BT_APP_EVT_STACK_UP = 0
};

static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
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

static void bt_app_hdl_stack_evt(uint16_t event, void *p_param)
{
    switch (event) {
    case BT_APP_EVT_STACK_UP:
        /* set up device name */
        esp_bt_dev_set_device_name(CONFIG_BT_NAME);

        /* register GAP callback */
        esp_bt_gap_register_callback(bt_app_gap_cb);

        esp_spp_register_callback(bt_app_spp_cb);
        esp_spp_init(ESP_SPP_MODE_CB);

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

        break;
    default:
        ESP_LOGW(BT_APP_TAG, "unhandled evt: %d", event);
        break;
    }
}

void bt_app_init(void)
{
    /* create application task */
    bt_app_task_start_up();

    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_app_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

    /*
     * set default parameters for Legacy Pairing
     * use fixed pin code
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '1';
    pin_code[1] = '2';
    pin_code[2] = '3';
    pin_code[3] = '4';
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    ESP_LOGI(BT_APP_TAG, "started.");
}
