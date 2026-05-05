#ifndef IP_TRIE_H
#define IP_TRIE_H

#include <stdint.h>
#include <stdlib.h>
#include "../include/bit_vector.h" /* Update path as per your folder structure */

/* ========================================================================= */
/* DATA STRUCTURES                                                           */
/* ========================================================================= */

typedef struct {
    BitVector *children[256];
} NodeL3;

typedef struct {
    NodeL3 *children[256];
} NodeL2;

typedef struct {
    NodeL2 *children[256];
    uint32_t count; 
} IpTrie;

/* ========================================================================= */
/* PUBLIC API                                                                */
/* ========================================================================= */

IpTrie* trie_create(void);
void trie_insert(IpTrie *trie, uint32_t ip);
int trie_has(const IpTrie *trie, uint32_t ip);
void trie_free(IpTrie *trie);

#endif /* IP_TRIE_H */