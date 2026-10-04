#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Knuth 32-bit Golden Ratio multiplicative constant.
 * 2^32 * (sqrt(5) - 1) / 2 = 2654435769U
 */
#define HASHMAP_GOLDEN_RATIO_32  (2654435769U)

/**
 * @brief Individual entry slot in the hash table.
 * Exactly 8 bytes on 32-bit architectures; 16 bytes on 64-bit architectures.
 */
typedef struct HashMapEntry {
    const void* key;     /**< Pointer key (NULL represents an empty slot). */
    void*       value;   /**< Pointer to user payload, metrics struct, or integer value. */
} HashMapEntry;

/**
 * @brief Core essential HashMap container.
 */
typedef struct HashMap {
    HashMapEntry* entries;    /**< Caller-allocated array of HashMapEntry. */
    size_t        capacity;   /**< Total number of slots (must be power of 2 >= 2). */
    size_t        mask;       /**< Fast bitmask (capacity - 1U). */
    size_t        max_count;  /**< Maximum permitted elements (75% load factor). */
    uint8_t       shift;      /**< Fibonacci shift count: 32 - log2(capacity). */
    size_t        count;      /**< Current number of occupied elements. */
} HashMap;

/**
 * @brief Lightweight, zero-allocation iterator over occupied hashmap entries.
 */
typedef struct HashMapIter {
    const HashMap* map;       /**< Pointer to parent HashMap instance. */
    size_t         index;     /**< Current table traversal slot index. */
} HashMapIter;

/**
 * @brief Fast Fibonacci multiplicative hash for 32-bit pointer keys.
 *
 * @param key Key pointer to hash.
 * @param shift Precomputed shift count: (32 - log2(capacity)).
 * @return Slot index in range [0, capacity - 1].
 */
static inline size_t hashmap_hash_key(const void* key, uint8_t shift)
{
    return (size_t)(((uint32_t)(uintptr_t)key * HASHMAP_GOLDEN_RATIO_32) >> shift);
}

/**
 * @brief Initializes the hash map instance over caller-allocated memory.
 *
 * Enforces power-of-two capacity (>= 2) and calculates 75% load limit.
 *
 * @param map Pointer to HashMap container.
 * @param entries Pointer to caller-allocated array of HashMapEntry.
 * @param capacity Number of entries in buffer (must be power of 2 >= 2).
 * @return true if initialized successfully, false on invalid parameters.
 */
bool hashmap_init(HashMap* map, HashMapEntry* entries, size_t capacity);

/**
 * @brief Inserts or updates a key-value mapping.
 *
 * Rejects key == NULL and rejects new insertions when count >= max_count (75% load).
 *
 * @param map Pointer to HashMap instance.
 * @param key Key to insert (must not be NULL).
 * @param value Value pointer to store.
 * @return true on success, false if table is at capacity limit or parameters invalid.
 */
bool hashmap_insert(HashMap* map, const void* key, void* value);

/**
 * @brief Retrieves the value associated with a key.
 *
 * @param map Pointer to HashMap instance.
 * @param key Key to look up (must not be NULL).
 * @param out_value Destination pointer to receive value (may be NULL).
 * @return true if key was found, false otherwise.
 */
bool hashmap_get(const HashMap* map, const void* key, void** out_value);

/**
 * @brief Retrieves a direct reference to the value slot within the hash map.
 *
 * Enables single-lookup in-place mutation of counters inside profiler hooks.
 *
 * @param map Pointer to HashMap instance.
 * @param key Key to look up (must not be NULL).
 * @return Direct pointer to internal (void*) value slot, or NULL if not found.
 */
void** hashmap_get_ref(HashMap* map, const void* key);

/**
 * @brief Checks if a key exists within the hash map.
 *
 * @param map Pointer to HashMap instance.
 * @param key Key to search for.
 * @return true if present, false otherwise.
 */
bool hashmap_contains(const HashMap* map, const void* key);

/**
 * @brief Removes a key and its value using Knuth Algorithm R (zero tombstones).
 *
 * @param map Pointer to HashMap instance.
 * @param key Key to remove.
 * @param out_removed_value Destination pointer to store removed value (may be NULL).
 * @return true if element was found and removed, false otherwise.
 */
bool hashmap_remove(HashMap* map, const void* key, void** out_removed_value);

/**
 * @brief Clears all entries from the hash map.
 *
 * @param map Pointer to HashMap instance.
 */
void hashmap_clear(HashMap* map);

/**
 * @brief Returns the number of elements currently stored.
 *
 * @param map Pointer to HashMap instance.
 * @return Number of elements, or 0 if map is NULL.
 */
size_t hashmap_size(const HashMap* map);

/**
 * @brief Returns the total capacity of the hash map.
 *
 * @param map Pointer to HashMap instance.
 * @return Total capacity, or 0 if map is NULL.
 */
size_t hashmap_capacity(const HashMap* map);

/**
 * @brief Checks if the hash map contains zero elements.
 *
 * @param map Pointer to HashMap instance.
 * @return true if empty or NULL, false otherwise.
 */
bool hashmap_is_empty(const HashMap* map);

/**
 * @brief Initializes an iterator for sequential table traversal.
 *
 * @param map Pointer to HashMap instance.
 * @return Initialized HashMapIter instance.
 */
HashMapIter hashmap_iter(const HashMap* map);

/**
 * @brief Advances iterator to the next occupied entry.
 *
 * @param iter Pointer to HashMapIter instance.
 * @param out_key Destination pointer to store key pointer (may be NULL).
 * @param out_value Destination pointer to store value pointer (may be NULL).
 * @return true if an entry was retrieved, false if iteration completed.
 */
bool hashmap_iter_next(HashMapIter* iter, const void** out_key, void** out_value);

#ifdef __cplusplus
}
#endif

#endif /* HASHMAP_H */
