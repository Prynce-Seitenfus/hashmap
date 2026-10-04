# HashMap

[![C99](https://img.shields.io/badge/standard-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![MISRA C:2012](https://img.shields.io/badge/compliance-MISRA%20C%3A2012-brightgreen.svg)](https://www.misra.org.uk/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A minimal, zero-allocation, ultra-low-latency open-addressing hash table designed for embedded systems, SerTOS, and real-time execution profiling (`__cyg_profile_func_enter` / `__cyg_profile_func_exit`).

---

## Features

- **Freestanding & Zero Allocation**: Operates purely on caller-allocated arrays. Zero heap usage (`malloc`/`free` free) per MISRA C:2012 Rule 21.3.
- **Cache-Line Optimized (8-Byte Entry)**: Each slot contains strictly `const void* key` and `void* value` (8 bytes on 32-bit cores). Four complete slots fit in a single 32-byte cache line.
- **Knuth Fibonacci Multiplicative Shift Hashing**: Computes slot index in 2 CPU cycles (`MUL` + `LSR`) using Knuth's 32-bit golden ratio constant (`2654435769U`), uniformly dispersing aligned code addresses across power-of-two tables.
- **Zero Tombstones (Knuth Algorithm R)**: Deletions cyclically shift subsequent cluster elements backward, maintaining contiguous probe chains without tombstone markers.
- **Bounded 75% Load Limit**: Deterministic $O(1)$ real-time execution with average probe lengths $\le 1.4$ accesses.
- **In-Place Mutation (`hashmap_get_ref`)**: Yields `void**` for single-lookup in-place accumulator updates in profiler hot paths.
- **Lightweight Streaming Iterator (`HashMapIter`)**: Sequential traversal without memory allocation or recursive callbacks.

---

## Quick Start

```c
#include "hashmap.h"

#define MAP_CAPACITY  (64U) /* Must be a power of two >= 2 */

static HashMapEntry s_entries[MAP_CAPACITY];
static HashMap s_map;

void example(void)
{
    /* Initialize hash map over static storage */
    hashmap_init(&s_map, s_entries, MAP_CAPACITY);

    /* Insert pointer key and value */
    void* func_ptr = (void*)0x08001000U;
    uint32_t counter = 42U;
    hashmap_insert(&s_map, func_ptr, (void*)(uintptr_t)counter);

    /* Ultra-fast in-place mutation for profiler */
    void** slot = hashmap_get_ref(&s_map, func_ptr);
    if (slot != NULL) {
        /* Update value directly without re-hashing or re-searching */
        *slot = (void*)((uintptr_t)(*slot) + 1U);
    }

    /* Iterate through entries */
    HashMapIter iter = hashmap_iter(&s_map);
    const void* key;
    void* value;
    while (hashmap_iter_next(&iter, &key, &value)) {
        /* Process (key, value) */
    }
}
```

---

## API Summary

| Function | Description |
| :--- | :--- |
| `hashmap_init` | Initializes the map over a caller-allocated buffer with power-of-2 capacity. |
| `hashmap_insert` | Inserts or updates an entry (bounded to 75% load capacity). |
| `hashmap_get` | Looks up the value associated with a key. |
| `hashmap_get_ref` | Returns a direct `void**` reference to the value slot for in-place mutation. |
| `hashmap_contains` | Checks key existence. |
| `hashmap_remove` | Removes a key and backward-shifts collision clusters (zero tombstones). |
| `hashmap_clear` | Resets all slots and entry count to zero. |
| `hashmap_size` | Returns the current number of occupied entries. |
| `hashmap_capacity` | Returns the total slot capacity. |
| `hashmap_is_empty` | Checks if count is 0. |
| `hashmap_iter` | Constructs a sequential traversal iterator. |
| `hashmap_iter_next` | Advances iterator to the next occupied entry. |

---

## Quality & Compliance

- **C Standard**: ANSI/ISO C99 (`-std=c99`)
- **MISRA C:2012**: Compliant (no heap, zero single-line `//` comments, Doxygen documentation)
- **Complexity Gate**: Max CCN $\le 10$, Max NLOC $\le 75$, Max Params $\le 5$