/* ------------------------------------------------------------------------
 * 
 * quickdata.h
 * authors: Daniel Elwell & Sumanta Das (QDrbTree)
 * date: november 2023
 * license: MIT
 * description: a single-header library for common, type-generic data structures
 * designed for ease of use
 * 
 * ------------------------------------------------------------------------
 * 
 * GENERAL NOTES:
 * 
 * to change or disable the function prefix (the default is "qd_"), change the macro on
 * line 147 to contain the desired prefix, or "#define QD_PREFIX" for no prefix
 * 
 * if you wish to provide your own memory allocation functions, #define QD_MALLOC, QD_FREE,
 * and QD_REALLOC to your desired functions before including this file
 * 
 * to use you must "#define QD_IMPLEMENTATION" in exactly one source file before including the library
 * 
 * ------------------------------------------------------------------------
 * 
 * FUNCTION PROTOTYPES:
 * 
 * void (*QDcopyFunc)(void* dest, void* src)
 * 		- optional parameter for most data structures, if passed as NULL, memcpy will be used
 * 		- copies src to dest, and ACTS AS A DESTRUCTOR WHEN dest == NULL
 * 
 * int32_t (*QDcompareFunc)(void* left, void* right)
 * 		- required parameter for some datas tructures/functions
 *		- returns a negative number if left < right, 0 if left == right, and 
 *			a positive number if left > right
 * 
 * uint64_t (*QDhashFunc)(void* data)
 * 		- optional parameter for QDhashmap
 * 		- returns the hash of data
 * 
 * ------------------------------------------------------------------------
 * 
 * DATA STRUCTURES + FUNCTIONS:
 * 
 * QDdynarray
 * 		.len - current number of elements
 * 		.cap - current maximum capacity, grows automatically
 * 		.arr - pointer to data array
 * 
 * 		QDdynarray* qd_dynarray_create(size_t elemSize, QDcopyFunc copy)
 * 		void        qd_dynarray_free  (QDdynarray* arr)
 * 		void*       qd_dynarray_get   (QDdynarray* arr, size_t i)
 * 		void        qd_dynarray_push  (QDdynarray* arr, void* elem)
 * 		void        qd_dynarray_insert(QDdynarray* arr, void* elem, size_t i)
 * 		void        qd_dynarray_remove(QDdynarray* arr, size_t i)
 * 		void        qd_dynarray_sort  (QDdynarray* arr, QDcompareFunc compare)
 * 
 * QDhashmap
 * 		.len - current number of elements
 * 		.cap - current maximum capacity, grows automatically
 * 
 * 		QDhashmap*    qd_hashmap_create          (size_t keySize, size_t valSize, QDcompareFunc keyCompare, 
 *		                                          QDcopyFunc keyCopy, QDcopyFunc valCopy, QDhashFunc hash)
 * 		void          qd_hashmap_free            (QDhashmap* map)
 * 		void*         qd_hashmap_get             (QDhashsmap* map, void* key) //returns NULL if no such key exists
 * 		void          qd_hashmap_insert          (QDhashmap* map, void* key, void* val)
 * 		void          qd_hashmap_remove          (QDhashmap* map, void* key)
 * 		qd_iterator_t qd_hashmap_iterate_start   (QDhashmap* map)
 * 		qd_iterator_t qd_hashmap_iterate         (QDhashmap* map, qd_iterator_t it, void** key, void** val)
 * 		qd_bool_t     qd_hashmap_iterate_finished(QDhashmap* map, qd_iterator_t it)
 * 
 * QDdeque
 * 		.len - current number of elements
 * 		.cap - current maximum capacity, grows automatically
 * 
 * 		QDdeque*      qd_deque_create          (size_t elemSize, QDcopyfunc copy)
 * 		void          qd_deque_free            (QDdeque* deque)
 * 		void          qd_deque_push_front      (QDdeque* deque, void* elem)
 * 		void          qd_deque_push_back       (QDdeque* deque, void* elem)
 * 		void*         qd_deque_pop_front       (QDdeque* deque) //returns NULL if deque is empty
 * 		void*         qd_deque_pop_back        (QDdeque* deque) //returns NULL if deque is empty
 * 		qd_iterator_t qd_deque_iterate_start   (QDdeque* deque)
 * 		qd_iterator_t qd_deque_iterate         (QDdeque* deque, qd_iterator_t it, void** elem)
 * 		qd_bool_t     qd_deque_iterate_finished(QDdeque* deque, qd_iterator_t it)
 * 
 * QDqueue
 * 		.len - current number of elements
 * 		.cap - current maximum capacity, grows automatically
 * 
 * 		QDqueue*      qd_queue_create          (size_t elemSize, QDcopyFunc copy)
 * 		void          qd_queue_free            (QDqueue* queue)
 * 		void          qd_queue_push            (QDqueue* queue, void* elem)
 * 		void*         qd_queue_pop             (QDqueue* queue) //returns NULL if the queue is empty
 * 		qd_iterator_t qd_queue_iterate_start   (QDqueue* queue)
 * 		qd_iterator_t qd_queue_iterate         (QDqueue* queue, qd_iterator_t it, void** elem)
 * 		qd_bool_t     qd_queue_iterate_finished(QDqueue* queue, qd_iterator_t it)
 * 
 * QDstack
 * 		.len - current number of elements
 * 		.cap - current maximum capacity, grows automatically
 * 
 * 		QDstack*      qd_stack_create          (size_t elemSize, QDcopyFunc copy)
 * 		void          qd_stack_free            (QDstack* stack)
 * 		void          qd_stack_push            (QDstack* stack, void* elem)
 * 		void*         qd_stack_pop             (QDstack* stack) //returns NULL if the stack is empty
 * 		qd_iterator_t qd_stack_iterate_start   (QDstack* stack)
 * 		qd_iterator_t qd_stack_iterate         (QDstack* stack, qd_iterator_t it, void** elem)
 * 		qd_bool_t     qd_stack_iterate_finished(QDstack* stack, qd_iterator_t it)
 * 
 * QDlinkedList
 * 		.head - starting element
 * 		.len  - current number of elements
 * 		.cap  - current maximum capacity, grows automatically
 * 
 * 		QDlinkedList* qd_ll_create(size_t elemSize, QDcopyFunc copy)
 * 		void          qd_ll_free  (QDlinkedList* list)
 * 		void          qd_ll_insert(QDlinkedList* list, QDllNode* pred, void* elem) //the elem will be the new head if pred == NULL
 * 		void          qd_ll_remove(QDlinkedList* list, QDllNode* node)
 * 
 * QDrbTree (balanced binary tree)
 * 		.root - root node of the tree
 * 		.len  - current number of elements
 * 		.cap  - current maximum capacity, grows automatically
 * 		
 * 		QDrbTree*   qd_tree_create          (size_t elemSize, QDcopyFunc copy, QDcompareFunc compare)
 * 		void        qd_tree_free            (QDrbTree* rbTree)
 * 		void        qd_tree_insert          (QDrbTree* rbTree, void* elem)
 * 		void        qd_tree_remove          (QDrbTree* rbTree, QDtreeNode* it) 
 * 		QDtreeNode* qd_tree_find            (QDrbTree* rbTree, void* elem)
 * 		QDtreeNode* qd_tree_iterate_start   (QDrbTree* rbTree)
 * 		QDtreeNode* qd_tree_iterate         (QDrbTree* rbTree, QDtreeNode* it)
 * 		qd_bool_t   qd_tree_iterate_finished(QDtreeNode* it)
 * 
 * ------------------------------------------------------------------------
 */

