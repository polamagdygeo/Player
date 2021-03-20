/*
 * wave_decoder.c
 *
 *  Created on: Aug 14, 2020
 *      Author: root
 */

#include "wave_decoder.h"
#include "string.h"
#include "math.h"

typedef struct{
    char chunk_id[4];           /*RIFF*/
    uint32_t chunk_size;
    char chunk_format[4];       /*WAVE*/
    char sub_chunk1_id[4];      /*fmt */
    uint32_t sub_chunk1_size;   /*16 for PCM*/
    uint16_t audio_format;
    uint16_t channels_no;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;       /*samples block*/
    uint16_t bits_per_sample;   /*8 ,16 ,...*/
    char sub_chunk2_id[4];      /*DATA*/
    uint32_t sub_chunk2_size;
}__attribute__((packed, aligned(1)))tWave_Frame;

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

int8_t Wave_DecodeFileSegment(const uint8_t *file_segment,uint32_t file_segment_size,tWave_DecodingInfo *info)
{
    int8_t ret = -1;
    tWave_Frame *found_frame_start = 0;

    /*frame start*/
    if(0 == info->total_samples_block_no)   /*Check if the frame start of this wave file was parsed before or not*/
    {
        /*Get frame start*/
        if((found_frame_start = (tWave_Frame *)strstr((const char*)frame_segment,"RIFF")) ||
                (found_frame_start = (tWave_Frame *)strstr((const char*)frame_segment,"RIFX")))
        {
            if(strncmp((const char*)found_frame_start,"RIFX",4) == 0) /*big endianess scheme*/
            {
                big_to_little_endian_in_place((uint8_t *)found_frame_start->chunk_format,4);
                big_to_little_endian_in_place((uint8_t *)found_frame_start->sub_chunk1_id,4);
                big_to_little_endian_in_place((uint8_t *)found_frame_start->sub_chunk2_id,4);
            }

            /*validation*/
            if(strncmp(found_frame_start->chunk_format,"WAVE",4) == 0 &&
                    strncmp(found_frame_start->sub_chunk1_id,"fmt ",4) == 0 &&
                    found_frame_start->audio_format == 1 &&   /*PCM*/
                    strncmp(found_frame_start->sub_chunk2_id,"data",4) == 0)
            {
                ret = 0; /*valid*/
            }

            if(!ret)
            {
                info->channels_no = found_frame_start->channels_no;
                info->bits_per_sample = found_frame_start->bits_per_sample;
                info->block_of_samples_size = found_frame_start->block_align;
                info->sample_period_us = round(1000000.0 / found_frame_start->sample_rate);

                info->pcm_buffer = (uint8_t*)found_frame_start + sizeof(tWave_Frame);
                info->finished_blocks_of_samples_no = (frame_segment_size - sizeof(tWave_Frame)) / info->block_of_samples_size;
                info->last_read_blocks_of_samples_no = info->finished_blocks_of_samples_no;
                info->total_samples_block_no = found_frame_start->sub_chunk2_size / info->block_of_samples_size;
            }
        }
    }
    else
    {
        /*remaining data*/
        info->pcm_buffer = frame_segment;
        info->last_read_blocks_of_samples_no = frame_segment_size / info->block_of_samples_size;
        info->finished_blocks_of_samples_no += info->last_read_blocks_of_samples_no;

        if(info->finished_blocks_of_samples_no >= info->total_samples_block_no)
        {
            info->last_read_blocks_of_samples_no = frame_segment_size - (info->finished_blocks_of_samples_no - info->total_samples_block_no);
            info->finished_blocks_of_samples_no = info->total_samples_block_no;
            ret = 1;
        }
        else
        {
            ret = 0;
        }
    }

    return ret;
}
