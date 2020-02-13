/*
 * mtd.h
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_MTD_H_
#define INC_USER_MTD_H_

#include "esp_spp_api.h"

extern void mtd_exec(esp_spp_cb_param_t *param);
extern void mtd_cong(bool cong, bool flag);
extern void mtd_end(void);

#endif /* INC_USER_MTD_H_ */