#ifndef QD_DATA_H
#define QD_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#define QD_INLINE static inline

//if you wish to set your own function prefix or remove it entirely,
//simply change this macro:
#define QD_PREFIX(name) qd_##name

#include <stdint.h>
#include <string.h>

//if you wish to supply your own memory allocation functions, simply
//#define them before including this header
#if !defined(QD_MALLOC) || !defined(QD_FREE) || !defined(QD_REALLOC)
	#include <stdlib.h>

	#define QD_MALLOC(s) malloc(s)
	#define QD_FREE(p) free(p)
	#define QD_REALLOC(p, s) realloc(p, s)
#endif

//----------------------------------------------------------------------//
//STRUCT DECLARATIONS:

typedef void (*QDcopyFunc)(void*, void*);
typedef int32_t (*QDcompareFunc)(void*, void*);
typedef uint64_t (*QDhashFunc)(void*);

//boolean return type
typedef enum qd_bool_t
{
	QD_TRUE = 1,
	QD_FALSE = 0
} qd_bool_t;

//iterator
typedef size_t qd_iterator_t;

//a dynamically-sized array
typedef struct QDdynArray
{
	QDcopyFunc copy;

	size_t elemSize;
	size_t cap;
	size_t len;
	void* arr;
} QDdynArray;

//a hashmap
typedef struct QDhashmap
{
	QDcopyFunc keyCopy;
	QDcopyFunc valCopy;
	QDcompareFunc keyCompare;
	QDhashFunc hash;

	size_t cap;
	size_t len;

	size_t keySize;
	void* keys;
	uint8_t* keysValid;

	size_t valSize;
	void* vals;
} QDhashmap;

//a deque (queue that can be popped from start or end, basically a queue+stack)
typedef struct QDdeque
{
	QDcopyFunc copy;

	size_t start;

	size_t elemSize;
	size_t cap;
	size_t len;
	void* arr;
} QDdeque;

//a queue (implemented as a deque)
typedef struct QDdeque QDqueue;

//a stack (implemented as a deque)
typedef struct QDdeque QDstack;

//a node in a linked list
typedef struct QDllNode
{
	void* data;
	struct QDllNode* next;
	struct QDllNode* prev;
	int32_t buffIdx;
} QDllNode;

//a free memory block in a linked list
typedef struct _QDllNodeFree
{
	int32_t buffIdx; 
	size_t blockIdx;
} _QDllNodeFree;

//memory block for a linked list
typedef union _QDllBlock
{
	QDllNode node;
	_QDllNodeFree next;
} _QDllBlock;

#define QD_BLOCK_NULL SIZE_MAX

//a linked list
typedef struct QDlinkedList
{
	QDcopyFunc copy;

	QDllNode* head;

	size_t len;
	size_t cap;
	
	_QDllNodeFree firstFree;
	_QDllBlock** nodeBuffs;

	size_t elemSize;
	void** dataBuffs;
} QDlinkedList;

//a node in a red-black tree
typedef struct QDtreeNode
{
	void* data;
	struct QDtreeNode* parent;
	struct QDtreeNode* l;
	struct QDtreeNode* r;

	qd_bool_t isRed;
	size_t buffIdx;
} QDtreeNode;

typedef union QDtreeNodeBlock
{
	QDtreeNode node;
	size_t nextFree;
} QDtreeNodeBlock;

//a red-black tree
typedef struct QDrbTree
{
	size_t elemSize;
	QDcopyFunc copy;
	QDcompareFunc compare;

	QDtreeNode* root;

	size_t firstFree;
	size_t len;
	size_t cap;

	size_t buffCount;
	size_t buffCapacity;
	QDtreeNodeBlock** nodeBuff;
	void** dataBuff;
} QDrbTree;

//----------------------------------------------------------------------//
//DYNAMIC ARRAY FUNCTION DECLARATIONS:

QDdynArray* QD_PREFIX(dynarray_create)(size_t elemSize, QDcopyFunc copy);
void QD_PREFIX(dynarray_free)(QDdynArray* arr);

void _qd_dynarray_maybe_resize(QDdynArray* arr);

QD_INLINE void QD_PREFIX(dynarray_push)(QDdynArray* arr, void* elem)
{
	void* dest = (char*)arr->arr + arr->len * arr->elemSize;
	arr->len++;

	if(arr->copy)
		arr->copy(dest, elem);
	else
		memcpy(dest, elem, arr->elemSize);
	
	_qd_dynarray_maybe_resize(arr);
}

QD_INLINE void* QD_PREFIX(dynarray_get)(QDdynArray* arr, size_t i)
{
	return (char*)arr->arr + arr->elemSize * i;
}

QD_INLINE void QD_PREFIX(dynarray_insert)(QDdynArray* arr, void* elem, size_t i)
{
	void* dest = QD_PREFIX(dynarray_get)(arr, i);
	memmove((char*)dest + arr->elemSize, dest, (arr->len - i) * arr->elemSize);
	arr->len++;

	if(arr->copy)
		arr->copy(dest, elem);
	else
		memcpy(dest, elem, arr->elemSize);

	_qd_dynarray_maybe_resize(arr);
}

QD_INLINE void QD_PREFIX(dynarray_remove)(QDdynArray* arr, size_t i)
{
	if(arr->len == 0)
		return;

	void* dest = QD_PREFIX(dynarray_get)(arr, i);

	if(arr->copy)
		arr->copy(NULL, dest); //copy func destructs when dest is NULL
	
	memmove(dest, (char*)dest + arr->elemSize, (arr->len - i - 1) * arr->elemSize);
	arr->len--;
}

void QD_PREFIX(dynarray_sort)(QDdynArray* arr, QDcompareFunc compare);

//----------------------------------------------------------------------//
//HASHMAP FUNCTION DECLARATIONS:

