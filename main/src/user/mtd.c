/*
 * mtd.c
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_spp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sfud.h"

#define MTD_TAG "mtd"

#define mtd_send_rsponse(X) \
    esp_spp_write(param->write.handle, strlen(rsp_str[X]), (uint8_t *)rsp_str[X])

#define mtd_send_data(X, N) \
    esp_spp_write(param->write.handle, N, (uint8_t *)X)

enum cmd_idx {
    CMD_IDX_ERASE_ALL = 0x0,
    CMD_IDX_ERASE     = 0x1,
    CMD_IDX_WRITE     = 0x2,
    CMD_IDX_READ      = 0x3,
    CMD_IDX_INFO      = 0x4,
};

typedef struct {
    const char id;
    const char fmt[32];
} cmd_fmt_t;

static const cmd_fmt_t cmd_fmt[] = {
    {.id = 14, .fmt = "MTD+ERASE:ALL!\r\n"},       // Erase Full Flash Chip
    {.id = 10, .fmt = "MTD+ERASE:0x%x+0x%x\r\n"},  // Erase Flash: Addr Length
    {.id = 10, .fmt = "MTD+WRITE:0x%x+0x%x\r\n"},  // Write Flash: Addr Length
    {.id =  9, .fmt = "MTD+READ:0x%x+0x%x\r\n"},   // Read Flash:  Addr Length
    {.id =  9, .fmt = "MTD+INFO?\r\n"},            // Flash Info
};

enum rsp_idx {
    RSP_IDX_OK    = 0x0,
    RSP_IDX_FAIL  = 0x1,
    RSP_IDX_DONE  = 0x2,
    RSP_IDX_ERROR = 0x3,
};

static const char rsp_str[][32] = {
    "OK\r\n",           // OK
    "FAIL\r\n",         // Fail
    "DONE\r\n",         // Done
    "ERROR\r\n",        // Error
};

static bool conn_cong = 0;
static bool conn_flag = 0;
static bool conn_write = 0;
static uint32_t conn_handle = 0;

static uint32_t data_addr = 0;
static uint32_t addr = 0, length = 0;
static sfud_flash *flash = NULL;

static int mtd_parse_command(esp_spp_cb_param_t *param)
{
    for (int i=0; i<sizeof(cmd_fmt)/sizeof(cmd_fmt_t); i++) {
        if (strncmp(cmd_fmt[i].fmt, (const char *)param->data_ind.data, cmd_fmt[i].id) == 0) {
            return i;
        }
    }
    return -1;
}

static void mtd_read_task(void *pvParameter)
{
    portTickType xLastWakeTime;
    sfud_err err = SFUD_SUCCESS;
    uint8_t data_buff[990] = {0};

    if (!conn_handle) {
        vTaskDelete(NULL);
    }

    ESP_LOGI(MTD_TAG, "read started.");

    size_t pkt = 0;
    for (pkt=0; pkt<length/990; pkt++) {
        err = sfud_read(flash, data_addr, 990, data_buff);

        data_addr += 990;

        if (err != SFUD_SUCCESS || !conn_handle) {
            ESP_LOGE(MTD_TAG, "read failed.");

            vTaskDelete(NULL);
        }

        conn_cong = 0;
        conn_flag = 0;

        esp_spp_write(conn_handle, 990, (uint8_t *)data_buff);

        while (conn_cong || !conn_flag) {
            xLastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&xLastWakeTime, 1 / portTICK_RATE_MS);
        }
    }

    uint32_t data_remain = length - pkt * 990;
    if (data_remain < 990) {
        err = sfud_read(flash, data_addr, data_remain, data_buff);

        data_addr += data_remain;

        if (err != SFUD_SUCCESS || !conn_handle) {
            ESP_LOGE(MTD_TAG, "read failed.");

            vTaskDelete(NULL);
        }

        conn_cong = 0;
        conn_flag = 0;

        esp_spp_write(conn_handle, data_remain, (uint8_t *)data_buff);

        while (conn_cong || !conn_flag) {
            xLastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&xLastWakeTime, 1 / portTICK_RATE_MS);
        }
    }

    ESP_LOGI(MTD_TAG, "read done.");

    conn_handle = 0;

    vTaskDelete(NULL);
}

void mtd_exec(esp_spp_cb_param_t *param)
{
    if (!conn_write) {
        int cmd_idx = mtd_parse_command(param);

        if (flash) {
            memset(&flash->chip, 0x00, sizeof(sfud_flash_chip));
        }

        switch (cmd_idx) {
            case CMD_IDX_ERASE_ALL: {
                sscanf((const char *)param->data_ind.data, cmd_fmt[CMD_IDX_ERASE_ALL].fmt);
                ESP_LOGI(MTD_TAG, "GET command: MTD+ERASE:ALL!");

                sfud_err err = sfud_init();
                if (err == SFUD_ERR_NOT_FOUND) {
                    ESP_LOGE(MTD_TAG, "target flash not found or not supported.");

                    mtd_send_rsponse(RSP_IDX_FAIL);
                } else if (err != SFUD_SUCCESS) {
                    ESP_LOGE(MTD_TAG, "failed to init target flash.");

                    mtd_send_rsponse(RSP_IDX_FAIL);
                } else {
                    flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                    ESP_LOGI(MTD_TAG, "chip erase started.");

                    err = sfud_chip_erase(flash);

                    if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "chip erase failed.");

                        mtd_send_rsponse(RSP_IDX_FAIL);
                    } else {
                        ESP_LOGI(MTD_TAG, "chip erase done.");

                        mtd_send_rsponse(RSP_IDX_DONE);
                    }
                }

                break;
            }
            case CMD_IDX_ERASE: {
                addr = length = 0;
                sscanf((const char *)param->data_ind.data, cmd_fmt[CMD_IDX_ERASE].fmt, &addr, &length);
                ESP_LOGI(MTD_TAG, "GET command: MTD+ERASE:0x%x+0x%x", addr, length);

                if (length != 0) {
                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        ESP_LOGE(MTD_TAG, "target flash not found or not supported.");

                        mtd_send_rsponse(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "failed to init target flash.");

                        mtd_send_rsponse(RSP_IDX_FAIL);
                    } else {
                        flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                        ESP_LOGI(MTD_TAG, "erase started.");

                        err = sfud_erase(flash, addr, length);

                        if (err != SFUD_SUCCESS) {
                            ESP_LOGE(MTD_TAG, "erase failed.");

                            mtd_send_rsponse(RSP_IDX_FAIL);
                        } else {
                            ESP_LOGI(MTD_TAG, "erase done.");

                            mtd_send_rsponse(RSP_IDX_DONE);
                        }
                    }
                } else {
                    mtd_send_rsponse(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_WRITE: {
                addr = length = 0;
                sscanf((const char *)param->data_ind.data, cmd_fmt[CMD_IDX_WRITE].fmt, &addr, &length);
                ESP_LOGI(MTD_TAG, "GET command: MTD+WRITE:0x%x+0x%x", addr, length);

                if (length != 0) {
                    conn_write = 1;

                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        ESP_LOGE(MTD_TAG, "target flash not found or not supported.");

                        mtd_send_rsponse(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "failed to init target flash.");

                        mtd_send_rsponse(RSP_IDX_FAIL);
                    } else {
                        data_addr = addr;
                        flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                        ESP_LOGI(MTD_TAG, "write started.");

                        mtd_send_rsponse(RSP_IDX_OK);
                    }
                } else {
                    mtd_send_rsponse(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_READ: {
                addr = length = 0;
                sscanf((const char *)param->data_ind.data, cmd_fmt[CMD_IDX_READ].fmt, &addr, &length);
                ESP_LOGI(MTD_TAG, "GET command: MTD+READ:0x%x+0x%x", addr, length);

                if (length != 0) {
                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        ESP_LOGE(MTD_TAG, "target flash not found or not supported.");

                        mtd_send_rsponse(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "failed to init target flash.");

                        mtd_send_rsponse(RSP_IDX_FAIL);
                    } else {
                        data_addr = addr;
                        flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                        conn_handle = param->write.handle;

                        xTaskCreatePinnedToCore(mtd_read_task, "mtdReadT", 4096, NULL, 9, NULL, 1);
                    }
                } else {
                    mtd_send_rsponse(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_INFO: {
                sscanf((const char *)param->data_ind.data, cmd_fmt[CMD_IDX_INFO].fmt);
                ESP_LOGI(MTD_TAG, "GET command: MTD+INFO?");

                sfud_err err = sfud_init();
                if (err == SFUD_ERR_NOT_FOUND) {
                    ESP_LOGE(MTD_TAG, "target flash not found or not supported.");

                    mtd_send_rsponse(RSP_IDX_FAIL);
                } else if (err != SFUD_SUCCESS) {
                    ESP_LOGE(MTD_TAG, "failed to init target flash.");

                    mtd_send_rsponse(RSP_IDX_FAIL);
                } else {
                    const sfud_mf mf_table[] = SFUD_MF_TABLE;
                    const char *flash_mf_name = NULL;

                    flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                    for (int i = 0; i < sizeof(mf_table) / sizeof(sfud_mf); i++) {
                        if (mf_table[i].id == flash->chip.mf_id) {
                            flash_mf_name = mf_table[i].name;
                            break;
                        }
                    }

                    char str_buf[40] = {0};
                    if (flash_mf_name && flash->chip.name) {
                        snprintf(str_buf, sizeof(str_buf), "INFO:%s,%s,%u\r\n", flash_mf_name, flash->chip.name, flash->chip.capacity);
                    } else if (flash_mf_name) {
                        snprintf(str_buf, sizeof(str_buf), "INFO:%s,%u\r\n", flash_mf_name, flash->chip.capacity);
                    } else {
                        snprintf(str_buf, sizeof(str_buf), "INFO:%u\r\n", flash->chip.capacity);
                    }

                    mtd_send_data(str_buf, strlen(str_buf));
                }

                break;
            }
            default:
                ESP_LOGW(MTD_TAG, "unknown command.");

                mtd_send_rsponse(RSP_IDX_ERROR);

                break;
        }
    } else {
        sfud_err err = SFUD_SUCCESS;
        uint32_t remain = length - (data_addr - addr);

        if (param->data_ind.len > remain) {
            err = sfud_write(flash, data_addr, remain, (const uint8_t *)param->data_ind.data);
            data_addr += remain;
        } else {
            err = sfud_write(flash, data_addr, param->data_ind.len, (const uint8_t *)param->data_ind.data);
            data_addr += param->data_ind.len;
        }

        if (err != SFUD_SUCCESS) {
            ESP_LOGE(MTD_TAG, "write failed.");

            conn_write = 0;

            mtd_send_rsponse(RSP_IDX_FAIL);
        } else if ((data_addr - addr) == length) {
            ESP_LOGI(MTD_TAG, "write done.");

            conn_write = 0;

            mtd_send_rsponse(RSP_IDX_DONE);
        }
    }
}

void mtd_cong(bool cong, bool flag)
{
    conn_cong = cong;
    conn_flag = flag;
}

void mtd_end(void)
{
    conn_write = 0;
    conn_handle = 0;
}
