/*
 * mp3_decoder.c
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_NO_STDIO

#include "Mp3Decoder.h"

static mp3dec_t mp3d;
static mp3dec_file_info_t info;

void Mp3_Init(void)
{
	 mp3dec_init(&mp3d);
}


mp3dec_file_info_t* Mp3_decode(unsigned char* input_buffer,unsigned int buffer_size)
{
	free(info.buffer);

	mp3dec_load_buf(&mp3d,input_buffer,buffer_size,&info,0,0);

    return &info;
}

