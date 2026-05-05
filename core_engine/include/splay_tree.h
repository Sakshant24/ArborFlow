#ifndef SPLAY_TREE_H
#define SPLAY_TREE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
} SessionKey;

typedef struct SessionNode {
    SessionKey key;
    uint64_t last_seen_epoch;
    uint64_t packet_count;
    uint64_t byte_count;
    struct SessionNode *left;
    struct SessionNode *right;
} SessionNode;

typedef struct {
    SessionNode *root;
    size_t session_count;
    uint64_t idle_timeout_sec;
} SessionManager;

void sm_init(SessionManager *manager, uint64_t idle_timeout_sec);
void sm_destroy(SessionManager *manager);

SessionNode *sm_touch(
    SessionManager *manager,
    SessionKey key,
    uint64_t packet_bytes,
    uint64_t now_epoch
);

int sm_contains(SessionManager *manager, SessionKey key);
int sm_remove(SessionManager *manager, SessionKey key);
size_t sm_expire_idle(SessionManager *manager, uint64_t now_epoch);

#ifdef __cplusplus
}
#endif

#endif