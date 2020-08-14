/*
 * wave_decoder.h
 *
 *  Created on: Aug 14, 2020
 *      Author: root
 */

#ifndef WAVE_DECODER_H_
#define WAVE_DECODER_H_

#include "stdint.h"

typedef struct{
    char chunk_id[4];
    uint32_t chunk_size;
    char chunk_format[4];
    char sub_chunk1_id[4];
    uint32_t sub_chunk1_size;
    uint16_t audio_format;
    uint16_t channels_no;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char sub_chunk2_id[4];
    uint32_t sub_chunk2_size;
}__attribute__((packed, aligned(1)))tWave_FrameInfo;

typedef struct{
    uint16_t left_channel_val;
    uint16_t right_channel_val;
}tWave_PcmFormat16Bit;

typedef struct{
    uint8_t left_channel_val;
    uint8_t right_channel_val;
}tWave_PcmFormat8Bit;

int8_t Wave_Decode(uint8_t *frame_block,uint32_t block_size,tWave_FrameInfo *info,void **pcm_buffer,uint32_t *samples_no);

#endif /* WAVE_DECODER_H_ */
