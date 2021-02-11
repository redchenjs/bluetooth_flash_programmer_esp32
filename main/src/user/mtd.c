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
#include "freertos/ringbuf.h"
#include "freertos/task.h"

#include "sfud.h"

#define MTD_TAG "mtd"

#define mtd_send_response(X) \
    esp_spp_write(conn_handle, strlen(rsp_str[X]), (uint8_t *)rsp_str[X])

#define mtd_send_data(X, N) \
    esp_spp_write(conn_handle, N, (uint8_t *)X)

#define RX_BUF_SIZE 990

#define CMD_FMT_ERASE_ALL "MTD+ERASE:ALL!"
#define CMD_FMT_ERASE     "MTD+ERASE:0x%x+0x%x"
#define CMD_FMT_WRITE     "MTD+WRITE:0x%x+0x%x"
#define CMD_FMT_READ      "MTD+READ:0x%x+0x%x"
#define CMD_FMT_INFO      "MTD+INFO?"

enum cmd_idx {
    CMD_IDX_ERASE_ALL = 0x0,
    CMD_IDX_ERASE     = 0x1,
    CMD_IDX_WRITE     = 0x2,
    CMD_IDX_READ      = 0x3,
    CMD_IDX_INFO      = 0x4
};

typedef struct {
    const char prefix;
    const char format[32];
} cmd_fmt_t;

static const cmd_fmt_t cmd_fmt[] = {
    { .prefix = 16, .format = CMD_FMT_ERASE_ALL"\r\n" },
    { .prefix = 12, .format = CMD_FMT_ERASE"\r\n" },
    { .prefix = 12, .format = CMD_FMT_WRITE"\r\n" },
    { .prefix = 11, .format = CMD_FMT_READ"\r\n" },
    { .prefix = 11, .format = CMD_FMT_INFO"\r\n" }
};

enum rsp_idx {
    RSP_IDX_OK    = 0x0,
    RSP_IDX_FAIL  = 0x1,
    RSP_IDX_DONE  = 0x2,
    RSP_IDX_ERROR = 0x3
};

static const char rsp_str[][32] = {
    "OK\r\n",
    "FAIL\r\n",
    "DONE\r\n",
    "ERROR\r\n"
};

static bool data_err = false;
static bool data_cong = false;
static bool data_sent = false;
static bool data_recv = false;
static uint32_t conn_handle = 0;

static uint8_t buff_data[RX_BUF_SIZE] = {0};
static StaticRingbuffer_t buff_struct = {0};

static RingbufHandle_t mtd_buff = NULL;

static uint32_t data_addr = 0;
static uint32_t addr = 0, length = 0;
static sfud_flash *flash = NULL;

static int mtd_parse_command(esp_spp_cb_param_t *param)
{
    for (int i = 0; i < sizeof(cmd_fmt) / sizeof(cmd_fmt_t); i++) {
        if (strncmp(cmd_fmt[i].format, (const char *)param->data_ind.data, cmd_fmt[i].prefix) == 0) {
            return i;
        }
    }
    return -1;
}

static void mtd_write_task(void *pvParameter)
{
    uint8_t *data = NULL;
    uint32_t size = 0;
    sfud_err err = SFUD_SUCCESS;

    ESP_LOGI(MTD_TAG, "write started.");

    while ((data_addr - addr) != length) {
        if (!conn_handle || !data_recv) {
            ESP_LOGE(MTD_TAG, "write aborted.");

            goto write_fail;
        }

        uint32_t remain = length - (data_addr - addr);

        if (remain >= RX_BUF_SIZE) {
            data = xRingbufferReceiveUpTo(mtd_buff, &size, 10 / portTICK_RATE_MS, RX_BUF_SIZE);
        } else {
            data = xRingbufferReceiveUpTo(mtd_buff, &size, 10 / portTICK_RATE_MS, remain);
        }

        if (data == NULL || size == 0) {
            continue;
        }

        err = sfud_write(flash, data_addr, size, (const uint8_t *)data);

        data_addr += size;

        if (err != SFUD_SUCCESS) {
            ESP_LOGE(MTD_TAG, "write failed.");

            data_err = true;

            mtd_send_response(RSP_IDX_FAIL);

            goto write_fail;
        }

        vRingbufferReturnItem(mtd_buff, data);
    }

    ESP_LOGI(MTD_TAG, "write done.");

    mtd_send_response(RSP_IDX_DONE);

write_fail:
    vRingbufferDelete(mtd_buff);
    mtd_buff = NULL;

    data_recv = false;
    conn_handle = 0;

    vTaskDelete(NULL);
}

