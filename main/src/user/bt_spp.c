/*
 * bt_spp.c
 *
 *  Created on: 2019-07-03 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_spp_api.h"
#include "esp_gap_bt_api.h"

#include "user/mtd.h"

#define BT_SPP_TAG "bt_spp"

static esp_bd_addr_t spp_remote_bda = {0};
static const char *s_spp_conn_state_str[] = {"disconnected", "connected"};

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

void bt_app_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        esp_spp_start_srv(sec_mask, role_slave, 0, CONFIG_BT_SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        break;
    case ESP_SPP_OPEN_EVT:
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(BT_SPP_TAG, "SPP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_spp_conn_state_str[0],
                 spp_remote_bda[0], spp_remote_bda[1], spp_remote_bda[2],
                 spp_remote_bda[3], spp_remote_bda[4], spp_remote_bda[5]);

        memset(&spp_remote_bda, 0x00, sizeof(esp_bd_addr_t));

        mtd_end();

        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
    case ESP_SPP_START_EVT:
        break;
    case ESP_SPP_CL_INIT_EVT:
        break;
    case ESP_SPP_DATA_IND_EVT:
        mtd_exec(param);
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGW(BT_SPP_TAG, "SPP connection congestion state: %d", param->cong.cong);
        break;
    case ESP_SPP_WRITE_EVT:
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

        memcpy(&spp_remote_bda, param->srv_open.rem_bda, sizeof(esp_bd_addr_t));

        ESP_LOGI(BT_SPP_TAG, "SPP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_spp_conn_state_str[1],
                 spp_remote_bda[0], spp_remote_bda[1], spp_remote_bda[2],
                 spp_remote_bda[3], spp_remote_bda[4], spp_remote_bda[5]);

        break;
    default:
        break;
    }
}
