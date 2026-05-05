#include "../include/splay_tree.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    SessionKey *items;
    size_t size;
    size_t capacity;
} SessionKeyBuffer;

static int session_key_cmp(SessionKey a, SessionKey b) {
    if (a.src_ip != b.src_ip) return (a.src_ip < b.src_ip) ? -1 : 1;
    if (a.dst_ip != b.dst_ip) return (a.dst_ip < b.dst_ip) ? -1 : 1;
    if (a.src_port != b.src_port) return (a.src_port < b.src_port) ? -1 : 1;
    if (a.dst_port != b.dst_port) return (a.dst_port < b.dst_port) ? -1 : 1;
    if (a.protocol != b.protocol) return (a.protocol < b.protocol) ? -1 : 1;
    return 0;
}

static SessionNode *new_node(SessionKey key, uint64_t bytes, uint64_t now_epoch) {
    SessionNode *node = (SessionNode *)calloc(1, sizeof(SessionNode));
    if (!node) return NULL;

    node->key = key;
    node->packet_count = 1;
    node->byte_count = bytes;
    node->last_seen_epoch = now_epoch;
    return node;
}

static SessionNode *rotate_right(SessionNode *node) {
    SessionNode *pivot = node->left;
    node->left = pivot->right;
    pivot->right = node;
    return pivot;
}

static SessionNode *rotate_left(SessionNode *node) {
    SessionNode *pivot = node->right;
    node->right = pivot->left;
    pivot->left = node;
    return pivot;
}

static SessionNode *splay(SessionNode *root, SessionKey key) {
    SessionNode header;
    SessionNode *left_tree_max;
    SessionNode *right_tree_min;

    if (!root) return NULL;

    memset(&header, 0, sizeof(header));
    left_tree_max = &header;
    right_tree_min = &header;

    while (1) {
        int cmp = session_key_cmp(key, root->key);

        if (cmp < 0) {
            if (!root->left) break;

            if (session_key_cmp(key, root->left->key) < 0) {
                root = rotate_right(root);
                if (!root->left) break;
            }

            right_tree_min->left = root;
            right_tree_min = root;
            root = root->left;
        } else if (cmp > 0) {
            if (!root->right) break;

            if (session_key_cmp(key, root->right->key) > 0) {
                root = rotate_left(root);
                if (!root->right) break;
            }

            left_tree_max->right = root;
            left_tree_max = root;
            root = root->right;
        } else {
            break;
        }
    }

    left_tree_max->right = root->left;
    right_tree_min->left = root->right;
    root->left = header.right;
    root->right = header.left;

    return root;
}

static void destroy_postorder(SessionNode *node) {
    if (!node) return;
    destroy_postorder(node->left);
    destroy_postorder(node->right);
    free(node);
}

static int key_buffer_push(SessionKeyBuffer *buffer, SessionKey key) {
    if (buffer->size == buffer->capacity) {
        size_t new_capacity = (buffer->capacity == 0) ? 16 : buffer->capacity * 2;
        SessionKey *next = (SessionKey *)realloc(buffer->items, new_capacity * sizeof(SessionKey));
        if (!next) return -1;
        buffer->items = next;
        buffer->capacity = new_capacity;
    }

    buffer->items[buffer->size++] = key;
    return 0;
}

static void collect_expired(
    SessionNode *node,
    uint64_t now_epoch,
    uint64_t idle_timeout_sec,
    SessionKeyBuffer *expired
) {
    if (!node) return;

    collect_expired(node->left, now_epoch, idle_timeout_sec, expired);

    if (now_epoch >= node->last_seen_epoch) {
        uint64_t age = now_epoch - node->last_seen_epoch;
        if (age > idle_timeout_sec) {
            key_buffer_push(expired, node->key);
        }
    }

    collect_expired(node->right, now_epoch, idle_timeout_sec, expired);
}

void sm_init(SessionManager *manager, uint64_t idle_timeout_sec) {
    if (!manager) return;

    manager->root = NULL;
    manager->session_count = 0;
    manager->idle_timeout_sec = idle_timeout_sec;
}

void sm_destroy(SessionManager *manager) {
    if (!manager) return;

    destroy_postorder(manager->root);
    manager->root = NULL;
    manager->session_count = 0;
}

SessionNode *sm_touch(
    SessionManager *manager,
    SessionKey key,
    uint64_t packet_bytes,
    uint64_t now_epoch
) {
    SessionNode *node;
    int cmp;

    if (!manager) return NULL;

    if (!manager->root) {
        manager->root = new_node(key, packet_bytes, now_epoch);
        if (!manager->root) return NULL;
        manager->session_count = 1;
        return manager->root;
    }

    manager->root = splay(manager->root, key);
    cmp = session_key_cmp(key, manager->root->key);

    if (cmp == 0) {
        manager->root->packet_count++;
        manager->root->byte_count += packet_bytes;
        manager->root->last_seen_epoch = now_epoch;
        return manager->root;
    }

    node = new_node(key, packet_bytes, now_epoch);
    if (!node) return NULL;

    if (cmp < 0) {
        node->left = manager->root->left;
        node->right = manager->root;
        manager->root->left = NULL;
    } else {
        node->right = manager->root->right;
        node->left = manager->root;
        manager->root->right = NULL;
    }

    manager->root = node;
    manager->session_count++;

    return manager->root;
}

int sm_contains(SessionManager *manager, SessionKey key) {
    int cmp;

    if (!manager || !manager->root) return 0;

    manager->root = splay(manager->root, key);
    cmp = session_key_cmp(key, manager->root->key);

    return (cmp == 0);
}

int sm_remove(SessionManager *manager, SessionKey key) {
    SessionNode *to_delete;

    if (!manager || !manager->root) return 0;

    manager->root = splay(manager->root, key);
    if (session_key_cmp(key, manager->root->key) != 0) {
        return 0;
    }

    to_delete = manager->root;

    if (!manager->root->left) {
        manager->root = manager->root->right;
    } else {
        manager->root = splay(manager->root->left, key);
        manager->root->right = to_delete->right;
    }

    free(to_delete);
    if (manager->session_count > 0) {
        manager->session_count--;
    }
    return 1;
}

size_t sm_expire_idle(SessionManager *manager, uint64_t now_epoch) {
    SessionKeyBuffer expired;
    size_t i;
    size_t removed = 0;

    if (!manager || !manager->root) return 0;

    expired.items = NULL;
    expired.size = 0;
    expired.capacity = 0;

    collect_expired(
        manager->root,
        now_epoch,
        manager->idle_timeout_sec,
        &expired
    );

    for (i = 0; i < expired.size; i++) {
        removed += (size_t)sm_remove(manager, expired.items[i]);
    }

    free(expired.items);
    return removed;
}
