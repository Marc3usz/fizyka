#include "sized_string.h"

String str_init(size_t length, Arena* arena) {
    String s = {0};
    s.length = length;
    s.data = (char*)arena_alloc(arena, length + 1);
    if (!s.data) {
        s.length = 0;
        return s;
    }
    s.data[length] = 0;
    return s;
}

String str_concat(String s1, String s2, Arena* arena) {
    size_t len = s1.length + s2.length;
    String s = str_init(len, arena);
    if (!s.data) {
        return s;
    }
    memcpy(s.data, s1.data, s1.length);
    memcpy(&s.data[s1.length], s2.data, s2.length);
    return s;
}

String str_substring(String s, size_t start, size_t end, Arena* a) {
    String r = {0};
    if (end <= s.length && start < end) {
        r = str_init(end - start, a);
        if (r.data) {
            memcpy(r.data, &s.data[start], r.length);
        }
    }
    return r;
}

bool str_contains(String haystack, String needle) {
    bool found = false;
    for (size_t i = 0, j = 0; i < haystack.length && !found; i += 1) {
        while (haystack.data[i] == needle.data[j]) {
            j += 1;
            i += 1;
            if (j == needle.length) {
                found = true;
                break;
            }
        }
    }
    return found;
}

size_t str_index_of(String haystack, String needle) {
    for (size_t i = 0; i < haystack.length; i += 1) {
        size_t j = 0;
        size_t start = i;
        while (haystack.data[i] == needle.data[j]) {
            j += 1;
            i += 1;
            if (j == needle.length) {
                return start;
            }
        }
    }
    return (size_t)-1;
}

String str_substring_view(String haystack, String needle) {
    String r = {0};
    size_t start_index = str_index_of(haystack, needle);
    if (start_index < haystack.length) {
        r.data = &haystack.data[start_index];
        r.length = needle.length;
    }
    return r;
}

bool str_equal(String a, String b) {
    if (a.length != b.length) {
        return false;
    }
    return memcmp(a.data, b.data, a.length) == 0;
}

String str_view(String s, size_t start, size_t end) {
    if (end < start || end - start > s.length) {
        return (String){0};
    }
    return (String){end - start, s.data + start};
}

String str_clone(String s, Arena* arena) {
    String r = {0};
    if (s.length) {
        r.data = (char*)arena_alloc(arena, s.length);
        if (!r.data) {
            return r;
        }
        r.length = s.length;
        memcpy(r.data, s.data, s.length);
    }
    return r;
}