static void mtd_read_task(void *pvParameter)
{
    portTickType xLastWakeTime;
    sfud_err err = SFUD_SUCCESS;

    ESP_LOGI(MTD_TAG, "read started.");

    data_cong = false;
    data_sent = true;

    uint32_t pkt = 0;
    for (pkt = 0; pkt < length / RX_BUF_SIZE; pkt++) {
        err = sfud_read(flash, data_addr, RX_BUF_SIZE, buff_data);

        data_addr += RX_BUF_SIZE;

        if (err != SFUD_SUCCESS) {
            ESP_LOGE(MTD_TAG, "read failed.");

            mtd_send_response(RSP_IDX_FAIL);

            goto read_fail;
        }

        while (data_cong || !data_sent) {
            xLastWakeTime = xTaskGetTickCount();

            if (!conn_handle) {
                ESP_LOGE(MTD_TAG, "read aborted.");

                goto read_fail;
            }

            vTaskDelayUntil(&xLastWakeTime, 1 / portTICK_RATE_MS);
        }

        data_cong = false;
        data_sent = false;

        if (!conn_handle) {
            ESP_LOGE(MTD_TAG, "read aborted.");

            goto read_fail;
        }

        mtd_send_data(buff_data, RX_BUF_SIZE);
    }

    uint32_t data_remain = length - pkt * RX_BUF_SIZE;
    if (data_remain != 0 && data_remain < RX_BUF_SIZE) {
        err = sfud_read(flash, data_addr, data_remain, buff_data);

        data_addr += data_remain;

        if (err != SFUD_SUCCESS) {
            ESP_LOGE(MTD_TAG, "read failed.");

            mtd_send_response(RSP_IDX_FAIL);

            goto read_fail;
        }

        while (data_cong || !data_sent) {
            xLastWakeTime = xTaskGetTickCount();

            if (!conn_handle) {
                ESP_LOGE(MTD_TAG, "read aborted.");

                goto read_fail;
            }

            vTaskDelayUntil(&xLastWakeTime, 1 / portTICK_RATE_MS);
        }

        if (!conn_handle) {
            ESP_LOGE(MTD_TAG, "read aborted.");

            goto read_fail;
        }

        mtd_send_data(buff_data, data_remain);
    }

    ESP_LOGI(MTD_TAG, "read done.");

read_fail:
    conn_handle = 0;

    vTaskDelete(NULL);
}

