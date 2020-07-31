/*
 * mp3_decoder.h
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#ifndef MP3DECODER_H_
#define MP3DECODER_H_

#include "minimp3_ex.h"

void Mp3_Init(void);
mp3dec_file_info_t* Mp3_decode(unsigned char* input_buffer,unsigned int buffer_size);

#endif /* MP3DECODER_H_ */
