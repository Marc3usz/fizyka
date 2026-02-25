#include "arena.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

static void *arena_os_alloc(size_t size)
{
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr == MAP_FAILED ? NULL : ptr;
#endif
}

static void arena_os_free(void *ptr, size_t size)
{
    if (!ptr)
    {
        return;
    }
#ifdef _WIN32
    (void)size;
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

Arena *init_arena(size_t size)
{
    const size_t total_size = size + sizeof(Arena);
    Arena *arena = (Arena *)arena_os_alloc(total_size);
    if (!arena)
    {
        return NULL;
    }
    arena->size = total_size;
    arena->offset = sizeof(Arena);
    return arena;
}

void free_arena(Arena *arena)
{
    arena_os_free(arena, arena ? arena->size : 0);
}

void *_arena_alloc_impl(Arena *arena, size_t size)
{
    if (!arena || size > arena->size || arena->offset > arena->size - size)
    {
        return NULL;
    }
    void *ptr = (char *)(arena) + arena->offset;
    arena->offset += size;
    return ptr;
}

void *arena_alloc(Arena *arena, size_t size)
{
    return _arena_alloc_impl(arena, size);
}

void _arena_dealloc_impl(Arena *arena, size_t size)
{
    if (!arena || sizeof(Arena) + size > arena->offset)
    {
        return;
    }
    arena->offset -= size;
}

void arena_dealloc(Arena *arena, size_t size)
{
    _arena_dealloc_impl(arena, size);
}

Arena *init_scratch_arena(Arena *base_arena, size_t size)
{
    const size_t total_size = size + sizeof(Arena);
    Arena *scratch_arena = (Arena *)arena_alloc(base_arena, total_size);
    if (!scratch_arena)
    {
        return NULL;
    }
    scratch_arena->size = total_size;
    scratch_arena->offset = sizeof(Arena);
    return scratch_arena;
}

void free_scratch_arena(Arena *base_arena, Arena *scratch_arena)
{
    if (!base_arena || !scratch_arena)
    {
        return;
    }
    arena_dealloc(base_arena, scratch_arena->size);
}
