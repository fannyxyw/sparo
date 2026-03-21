#ifndef __PRIMAY_HEAP_
#define __PRIMAY_HEAP_ 1


struct HeapStruct;
typedef struct HeapStruct *PriorityQueue;
typedef int ElementType;

PriorityQueue Initialize(int MaxElemnts);

void Destroy(PriorityQueue h);
int IsFull(PriorityQueue h);

void Insert(PriorityQueue h, ElementType element);
void DeleteMin(PriorityQueue h);
#endif

#include <stdlib.h>
#include <stdio.h>

struct HeapStruct {
  int capacity;
  int size;
  ElementType *elements;
};

PriorityQueue Initialize(int max_elements) {
  PriorityQueue h;
  h = (PriorityQueue)malloc(sizeof(struct HeapStruct));
  h->elements = (ElementType*)malloc(sizeof(ElementType) * (max_elements + 1));
  h->capacity = max_elements;
  h->size = 0;
  h->elements[0] = -1;
  return h;
}

void Insert(PriorityQueue h, ElementType element) {
  if (IsFull(h)) {
    return;
  }

  int i = 0;
  for (i = ++h->size; h->elements[i/2] > element; i/=2) {
    h->elements[i] = h->elements[i/2];
  }
  h->elements[i] = element;
}

void DeleteMin(PriorityQueue h) {
  // TODO

}


int IsFull(PriorityQueue h) {
  return (h->size >= h->capacity) ? 1 : 0;
}

void Destroy(PriorityQueue h) {
  free(h->elements);
  free(h);
}

void Print(PriorityQueue h) {
  printf("\n");
  for (int i = 1; i < h->size+1; i++){
    printf(" %d", h->elements[i]);
  }
}


void TestMinHeap() {
  PriorityQueue h = Initialize(11);
  Insert(h, 11);
  Insert(h, 10);
  Insert(h, 18);
  Insert(h, 13);
  Print(h);
  Destroy(h);
}