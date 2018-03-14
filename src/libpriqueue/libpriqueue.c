/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
  q->compare= comparer;
  q->size = 0;
  q->qArr = malloc(sizeof(void*[1]));
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
  if(q->size==0){
    if(q->qArr == NULL){
      q->qArr = malloc(sizeof(void*[1]));
    }
    q->qArr[0] = ptr;
    q->size++;
    return 0;
  }
  int newSize = q->size+1;
  //build a new array 1 bigger
  void** newArr = malloc(sizeof(void*[newSize]));
  int newArrIndex =0;
  int placed = 0;
  int placedIndex = 0;
  //iterate 1 more than q.size in case ptr is going to be the last element
  for(int i =0;i<=q->size;i++){
    if(!placed){
      //if we're still within the bounds of the old array, check if we've found a spot using comparator
      //place in front of first array element w/lower priority
      if(i!=q->size){
        int res = q->compare(ptr, q->qArr[i]); //comparators should return -1 if b is lower priority than a
        //found ptr's spot, insert it
        if(res<0){
          placed = 1;        
          newArr[newArrIndex] = ptr;
          placedIndex = newArrIndex;
          newArrIndex++;
        }
      }
      //if we get to the end without placing it, just place it
      else{
        placed = 1;
        newArr[newArrIndex] = ptr;
        placedIndex = newArrIndex;
      }
    }
    //copy other elements of old arr into new arr without modification
    if(i!=q->size){
      newArr[newArrIndex] = q->qArr[i];
    }
    newArrIndex++;
  }
  free(q->qArr);
  q->qArr = newArr;
  q->size = newSize;
  return placedIndex;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
  if(q->size==0) return NULL;
	return q->qArr[0];
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
  if(q->size==0) return NULL;
  void* ret = q->qArr[0];
  if(q->size==1){
    q->size--;
    free(q->qArr);    
    return ret;
  }
  //move everything forward
  int newSize = q->size-1;
  void** newArr = malloc(sizeof(void*[newSize]));
  for(int i=0; i<newSize;i++){
    newArr[i] = q->qArr[i+1];
  }
  free(q->qArr);
  q->qArr = newArr;  
  q->size = newSize;
  return ret;

}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
  if(index>=q->size || index<0) return NULL;
  return q->qArr[index];
  
}


/**
  Removes all instances of ptr from the queue. 
  
  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
  int count = 0;
  for(int i =0;i<q->size;i++){
    if(q->qArr[i]==ptr){
      count++;
    } 
  }
  if (count==0) return 0;
  if (count==q->size){
    free(q->qArr);
    q->size = 0;
    return count;
  }
  int newSize = q->size-count;
  int newArrIndex =0;
  void** newArr = malloc(sizeof(void*[newSize]));
  for(int i=0;i<q->size;i++){
    if(q->qArr[i]!=ptr){
      newArr[newArrIndex] = q->qArr[i];
      newArrIndex++;
    }
  }
  free(q->qArr);  
  q->size -= count;
  q->qArr = newArr;
	return count;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
  void* ret;
  if(q->qArr==NULL) return NULL;
  if(index>=q->size || index<0) return NULL;
  ret = q->qArr[index];
  
  void** newArr = malloc(sizeof(void*[q->size-1]));
  if(index == q->size-1){
    for(int i =0;i<q->size-1;i++){
      newArr[i] = q->qArr[i];
    }
  }
  else{
    int newArrIndex = 0;
    for(int i=0;i<q->size;i++){
      if(i!=index){
        newArr[newArrIndex] = q->qArr[i];
        newArrIndex++;
      }
    }
  }
  free(q->qArr);
  q->size--;
  q->qArr = newArr;
  return ret;

}


/**
  Returns the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->size;
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
  free(q->qArr);
}
