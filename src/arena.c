#include "arena.h"

Arena* init_arena(size_t size) {
    Arena* arena = (Arena*)malloc(size + sizeof(Arena));
    if (!arena) {
        return NULL;
    }
    arena->size = size + sizeof(Arena);
    arena->offset = sizeof(Arena);
    return arena;
}

void free_arena(Arena* arena) {
    free(arena);
}

void* _arena_alloc_impl(Arena* arena, size_t size) {
    if (!arena || arena->offset + size > arena->size) {
        return NULL;
    }
    void* ptr = (char*)(arena) + arena->offset;
    arena->offset += size;
    return ptr;
}

void* arena_alloc(Arena* arena, size_t size) {
    return _arena_alloc_impl(arena, size);
}

void _arena_dealloc_impl(Arena* arena, size_t size) {
    if (!arena || sizeof(Arena) + size > arena->offset) {
        return;
    }
    arena->offset -= size;
}

void arena_dealloc(Arena* arena, size_t size) {
    _arena_dealloc_impl(arena, size);
}

Arena* init_scratch_arena(Arena* base_arena, size_t size) {
    return (Arena*)arena_alloc(base_arena, size + sizeof(Arena));
}

void free_scratch_arena(Arena* base_arena, Arena* scratch_arena) {
    if (!base_arena || !scratch_arena) {
        return;
    }
    arena_dealloc(base_arena, scratch_arena->size);
}
