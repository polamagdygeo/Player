/**
 ******************************************************************************
 * @file           : 
 * @brief          : 
 ******************************************************************************
 */

/* Private includes ----------------------------------------------------------*/
#include "Linked_List.h"
#include "mem.h"

typedef struct tList_Node{
	struct tList_Node* pNext;
	struct tList_Node* pPrev;
	void* data;
}tList_Node;

typedef struct list{
	tList_Node* pTail;
	tList_Node* pHead;
	uint8_t nodes_number;
}tList;

/* Private function prototypes -----------------------------------------------*/
static tList_Node* List_CreateNode(tList* list,void* s);
static tList_Node* List_GetNode(tList* list,uint8_t idx);

/**
    *@brief 
    *@param void
    *@retval void
*/
static tList_Node* List_CreateNode(tList* list,void* s)
{
	tList_Node* pN = os_zalloc(sizeof(tList_Node));
	if(pN!=0)
	{
		pN->data = s;
		pN->pNext = 0;
		pN->pPrev = 0;
		list->nodes_number++;
	}
	return pN;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
static tList_Node* List_GetNode(tList* list,uint8_t idx)
{
	uint8_t i;
	tList_Node* p_curr_node = list->pHead;

	if(p_curr_node == 0)
	{
		return p_curr_node;
	}

	for(i = 0 ; i < idx ; i++)
	{
		if(p_curr_node->pNext != 0)
		{
			p_curr_node = p_curr_node->pNext;
		}
		else
		{
			p_curr_node = 0;
			break;
		}
	}

	return p_curr_node;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
tList* List_Create(void)
{
	tList* pL = os_zalloc(sizeof(tList));
	if(pL!=0)
	{
		pL->pTail = 0;
		pL->pHead = 0;
		pL->nodes_number = 0;
	}
	return pL;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
uint8_t List_Append(tList* list,void* s)
{
	uint8_t ret_val = 0;

	tList_Node* pN = List_CreateNode(list,s);

	if(list != 0 &&
		pN != 0)
	{
		if(list->pTail == 0)
		{
			list->pTail = pN;
			list->pHead = pN;
		}
		else
		{
			pN->pPrev = list->pTail;
			list->pTail->pNext = pN;
			list->pTail = pN;
		}

		ret_val = 1;
	}

	return ret_val;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
void* List_GetDataAt(tList* list,uint8_t idx)
{
	void* d = 0;
	
	if(list != 0 &&
		list->nodes_number != 0 &&
			list->nodes_number >= idx)
	{
		d = List_GetNode(list,idx)->data;
	}
	
	return d;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
uint8_t List_Count(tList* list)
{
	uint8_t ret = 0;

	if(list != 0)
	{
		ret = list->nodes_number;
	}

	return ret;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
uint8_t List_Remove(tList* list,uint8_t idx)
{
	tList_Node *node = 0;
	void* d = 0;
	uint8_t ret = 0;

	if(list != 0 &&
		list->nodes_number != 0 &&
		list->nodes_number >= idx)
	{
		node = List_GetNode(list,idx);
		node->pPrev->pNext = node->pNext;
		node->pNext->pPrev = node->pPrev;
		d = node->data;
		os_free(d);
		os_free(node);
		ret = 1;
	}

	return ret;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
uint8_t List_Traverse(tList* list,pTravFunc p_trav_func,void *ptr_param,void **ptr_ptr_param)
{
	uint8_t i;
	tList_Node* p_curr_node = list->pHead;
	uint8_t isDone = 0;

	for(i = 0 ; i < List_Count(list); i++)
	{
		isDone = p_trav_func(p_curr_node->data,ptr_param,ptr_ptr_param);

		if(p_curr_node->pNext != 0 && isDone == 0)
		{
			p_curr_node = p_curr_node->pNext;
		}
		else
		{
			break;
		}
		
	}

	return i;
}

