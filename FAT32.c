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
}tFatDirEntry;	/*in FAT32 standard directory entries are part of the data section in volume map*/

typedef struct{
	tFatDirEntry dir_info;
	tList *child_list;
	tDir *parent;
}tDir;

typedef struct{
	uint32_t fat_table_sectors_no;
	uint32_t root_dir_starting_cluster_idx;
	uint32_t hidden_sectors_no;
	uint32_t total_sectors_no;
	uint16_t byts_per_sector;
	uint16_t reserved_area_sectors_no;
	uint8_t sector_per_cluster;
	uint8_t fat_table_entries_no;
}tBPB_Info;

/*in sector unit*/
typedef struct{
    uint64_t boot_area_start_sector;
	uint64_t fat_table_start_sector;
	uint64_t data_start_sector;
}tVolume_Map;

static tBPB_Info bpb_info;
static tVolume_Map volume_map;
static tDir root_dir;
static tDir *active_dir_ptr = &root_dir;

static void	FAT32_ExtractBPBInfo(void);
static void	FAT32_CalcVolumeMapBoundries(void);
static uint32_t FAT32_CalcFileSectorsNo(uint32_t File_Size_In_Bytes);
static uint32_t FAT32_GetClusterFirstSectorId(uint32_t cluster_idx);
static void FAT32_GetClusterAssocFatSectorAndOffset(uint32_t cluster_idx,uint32_t* FAT_Sector,uint32_t* FAT_Offset);
static uint32_t FAT32_GetNextChainCluster(uint32_t cluster_idx);
static void FAT32_AppendFileToDirChildList(tFatDirEntry* parent,tFatDirEntry* s);
static void FAT32_ScanDir(uint32_t dir_first_cluster_idx);
static tFatDirEntry* FAT32_GetSearchFileWithIndex(uint8_t index);
static uint8_t FAT32_CompareDirName(void* ptr_dir,void *dir_name,void** dir);
static uint8_t FAT32_AppendNameIfDir(void* ptr_dir,void *count,void** dir_names_arr);
static uint8_t FAT32_AppendNameIfFile(void* ptr_dir,void *count,void** file_names_arr);
static void FAT32_GetFileWithName(char* file_name,tDir **dir);
static uint8_t FAT32_CompareFileName(void* ptr_dir,void *file_name,void** dir);

/**
    *@brief
    *@param void
    *@retval void
*/
static void	FAT32_ExtractBPBInfo(void)
{
	uint8_t sector_buffer_tmp[SD_BLOCK_SIZE];
    uint32_t idx;

    for(idx = 0 ; idx < UINT_MAX ;)
    {
        if(Sd_ReadBlock((idx)*SD_BLOCK_SIZE,sector_buffer_tmp) == 1)
        {
            /*Searching for the OEM is not enough also getting first sector not always contain BPB (BIOS parameter block) info*/
            if(strcmp((const char*)(sector_buffer_tmp+3),(const char*)"MSDOS5.0") == 0)
            {
                volume_map.boot_area_start_sector = idx;
                bpb_info.byts_per_sector = *((uint16_t*)(sector_buffer_tmp + VOL_CONF_BYTE_PER_SECT_OFFSET));
                bpb_info.reserved_area_sectors_no = *((uint16_t*)(sector_buffer_tmp + VOL_CONF_RES_AREA_SIZE_OFFSET));
                bpb_info.fat_table_sectors_no = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_FAT_SEC_SEC_NO_OFFSET));
                bpb_info.root_dir_starting_cluster_idx = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_FIRST_CLUST_IDX_OFFSET));
                bpb_info.hidden_sectors_no = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_HIDDEN_SECT_OFFSET));
                bpb_info.sector_per_cluster = sector_buffer_tmp[13];
                bpb_info.fat_table_entries_no = sector_buffer_tmp[16];
                bpb_info.total_sectors_no = *((uint32_t*)(sector_buffer_tmp + VOL_CONF_TOTAL_SECT_OFFSET));
                break;
            }

            idx++;
        }
    }
}

/**
    *@brief
    *@param void
    *@retval void
*/
static void	FAT32_CalcVolumeMapBoundries(void)
{
	volume_map.fat_table_start_sector = volume_map.boot_area_start_sector +
	        bpb_info.reserved_area_sectors_no;

	volume_map.data_start_sector = volume_map.fat_table_start_sector +
			(bpb_info.fat_table_sectors_no*bpb_info.fat_table_entries_no);
}

/**
    *@brief
    *@param void
    *@retval void
*/
static uint32_t FAT32_CalcFileSectorsNo(uint32_t File_Size_In_Bytes)
{
	return (File_Size_In_Bytes / (bpb_info.byts_per_sector)) + 1;

}

/**
    *@brief
    *@param void
    *@retval void
*/
static uint32_t FAT32_GetClusterFirstSectorId(uint32_t cluster_idx)
{
	uint32_t Cluster_First_SectorID = 0;

	if(cluster_idx >= 2)
	{
		Cluster_First_SectorID = volume_map.data_start_sector + ((cluster_idx-2)*bpb_info.sector_per_cluster);
	}

	return Cluster_First_SectorID;
}

