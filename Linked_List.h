/**
  ******************************************************************************
  * @file           : 
  * @version        :
  * @brief          :
  ******************************************************************************
  ******************************************************************************
*/

#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
typedef struct list tList;
typedef uint8_t (*pTravFunc)(void*,void*,void**);

/* Exported functions -------------------------------------------------------*/
tList* List_Create(void);
uint8_t List_Append(tList* list,void* n);
void* List_GetDataAt(tList* list,uint8_t idx);
uint8_t List_Count(tList* list);
uint8_t List_Remove(tList* list,uint8_t idx);
uint8_t List_Traverse(tList* list,pTravFunc p_trav_func,void *ptr_param,void **ptr_ptr_param);

#endif /* LINKED_LIST_H_C_ */