void mtd_exec(esp_spp_cb_param_t *param)
{
    if (data_err) {
        return;
    }

    if (!data_recv) {
        int cmd_idx = mtd_parse_command(param);

        if (flash) {
            memset(&flash->chip, 0x00, sizeof(sfud_flash_chip));
        }

        conn_handle = param->write.handle;

        switch (cmd_idx) {
            case CMD_IDX_ERASE_ALL: {
                ESP_LOGI(MTD_TAG, "GET command: "CMD_FMT_ERASE_ALL);

                sfud_err err = sfud_init();
                if (err == SFUD_ERR_NOT_FOUND) {
                    ESP_LOGE(MTD_TAG, "target flash not found or not supported");

                    mtd_send_response(RSP_IDX_FAIL);
                } else if (err != SFUD_SUCCESS) {
                    ESP_LOGE(MTD_TAG, "failed to init target flash");

                    mtd_send_response(RSP_IDX_FAIL);
                } else {
                    flash = sfud_get_device(SFUD_FLASH_DEVICE_INDEX);

                    ESP_LOGI(MTD_TAG, "chip erase started.");

                    err = sfud_chip_erase(flash);

                    if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "chip erase failed.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        ESP_LOGI(MTD_TAG, "chip erase done.");

                        mtd_send_response(RSP_IDX_DONE);
                    }
                }

                break;
            }
            case CMD_IDX_ERASE: {
                addr = length = 0;
                sscanf((const char *)param->data_ind.data, CMD_FMT_ERASE, &addr, &length);
                ESP_LOGI(MTD_TAG, "GET command: "CMD_FMT_ERASE, addr, length);

                if (length != 0) {
                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        ESP_LOGE(MTD_TAG, "target flash not found or not supported");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "failed to init target flash");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        flash = sfud_get_device(SFUD_FLASH_DEVICE_INDEX);

                        ESP_LOGI(MTD_TAG, "erase started.");

                        err = sfud_erase(flash, addr, length);

                        if (err != SFUD_SUCCESS) {
                            ESP_LOGE(MTD_TAG, "erase failed.");

                            mtd_send_response(RSP_IDX_FAIL);
                        } else {
                            ESP_LOGI(MTD_TAG, "erase done.");

                            mtd_send_response(RSP_IDX_DONE);
                        }
                    }
                } else {
                    mtd_send_response(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_WRITE: {
                addr = length = 0;
                sscanf((const char *)param->data_ind.data, CMD_FMT_WRITE, &addr, &length);
                ESP_LOGI(MTD_TAG, "GET command: "CMD_FMT_WRITE, addr, length);

                if (length != 0) {
                    data_recv = true;

                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        ESP_LOGE(MTD_TAG, "target flash not found or not supported");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "failed to init target flash");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        data_addr = addr;
                        flash = sfud_get_device(SFUD_FLASH_DEVICE_INDEX);

                        mtd_buff = xRingbufferCreateStatic(sizeof(buff_data), RINGBUF_TYPE_BYTEBUF, buff_data, &buff_struct);
                        if (!mtd_buff) {
                            mtd_send_response(RSP_IDX_ERROR);
                        } else {
                            mtd_send_response(RSP_IDX_OK);

                            xTaskCreatePinnedToCore(mtd_write_task, "mtdWriteT", 4096, NULL, 9, NULL, 1);
                        }
                    }
                } else {
                    mtd_send_response(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_READ: {
                addr = length = 0;
                sscanf((const char *)param->data_ind.data, CMD_FMT_READ, &addr, &length);
                ESP_LOGI(MTD_TAG, "GET command: "CMD_FMT_READ, addr, length);

                if (length != 0) {
                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        ESP_LOGE(MTD_TAG, "target flash not found or not supported");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        ESP_LOGE(MTD_TAG, "failed to init target flash");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        data_addr = addr;
                        flash = sfud_get_device(SFUD_FLASH_DEVICE_INDEX);

                        mtd_send_response(RSP_IDX_OK);

                        xTaskCreatePinnedToCore(mtd_read_task, "mtdReadT", 4096, NULL, 9, NULL, 1);
                    }
                } else {
                    mtd_send_response(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_INFO: {
                ESP_LOGI(MTD_TAG, "GET command: "CMD_FMT_INFO);

                sfud_err err = sfud_init();
                if (err == SFUD_ERR_NOT_FOUND) {
                    ESP_LOGE(MTD_TAG, "target flash not found or not supported");

                    mtd_send_response(RSP_IDX_FAIL);
                } else if (err != SFUD_SUCCESS) {
                    ESP_LOGE(MTD_TAG, "failed to init target flash");

                    mtd_send_response(RSP_IDX_FAIL);
                } else {
                    const sfud_mf mf_table[] = SFUD_MF_TABLE;
                    const char *flash_mf_name = NULL;

                    flash = sfud_get_device(SFUD_FLASH_DEVICE_INDEX);

                    for (int i = 0; i < sizeof(mf_table) / sizeof(sfud_mf); i++) {
                        if (mf_table[i].id == flash->chip.mf_id) {
                            flash_mf_name = mf_table[i].name;
                            break;
                        }
                    }

                    char rsp_str[40] = {0};
                    if (flash_mf_name && flash->chip.name) {
                        snprintf(rsp_str, sizeof(rsp_str), "%s,%s,%u\r\n", flash_mf_name, flash->chip.name, flash->chip.capacity);
                        ESP_LOGI(MTD_TAG, "manufactor: %s, chip name: %s, capacity: %u bytes", flash_mf_name, flash->chip.name, flash->chip.capacity);
                    } else if (flash_mf_name) {
                        snprintf(rsp_str, sizeof(rsp_str), "%s,%u\r\n", flash_mf_name, flash->chip.capacity);
                        ESP_LOGI(MTD_TAG, "manufactor: %s, capacity: %u bytes", flash_mf_name, flash->chip.capacity);
                    } else {
                        snprintf(rsp_str, sizeof(rsp_str), "%u\r\n", flash->chip.capacity);
                        ESP_LOGI(MTD_TAG, "capacity: %u bytes", flash->chip.capacity);
                    }

                    mtd_send_data(rsp_str, strlen(rsp_str));
                }

                break;
            }
            default:
                ESP_LOGW(MTD_TAG, "unknown command");

                mtd_send_response(RSP_IDX_ERROR);

                break;
        }
    } else {
        if (mtd_buff) {
            xRingbufferSend(mtd_buff, param->data_ind.data, param->data_ind.len, portMAX_DELAY);
        }
    }
}

void mtd_update(bool cong, bool sent)
{
    data_cong = cong;
    data_sent = sent;
}

void mtd_end(void)
{
    conn_handle = 0;

    data_err = false;
    data_recv = false;
}