QDhashmap* QD_PREFIX(hashmap_create)(size_t keySize, size_t valSize, QDcompareFunc compare, QDcopyFunc keyCopy, QDcopyFunc valCopy, QDhashFunc hash);
void QD_PREFIX(hashmap_free)(QDhashmap* map);

void* QD_PREFIX(hashmap_get)(QDhashmap* map, void* key);
void QD_PREFIX(hashmap_insert)(QDhashmap* map, void* key, void* val);
void QD_PREFIX(hashmap_remove)(QDhashmap* map, void* key);

QD_INLINE qd_iterator_t QD_PREFIX(hashmap_iterate_start)(QDhashmap* map)
{
	qd_iterator_t it = 0;
	while(it < map->cap && !map->keysValid[it])
		it++;
	
	return it;
}

QD_INLINE qd_iterator_t QD_PREFIX(hashmap_iterate)(QDhashmap* map, qd_iterator_t it, void** key, void** val)
{
	if(key)
		*key = (char*)map->keys + it * map->keySize;
	if(val)
		*val = (char*)map->vals + it * map->valSize;

	it++;
	while(it < map->cap && !map->keysValid[it])
		it++;
	
	return it;
}

QD_INLINE qd_bool_t QD_PREFIX(hashmap_iterate_finished)(QDhashmap* map, qd_iterator_t it)
{
	return it >= map->cap ? QD_TRUE : QD_FALSE;
}

//----------------------------------------------------------------------//
//DEQUE FUNCTION DECLARATIONS:

QDdeque* QD_PREFIX(deque_create)(size_t elemSize, QDcopyFunc copy);
void QD_PREFIX(deque_free)(QDdeque* deque);

void _qd_deque_maybe_resize(QDdeque* arr);

QD_INLINE void QD_PREFIX(deque_push_front)(QDdeque* deque, void* elem)
{
	deque->len++;
	if(deque->start == 0)
		deque->start = deque->cap - 1;
	else
		deque->start--;

	void* dest = (char*)deque->arr + deque->start * deque->elemSize;

	if(deque->copy)
		deque->copy(dest, elem);
	else
		memcpy(dest, elem, deque->elemSize);

	_qd_deque_maybe_resize(deque);
}

QD_INLINE void QD_PREFIX(deque_push_back)(QDdeque* deque, void* elem)
{
	size_t pos = (deque->start + deque->len) % deque->cap;
	void* dest = (char*)deque->arr + pos * deque->elemSize;
	deque->len++;

	if(deque->copy)
		deque->copy(dest, elem);
	else
		memcpy(dest, elem, deque->elemSize);

	_qd_deque_maybe_resize(deque);
}

QD_INLINE void* QD_PREFIX(deque_pop_front)(QDdeque* deque)
{
	if(deque->len == 0)
		return NULL;
	
	void* dest = (char*)deque->arr + deque->start * deque->elemSize;

	deque->len--;
	deque->start = (deque->start + 1) % deque->cap;

	return dest;
}

QD_INLINE void* QD_PREFIX(deque_pop_back)(QDdeque* deque)
{
	if(deque->len == 0)
		return NULL;

	deque->len--;

	size_t pos = (deque->start + deque->len) % deque->cap;
	return (char*)deque->arr + pos * deque->elemSize;
}

QD_INLINE qd_iterator_t QD_PREFIX(deque_iterate_start)(QDdeque* deque)
{
	return 0;
}

QD_INLINE qd_iterator_t QD_PREFIX(deque_iterate)(QDdeque* deque, qd_iterator_t it, void** elem)
{
	size_t pos = (deque->start + it) % deque->cap;
	*elem = (char*)deque->arr + pos * deque->elemSize;

	it++;
	return it;
}

QD_INLINE qd_bool_t QD_PREFIX(deque_iterate_finished)(QDdeque* deque, qd_iterator_t it)
{
	return it >= deque->len ? QD_TRUE : QD_FALSE;
}

//----------------------------------------------------------------------//
//QUEUE FUNCTION DECLARATIONS:

QD_INLINE QDqueue* QD_PREFIX(queue_create)(size_t elemSize, QDcopyFunc copy)
{
	return QD_PREFIX(deque_create)(elemSize, copy);
}

QD_INLINE void QD_PREFIX(queue_free)(QDqueue* queue)
{
	QD_PREFIX(deque_free)(queue);
}

QD_INLINE void QD_PREFIX(queue_push)(QDqueue* queue, void* elem)
{
	QD_PREFIX(deque_push_back)(queue, elem);
}

QD_INLINE void* QD_PREFIX(queue_pop)(QDqueue* queue)
{
	return QD_PREFIX(deque_pop_front)(queue);
}

QD_INLINE qd_iterator_t QD_PREFIX(queue_iterate_start)(QDqueue* queue)
{
	return QD_PREFIX(deque_iterate_start)(queue);
}

QD_INLINE qd_iterator_t QD_PREFIX(queue_iterate)(QDqueue* queue, qd_iterator_t it, void** elem)
{
	return QD_PREFIX(deque_iterate)(queue, it, elem);
}

QD_INLINE int32_t QD_PREFIX(queue_iterate_finished)(QDqueue* queue, qd_iterator_t it)
{
	return QD_PREFIX(deque_iterate_finished)(queue, it);
}

//----------------------------------------------------------------------//
//STACK FUNCTION DECLARATIONS:

QD_INLINE QDstack* QD_PREFIX(stack_create)(size_t elemSize, QDcopyFunc copy)
{
	return QD_PREFIX(deque_create)(elemSize, copy);
}

QD_INLINE void QD_PREFIX(stack_free)(QDstack* stack)
{
	QD_PREFIX(deque_free)(stack);
}

QD_INLINE void QD_PREFIX(stack_push)(QDstack* stack, void* elem)
{
	QD_PREFIX(deque_push_back)(stack, elem);
}

QD_INLINE void* QD_PREFIX(stack_pop)(QDstack* stack)
{
	return QD_PREFIX(deque_pop_back)(stack);
}

QD_INLINE qd_iterator_t QD_PREFIX(stack_iterate_start)(QDstack* stack)
{
	return QD_PREFIX(deque_iterate_start)(stack);
}

QD_INLINE qd_iterator_t QD_PREFIX(stack_iterate)(QDstack* stack, qd_iterator_t it, void** elem)
{
	return QD_PREFIX(deque_iterate)(stack, it, elem);
}

QD_INLINE int32_t QD_PREFIX(stack_iterate_finished)(QDstack* stack, qd_iterator_t it)
{
	return QD_PREFIX(deque_iterate_finished)(stack, it);
}

//----------------------------------------------------------------------//
//LINKED LIST FUNCTION DECLARATIONS:

