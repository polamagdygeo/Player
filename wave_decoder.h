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
    uint32_t sample_period_us;
    uint32_t total_samples_block_no;
    uint32_t read_samples_block_no;
    uint32_t last_read_samples_block_no;
    void *pcm_buffer;
    uint8_t channels_no;
    uint8_t bits_per_sample;
    uint8_t samples_block_size;
}tWave_DecodingInfo;

typedef struct{
    int16_t left_channel_val;
    int16_t right_channel_val;
}tWave_RawPcmStreo16BitSample;

typedef struct{
    uint8_t left_channel_val;
    uint8_t right_channel_val;
}tWave_RawPcmStreo8BitSample;

typedef int8_t  tWave_RawPcmMono16BitSample;
typedef uint8_t tWave_RawPcmMono8BitSample;

int8_t Wave_DecodeFrameSegment(uint8_t *frame_segment,uint32_t frame_segment_size,tWave_DecodingInfo *info);

#endif /* WAVE_DECODER_H_ */
