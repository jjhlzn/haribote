#ifndef	_LINKEDLIST_H
#define	_LINKEDLIST_H


struct Node {
	void *data;
	struct Node *next;
};

void Add(struct Node *node, struct Node *newNode);
void Append(struct Node *head, struct Node *newNode);
struct Node *CreateNode(void *data);
void FreeNode(struct Node *data);
int GetSize(struct Node *head);
#endif




