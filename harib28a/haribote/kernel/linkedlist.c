#include "bootpack.h"
#include "linkedlist.h"


static struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

/* ���һ���µĽڵ�ŵ�node���� */
void Add(struct Node *node, struct Node *newNode){
	assert(node != NULL);
	
	struct Node *oldNext = node->next;
	node->next = newNode;
	newNode->next = oldNext;
}

void Append(struct Node *head, struct Node *newNode){
	assert(head != NULL);
	
	//�ҵ����һ���ڵ�
	struct Node *tail = head;
	while(tail->next != NULL){
		tail = tail->next;
	}
	
	Add(tail,newNode);
}

int GetSize(struct Node *head)
{
	assert(head != NULL);
	int count = 0;
	struct Node *tail = head;
	while(tail->next != NULL){
		tail = tail->next;
		count ++;
	}
	count++;
	return count;
}

/* �����½ڵ� */
struct Node * CreateNode(void *data){
	struct Node *node = (struct Node *)memman_alloc(memman,sizeof(struct Node));
	node->data = data;
	node->next = NULL;
	return node;
}

/* �ͷŽڵ���ڴ� */
void FreeNode(struct Node *data){
	memman_free(memman,(unsigned int)data,sizeof(struct Node));
}

