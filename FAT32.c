/*
 * FAT.c
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#include "FAT32.h"
#include "string.h"
#include <stdlib.h>
#include "limits.h"
#include "Linked_List.h"
#include "SD_Card.h"

#define VOL_CONF_BYTE_PER_SECT_OFFSET		11
#define VOL_CONF_RES_AREA_SIZE_OFFSET		14
#define VOL_CONF_FAT_SEC_SEC_NO_OFFSET		36
#define VOL_CONF_FIRST_CLUST_IDX_OFFSET		44
#define VOL_CONF_HIDDEN_SECT_OFFSET			28
#define VOL_CONF_TOTAL_SECT_OFFSET			32

typedef uint32_t tFATEntry;

typedef enum{
	FAT_ENTRY_FREE_LOWER_LIM_CLUSTER 	= 0x00000000,
	FAT_ENTRY_FREE_UPPER_LIM_CLUSTER 	= 0x10000000,
	FAT_RESERVED_LOWER_LIM_CLUSTER 		= 0x00000001,
	FAT_RESERVED_UPPER_LIM_CLUSTER 		= 0x10000001,
	FAT_USED_LOWER_LIM_CLUSTER 			= 0x00000002,
	FAT_USED_UPPER_LIM_CLUSTER 			= 0x1FFFFFEF,
	FAT_RESERVED_LOWER_LIM_VAL 			= 0x0FFFFFF0,
	FAT_RESERVED_UPPER_LIM_VAL 			= 0x1FFFFFF6,
	FAT_BAD_CLUSTER 					= 0x0FFFFFF7,
	FAT_EOC_CLUSTER 					= 0x0FFFFFF8
}tFAT32_ClusterVal;

typedef enum{
	ATTR_READ_ONLY 						= 0x01,
	ATTR_HIDDEN_FILE 					= 0x02,
	ATTR_SYS_FILE 						= 0x04,
	ATTR_VOL_LABEL 						= 0x08,
	ATTR_LONG_FILE_NAME 				= 0x0f,
	ATTR_SUBDIRECTORY 					= 0x10, /*Has size of zero*/
	ATTR_ARCHEIVE 						= 0x20
}tAttrMask;

typedef enum{
	FILE_NO_SUBSEQUENT 					= 0x00,
	FILE_FIRST_CHAR_0XE5 				= 0x05,
	FILE_DOT_ENTRY 						= 0x2E, /*Root or parent directory*/
	FILE_DELETED 						= 0xe5
}tFileStatus;

typedef struct{
	uint32_t fat_table_sectors_no;
	uint32_t root_dir_starting_cluster_idx;
	uint32_t hidden_sectors_no;
	uint32_t total_sectors_no;
	uint16_t byts_per_sector;
	uint16_t reserved_area_sectors_no;
	uint8_t sector_per_cluster;
	uint8_t fat_table_entries_no;
}tBootArea;

/*in sector unit*/
typedef struct{
    uint64_t boot_area_start_sector;
	uint64_t fat_table_start_sector;
	uint64_t data_start_sector;
}tVolume_Map;

static tBootArea volume_boot_area;
static tVolume_Map volume_map;
static tList *files_list_ptr;

static void	FAT32_ParseVolumeBootArea(void);
static void	FAT32_CalcVolumeMapBoundries(void);
static uint32_t FAT32_CalcFileSectorsNo(uint32_t File_Size_In_Bytes);
static uint32_t FAT32_GetClusterFirstSectorId(uint32_t cluster_idx);
static void FAT32_GetClusterAssocFatSectorAndOffset(uint32_t cluster_idx,uint32_t* FAT_Sector,uint32_t* FAT_Offset);
static uint32_t FAT32_GetNextChainCluster(uint32_t cluster_idx);
static void FAT32_AppendFileInfoToList(tDirectoryEntry* s);
static void FAT32_ScanDirectory(uint32_t dir_first_cluster_idx);
static tDirectoryEntry* FAT32_GetSearchFileWithIndex(uint8_t index);

static void	FAT32_ParseVolumeBootArea(void)
{
	uint8_t sector_buffer_tmp[SD_BLOCK_SIZE];
    uint32_t idx;

    for(idx = 0 ; idx < UINT_MAX ;)
    {
        if(Sd_ReadBlock((idx)*SD_BLOCK_SIZE,sector_buffer_tmp) == 1)
        {
            /*Searching for the OEM is not enough also getting first sector not always contain BRB info*/
            if(strcmp((const char*)(sector_buffer_tmp+3),(const char*)"MSDOS5.0") == 0)
            {
                volume_map.boot_area_start_sector = idx;
                volume_boot_area.byts_per_sector = *((uint16_t*)(sector_buffer_tmp + VOL_CONF_BYTE_PER_SECT_OFFSET));
                volume_boot_area.reserved_area_sectors_no = *((uint16_t*)(sector_buffer_tmp + VOL_CONF_RES_AREA_SIZE_OFFSET));
                volume_boot_area.fat_table_sectors_no = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_FAT_SEC_SEC_NO_OFFSET));
                volume_boot_area.root_dir_starting_cluster_idx = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_FIRST_CLUST_IDX_OFFSET));
                volume_boot_area.hidden_sectors_no = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_HIDDEN_SECT_OFFSET));
                volume_boot_area.sector_per_cluster = sector_buffer_tmp[13];
                volume_boot_area.fat_table_entries_no = sector_buffer_tmp[16];
                volume_boot_area.total_sectors_no = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_TOTAL_SECT_OFFSET));
                break;
            }

            idx++;
        }
    }
}

