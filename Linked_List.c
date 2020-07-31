/*
 * linked_list_src.c
 *
 *  Created on: ??�/??�/????
 *      Author: Pola
 */

#include "Linked_List.h"
#include "stdlib.h"

typedef struct tList_Node{
	struct tList_Node* pNext;
	struct tList_Node* pPrev;
	void* data;
}tList_Node;

typedef struct list{
	tList_Node* pHead;
	tList_Node* pTail;
	uint8_t nodes_number;
}tList;

static tList_Node* List_CreateNode(tList* list,void* s);
static tList_Node* List_GetNode(tList* list,uint8_t idx);

static tList_Node* List_CreateNode(tList* list,void* s)
{
	tList_Node* pN = malloc(sizeof(tList_Node));
	if(pN!=0)
	{
		pN->data = s;
		pN->pNext = 0;
		pN->pPrev = 0;
		list->nodes_number++;
	}
	return pN;
}

static tList_Node* List_GetNode(tList* list,uint8_t idx)
{
	uint8_t i;
	tList_Node* target = list->pTail;

	if(target == 0)
	{
		return target;
	}

	for(i = 0 ; i < idx ; i++)
	{
		if(target->pNext != 0)
		{
			target = target->pNext;
		}
		else
		{
			target = 0;
			break;
		}
	}

	return target;
}

tList* List_Create(void)
{
	tList* pL = malloc(sizeof(tList));
	if(pL!=0)
	{
		pL->pHead = 0;
		pL->pTail = 0;
		pL->nodes_number = 0;
	}
	return pL;
}

uint8_t List_Append(tList* list,void* s)
{
	uint8_t ret_val = 0;

	tList_Node* pN = List_CreateNode(list,s);

	if(pN != 0)
	{
		if(list->pHead == 0)
		{
			list->pHead = pN;
			list->pTail = pN;
		}
		else
		{
			pN->pPrev = list->pHead;
			list->pHead->pNext = pN;
			list->pHead = pN;
		}

		ret_val = 1;
	}

	return ret_val;
}


void* List_GetDataAt(tList* list,uint8_t idx)
{
	void* d = 0;
	
	if(list->nodes_number != 0
			&& list->nodes_number >= idx)
	{
		d = List_GetNode(list,idx)->data;
	}
	
	return d;
}

uint8_t List_Count(tList* list)
{
	return list->nodes_number;
}

uint8_t List_Remove(tList* list,uint8_t idx)
{
	tList_Node *node = 0;
	void* d = 0;
	uint8_t ret = 0;

	if(list->nodes_number != 0
			&& list->nodes_number >= idx)
	{
		node = List_GetNode(list,idx);
		d = node->data;
		free(d);
		free(node);
		ret = 1;
	}

	return ret;
}

