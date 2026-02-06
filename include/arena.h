#ifndef ARENA_H
#define ARENA_H
#include <stdlib.h>

typedef struct {
    size_t size, offset;
} Arena;

Arena* init_arena(size_t size) {
    Arena* arena = (Arena*)malloc(size + sizeof(Arena));
    arena->size = size + sizeof(Arena);
    arena->offset = sizeof(Arena);
    return arena;
}

void free_arena(Arena* arena) {
    free(arena);
}

void* _arena_alloc_impl(Arena* arena, size_t size) {
    if (arena->offset + size > arena->size) {
        return NULL;
    };
    void* ptr = (char*)(arena) + arena->offset;
    arena->offset += size;
    return ptr; 
}

void* arena_alloc(Arena* arena, size_t size) {
    return _arena_alloc_impl(arena, size);
}

void _arena_dealloc_impl(Arena* arena, size_t size) {
    if (sizeof(Arena) + size > arena->offset) {
        return;
    };
    arena->offset -= size;
}

void arena_dealloc(Arena* arena, size_t size) {
    _arena_dealloc_impl(arena, size);
}

#define arena_push(arena, typename) ((typename*)_arena_alloc_impl(arena, sizeof(typename)))

#define arena_pop(arena, typename) (_arena_dealloc_impl(arena, sizeof(typename)))

#endif