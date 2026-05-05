#include "../include/bit_vector.h"
#include <stdio.h>
#include <string.h>


BitVector *bv_create(uint32_t capacity) {
    if (capacity == 0) return NULL;

    BitVector *bv = (BitVector *)malloc(sizeof(BitVector));
    // check failed allocation

    // Round up to the nearest 64-bit word boundary 
    uint32_t num_words = (capacity + BV_WORD_SIZE - 1) / BV_WORD_SIZE;

    bv->words = (uint64_t *)calloc(num_words, sizeof(uint64_t)); 
    // check failed allocation

    bv->capacity  = capacity;
    bv->num_words = num_words;
    return bv;
}

void bv_destroy(BitVector *bv) {
    if (!bv) return;
    free(bv->words);
    free(bv);
}

void bv_set(BitVector *bv, uint32_t index) {
    if (!bv || index >= bv->capacity) return;
    bv->words[index >> BV_WORD_SHIFT] |= (1ULL << (index & BV_WORD_MASK));
}

void bv_clear(BitVector *bv, uint32_t index) {
    if (!bv) return;
    if (index >= bv->capacity) return;
    bv->words[index >> BV_WORD_SHIFT] &= ~(1ULL << (index & BV_WORD_MASK));
}

int bv_contains(const BitVector *bv, uint32_t index) {
    if (!bv || index >= bv->capacity) return 0;
    return (bv->words[index >> BV_WORD_SHIFT] >> (index & BV_WORD_MASK)) & 1;
}

void bv_reset(BitVector *bv) {
    if (!bv) return;
    memset(bv->words, 0, bv->num_words * sizeof(uint64_t));
}