/**
    *@brief
    *@param void
    *@retval void
*/
static void FAT32_GetClusterAssocFatSectorAndOffset(uint32_t cluster_idx,uint32_t* FAT_Sector,uint32_t* FAT_Offset)
{
	uint32_t FAT_Id = cluster_idx;
	uint32_t FATperSector = (bpb_info.byts_per_sector) / sizeof(tFATEntry);

	*FAT_Sector = volume_map.fat_table_start_sector + (FAT_Id/FATperSector);
	*FAT_Offset = FAT_Id % FATperSector;
}

/**
    *@brief
    *@param void
    *@retval void
*/
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

/**
    *@brief
    *@param void
    *@retval void
*/
static void FAT32_AppendFileToDirChildList(tDir* parent,tFatDirEntry* s)
{
	tDir *pS = calloc(1,sizeof(tDir));

	if(pS)
	{
		memcpy(&pS->dir_info,s,sizeof(tFatDirEntry));

		pS->parent = parent;

		List_Append(parent->child_list,pS);
	}
}

/**
    *@brief
    *@param void
    *@retval void
*/
static void FAT32_ScanDir(tDir *parent_dir)
{
	const uint8_t max_entries_per_sector = bpb_info.byts_per_sector / sizeof(tFatDirEntry);
	uint32_t curr_clusters_curr_sector_id;
	uint32_t dir_curr_cluster = parent_dir->dir_info.starting_cluster;
	uint8_t sector_buffer_tmp[SD_BLOCK_SIZE];
	uint16_t root_entery_iterator;
	uint16_t sector_iterator = 0;
	tFatDirEntry* ptr_directory_entry;
	
	if(!parent_dir->child_list)
	{
		parent_dir->child_list = List_Create();

		if(parent_dir->child_list == 0)
			while(1);
	}

	do{
		/*iterate on clusters entries*/
		curr_clusters_curr_sector_id = FAT32_GetClusterFirstSectorId(dir_curr_cluster);
		
		do 
		{
			memset(sector_buffer_tmp,0xff,SD_BLOCK_SIZE);

			if(Sd_ReadBlock(curr_clusters_curr_sector_id * SD_BLOCK_SIZE,sector_buffer_tmp) == 1)
			{
				ptr_directory_entry = (tFatDirEntry*)&sector_buffer_tmp;

				for(root_entery_iterator = 0 ; root_entery_iterator < max_entries_per_sector ; root_entery_iterator++)
				{
					if(ptr_directory_entry != 0)
					{
						if(ptr_directory_entry->file_name[0] == FILE_NO_SUBSEQUENT)
						{
							return;
						}
						else if (ptr_directory_entry->file_name[0] != FILE_DELETED) /*deleted*/	
						{
							/*Append files and sub directories*/
						    /*Attributes check should be revised*/
							if( ptr_directory_entry->attr == 0x0f ||
							    (ptr_directory_entry->attr & ATTR_ARCHEIVE) != 0 ||
								(ptr_directory_entry->attr & ATTR_SUBDIRECTORY != 0) ||
							    (ptr_directory_entry->attr & ATTR_HIDDEN_FILE == 0 &&
								ptr_directory_entry->attr & ATTR_SYS_FILE == 0 &&
								ptr_directory_entry->attr & ATTR_VOL_LABEL  == 0))
							{
								FAT32_AppendFileToDirChildList(parent_dir,ptr_directory_entry);
							}
						}
					}

					/*Next Entry*/
					ptr_directory_entry++;
				}

				curr_clusters_curr_sector_id++;

				sector_iterator++;
			}
		}while(sector_iterator <= bpb_info.sector_per_cluster);
		
		sector_iterator = 0;
		
		dir_curr_cluster = FAT32_GetNextChainCluster(dir_curr_cluster);
		
	}while(dir_curr_cluster != FAT_EOC_CLUSTER && 
		dir_curr_cluster >= FAT_USED_LOWER_LIM_CLUSTER &&
		dir_curr_cluster <= FAT_USED_UPPER_LIM_CLUSTER);
}

/**
    *@brief
    *@param void
    *@retval void
*/
void FAT32_Init(void)
{
	root_dir->dir_info.starting_cluster = bpb_info.root_dir_starting_cluster_idx;

	FAT32_ExtractBPBInfo();
	FAT32_CalcVolumeMapBoundries();
	FAT32_ScanDir(&root_dir);

	/*
		Scan loop on directory queue (init with root dir first cluster idx) as long as it has elements
	*/
}

