/*
 * SD_Card.h
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#ifndef SD_CARD_H_
#define SD_CARD_H_

#include <stdint.h>

#define SD_BLOCK_SIZE 			(512U)

int8_t Sd_Init(void);
uint8_t Sd_ReadBlock(uint32_t data_block_addr,uint8_t* buffer);
uint8_t Sd_WriteBlock(uint32_t data_block_addr,uint8_t* buffer);
uint8_t Sd_ReadMultipleBlock(uint32_t data_block_addr,uint8_t blocks_number,uint8_t* buffer);
uint8_t Sd_WriteMultipleBlock(uint32_t data_block_addr,uint8_t blocks_number,uint8_t* buffer);

#endif /* SD_CARD_H_ */
