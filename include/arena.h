#ifndef ARENA_H
#define ARENA_H
#include <stdlib.h>

typedef struct {
    size_t size, offset;
} Arena;

Arena* init_arena(size_t size);
void free_arena(Arena* arena);
void* _arena_alloc_impl(Arena* arena, size_t size);
void* arena_alloc(Arena* arena, size_t size);
void _arena_dealloc_impl(Arena* arena, size_t size);
void arena_dealloc(Arena* arena, size_t size);

#define arena_push(arena, typename) ((typename*)_arena_alloc_impl(arena, sizeof(typename)))

#define arena_pop(arena, typename) (_arena_dealloc_impl(arena, sizeof(typename)))

Arena* init_scratch_arena(Arena* base_arena, size_t size);
void free_scratch_arena(Arena* base_arena, Arena* scratch_arena);

#endif