QDlinkedList* QD_PREFIX(ll_create)(size_t elemSize, QDcopyFunc copy);
void QD_PREFIX(ll_free)(QDlinkedList* list);

void QD_PREFIX(ll_insert)(QDlinkedList* list, QDllNode* pred, void* elem);
void QD_PREFIX(ll_remove)(QDlinkedList* list, QDllNode* node);

//----------------------------------------------------------------------//
//RED-BLACK TREE FUNCTION DECLARATIONS:

QDrbTree* QD_PREFIX(tree_create)(size_t elemSize, QDcopyFunc copy, QDcompareFunc compare);
void QD_PREFIX(tree_free)(QDrbTree* rbTree);

QDtreeNode* QD_PREFIX(tree_find)(QDrbTree* rbTree, void* elem);
void QD_PREFIX(tree_insert)(QDrbTree* rbTree, void* elem);
void QD_PREFIX(tree_remove)(QDrbTree* rbTree, QDtreeNode* it);

QDtreeNode* QD_PREFIX(tree_iterate_start)(QDrbTree* rbTree);
QDtreeNode* QD_PREFIX(tree_iterate)(QDrbTree* rbTree, QDtreeNode* it);
qd_bool_t QD_PREFIX(tree_iterate_finished)(QDtreeNode* it);

#ifdef QD_IMPLEMENTATION

//----------------------------------------------------------------------//
//DYNAMIC ARRAY FUNCTION DEFINITIONS:

QDdynArray* QD_PREFIX(dynarray_create)(size_t elemSize, QDcopyFunc copy)
{
	QDdynArray* arr = (QDdynArray*)QD_MALLOC(sizeof(QDdynArray));
	if(!arr)
		return NULL;

	arr->copy = copy;
	arr->elemSize = elemSize;
	arr->cap = 1;
	arr->len = 0;

	arr->arr = QD_MALLOC(arr->cap * arr->elemSize);
	if(!arr->arr)
	{
		QD_FREE(arr);
		return NULL;
	}

	return arr;
}

void QD_PREFIX(dynarray_free)(QDdynArray* arr)
{
	if(arr->copy)
		for(size_t i = 0; i < arr->len; i++)
			arr->copy(NULL, QD_PREFIX(dynarray_get)(arr, i)); //copy func destructs when dest==NULL

	QD_FREE(arr->arr);
	QD_FREE(arr);
}

void _qd_dynarray_maybe_resize(QDdynArray* arr)
{
	if(arr->len < arr->cap)
		return;

	arr->cap *= 2;
	arr->arr = QD_REALLOC(arr->arr, arr->cap * arr->elemSize);
}

QD_INLINE void _qd_dynarray_sort_swap(QDdynArray* arr, size_t i1, size_t i2, void* copyBuf)
{
	memcpy(copyBuf, QD_PREFIX(dynarray_get)(arr, i1), arr->elemSize);
	memcpy(QD_PREFIX(dynarray_get)(arr, i1), QD_PREFIX(dynarray_get)(arr, i2), arr->elemSize);
	memcpy(QD_PREFIX(dynarray_get)(arr, i2), copyBuf, arr->elemSize);
}

size_t _qd_dynarray_sort_partition(QDdynArray* arr, size_t start, size_t end, QDcompareFunc compare, void* copyBuf)
{
	void* pivot = QD_PREFIX(dynarray_get)(arr, end);
	size_t i = start - 1;

	for(size_t j = start; j < end; j++)
		if(compare(QD_PREFIX(dynarray_get)(arr, j), pivot) <= 0)
		{
			i++;
			_qd_dynarray_sort_swap(arr, i, j, copyBuf);
		}
	
	_qd_dynarray_sort_swap(arr, i + 1, end, copyBuf);
	return i + 1;
}

void QD_PREFIX(dynarray_sort)(QDdynArray* arr, QDcompareFunc compare)
{
	//stack-based quicksort:

	size_t stack[64];     //stack of subarray start/end positions, only needs
	int32_t stackPos = 0; //64 elements since this allows for array size of 2^64
						  //to be sorted, which is the maximum index range anyway

	stack[stackPos++] = 0;
	stack[stackPos++] = arr->len - 1;

	void* copyBuf = QD_MALLOC(arr->elemSize);
	if(!copyBuf)
		return;

	while(stackPos > 0)
	{
		size_t end = stack[--stackPos];
		size_t start = stack[--stackPos];

		size_t p = _qd_dynarray_sort_partition(arr, start, end, compare, copyBuf);

		if(p > 0 && p - 1 > start)
		{
			stack[stackPos++] = start;
			stack[stackPos++] = p - 1;
		}

		if(p + 1 < end)
		{
			stack[stackPos++] = p + 1;
			stack[stackPos++] = end;
		}
	}

	QD_FREE(copyBuf);
}

//----------------------------------------------------------------------//
//HASHMAP FUNCTION DEFINITIONS:

uint64_t _qd_hashmap_default_hash(void* key, size_t size)
{
	//FNV-1a

	const uint64_t FNV_PRIME = 1099511628211;
	uint64_t hash = 14695981039346656037u;

	char* bytes = (char*)key;
	for(size_t i = 0; i < size; i++)
	{
		hash ^= (uint64_t)(*bytes);
		hash *= FNV_PRIME;
		bytes++;
	}

	return hash;
}

QDhashmap* QD_PREFIX(hashmap_create)(size_t keySize, size_t valSize, QDcompareFunc compare, QDcopyFunc keyCopy, QDcopyFunc valCopy, QDhashFunc hash)
{
	QDhashmap* map = (QDhashmap*)QD_MALLOC(sizeof(QDhashmap));
	if(!map)
		return NULL;
	
	map->keyCompare = compare;
	map->keyCopy = keyCopy;
	map->valCopy = valCopy;
	map->hash = hash;
	map->keySize = keySize;
	map->valSize = valSize;
	map->cap = 1;
	map->len = 0;

	map->keys = QD_MALLOC(map->cap * map->keySize);
	if(!map->keys)
	{
		QD_FREE(map);
		return NULL;
	}

	map->keysValid = (uint8_t*)QD_MALLOC(map->cap * sizeof(uint8_t));
	if(!map->keysValid)
	{
		QD_FREE(map->keys);
		QD_FREE(map);
		return NULL;
	}
	memset(map->keysValid, 0, map->cap * sizeof(uint8_t));

	map->vals = QD_MALLOC(map->cap * map->valSize);
	if(!map->vals)
	{
		QD_FREE(map->keys);
		QD_FREE(map->keysValid);
		QD_FREE(map);
		return NULL;
	}

	return map;
}

