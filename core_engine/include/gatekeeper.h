#ifndef GATEKEEPER_H
#define GATEKEEPER_H

/*
 * gatekeeper.h — ArborFlow Filtering Gatekeeper (Static IP Trie)
 * -----------------------------------------------
 * Provides a high-speed IP filtering pipeline.
 * Only utilizes a static Blacklist loaded from file.
 */

#include <stdint.h>
#include "ip_trie.h"

typedef struct {
    IpTrie   *blacklist;       /* Static known-bad IPs      */
    uint32_t  drop_count;      /* Total dropped packets     */
    uint32_t  pass_count;      /* Total passed packets      */
} Gatekeeper;

int gk_init(Gatekeeper *gk);
void gk_destroy(Gatekeeper *gk);
int gk_load_blacklist(Gatekeeper *gk, const char *filepath);
int check_ip(Gatekeeper *gk, uint32_t ip);
void gk_print_stats(const Gatekeeper *gk);

#endif /* GATEKEEPER_H */