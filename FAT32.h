/*
 * FAT.h
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#ifndef FAT32_H_
#define FAT32_H_

#include <stdint.h>

void FAT32_Init(void);
int8_t FAT32_ReadFileAsBlocks(char* file_name,uint8_t (*pBuffer)[SD_BLOCK_SIZE]);
uint8_t FAT32_ListFiles(char** result_files,uint8_t* result_count);
uint8_t FAT32_ListDirs(char** result_dir,uint8_t* result_count);
uint8_t FAT32_MountRelativeDir(char *dir_name);
uint8_t FAT32_MountParentDir(void);


#endif /* FAT32_H_ */