void QD_PREFIX(hashmap_free)(QDhashmap* map)
{
	for(size_t i = 0; i < map->cap; i++)
	{
		if(!map->keysValid[i])
			continue;
		
		if(map->keyCopy)
			map->keyCopy(NULL, (char*)map->keys + i * map->keySize); //copy func destructs when dest==NULL
		
		if(map->valCopy)
			map->valCopy(NULL, (char*)map->vals + i * map->valSize);
	}

	QD_FREE(map->keys);
	QD_FREE(map->keysValid);
	QD_FREE(map->vals);
	QD_FREE(map);
}

uint64_t _qd_hashmap_linear_probing(QDhashmap* map, void* key)
{
	uint64_t hash;
	if(map->hash) 
		hash = map->hash(key) % map->cap;
	else
		hash = _qd_hashmap_default_hash(key, map->keySize);

	while(map->keysValid[hash] && map->keyCompare(key, (char*)map->keys + hash * map->keySize) != 0)
		hash = (hash + 1) % map->cap;

	return hash;	
}

void* QD_PREFIX(hashmap_get)(QDhashmap* map, void* key)
{
	uint64_t hash = _qd_hashmap_linear_probing(map, key);
	
	if(map->keysValid[hash])
		return (char*)map->vals + hash * map->valSize;
	else
		return NULL;
}

void QD_PREFIX(hashmap_insert)(QDhashmap* map, void* key, void* val)
{
	if(map->len == map->cap)
		return;

	uint64_t hash = _qd_hashmap_linear_probing(map, key);
	if(map->keysValid[hash])
		return;
	
	void* keyDest = (char*)map->keys + hash * map->keySize;
	if(map->keyCopy)
		map->keyCopy(keyDest, key);
	else
		memcpy(keyDest, key, map->keySize);
	
	map->keysValid[hash] = 1;

	void* valDest = (char*)map->vals + hash * map->valSize;
	if(map->valCopy)
		map->valCopy(valDest, val);
	else
		memcpy(valDest, val, map->valSize);
	
	map->len++;
	if((float)map->len / map->cap >= 0.5f) //load factor 0.5
	{
		QDhashmap newMap = *map;
		newMap.cap = map->cap * 2;
		newMap.len = 0;
		
		newMap.keys = QD_MALLOC(newMap.cap * newMap.keySize);
		if(!newMap.keys)
			return;
		
		newMap.keysValid = (uint8_t*)QD_MALLOC(newMap.cap * sizeof(uint8_t));
		if(!newMap.keysValid)
		{
			QD_FREE(newMap.keys);
			return;
		}
		memset(newMap.keysValid, 0, newMap.cap * sizeof(uint8_t));

		newMap.vals = QD_MALLOC(newMap.cap * newMap.valSize);
		if(!newMap.vals)
		{
			QD_FREE(newMap.keys);
			QD_FREE(newMap.keysValid);
			return;
		}

		newMap.keyCopy = NULL; //set copy functions to NULL so we just do a basic memcpy, avoids
		newMap.valCopy = NULL; //mem leaks in case user-supplied copy function makes allocs
		for(size_t i = 0; i < map->cap; i++)
			if(map->keysValid[i])
				QD_PREFIX(hashmap_insert)(&newMap, (char*)map->keys + i * map->keySize, (char*)map->vals + i * map->valSize);

		newMap.keyCopy = map->keyCopy;
		newMap.valCopy = map->valCopy;

		QD_FREE(map->keys);
		QD_FREE(map->keysValid);
		QD_FREE(map->vals);
		*map = newMap;
	}
}

void QD_PREFIX(hashmap_remove)(QDhashmap* map, void* key)
{
	uint64_t hash = _qd_hashmap_linear_probing(map, key);
	if(!map->keysValid[hash])
		return;
	
	if(map->keyCopy)
		map->keyCopy(NULL, (char*)map->keys + hash * map->keySize); //copy funcs destruct when dest==NULL
	
	if(map->valCopy)
		map->valCopy(NULL, (char*)map->vals + hash * map->valSize);

	map->keysValid[hash] = 0;
}

//----------------------------------------------------------------------//
//DEQUE FUNCTION DEFINITIONS:

QDdeque* QD_PREFIX(deque_create)(size_t elemSize, QDcopyFunc copy)
{
	QDdeque* deque = (QDdeque*)QD_MALLOC(sizeof(QDdeque));
	if(!deque)
		return NULL;

	deque->copy = copy;
	deque->elemSize = elemSize;
	deque->start = 0;
	deque->cap = 1;
	deque->len = 0;

	deque->arr = QD_MALLOC(deque->cap * deque->elemSize);
	if(!deque->arr)
	{
		QD_FREE(deque);
		return NULL;
	}

	return deque;
}

void QD_PREFIX(deque_free)(QDdeque* deque)
{
	if(deque->copy)
	{
		for(int i = 0; i < deque->len; i++)
		{
			size_t pos = deque->start + i % deque->cap;
			deque->copy(NULL, (char*)deque->arr + pos * deque->elemSize);
		}
	}

	QD_FREE(deque->arr);
	QD_FREE(deque);
}

void _qd_deque_maybe_resize(QDdeque* deque)
{
	if(deque->len < deque->cap)
		return;

	size_t oldCap = deque->cap;

	deque->cap *= 2;
	deque->arr = QD_REALLOC(deque->arr, deque->cap * deque->elemSize);

	if(deque->start + deque->len > oldCap)
	{
		size_t elemsToCap = oldCap - deque->start;
		size_t newStart = deque->cap - elemsToCap;

		memcpy((char*)deque->arr + newStart * deque->elemSize, (char*)deque->arr + deque->start * deque->elemSize, elemsToCap * deque->elemSize);
		deque->start = newStart;
	}
}

//----------------------------------------------------------------------//
//LINKED LIST FUNCTION DEFINITIONS:

QDlinkedList* QD_PREFIX(ll_create)(size_t elemSize, QDcopyFunc copy)
{
	QDlinkedList* list = (QDlinkedList*)QD_MALLOC(sizeof(QDlinkedList));
	if(!list)
		return NULL;

	list->copy = copy;
	list->elemSize = elemSize;
	list->cap = 1;
	list->len = 0;
	list->head = NULL;

	size_t tmp = list->cap;
	int32_t numBuffs = 1;
	while(tmp >>= 1)
		numBuffs++;

	list->nodeBuffs = (_QDllBlock**)QD_MALLOC(numBuffs * sizeof(_QDllBlock*));
	if(!list->nodeBuffs)
	{
		QD_FREE(list);
		return NULL;
	}

	list->dataBuffs = (void**)QD_MALLOC(numBuffs * sizeof(void*));
	if(!list->dataBuffs)
	{
		QD_FREE(list->nodeBuffs);
		QD_FREE(list);
		return NULL;
	}

	size_t buffSize = 1;
	for(int32_t i = 0; i < numBuffs; i++)
	{
		list->nodeBuffs[i] = (_QDllBlock*)QD_MALLOC(buffSize * sizeof(_QDllBlock));
		list->dataBuffs[i] = QD_MALLOC(buffSize * list->elemSize);

		for(size_t j = 0; j < buffSize; j++)
		{
			if(j == buffSize - 1)
			{
				list->nodeBuffs[i][j].next.buffIdx = i + 1;
				if(i == numBuffs - 1)
					list->nodeBuffs[i][j].next.blockIdx = QD_BLOCK_NULL;
				else
					list->nodeBuffs[i][j].next.blockIdx = 0;
			}
			else
			{
				list->nodeBuffs[i][j].next.buffIdx = i;
				list->nodeBuffs[i][j].next.blockIdx = j + 1;
			}
		}

		buffSize *= 2;
	}

	list->firstFree.buffIdx = 0;
	list->firstFree.blockIdx = 0;

	return list;
}