static void	FAT32_CalcVolumeMapBoundries(void)
{
	volume_map.fat_table_start_sector = volume_map.boot_area_start_sector +
	        volume_boot_area.reserved_area_sectors_no;

	volume_map.data_start_sector = volume_map.fat_table_start_sector +
			(volume_boot_area.fat_table_sectors_no*volume_boot_area.fat_table_entries_no);
}


static uint32_t FAT32_CalcFileSectorsNo(uint32_t File_Size_In_Bytes)
{
	return (File_Size_In_Bytes / (volume_boot_area.byts_per_sector)) + 1;

}

static uint32_t FAT32_GetClusterFirstSectorId(uint32_t cluster_idx)
{
	uint32_t Cluster_First_SectorID = 0;

	if(cluster_idx >= 2)
	{
		Cluster_First_SectorID = volume_map.data_start_sector + ((cluster_idx-2)*volume_boot_area.sector_per_cluster);
	}

	return Cluster_First_SectorID;
}


static void FAT32_GetClusterAssocFatSectorAndOffset(uint32_t cluster_idx,uint32_t* FAT_Sector,uint32_t* FAT_Offset)
{
	uint32_t FAT_Id = cluster_idx;
	uint32_t FATperSector = (volume_boot_area.byts_per_sector) / sizeof(tFATEntry);

	*FAT_Sector = volume_map.fat_table_start_sector + (FAT_Id/FATperSector);
	*FAT_Offset = FAT_Id % FATperSector;
}

static uint32_t FAT32_GetNextChainCluster(uint32_t cluster_idx)
{
	uint8_t sector_buffer_tmp[SD_BLOCK_SIZE];
	uint32_t next_cluster = 0;
	uint32_t cluster_FAT_sector;
	uint32_t cluster_FAT_Offset;
	tFATEntry* pFAT_Entry;

	FAT32_GetClusterAssocFatSectorAndOffset(cluster_idx,&cluster_FAT_sector,&cluster_FAT_Offset);

	memset(sector_buffer_tmp,0xff,SD_BLOCK_SIZE);

	if(Sd_ReadBlock(cluster_FAT_sector * SD_BLOCK_SIZE,sector_buffer_tmp) == 1)
	{
		pFAT_Entry = (tFATEntry*)sector_buffer_tmp;	
		next_cluster = pFAT_Entry[cluster_FAT_Offset];
	}

	return next_cluster;
}


static void FAT32_AppendFileInfoToList(tDirectoryEntry* s)
{
	tDirectoryEntry *pS = malloc(sizeof(tDirectoryEntry));

	memcpy(pS,s,sizeof(tDirectoryEntry));

	List_Append(files_list_ptr,pS);
}

static void FAT32_ScanDirectory(uint32_t dir_first_cluster_idx)
{
	const uint8_t root_entries_per_sector = volume_boot_area.byts_per_sector / sizeof(tDirectoryEntry);
	uint32_t curr_clusters_curr_sector_id;
	uint32_t dir_curr_cluster = dir_first_cluster_idx;
	uint8_t sector_buffer_tmp[SD_BLOCK_SIZE];
	uint16_t root_entery_iterator;
	uint16_t sector_iterator = 0;
	tDirectoryEntry* pDirectory_Entry;
	
	do{
		/*iterate on clusters entries*/
		curr_clusters_curr_sector_id = FAT32_GetClusterFirstSectorId(dir_curr_cluster);
		
		do 
		{
			memset(sector_buffer_tmp,0xff,SD_BLOCK_SIZE);

			if(Sd_ReadBlock(curr_clusters_curr_sector_id * SD_BLOCK_SIZE,sector_buffer_tmp) == 1)
			{
				pDirectory_Entry = (tDirectoryEntry*)&sector_buffer_tmp;

				for(root_entery_iterator = 0 ; root_entery_iterator < root_entries_per_sector ; root_entery_iterator++)
				{
					if(pDirectory_Entry != 0)
					{
						if(pDirectory_Entry->file_name[0] == FILE_NO_SUBSEQUENT)
						{
							return;
						}
						else if (pDirectory_Entry->file_name[0] != FILE_DELETED) /*deleted*/	
						{
						    /*Attributes check should be revised*/
							if( pDirectory_Entry->attr == 0x0f ||
							    (pDirectory_Entry->attr & ATTR_ARCHEIVE) != 0 ||
							    (pDirectory_Entry->attr & ATTR_HIDDEN_FILE == 0 &&
								pDirectory_Entry->attr & ATTR_SYS_FILE == 0 &&
								pDirectory_Entry->attr & ATTR_VOL_LABEL  == 0 &&
								pDirectory_Entry->attr & ATTR_SUBDIRECTORY == 0))
							{
								FAT32_AppendFileInfoToList(pDirectory_Entry);
							}
							else if(pDirectory_Entry->attr & ATTR_SUBDIRECTORY != 0 &&
										pDirectory_Entry->file_size == 0)
							{
								/*add sub directory*/
							}
						}
					}

					/*Next Entry*/
					pDirectory_Entry++;
				}

				curr_clusters_curr_sector_id++;

				sector_iterator++;
			}
		}while(sector_iterator <= volume_boot_area.sector_per_cluster);
		
		sector_iterator = 0;
		
		dir_curr_cluster = FAT32_GetNextChainCluster(dir_curr_cluster);
		
	}while(dir_curr_cluster != FAT_EOC_CLUSTER && 
		dir_curr_cluster >= FAT_USED_LOWER_LIM_CLUSTER &&
		dir_curr_cluster <= FAT_USED_UPPER_LIM_CLUSTER);
}