/**
    *@brief
    *@param void
    *@retval void
*/
static uint8_t FAT32_CompareFileName(void* ptr_dir,void *file_name,void** dir)
{
	tDir *local_ptr_dir = ptr_dir;
	uint8_t ret = 0;
	char *ext_start = strtok((char*)file_name,".");

	if((strncmp(local_ptr_dir->dir_info.file_name,(char*)file_name,8) == 0) &&
			(strncmp(local_ptr_dir->dir_info.ext,ext_start,3) == 0) &&
			(local_ptr_dir->attr != ATTR_SUBDIRECTORY))
	{
		*((tDir**)dir) = local_ptr_dir;
		ret = 1;
	}

	return ret;
}

/**
    *@brief
    *@param void
    *@retval void
*/
static void FAT32_GetFileWithName(char* file_name,tDir **dir)
{
	List_Traverse(active_dir_ptr->child_list,FAT32_CompareFileName,file_name,dir);
}

/**
    *@brief
    *@param void
    *@retval void
*/
int8_t FAT32_ReadFileAsBlocks(char* file_name,uint8_t (*pBuffer)[SD_BLOCK_SIZE])
{
	static uint8_t in_progress = 0;
	static uint32_t curr_cluster_idx;
	static uint32_t curr_cluster_sector_idx;
	static uint32_t overall_file_sectors_number;
	static uint32_t file_sectors_itr;
	static tDir *s;
	int8_t ret_val = -1;
	
	if(in_progress == 0)
	{
		FAT32_GetFileWithName(file_name,&s);

		if(s)
		{
			in_progress = 1;

			curr_cluster_idx = s->dir_info.starting_cluster;

			curr_cluster_sector_idx = FAT32_GetClusterFirstSectorId(curr_cluster_idx);

			overall_file_sectors_number = FAT32_CalcFileSectorsNo(s->dir_info.file_size);

			file_sectors_itr = 0;
		}
	}

	if((in_progress == 1) &&
			Sd_ReadBlock(curr_cluster_sector_idx * SD_BLOCK_SIZE,*pBuffer) == 1)
	{
		curr_cluster_sector_idx++;

		file_sectors_itr++;

		if(file_sectors_itr == overall_file_sectors_number) /*if file sectors completed*/
		{
			in_progress = 0;

			ret_val = 1; /*success*/
		}
		else if(file_sectors_itr % bpb_info.sector_per_cluster == 0)
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


/**
    *@brief
    *@param void
    *@retval void
*/
static uint8_t FAT32_AppendNameIfFile(void* ptr_dir,void *count,void** file_names_arr)
{
	tDir *local_ptr_dir = ptr_dir;

	if(local_ptr_dir->dir_info.attr != ATTR_SUBDIRECTORY)
	{
		((char**)file_names_arr)[(*(uint8_t*)count)++] = local_ptr_dir->dir_info.file_name_w_ext;
	}

	return 0;
}

/**
    *@brief
    *@param void
    *@retval void
*/
uint8_t FAT32_ListFiles(char** file_names_arr,uint8_t* result_count)
{
	uint8_t ret_val = 0;

	List_Traverse(active_dir_ptr->child_list,FAT32_AppendNameIfFile,result_count,file_names_arr);

	return ret_val;
}

/**
    *@brief
    *@param void
    *@retval void
*/
static uint8_t FAT32_AppendNameIfDir(void* ptr_dir,void *count,void** dir_names_arr)
{
	tDir *local_ptr_dir = ptr_dir;

	if((local_ptr_dir->dir_info.attr == ATTR_SUBDIRECTORY) &&
			(local_ptr_dir->dir_info.file_size == 0))
	{
		((char**)dir_names_arr)[(*(uint8_t*)count)++] = local_ptr_dir->dir_info.file_name_w_ext;
	}

	return 0;
}

/**
    *@brief
    *@param void
    *@retval void
*/
uint8_t FAT32_ListDirs(char** dir_names_arr,uint8_t* result_count)
{
	uint8_t ret_val = 0;

	List_Traverse(active_dir_ptr->child_list,FAT32_AppendNameIfDir,result_count,dir_names_arr);

	return ret_val;
}

/**
    *@brief
    *@param void
    *@retval void
*/
static uint8_t FAT32_CompareDirName(void* ptr_dir,void *dir_name,void** dir)
{
	tDir *local_ptr_dir = ptr_dir;
	uint8_t ret = 0;

	if((strncmp(local_ptr_dir->dir_info.file_name,(char*)dir_name,8) == 0) &&
			(local_ptr_dir->attr == ATTR_SUBDIRECTORY) &&
			(local_ptr_dir->file_size == 0))
	{
		*((tDir**)dir) = local_ptr_dir;
		ret = 1;
	}

	return ret;
}

/**
    *@brief
    *@param void
    *@retval void
*/
uint8_t FAT32_MountDir(char *dir_name)
{
	uint8_t ret = 0;
	tDir *found_dir = 0;

	List_Traverse(active_dir_ptr->child_list,FAT32_CompareDirName,dir_name,&found_dir);

	if(found_dir)
	{
		active_dir_ptr = found_dir;
		ret = 1;
	}

	return ret;
}


/**
    *@brief
    *@param void
    *@retval void
*/
char* FAT32_GetMountedDirParent(void)
{
	return active_dir_ptr->parent;
}

