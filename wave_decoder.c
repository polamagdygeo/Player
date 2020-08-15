/*
 * wave_decoder.c
 *
 *  Created on: Aug 14, 2020
 *      Author: root
 */

#include "wave_decoder.h"
#include "string.h"

/*Not used helper function*/
static void big_to_little_endian_in_place(uint8_t *start_byte_ptr,uint8_t bytes_no)
{
    uint8_t itr;

    for(itr = 0 ; itr < (bytes_no / 2) ; itr++)
    {
        uint8_t temp = start_byte_ptr[itr];
        start_byte_ptr[itr] = start_byte_ptr[bytes_no - itr - 1];
        start_byte_ptr[bytes_no - itr - 1] = temp;
    }
}

int8_t Wave_Decode(uint8_t *frame_block,uint32_t block_size,tWave_FrameInfo *info,void **pcm_buffer,uint32_t *samples_no)
{
    int8_t ret = -1;
    static tWave_FrameInfo *in_progress_info_ref;
    static uint32_t total_samples_no;
    tWave_FrameInfo *found_frame_start;
    uint32_t current_block_data_size;

    /*frame start*/
    if(in_progress_info_ref != info) /*this check for same file info is not enough as it only compares the pointers not unique data*/
    {
        /*Get frame start*/
        if(found_frame_start = strstr((const char*)frame_block,"RIFF"))
        {
            /*validation*/
            if(strncmp(found_frame_start->chunk_format,"WAVE",4) == 0 &&
                    strncmp(found_frame_start->sub_chunk1_id,"fmt ",4) == 0 &&
                    found_frame_start->audio_format == 1 &&   /*PCM*/
                    strncmp(found_frame_start->sub_chunk2_id,"data",4) == 0)
            {
                ret = 0; /*valid*/
                memcpy(info,found_frame_start,sizeof(tWave_FrameInfo));
                in_progress_info_ref = info;
            }

            if(!ret)
            {
                *pcm_buffer = (uint8_t*)info + sizeof(tWave_FrameInfo);
                *samples_no = 0;
                current_block_data_size = block_size - sizeof(tWave_FrameInfo);
                *samples_no += current_block_data_size / sizeof(tWave_PcmFormat16Bit);
                total_samples_no = info->sub_chunk2_size / sizeof(tWave_PcmFormat16Bit);
            }
        }
    }
    else
    {
        /*remaining data*/
        *pcm_buffer = frame_block;
        *samples_no += block_size / sizeof(tWave_PcmFormat16Bit);

        if(*samples_no >= total_samples_no)
        {
            in_progress_info_ref = 0;
            ret = 1;
        }
        else
        {
            ret = 0;
        }
    }

    return ret;
}
