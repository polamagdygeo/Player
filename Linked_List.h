/*
 * linked_list_h.c
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#ifndef LINKED_LIST_H_C_
#define LINKED_LIST_H_C_

#include <stdint.h>

typedef struct list tList;

tList* List_Create(void);
uint8_t List_Append(tList* list,void* n);
void* List_GetDataAt(tList* list,uint8_t idx);
uint8_t List_Count(tList* list);
uint8_t List_Remove(tList* list,uint8_t idx);

#endif /* LINKED_LIST_H_C_ */
