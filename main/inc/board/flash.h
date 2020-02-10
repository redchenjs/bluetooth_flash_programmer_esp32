/*
 * flash.h
 *
 *  Created on: 2018-03-16 12:30
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_BOARD_FLASH_H_
#define INC_BOARD_FLASH_H_

#include <stdint.h>

extern void flash_init(void);

extern void flash_setpin_cs(uint8_t val);

extern void flash_write_read(const uint8_t *write_buf, size_t write_size, uint8_t *read_buf, size_t read_size);

#endif /* INC_BOARD_FLASH_H_ */
