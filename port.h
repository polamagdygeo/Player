/*
 * port.h
 *
 *  Created on: May 2, 2020
 *      Author: pola
 */

#ifndef PORT_H_
#define PORT_H_

#include "inc/hw_memmap.h"

#define BUTTON_PORT             GPIO_PORTB_BASE
#define BUTTON_UP_PIN           0
#define BUTTON_DOWN_PIN         1
#define BUTTON_OK_PIN           2

#define SPI_PORT                GPIO_PORTB_BASE
#define SPI_CLK_PIN             4
#define SPI_CS_PIN              5
#define SPI_MISO_PIN            6
#define SPI_MOSI_PIN            7


#endif /* PORT_H_ */
