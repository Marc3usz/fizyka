#ifndef SIZED_STRING_H
#define SIZED_STRING_H

#include "arena.h"
#include <stdbool.h>
#include <string.h>

typedef struct {
    size_t length;
    char* data;
} String;

#define String(x) (String){strlen(x), x}

String str_init(size_t length, Arena* arena);
String str_concat(String s1, String s2, Arena* arena);
String str_substring(String s, size_t start, size_t end, Arena* a);
bool str_contains(String haystack, String needle);
size_t str_index_of(String haystack, String needle);
String str_substring_view(String haystack, String needle);
bool str_equal(String a, String b);
String str_view(String s, size_t start, size_t end);
String str_clone(String s, Arena* arena);

#endif

