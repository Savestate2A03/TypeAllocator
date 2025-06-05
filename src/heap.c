#include "shared.h"

// Some definitions to make code a bit more readable/flexibile.
#define MINIMUM_DATA_TYPE_SIZE sizeof(uint32_t)
#define ALIGNED_32_BIT(n) (n + MINIMUM_DATA_TYPE_SIZE - 1) / MINIMUM_DATA_TYPE_SIZE * MINIMUM_DATA_TYPE_SIZE
#define NODE_SIZE sizeof(HeapNode)
#define MINIMUM_DATA_SIZE NODE_SIZE + MINIMUM_DATA_TYPE_SIZE
#define DESC_LENGTH_MAX 32

Heap* Heap_Init(uint32_t size, const char* description) {
	// Require the heap to hold at least one HeapNode + one 32-bit word.
	if (size < MINIMUM_DATA_SIZE) return NULL;
	// Allocate the heap struct.
	Heap* heap = malloc(sizeof(Heap));
	if (!heap) return NULL;
	// Align heap to 4 byte boundaries before allocate the actual heap.
	// (less headache with platform-specific mystery word boundary nonsense)
	size = ALIGNED_32_BIT(size);
	heap->size = size;
	heap->data = malloc(size);
	// If a description is provided, copy up to DESC_LENGTH_MAX characters
	// into malloc'd memory for users' use and the Heap_Print function.
	if (description) {
		// If a NULL terminator is found in the first x characters of
		// the string, use that to figure out how to malloc and copy,
		// otherwise cap out at a defined length (DESC_LENGTH_MAX).
		char* terminator = memchr(description, '\0', DESC_LENGTH_MAX);
		size_t length = terminator ? terminator - description : DESC_LENGTH_MAX;
		heap->description = malloc(length + 1);
		if (heap->description) {
			// If we successfully malloc'd, copy then force a
			// null terminator regardless of the determined length.
			for (size_t i = 0; i < length; i++) {
				heap->description[i] = description[i];
			}
			heap->description[length] = '\0';
		}
	}
	else {
		heap->description = NULL;
	}
	// Prepare the heap with the first free node.
	Heap_Clear(heap);
	return heap;
}

bool Heap_Clear(Heap* heap) {
	for (size_t i = 0; i < heap->size; i++) {
		// Prepare the heap with zeros to clear out any existing data that my
		// be in it. Malloc doesn't do this automatically.
		((uint8_t*)heap->data)[i] = 0x00;
	}
	// Write the first heap node, marking it as free and updating the used
	// number of bytes for the node and the heap itself.
	HeapNode* head = heap->data;
	head->type = HEAP_NODE_FREE;
	head->size = heap->size - NODE_SIZE;
	return true;
}

HeapNode* Heap_Find_Free(Heap* heap, uint32_t size) {
	// Find the next node that is free and has the reqired size capacity.
	HeapNode* traverse = heap->data;
	while (true) {
		if (traverse - (HeapNode*)heap->data >= heap->size) {
			// If we've shot past the end of the heap, give up.
			return NULL;
		}
		if (traverse->type == HEAP_NODE_FREE && traverse->size >= size) {
			// If we find a valid free node, return it.
			return traverse;
		}
		// Cast to uint8_t* for pointer arethemtic by byte, then cast back
		// before returning to the start of the traversal loop.
		traverse = (HeapNode*)(((uint8_t*)traverse) + NODE_SIZE + traverse->size);
	}
}

bool Heap_Use_Node(HeapNode* node, uint32_t size) {
	size = ALIGNED_32_BIT(size); // While technically possible, less headache.
	if (size == 0 || node->type != HEAP_NODE_FREE || size > node->size) return false;
	// If we have the room to create a free HeapNode with at least one word
	// of free space after the data that is being placed in the heap, do so.
	// Otherwise, don't bother as we either don't have the room or it's not
	// worth making a HeapNode with size 0.
	uint32_t unused_data_size = node->size - size;
	if (unused_data_size >= MINIMUM_DATA_SIZE) {
		HeapNode* free_node = (HeapNode*)(((uint8_t*)node) + NODE_SIZE + size);
		free_node->size = unused_data_size - NODE_SIZE;
		free_node->type = HEAP_NODE_FREE;
	}
	node->type = HEAP_NODE_USED;
	node->size = size;
	return true;
}

HeapNode* Heap_Next_Node(Heap* heap, void* node) {
	// Useful for traversing forward in the heap. Uses pointer arethmitic. 
	void* heap_lo = heap->data;
	void* heap_hi = ((uint8_t*)heap->data) + heap->size - MINIMUM_DATA_SIZE;
	// Set our next node pointer to after this node + data. If it's outside
	// the bounds of the heap, then we bail with a returned NULL pointer.
	node = (HeapNode*)(((uint8_t*)node) + ((HeapNode*)node)->size + NODE_SIZE);
	if (node >= heap_hi || node < heap_lo) {
		return NULL;
	}
	return node;
}

