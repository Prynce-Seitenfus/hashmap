#include "hashmap.h"
#include <string.h>

/* Helper: Validates that capacity is a power of two (>= 2) */
static bool is_power_of_two(size_t val)
{
    return ((val >= 2U) && ((val & (val - 1U)) == 0U));
}

/* Helper: Computes Fibonacci shift factor: 32 - log2(capacity) */
static uint8_t compute_shift(size_t capacity)
{
    uint8_t bits = 0U;
    size_t temp = capacity;
    while (temp > 1U) {
        temp >>= 1U;
        bits++;
    }
    return (uint8_t)(32U - bits);
}

/* Helper: Knuth Algorithm R backward-shift to restore collision cluster continuity */
static void shift_cluster_backward(HashMap* map, size_t hole_index)
{
    size_t i = hole_index;
    size_t j = i;
    size_t mask = map->mask;
    uint8_t shift = map->shift;
    HashMapEntry* entries = map->entries;

    while (true) {
        entries[i].key = NULL;
        entries[i].value = NULL;

        while (true) {
            j = (j + 1U) & mask;
            if (entries[j].key == NULL) {
                map->count--;
                return;
            }
            size_t r = hashmap_hash_key(entries[j].key, shift);
            bool between = (i <= j) ? ((r <= i) || (r > j)) : ((r <= i) && (r > j));
            if (between) {
                break;
            }
        }

        entries[i] = entries[j];
        i = j;
    }
}

bool hashmap_init(HashMap* map, HashMapEntry* entries, size_t capacity)
{
    if ((map == NULL) || (entries == NULL) || (!is_power_of_two(capacity))) {
        return false;
    }

    map->entries = entries;
    map->capacity = capacity;
    map->mask = capacity - 1U;
    map->max_count = (capacity * 3U) >> 2U; /* 75% max load factor limit */
    map->shift = compute_shift(capacity);
    map->count = 0U;

    for (size_t i = 0U; i < capacity; i++) {
        map->entries[i].key = NULL;
        map->entries[i].value = NULL;
    }

    return true;
}

bool hashmap_insert(HashMap* map, const void* key, void* value)
{
    if ((map == NULL) || (key == NULL)) {
        return false;
    }

    size_t mask = map->mask;
    size_t idx = hashmap_hash_key(key, map->shift);
    HashMapEntry* entries = map->entries;

    while (entries[idx].key != NULL) {
        if (entries[idx].key == key) {
            entries[idx].value = value;
            return true;
        }
        idx = (idx + 1U) & mask;
    }

    if (map->count >= map->max_count) {
        return false; /* 75% load limit reached */
    }

    entries[idx].key = key;
    entries[idx].value = value;
    map->count++;
    return true;
}

bool hashmap_get(const HashMap* map, const void* key, void** out_value)
{
    if ((map == NULL) || (key == NULL)) {
        return false;
    }

    size_t mask = map->mask;
    size_t idx = hashmap_hash_key(key, map->shift);
    const HashMapEntry* entries = map->entries;

    while (entries[idx].key != NULL) {
        if (entries[idx].key == key) {
            if (out_value != NULL) {
                *out_value = entries[idx].value;
            }
            return true;
        }
        idx = (idx + 1U) & mask;
    }

    return false;
}

void** hashmap_get_ref(HashMap* map, const void* key)
{
    if ((map == NULL) || (key == NULL)) {
        return NULL;
    }

    size_t mask = map->mask;
    size_t idx = hashmap_hash_key(key, map->shift);
    HashMapEntry* entries = map->entries;

    while (entries[idx].key != NULL) {
        if (entries[idx].key == key) {
            return &entries[idx].value;
        }
        idx = (idx + 1U) & mask;
    }

    return NULL;
}

bool hashmap_contains(const HashMap* map, const void* key)
{
    return hashmap_get(map, key, NULL);
}

bool hashmap_remove(HashMap* map, const void* key, void** out_removed_value)
{
    if ((map == NULL) || (key == NULL) || (map->count == 0U)) {
        return false;
    }

    size_t mask = map->mask;
    size_t i = hashmap_hash_key(key, map->shift);
    const HashMapEntry* entries = map->entries;

    while (entries[i].key != NULL) {
        if (entries[i].key == key) {
            break;
        }
        i = (i + 1U) & mask;
    }

    if (entries[i].key == NULL) {
        return false;
    }

    if (out_removed_value != NULL) {
        *out_removed_value = entries[i].value;
    }

    shift_cluster_backward(map, i);
    return true;
}

void hashmap_clear(HashMap* map)
{
    if (map != NULL) {
        for (size_t i = 0U; i < map->capacity; i++) {
            map->entries[i].key = NULL;
            map->entries[i].value = NULL;
        }
        map->count = 0U;
    }
}

size_t hashmap_size(const HashMap* map)
{
    return (map != NULL) ? map->count : 0U;
}

size_t hashmap_capacity(const HashMap* map)
{
    return (map != NULL) ? map->capacity : 0U;
}

bool hashmap_is_empty(const HashMap* map)
{
    return ((map == NULL) || (map->count == 0U));
}

HashMapIter hashmap_iter(const HashMap* map)
{
    HashMapIter iter;
    iter.map = map;
    iter.index = 0U;
    return iter;
}

bool hashmap_iter_next(HashMapIter* iter, const void** out_key, void** out_value)
{
    if ((iter == NULL) || (iter->map == NULL)) {
        return false;
    }

    const HashMap* map = iter->map;
    while (iter->index < map->capacity) {
        size_t curr = iter->index;
        iter->index++;
        if (map->entries[curr].key != NULL) {
            if (out_key != NULL) {
                *out_key = map->entries[curr].key;
            }
            if (out_value != NULL) {
                *out_value = map->entries[curr].value;
            }
            return true;
        }
    }

    return false;
}
