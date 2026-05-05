#include "../include/gatekeeper.h"
#include <stdio.h>
#include <string.h>

/*
 * gatekeeper.c — ArborFlow Filtering Gatekeeper
 * -----------------------------------------------
 */

int gk_init(Gatekeeper *gk) {
    if (!gk) return -1;

    memset(gk, 0, sizeof(Gatekeeper));

    /* Initialize the single static blacklist Trie */
    gk->blacklist = trie_create();
    if (!gk->blacklist) {
        fprintf(stderr, "[Gatekeeper] Failed to create blacklist Trie\n");
        return -1;
    }

    gk->drop_count = 0;
    gk->pass_count = 0;

    printf("[Gatekeeper] Initialized. Static IP Trie engine ready.\n");
    return 0;
}

void gk_destroy(Gatekeeper *gk) {
    if (!gk) return;
    trie_free(gk->blacklist);
    gk->blacklist = NULL;
}

/*
 * gk_load_blacklist — Parse a text file of IPs and load into the Trie.
 * Supports Dotted-decimal (192.168.1.1) and Raw hex (0xC0A80101).
 */
int gk_load_blacklist(Gatekeeper *gk, const char *filepath) {
    if (!gk || !filepath) return -1;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "[Gatekeeper] Cannot open blacklist file: %s\n", filepath);
        return -1;
    }

    char line[64];
    int loaded = 0;

    while (fgets(line, sizeof(line), f)) {
        /* Strip newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\0') continue;

        uint32_t ip = 0;

        if (strncmp(line, "0x", 2) == 0 || strncmp(line, "0X", 2) == 0) {
            /* Hex format */
            ip = (uint32_t)strtoul(line, NULL, 16);
        } else {
            /* Dotted-decimal format */
            unsigned int a, b, c, d;
            if (sscanf(line, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 ||
                a > 255 || b > 255 || c > 255 || d > 255) {
                fprintf(stderr, "[Gatekeeper] Skipping invalid IP: %s\n", line);
                continue;
            }
            ip = (a << 24) | (b << 16) | (c << 8) | d;
        }
        
        /* Insert directly into the Trie */
        trie_insert(gk->blacklist, ip);
        loaded++;
    }

    fclose(f);
    printf("[Gatekeeper] Loaded %d new IPs from %s\n", loaded, filepath);
    return loaded;
}

/*
 * check_ip — The core per-packet decision function.
 */
int check_ip(Gatekeeper *gk, uint32_t ip) {
    if (!gk) return 0;  /* Fail-open if misconfigured */

    /* Check static known-bad IPs */
    if (trie_has(gk->blacklist, ip)) {
        gk->drop_count++;
        return 1;  /* DROP */
    }

    gk->pass_count++;
    return 0;  /* PASS */
}

void gk_print_stats(const Gatekeeper *gk) {
    if (!gk) return;
    printf("[Gatekeeper Stats]\n");
    printf("  Blacklist IPs loaded : %u\n",   gk->blacklist ? gk->blacklist->count : 0);
    printf("  Packets dropped      : %u\n",   gk->drop_count);
    printf("  Packets passed       : %u\n",   gk->pass_count);
    
    uint32_t total = gk->drop_count + gk->pass_count;
    if (total > 0) {
        printf("  Drop rate            : %.2f%%\n", 100.0f * gk->drop_count / total);
    }
}