bool Heap_Free_Combiner(Heap* heap) {
	// Finds multiple free nodes next to each other and merges them into
	// a singular free node so free space is accurately represented.
	if (!heap) return false;
	if (!heap->data) return false;
	HeapNode* curr_node = heap->data;
	HeapNode* prev_node = heap->data;
	while (curr_node = Heap_Next_Node(heap, curr_node)) {
		// If the previous node and the current node are free nodes, merge
		// them together by increasing the free size of the previous node
		// by the next node's size + the size of a node itself.
		if (prev_node->type == HEAP_NODE_FREE 
         && curr_node->type == HEAP_NODE_FREE) {
			prev_node->size += curr_node->size + NODE_SIZE;
			curr_node = prev_node;
		}
		else {
			prev_node = curr_node;
		}
	}
	return true;
}

void* Heap_Alloc(Heap* heap, uint32_t size) {
	// Returns a pointer to a pre-allocated spot in the heap
	// of the requested size, or NULL if unable to do so.
	if (!heap) return NULL;
	// While it's technically possible to do byte alignment, I
	// would rather not risk undefined word boundary behaviour...
	size = ALIGNED_32_BIT(size);
	// Defer finding the next available free node to Heap_Find_Free.
	HeapNode* node = Heap_Find_Free(heap, size);
	if (!node) return NULL;
	if (Heap_Use_Node(node, size)) {
		// Cast to byte first for easy pointer arethemtic.
		return (void*)(((uint8_t*)node) + NODE_SIZE);
	}
	return NULL;
}

bool Heap_Dealloc(Heap* heap, void* ptr) {
	// Deallocate the heap node. Doesn't actually "dealloc/free()", rather
	// it just marks the node as free. Please set the pointer that was
	// pointing to the node to NULL after deallocation otherwise you will
	// probably not have a good time figuring out that Mysterious Bug.
	void* heap_lo = ((uint8_t*)heap->data) + NODE_SIZE;
	void* heap_hi = ((uint8_t*)heap->data) + heap->size - MINIMUM_DATA_TYPE_SIZE;
	// If the pointer is not pointing to our heap, exit early.
	if (ptr > heap_hi || ptr < heap_lo) {
		return false;
	}
	// Cast the pointer to a node so we can try to read the type. We use
	// magic ENUM numbers for the heap node types. It's to prevent accidents,
	// but malicious / crafty code can still utilize this to Break Memory in
	// an unsecured environement. Have fun brainstorming!
	HeapNode* node = (HeapNode*)((uint8_t*)ptr - NODE_SIZE);
	if (node->type == HEAP_NODE_USED) {
		for (size_t i = 0; i < node->size; i++) {
			// Write zeros to the existing data because why not!
			// Not necessary but it's a nice to have...
			((uint8_t*)ptr)[i] = 0x00;
		}
		node->type = HEAP_NODE_FREE;
		Heap_Free_Combiner(heap);
		return true;
	}
	return false;
}

void Heap_Destroy(Heap* heap) {
	// Free all memory that was malloc'd for the heap, incl. the heap itself.
	if (heap) {
		if (heap->data)
			free(heap->data);
		if (heap->description) 
			free(heap->description);
		free(heap);
	}
}

void Heap_Print(Heap* heap, const char* title) {
	// HEAP TITLE
	printf(title ? " :: %s\n" : "\n", title);
	printf(" ############################\n");
	printf(" ########### HEAP ###########\n");
	printf(" ############################\n");
	printf("\n");

	// HEAP
	if (!heap) {
		printf("Heap: null\n");
		printf("\n");
		return;
	}
	else {
		printf("Heap: %p\n", heap);
	}

	// HEAP -> DESCRIPTION
	if (heap->description) {
		printf("  Description: %p\n", heap->description);
		printf("  -> \"%s\"\n", heap->description);
	}
	else {
		printf("  Description: null\n");
	}

	// HEAP -> SIZE
	printf("  Size: %d bytes\n", heap->size);

	// HEAP -> DATA
	if (heap->data) {
		printf("  Data: %p\n", heap->data);
	}
	else {
		printf("  Data: null\n");
		printf("\n");
		return;
	}

	// HEAPNODE EXPLORE
	HeapNode* node = heap->data;
	if (!(node->type == HEAP_NODE_FREE || node->type == HEAP_NODE_USED)) {
		printf("    Node: INVALID\n");
		printf("\n");
		return;
	}

	do {
		printf("    Node: %p -> [%s] %d bytes\n", node, 
			node->type == HEAP_NODE_FREE ? "FREE" :
			node->type == HEAP_NODE_USED ? "USED" : "ERROR",
			node->size);
	} while (node = Heap_Next_Node(heap, node));
	printf("\n");

}