void QD_PREFIX(ll_free)(QDlinkedList* list)
{
	if(list->copy)
	{
		QDllNode* cur = list->head;
		while(cur)
		{
			list->copy(NULL, cur->data);
			cur = cur->next;
		}
	}

	size_t tmp = list->cap;
	size_t numBuffs = 1;
	while(tmp >>= 1)
		numBuffs++;

	for(size_t i = 0; i < numBuffs; i++)
	{
		QD_FREE(list->nodeBuffs[i]);
		QD_FREE(list->dataBuffs[i]);
	}

	QD_FREE(list->nodeBuffs);
	QD_FREE(list->dataBuffs);
	QD_FREE(list);
}

void QD_PREFIX(ll_insert)(QDlinkedList* list, QDllNode* pred, void* elem)
{
	int32_t buffIdx = list->firstFree.buffIdx;
	size_t blockIdx = list->firstFree.blockIdx;

	list->firstFree = list->nodeBuffs[buffIdx][blockIdx].next;
	QDllNode* node = &list->nodeBuffs[buffIdx][blockIdx].node;
	node->data = (char*)list->dataBuffs[buffIdx] + blockIdx * list->elemSize;
	node->buffIdx = buffIdx;

	if(list->copy)
		list->copy(node->data, elem);
	else
		memcpy(node->data, elem, list->elemSize);

	if(pred)
	{
		node->next = pred->next;
		node->prev = pred;
	}
	else
	{
		node->next = list->head;
		node->prev = NULL;
		list->head = node;
	}

	list->len++;

	if(list->firstFree.blockIdx == QD_BLOCK_NULL)
	{
		int32_t numBuffs = 1;
		size_t tmp = list->cap;
		while(tmp >>= 1)
			numBuffs++;

		list->nodeBuffs = (_QDllBlock**)QD_REALLOC(list->nodeBuffs, (numBuffs + 1) * sizeof(_QDllBlock*));
		list->dataBuffs = (void**)QD_REALLOC(list->dataBuffs, (numBuffs + 1) * sizeof(void*));

		list->nodeBuffs[numBuffs] = (_QDllBlock*)QD_MALLOC(list->cap * sizeof(_QDllBlock));
		list->dataBuffs[numBuffs] = QD_MALLOC(list->cap * list->elemSize);

		for(size_t i = 0; i < list->cap; i++)
		{
			list->nodeBuffs[numBuffs][i].next.buffIdx = numBuffs;

			if(i == list->cap - 1)
				list->nodeBuffs[numBuffs][i].next.blockIdx = QD_BLOCK_NULL;
			else
				list->nodeBuffs[numBuffs][i].next.blockIdx = i + 1;
		}

		list->firstFree.buffIdx = numBuffs;
		list->firstFree.blockIdx = 0;

		list->cap *= 2;
	}
}

void QD_PREFIX(ll_remove)(QDlinkedList* list, QDllNode* node)
{
	int32_t buffIdx = node->buffIdx;
	size_t blockIdx = ((char*)node - (char*)list->nodeBuffs[buffIdx]) / sizeof(_QDllBlock);

	size_t buffSize = 1;
	for(int32_t i = 0; i < node->buffIdx; i++)
		buffSize *= 2;

	buffSize /= 2;

	if(node == list->head)
	{
		if(list->head->next)
			list->head->next->prev = NULL;

		list->head = list->head->next;
	}
	else
	{
		if(node->next)
			node->next->prev = node->prev;

		node->prev->next = node->next;
	}

	list->nodeBuffs[node->buffIdx][blockIdx].next = list->firstFree;

	list->firstFree.buffIdx = buffIdx;
	list->firstFree.blockIdx = blockIdx;

	list->len--;
}

//----------------------------------------------------------------------//
//RED BLACK TREE FUNCTIONS:

QDrbTree* QD_PREFIX(tree_create)(size_t elemSize, QDcopyFunc copy, QDcompareFunc compare)
{
	QDrbTree* rbTree = (QDrbTree*)QD_MALLOC(sizeof(QDrbTree));
	if (!rbTree)
		return NULL;

	rbTree->elemSize = elemSize;
	rbTree->copy = copy;
	rbTree->compare = compare;
	rbTree->root = NULL;

	rbTree->firstFree = 0;
	rbTree->len = 0;
	rbTree->cap = 1;
	rbTree->buffCount = 1;
	rbTree->buffCapacity = 1;

	rbTree->nodeBuff = (QDtreeNodeBlock**)QD_MALLOC(sizeof(QDtreeNodeBlock*));
	rbTree->nodeBuff[0] = (QDtreeNodeBlock*)QD_MALLOC(sizeof(QDtreeNodeBlock));
	rbTree->nodeBuff[0][0].nextFree = SIZE_MAX; // SIZE_MAX is equivalent to NULL, this means a resize is needed
	rbTree->dataBuff = (void**)QD_MALLOC(sizeof(void*));
	rbTree->dataBuff[0] = (void*)QD_MALLOC(rbTree->elemSize);

	if (!rbTree->nodeBuff || !rbTree->dataBuff) {
		QD_FREE(rbTree);
		return NULL;
	}

	return rbTree;
}

QDtreeNode* QD_PREFIX(tree_iterate_start)(QDrbTree* rbTree)
{
	QDtreeNode* currNode = rbTree->root;
	QDtreeNode* prevNode = NULL;

	while (currNode != NULL)
	{
		prevNode = currNode;
		currNode = currNode->l;
	}

	return prevNode;
}

