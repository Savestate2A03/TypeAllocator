#include "shared.h"

#define SOME_SIZE 64

// Written by Rei El-Khouri for no particular reason on 2025-06-05

int main(int argc, char* argv[]) {
	Heap* heap = Heap_Init(80444, "My Heap Yay :3");
	Heap_Print(heap, "INIT");
	uint64_t* ull_array1 = Heap_Alloc(heap, sizeof(uint64_t) * SOME_SIZE);
	uint64_t* ull_array2 = Heap_Alloc(heap, sizeof(uint64_t) * SOME_SIZE + 8);
	uint64_t* ull_array3 = Heap_Alloc(heap, sizeof(uint64_t) * SOME_SIZE + 10);
	uint64_t* ull_array4 = Heap_Alloc(heap, sizeof(uint64_t) * SOME_SIZE + 101);
	uint8_t*  byte_ptr = Heap_Alloc(heap, sizeof(uint8_t));
	uint32_t* word_ptr = Heap_Alloc(heap, sizeof(uint32_t));
	printf("ull_array1 -> %p\n", ull_array1);
	printf("ull_array2 -> %p\n", ull_array2);
	printf("ull_array3 -> %p\n", ull_array3);
	printf("ull_array4 -> %p\n", ull_array4);
	printf("byte_ptr ---> %p\n", byte_ptr);
	printf("word_ptr ---> %p\n", word_ptr);
	printf("\n");
	Heap_Print(heap, "ALLOCATED");
	Heap_Dealloc(heap, ull_array4);
	Heap_Print(heap, "DEALLOC ull_array4");
	Heap_Dealloc(heap, byte_ptr);
	Heap_Print(heap, "DEALLOC byte_ptr");
	char* string_buffer = Heap_Alloc(heap, sizeof(char[64]));
	Heap_Print(heap, "ALLOCATE char[64] BUFFER");
	Heap_Dealloc(heap, ull_array1);
	Heap_Dealloc(heap, ull_array2);
	Heap_Dealloc(heap, ull_array3);
	Heap_Dealloc(heap, word_ptr);
	Heap_Print(heap, "DEALLOC ALL BUT STRING BUFFER");
	Heap_Dealloc(heap, string_buffer);
	Heap_Print(heap, "DEALLOC ALL");
	Heap_Destroy(heap);
	heap = NULL;
}