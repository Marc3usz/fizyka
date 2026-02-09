#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "arena.h"
#include <string.h>

/*
 * Simple macro-based dynamic array for arbitrary types.
 * 
 * Usage example:
 *   DEFINE_ARRAY(int);  // Define Array_int type
 *   
 *   Arena arena = {0};
 *   arena_init(&arena, 1024);
 *   
 *   Array_int arr = {0};
 *   array_init(&arr, 4, &arena);
 *   
 *   array_push(&arr, 42, &arena);
 *   array_push(&arr, 100, &arena);
 *   
 *   int val = array_get(&arr, 0);  // val = 42
 *   array_set(&arr, 1, 200);       // arr[1] = 200
 *   
 *   for (size_t i = 0; i < array_len(&arr); i++) {
 *       printf("%d\n", array_get(&arr, i));
 *   }
 */

// Define a dynamic array type for a specific type
#define DEFINE_ARRAY(T) \
    typedef struct { \
        T* data; \
        size_t length; \
        size_t capacity; \
    } Array_##T

// Initialize an array with initial capacity
#define array_init(arr, cap, arena) \
    do { \
        (arr)->capacity = (cap); \
        (arr)->length = 0; \
        (arr)->data = arena_alloc((arena), (cap) * sizeof(*(arr)->data)); \
    } while(0)

// Push an element to the array (grows if needed) - returns the index of the new element
#define array_push(arr, value, arena) \
    ({ \
        if ((arr)->length >= (arr)->capacity) { \
            size_t new_cap = (arr)->capacity == 0 ? 8 : (arr)->capacity * 2; \
            typeof((arr)->data) new_data = arena_alloc((arena), new_cap * sizeof(*(arr)->data)); \
            if ((arr)->data) { \
                memcpy(new_data, (arr)->data, (arr)->length * sizeof(*(arr)->data)); \
            } \
            (arr)->data = new_data; \
            (arr)->capacity = new_cap; \
        } \
        size_t _index = (arr)->length; \
        (arr)->data[(arr)->length++] = (value); \
        _index; \
    })

// Get element at index
#define array_get(arr, index) ((arr)->data[index])

// Set element at index
#define array_set(arr, index, value) ((arr)->data[index] = (value))

// Clear array (reset length to 0)
#define array_clear(arr) ((arr)->length = 0)

// Get array length
#define array_len(arr) ((arr)->length)

// Iterate over array elements
#define foreach(arr, item_ptr) \
    for (item_ptr = (arr)->data; item_ptr < (arr)->data + (arr)->length; item_ptr++)

#endif