QDtreeNode* QD_PREFIX(tree_iterate)(QDrbTree* rbTree, QDtreeNode* it)
{
	if (it->r)
	{
		QDtreeNode* currNode = it->r;
		QDtreeNode* prevNode = NULL;
		while (currNode != NULL)
		{
			prevNode = currNode;
			currNode = currNode->l;
		}

		return prevNode;
	}

	QDtreeNode* childPtr = it;
	QDtreeNode* parentPtr = it->parent;
	while (parentPtr && parentPtr->l != childPtr)
	{
		childPtr = parentPtr;
		parentPtr = parentPtr->parent;
	}

	return parentPtr;
}

qd_bool_t QD_PREFIX(tree_iterate_finished)(QDtreeNode* it)
{
	return (qd_bool_t)(it == NULL);
}

void QD_PREFIX(tree_free)(QDrbTree* rbTree)
{
	for (QDtreeNode* it = QD_PREFIX(tree_iterate_start)(rbTree); !QD_PREFIX(tree_iterate_finished)(it); it = QD_PREFIX(tree_iterate)(rbTree, it))
		if (rbTree->copy)
			rbTree->copy(NULL, it->data);

	for (int i = 0; i < rbTree->buffCount; i++)
	{
		QD_FREE(rbTree->nodeBuff[i]);
		QD_FREE(rbTree->dataBuff[i]);
	}

	QD_FREE(rbTree->nodeBuff);
	QD_FREE(rbTree->dataBuff);
	QD_FREE(rbTree);
}

void QD_PREFIX(tree_clear)(QDrbTree* rbTree) 
{
	size_t elemSize = rbTree->elemSize;
	QDcopyFunc copy = rbTree->copy;
	QDcompareFunc compare = rbTree->compare;

	QD_PREFIX(tree_free)(rbTree);
	QD_PREFIX(tree_create)(elemSize, copy, compare);
}

void _qd_tree_rotate_left(QDrbTree* rbTree, QDtreeNode* currNode)
{
	QDtreeNode* rightChild = currNode->r;
	currNode->r = rightChild->l;
	if (rightChild->l)
		rightChild->l->parent = currNode;

	rightChild->parent = currNode->parent;
	if (!currNode->parent)
		rbTree->root = rightChild;
	else if (currNode->parent->l == currNode)
		currNode->parent->l = rightChild;
	else
		currNode->parent->r = rightChild;

	rightChild->l = currNode;
	currNode->parent = rightChild;
}

void _qd_tree_rotate_right(QDrbTree* rbTree, QDtreeNode* currNode)
{
	QDtreeNode* leftChild = currNode->l;
	currNode->l = leftChild->r;
	if (leftChild->r)
		leftChild->r->parent = currNode;

	leftChild->parent = currNode->parent;
	if (!currNode->parent)
		rbTree->root = leftChild;
	else if (currNode->parent->l == currNode)
		currNode->parent->l = leftChild;
	else
		currNode->parent->r = leftChild;

	leftChild->r = currNode;
	currNode->parent = leftChild;

}

void QD_PREFIX(tree_insert)(QDrbTree* rbTree, void* elem) 
{
	QDtreeNode* prevPtr = NULL;
	QDtreeNode* currPtr = rbTree->root;
	while (currPtr)
	{
		prevPtr = currPtr;
		int32_t cmp = rbTree->compare(elem, currPtr->data);
		if (cmp < 0)
			currPtr = currPtr->l;
		else if (cmp > 0)
			currPtr = currPtr->r;
	}

	size_t blockIdx;
	size_t buffIdx;

	if (rbTree->firstFree != SIZE_MAX)
	{
		buffIdx = 0;

		size_t buffSize = 1;
		size_t tmp = rbTree->firstFree + 1;
		while (tmp >>= 1)
		{
			buffIdx++;
			buffSize *= 2;
		}

		blockIdx = (rbTree->firstFree + 1) % buffSize;
	}
	else
	{
		size_t newBufferSize = rbTree->cap + 1;
		size_t oldCapacity = rbTree->cap;
		rbTree->cap += (rbTree->cap + 1);

		size_t newBuffCount = 0;
		size_t tmp = rbTree->cap + 1;
		while (tmp >>= 1)
			newBuffCount++;

		rbTree->buffCount = newBuffCount;
		if (rbTree->buffCount > rbTree->buffCapacity) 
		{
			rbTree->buffCapacity *= 2;
			rbTree->nodeBuff = (QDtreeNodeBlock**)QD_REALLOC(rbTree->nodeBuff, rbTree->buffCapacity * sizeof(QDtreeNodeBlock*));
			rbTree->dataBuff = (void**)QD_REALLOC(rbTree->dataBuff, rbTree->buffCapacity * sizeof(void*));
		}

		buffIdx = rbTree->buffCount - 1;
		rbTree->dataBuff[buffIdx] = QD_MALLOC(newBufferSize * rbTree->elemSize);
		rbTree->nodeBuff[buffIdx] = (QDtreeNodeBlock*)QD_MALLOC(newBufferSize * sizeof(QDtreeNodeBlock));
		for (size_t i = 0; i < newBufferSize; i++)
			rbTree->nodeBuff[buffIdx][i].nextFree = (i == newBufferSize - 1) ? SIZE_MAX : oldCapacity + i + 1;
	
		blockIdx = 0;
	}

	rbTree->firstFree = rbTree->nodeBuff[buffIdx][blockIdx].nextFree;

	void* dataToInsert = (char*)rbTree->dataBuff[buffIdx] + (blockIdx * rbTree->elemSize);
	if (rbTree->copy)
		rbTree->copy(dataToInsert, elem);
	else
		memcpy(dataToInsert, elem, rbTree->elemSize);
	
	QDtreeNode* nodeToInsert = &rbTree->nodeBuff[buffIdx][blockIdx].node;
	nodeToInsert->data = dataToInsert;

	nodeToInsert->parent = prevPtr;
	if (!prevPtr)
		rbTree->root = nodeToInsert;
	else if (rbTree->compare(nodeToInsert->data, prevPtr->data) < 0)
		prevPtr->l = nodeToInsert;
	else
		prevPtr->r = nodeToInsert;

	nodeToInsert->l = NULL;
	nodeToInsert->r = NULL;
	nodeToInsert->isRed = QD_TRUE;
	nodeToInsert->buffIdx = buffIdx;
	rbTree->len++;

	while (nodeToInsert->parent && nodeToInsert->parent->isRed)
	{
		if (nodeToInsert->parent == nodeToInsert->parent->parent->l)
		{
			QDtreeNode* uncle = nodeToInsert->parent->parent->r;
			if (uncle && uncle->isRed)
			{
				nodeToInsert->parent->isRed = QD_FALSE;
				uncle->isRed = QD_FALSE;
				nodeToInsert->parent->parent->isRed = QD_TRUE;

				nodeToInsert = nodeToInsert->parent->parent;
				continue;
			}

			if (nodeToInsert->parent->r == nodeToInsert)
			{
				nodeToInsert = nodeToInsert->parent;
				_qd_tree_rotate_left(rbTree, nodeToInsert);
			}

			nodeToInsert->parent->isRed = QD_FALSE;
			nodeToInsert->parent->parent->isRed = QD_TRUE;
			_qd_tree_rotate_right(rbTree, nodeToInsert->parent->parent);
		}
		else
		{
			QDtreeNode* uncle = nodeToInsert->parent->parent->l;
			if (uncle && uncle->isRed)
			{
				nodeToInsert->parent->isRed = QD_FALSE;
				uncle->isRed = QD_FALSE;

				nodeToInsert->parent->parent->isRed = QD_TRUE;
				nodeToInsert = nodeToInsert->parent->parent;
				continue;
			}

			if (nodeToInsert->parent->l == nodeToInsert)
			{
				nodeToInsert = nodeToInsert->parent;
				_qd_tree_rotate_right(rbTree, nodeToInsert);
			}

			nodeToInsert->parent->isRed = QD_FALSE;
			nodeToInsert->parent->parent->isRed = QD_TRUE;
			_qd_tree_rotate_left(rbTree, nodeToInsert->parent->parent);
		}
	}

	rbTree->root->isRed = QD_FALSE;
}

