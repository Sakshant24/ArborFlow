#ifndef BIT_VECTOR_H
#define BIT_VECTOR_H

#include <stdint.h>
#include <stdlib.h>

// uint64_t  -- > unsigned long long
// uint32_t  -- > unsigned int
// uint16_t  -- > unsigned short

// Number of bits per word (using 64-bit words for efficiency) 
#define BV_WORD_SIZE   64
#define BV_WORD_SHIFT  6                        // log2(64) 
#define BV_WORD_MASK   (BV_WORD_SIZE - 1)       // 63    

struct BitVector{
    uint64_t *words;     // The underlying bit array               */
    uint32_t  capacity;  // Total number of bits this vector holds */
    uint32_t  num_words; // Number of 64-bit words allocated       */
};

typedef struct BitVector BitVector;

 // bv_create  — Allocate a BitVector with `capacity` bits, all set to 0.
 //             Returns NULL on allocation failure.

BitVector *bv_create(uint32_t capacity);

 // bv_destroy — Free all memory associated with a BitVector.

void bv_destroy(BitVector *bv);

 // bv_set     — Mark bit `index` as 1 (IP is blacklisted).

void bv_set(BitVector *bv, uint32_t index);

 // bv_clear   — Mark bit `index` as 0 (remove from blacklist).

// void bv_clear(BitVector *bv, uint32_t index);

 // bv_contains — Returns 1 if bit `index` is set, 0 otherwise. O(1).

int bv_contains(const BitVector *bv, uint32_t index);

 // bv_reset   — Set all bits to 0 (clear the entire blacklist).
 
// void bv_reset(BitVector *bv);

#endif /* BIT_VECTOR_H */
