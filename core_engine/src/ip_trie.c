#include "../include/ip_trie.h"

// Creates and initializes an empty Trie
IpTrie* trie_create(void) {
    return (IpTrie *)calloc(1, sizeof(IpTrie)); 
}

// Inserts an IP address into the Trie
void trie_insert(IpTrie *trie, uint32_t ip) {
    if (!trie) return;

    uint8_t b1 = (ip >> 24) & 0xFF;
    uint8_t b2 = (ip >> 16) & 0xFF;
    uint8_t b3 = (ip >> 8)  & 0xFF;
    uint8_t b4 =  ip        & 0xFF;

    if (!trie->children[b1]) {
        trie->children[b1] = (NodeL2 *)calloc(1, sizeof(NodeL2));
    }
    if (!trie->children[b1]->children[b2]) {
        trie->children[b1]->children[b2] = (NodeL3 *)calloc(1, sizeof(NodeL3));
    }
    
    // Create the BitVector leaf if it doesn't exist (size 256 for one byte)
    if (!trie->children[b1]->children[b2]->children[b3]) {
        trie->children[b1]->children[b2]->children[b3] = bv_create(256);
    }

    BitVector *leaf = trie->children[b1]->children[b2]->children[b3];

    // Use standard BitVector API
    if (!bv_contains(leaf, b4)) {
        bv_set(leaf, b4);
        trie->count++;
    }
}

// Checks if an IP is currently flagged in the Trie
int trie_has(const IpTrie *trie, uint32_t ip) {
    if (!trie) return 0;

    uint8_t b1 = (ip >> 24) & 0xFF;
    uint8_t b2 = (ip >> 16) & 0xFF;
    uint8_t b3 = (ip >> 8)  & 0xFF;
    uint8_t b4 =  ip        & 0xFF;

    if (!trie->children[b1]) return 0;
    if (!trie->children[b1]->children[b2]) return 0;
    if (!trie->children[b1]->children[b2]->children[b3]) return 0;

    BitVector *leaf = trie->children[b1]->children[b2]->children[b3];

    // Use standard BitVector
    return bv_contains(leaf, b4);
}

// Fully destroys the trie and frees all associated memory
void trie_free(IpTrie *trie) {
    if (!trie) return;

    for (int b1 = 0; b1 < 256; b1++) {
        NodeL2 *n2 = trie->children[b1];
        if (n2) {
            for (int b2 = 0; b2 < 256; b2++) {
                NodeL3 *n3 = n2->children[b2];
                if (n3) {
                    for (int b3 = 0; b3 < 256; b3++) {
                        BitVector *leaf = n3->children[b3];
                        if (leaf) bv_destroy(leaf);
                    }
                    free(n3);
                }
            }
            free(n2);
        }
    }
    free(trie);
}