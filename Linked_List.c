/**
 ******************************************************************************
 * @file           : 
 * @brief          : 
 ******************************************************************************
 */

/* Private includes ----------------------------------------------------------*/
#include "Linked_List.h"
#include <stdlib.h>

typedef struct tList_Node{
	struct tList_Node* p_next;
	struct tList_Node* p_prev;
	void* data;
}tList_Node;

struct list{
	tList_Node* p_tail;
	tList_Node* p_head;
	uint8_t nodes_number;
};

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
	tList_Node* p_node = calloc(1,sizeof(tList_Node));
	if(p_node!=0)
	{
		p_node->data = s;
		p_node->p_next = 0;
		p_node->p_prev = 0;
		list->nodes_number++;
	}
	return p_node;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
static tList_Node* List_GetNode(tList* list,uint8_t idx)
{
	uint8_t i;
	tList_Node* p_curr_node = list->p_head;

	if(p_curr_node == 0)
	{
		return p_curr_node;
	}

	for(i = 0 ; i < idx ; i++)
	{
		if(p_curr_node->p_next != 0)
		{
			p_curr_node = p_curr_node->p_next;
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
	tList* p_list = calloc(1,sizeof(tList));
	if(p_list!=0)
	{
		p_list->p_tail = 0;
		p_list->p_head = 0;
		p_list->nodes_number = 0;
	}
	return p_list;
}

/**
    *@brief 
    *@param void
    *@retval void
*/
uint8_t List_Append(tList* list,void* s)
{
	uint8_t ret_val = 0;

	tList_Node* p_node = List_CreateNode(list,s);

	if(list != 0 &&
		p_node != 0)
	{
		if(list->p_tail == 0)
		{
			list->p_tail = p_node;
			list->p_head = p_node;
		}
		else
		{
			p_node->p_prev = list->p_tail;
			list->p_tail->p_next = p_node;
			list->p_tail = p_node;
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
		node->p_prev->p_next = node->p_next;
		node->p_next->p_prev = node->p_prev;
		d = node->data;
		free(d);
		free(node);
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
	tList_Node* p_curr_node = list->p_head;
	uint8_t isDone = 0;

	for(i = 0 ; i < List_Count(list); i++)
	{
		isDone = p_trav_func(p_curr_node->data,ptr_param,ptr_ptr_param);

		if(p_curr_node->p_next != 0 && isDone == 0)
		{
			p_curr_node = p_curr_node->p_next;
		}
		else
		{
			break;
		}
		
	}

	return i;
}

