#ifndef H_HEAP
#define H_HEAP

#include "shared.h"

typedef enum Heap_Node_Type {
	HEAP_NODE_FREE = 0x3418972F, // magic numbers...
	HEAP_NODE_USED = 0x7A727D7B
} Heap_Node_Type;

typedef struct HeapNode {
	Heap_Node_Type type;
	uint32_t       size;
} HeapNode;

typedef struct Heap {
	uint32_t size;
	void*    data;
	char*    description;
} Heap;

Heap* Heap_Init(uint32_t size, char* description);
bool  Heap_Clear(Heap* heap);
void* Heap_Alloc(Heap* heap, uint32_t size);
bool  Heap_Dealloc(Heap* heap, void* ptr);
void  Heap_Destroy(Heap* heap);
void  Heap_Print(Heap* heap, const char* title);

#endif /* H_HEAP */