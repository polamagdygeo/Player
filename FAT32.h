/*
 * FAT.h
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#ifndef FAT32_H_
#define FAT32_H_

#include <stdint.h>
#include "SD_Card.h"

typedef struct{
	uint8_t file_name[8];
	uint8_t ext[3];
	uint8_t attr;
	uint8_t file_case;
	uint8_t creation_time_sec;
	uint8_t creation_time[2];
	uint8_t creation_date[2];
	uint8_t last_access_date[2];
	uint8_t reserved[2];
	uint8_t last_mod_time[2];
	uint8_t last_mod_date[2];
	uint16_t starting_cluster;
	uint32_t file_size;
}tDirectoryEntry;	/*in FAT32 standard directory entries are part of the data section in volume map*/

void FAT32_Init(void);
uint8_t FAT32_FilesCount(void);
uint8_t FAT32_SearchFilesWithExtension(const char* extension,tDirectoryEntry** result_files,uint8_t* result_count);
int8_t FAT32_ReadFileAsBlocks(tDirectoryEntry* s,uint8_t (*pBuffer)[SD_BLOCK_SIZE]);


#endif /* FAT32_H_ */
