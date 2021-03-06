/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _HEAP_H_
#define _HEAP_H_

#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

struct heap_node {
    struct heap_node *parent;
    struct heap_node *left;
    struct heap_node *right;
};

struct heap_root {
    struct heap_node *node;
    unsigned int count;
};

#define HEAP_STATIC \
    {NULL, 0}

#define HEAP_CACHED_STATIC \
    {{NULL}, NULL}

#define HEAP_INIT \
    (struct heap_root) HEAP_STATIC

#define HEAP_ROOT(name) \
    struct heap_root name = HEAP_INIT

#define HEAP_EMPTY_ROOT(root) \
    ((root)->node == NULL)

#define HEAP_EMPTY_NODE(node) \
    ((node)->parent == (node))

#define HEAP_CLEAR_NODE(node) \
    ((node)->parent = (node))

#define HEAP_ROOT_NODE(root) \
    ((root)->node)

#define HEAP_NODE_COUNT(root) \
    ((root)->count)

/**
 * heap_entry - get the struct for this entry.
 * @ptr: the &struct heap_node pointer.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the heap_node within the struct.
 */
#define	heap_entry(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type,member)); \
})

/**
 * heap_entry_safe - get the struct for this entry or null.
 * @ptr: the &struct heap_node pointer.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the heap_node within the struct.
 */
#define heap_entry_safe(ptr, type, member) ({ \
    typeof(ptr) _ptr = (ptr); \
    _ptr ? heap_entry(_ptr, type, member) : NULL; \
})

#ifndef POISON_OFFSET
# define POISON_OFFSET 0
#endif

#define POISON_HPNODE1 ((void *) POISON_OFFSET + 0x10)
#define POISON_HPNODE2 ((void *) POISON_OFFSET + 0x20)
#define POISON_HPNODE3 ((void *) POISON_OFFSET + 0x30)

#if DEBUG_HEAP
extern bool heap_debug_link_check(struct heap_node *parent, struct heap_node **link, struct heap_node *node);
extern bool heap_debug_delete_check(struct heap_node *node);
#endif

typedef long (*heap_cmp_t)(const struct heap_node *nodea, const struct heap_node *nodeb);
extern void heap_fixup(struct heap_root *root, struct heap_node *node, heap_cmp_t cmp);
extern void heap_erase(struct heap_root *root, struct heap_node *node, heap_cmp_t cmp);
extern struct heap_node *heap_remove(struct heap_root *root, struct heap_node *node);
extern struct heap_node **heap_parent(struct heap_root *root, struct heap_node **parentp, struct heap_node *node);
extern struct heap_node *heap_find(struct heap_root *root, unsigned int index);

/* Preorder iteration (Root-first) - always access the left node first */
extern struct heap_node *heap_level_first(const struct heap_root *root, unsigned long *index);
extern struct heap_node *heap_level_next(const struct heap_root *root, unsigned long *index);

/**
 * heap_pre_first_entry - get the preorder first element from a heaptree.
 * @ptr: the heaptree root to take the element from.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the heap_node within the struct.
 */
#define heap_first_entry(root, index, type, member) \
    heap_entry_safe(heap_level_first(root, index), type, member)

/**
 * heap_pre_next_entry - get the preorder next element in heaptree.
 * @pos: the type * to cursor.
 * @member: the name of the heap_node within the struct.
 */
#define heap_next_entry(root, index, type, member) \
    heap_entry_safe(heap_level_next(root, index), type, member)

/**
 * heap_pre_for_each - preorder iterate over a heaptree.
 * @pos: the &struct heap_node to use as a loop cursor.
 * @root: the root for your heaptree.
 */
#define heap_for_each(pos, index, root) \
    for (pos = heap_level_first(root, index); \
         pos; pos = heap_level_next(root, index))

/**
 * heap_pre_for_each_from - preorder iterate over a heaptree from the current point.
 * @pos: the &struct heap_node to use as a loop cursor.
 */
#define heap_for_each_from(pos, index, root) \
    for (; pos; pos = heap_level_next(root, index))

/**
 * heap_pre_for_each_continue - continue preorder iteration over a heaptree.
 * @pos: the &struct heap_node to use as a loop cursor.
 */
#define heap_for_each_continue(pos, index, root) \
    for (pos = heap_level_next(root, index); \
         pos; pos = heap_level_next(root, index))

/**
 * heap_pre_for_each_entry - preorder iterate over heaptree of given type.
 * @pos: the type * to use as a loop cursor.
 * @root: the root for your heaptree.
 * @member: the name of the heap_node within the struct.
 */
#define heap_for_each_entry(pos, index, root, member) \
    for (pos = heap_first_entry(root, index, typeof(*pos), member); \
         pos; pos = heap_next_entry(root, index, typeof(*pos), member))

/**
 * heap_pre_for_each_entry_from - preorder iterate over heaptree of given type from the current point.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the heap_node within the struct.
 */
#define heap_for_each_entry_from(pos, index, root, member) \
    for (; pos; pos = heap_next_entry(root, index, typeof(*pos), member))

/**
 * heap_pre_for_each_entry_continue - continue preorder iteration over heaptree of given type.
 * @pos: the type * to use as a loop cursor.
 * @member: the name of the heap_node within the struct.
 */
#define heap_for_each_entry_continue(pos, index, root, member) \
    for (pos = heap_next_entry(root, index, typeof(*pos), member); \
         pos; pos = heap_next_entry(root, index, typeof(*pos), member))

/**
 * heap_link - link node to parent.
 * @root: heaptree root of node.
 * @parent: point to parent node.
 * @link: point to pointer to child node.
 * @node: new node to link.
 */
static inline void heap_link(struct heap_root *root, struct heap_node *parent,
                             struct heap_node **link, struct heap_node *node)
{
#ifdef DEBUG_HEAP
    if (unlikely(!heap_debug_link_check(parent, link, node)))
        return;
#endif

    /* link = &parent->left/right */
    *link = node;
    node->parent = parent;
    node->left = node->right = NULL;
    root->count++;
}

/**
 * heap_insert_node - link node to parent and fixup heaptree.
 * @root: heaptree root of node.
 * @parent: parent node of node.
 * @link: point to pointer to child node.
 * @node: new node to link.
 */
static inline void heap_insert_node(struct heap_root *root, struct heap_node *parent, struct heap_node **link,
                                    struct heap_node *node, heap_cmp_t cmp)
{
    heap_link(root, parent, link, node);
    heap_fixup(root, node, cmp);
}

/**
 * heap_insert - find the parent node and insert new node.
 * @root: heaptree root of node.
 * @node: new node to insert.
 * @cmp: operator defining the node order.
 */
static inline void heap_insert(struct heap_root *root, struct heap_node *node, heap_cmp_t cmp)
{
    struct heap_node *parent, **link;

    link = heap_parent(root, &parent, node);
    heap_insert_node(root, parent, link, node, cmp);
}

/**
 * heap_delete - delete node and fixup heaptree.
 * @root: heaptree root of node.
 * @node: node to delete.
 */
static inline void heap_delete(struct heap_root *root, struct heap_node *node, heap_cmp_t cmp)
{
    struct heap_node *rebalance;

#ifdef DEBUG_HEAP
    if (unlikely(!heap_debug_delete_check(node)))
        return;
#endif

    if ((rebalance = heap_remove(root, node)))
        heap_erase(root, rebalance, cmp);

    node->left = POISON_HPNODE1;
    node->right = POISON_HPNODE2;
    node->parent = POISON_HPNODE3;
}

#endif  /* _HEAP_H_ */