void FAT32_Init(void)
{
	files_list_ptr = List_Create();

	if(files_list_ptr == 0)
		while(1);

	FAT32_ParseVolumeBootArea();
	FAT32_CalcVolumeMapBoundries();
	FAT32_ScanDirectory(volume_boot_area.root_dir_starting_cluster_idx);

	/*
		Scan loop on directory queue (init with root dir first cluster idx) as long as it has elements
	*/
}

uint8_t FAT32_FilesCount(void)
{
	return List_Count(files_list_ptr);
}

static tDirectoryEntry* FAT32_GetSearchFileWithIndex(uint8_t index)
{
	tDirectoryEntry* ret_val = 0;

	if(index < FAT32_FilesCount())
	{
		ret_val = List_GetDataAt(files_list_ptr,index);
	}

	return ret_val;
}


int8_t FAT32_ReadFileAsBlocks(tDirectoryEntry* s,uint8_t (*pBuffer)[SD_BLOCK_SIZE])
{
	static uint8_t in_progress = 0;
	static uint32_t curr_cluster_idx;
	static uint32_t curr_cluster_sector_idx;
	static uint32_t overall_file_sectors_number;
	static uint32_t file_sectors_itr;
	int8_t ret_val = -1;
	
	if(in_progress == 0)
	{
		in_progress = 1;

		curr_cluster_idx = s->starting_cluster;

		curr_cluster_sector_idx = FAT32_GetClusterFirstSectorId(curr_cluster_idx);

		overall_file_sectors_number = FAT32_CalcFileSectorsNo(s->file_size);

		file_sectors_itr = 0;
	}

	if(Sd_ReadBlock(curr_cluster_sector_idx * SD_BLOCK_SIZE,*pBuffer) == 1)
	{
		curr_cluster_sector_idx++;

		file_sectors_itr++;

		if(file_sectors_itr == overall_file_sectors_number) /*if file sectors completed*/
		{
			in_progress = 0;

			ret_val = 1; /*success*/
		}
		else if(file_sectors_itr % volume_boot_area.sector_per_cluster == 0)
		{
			curr_cluster_idx = FAT32_GetNextChainCluster(curr_cluster_idx);

			if(curr_cluster_idx == FAT_EOC_CLUSTER)
			{
				in_progress = 0; /*Considered fail as file sectors wasn't completed*/
			}
			else if(curr_cluster_idx >= FAT_USED_LOWER_LIM_CLUSTER &&
						curr_cluster_idx <= FAT_USED_UPPER_LIM_CLUSTER)
			{
				curr_cluster_sector_idx = FAT32_GetClusterFirstSectorId(curr_cluster_idx);

				ret_val = 0; /*still in progress*/
			}
			else
			{
				in_progress = 0;
			}
		}
	}

	return ret_val;
}

uint8_t FAT32_SearchFilesWithExtension(const char* extension,tDirectoryEntry** result_files,uint8_t* result_count)
{
	uint8_t ret_val = 0;
	uint8_t files_no = FAT32_FilesCount();
	uint8_t files_iterator;
	tDirectoryEntry* temp_file_info;

	*result_count = 0;

	*result_files = malloc(sizeof(tDirectoryEntry)*files_no);

	for(files_iterator = 0 ; files_iterator < files_no ; files_iterator++)
	{
		temp_file_info = FAT32_GetSearchFileWithIndex(files_iterator);

		if(strncmp((const char*)temp_file_info->ext,extension,3) == 0)
		{
			(*result_files)[*result_count] = *temp_file_info;

			(*result_count)++;

			ret_val = 1;
		}
	}

	return ret_val;
}