void _qd_tree_transplant(QDrbTree* rbTree, QDtreeNode* ptr1, QDtreeNode* ptr2)
{
	if (!ptr1->parent)
		rbTree->root = ptr2;
	else if (ptr1->parent->l == ptr1)
		ptr1->parent->l = ptr2;
	else
		ptr1->parent->r = ptr2;

	if (ptr2)
		ptr2->parent = ptr1->parent;
}

QDtreeNode* QD_PREFIX(tree_find)(QDrbTree* rbTree, void* elem)
{
	QDtreeNode* it = rbTree->root;
	while (it)
	{
		int32_t cmp = rbTree->compare(elem, it->data);
		if(cmp == 0)
			return it;
		else if(cmp < 0)
			it = it->l;
		else if(cmp > 0)
			it = it->r;
	}

	return NULL;
}

void QD_PREFIX(tree_remove)(QDrbTree* rbTree, QDtreeNode* it) 
{
	QDtreeNode* fixupNode;
	qd_bool_t toDeleteIsRed = it->isRed;

	if (!it->l)
	{
		fixupNode = it->r;
		_qd_tree_transplant(rbTree, it, it->r);
	}
	else if (!it->r)
	{
		fixupNode = it->l;
		_qd_tree_transplant(rbTree, it, it->l);
	}
	else
	{
		QDtreeNode* inorderSuccessor = it->r;
		while (inorderSuccessor->l)
			inorderSuccessor = inorderSuccessor->l;

		toDeleteIsRed = inorderSuccessor->isRed;
		fixupNode = inorderSuccessor->r;

		if (inorderSuccessor->parent == it) 
		{
			if (fixupNode)
				fixupNode->parent = inorderSuccessor;
		}
		else
		{
			_qd_tree_transplant(rbTree, inorderSuccessor, inorderSuccessor->r);
			inorderSuccessor->r = it->r;
			inorderSuccessor->r->parent = inorderSuccessor;
		}

		_qd_tree_transplant(rbTree, it, inorderSuccessor);
		inorderSuccessor->l = it->l;
		inorderSuccessor->l->parent = inorderSuccessor;
		inorderSuccessor->isRed = it->isRed;
	}

	if (rbTree->copy)
		rbTree->copy(NULL, it->data); // copy function destructs when dest=NULL

	size_t buffIdx = it->buffIdx;
	size_t blockIdx = (QDtreeNodeBlock*)it - rbTree->nodeBuff[buffIdx];

	size_t nextFree = 1;
	for (int i = 0; i < buffIdx; i++)
		nextFree <<= 1;
	nextFree += (blockIdx - 1);

	rbTree->nodeBuff[buffIdx][blockIdx].nextFree = rbTree->firstFree;
	rbTree->firstFree = nextFree;
	rbTree->len--;

	if (!toDeleteIsRed)
		return;

	while (fixupNode && fixupNode != rbTree->root && !(fixupNode && fixupNode->isRed))
	{
		if (fixupNode->parent->l == fixupNode)
		{
			QDtreeNode* sibling = fixupNode->parent->r;
			if (sibling && sibling->isRed)
			{
				sibling->isRed = QD_FALSE;
				fixupNode->parent->isRed = QD_TRUE;
				_qd_tree_rotate_left(rbTree, fixupNode->parent);
				sibling = fixupNode->parent->r;
			}

			if (!(sibling->l && sibling->l->isRed) && !(sibling->r && sibling->r->isRed))
			{
				sibling->isRed = QD_TRUE;
				fixupNode = fixupNode->parent;
			}
			else
			{
				if (!(sibling->r && sibling->r->isRed))
				{
					sibling->l->isRed = QD_FALSE;
					sibling->isRed = QD_TRUE;
					_qd_tree_rotate_right(rbTree, sibling);
					sibling = fixupNode->parent->r;
				}

				sibling->isRed = fixupNode->parent->isRed;
				fixupNode->parent->isRed = QD_FALSE;
				sibling->r->isRed = QD_FALSE;
				_qd_tree_rotate_left(rbTree, fixupNode->parent);
				fixupNode = rbTree->root;
			}
		}
		else
		{
			QDtreeNode* sibling = fixupNode->parent->l;
			if (sibling && sibling->isRed)
			{
				sibling->isRed = QD_FALSE;
				fixupNode->parent->isRed = QD_TRUE;
				_qd_tree_rotate_right(rbTree, fixupNode->parent);
				sibling = fixupNode->parent->l;
			}

			if (!(sibling->l && sibling->l->isRed) && !(sibling->r && sibling->r->isRed))
			{
				sibling->isRed = QD_TRUE;
				fixupNode = fixupNode->parent;
			}
			else
			{
				if (!(sibling->l && sibling->l->isRed))
				{
					sibling->r->isRed = QD_FALSE;
					sibling->isRed = QD_TRUE;
					_qd_tree_rotate_left(rbTree, sibling);
					sibling = fixupNode->parent->l;
				}

				sibling->isRed = fixupNode->parent->isRed;
				fixupNode->parent->isRed = QD_FALSE;
				sibling->l->isRed = QD_FALSE;
				_qd_tree_rotate_right(rbTree, fixupNode->parent);
				fixupNode = rbTree->root;
			}
		}
	}

	if (fixupNode)
		fixupNode->isRed = QD_FALSE;
}

#endif //#ifdef QD_IMPLEMENTATION

#ifdef __cplusplus
} //extern "C"
#endif

#endif //#ifdef QD_DATA_H