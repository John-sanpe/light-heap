/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include "heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h>

#define HEAP_DEBUG  0
#define TEST_LEN    1000000

struct bench_node {
    struct heap_node node;
    unsigned int num;
    unsigned int data;
};

#define heap_to_bench(ptr) \
    heap_entry_safe(ptr, struct bench_node, node)

#if HEAP_DEBUG
static void node_dump(struct bench_node *bnode)
{
    printf("  %04d: ", bnode->num);
    printf("parent %-4d ", bnode->node.parent ? heap_to_bench(bnode->node.parent)->num : 0);
    printf("left %-4d ", bnode->node.left ? heap_to_bench(bnode->node.left)->num : 0);
    printf("right %-4d ", bnode->node.right ? heap_to_bench(bnode->node.right)->num : 0);
    printf("data 0x%8x ", bnode->data);
    printf("\n");
}
#else
# define node_dump(node) ((void)(node))
#endif

static HEAP_ROOT(bench_root);

static void time_dump(int ticks, clock_t start, clock_t stop, struct tms *start_tms, struct tms *stop_tms)
{
    printf("  real time: %lf\n", (stop - start) / (double)ticks);
    printf("  user time: %lf\n", (stop_tms->tms_utime - start_tms->tms_utime) / (double)ticks);
    printf("  kern time: %lf\n", (stop_tms->tms_stime - start_tms->tms_stime) / (double)ticks);
}

static unsigned int test_deepth(struct heap_node *node)
{
    unsigned int left_deepth, right_deepth;

    if (!node)
        return 0;

    left_deepth = test_deepth(node->left);
    right_deepth = test_deepth(node->right);
    return left_deepth > right_deepth ? (left_deepth + 1) : (right_deepth + 1);
}

static long bench_cmp(const struct heap_node *hpa, const struct heap_node *hpb)
{
    struct bench_node *nodea = heap_to_bench(hpa);
    struct bench_node *nodeb = heap_to_bench(hpb);
    return nodea->data < nodeb->data ? -1 : 1;
}

int main(void)
{
    struct bench_node *bnode;
    struct tms start_tms, stop_tms;
    clock_t start, stop;
    unsigned int count, ticks;
    unsigned long index;
    int ret = 0;

    ticks = sysconf(_SC_CLK_TCK);

    printf("Generate %u bnode:\n", TEST_LEN);
    start = times(&start_tms);
    for (count = 0; count < TEST_LEN; ++count) {
        bnode = malloc(sizeof(*bnode));
        if ((ret = !bnode)) {
            printf("Insufficient Memory!\n");
            goto error;
        }

        bnode->num = count + 1;
        bnode->data = rand();

#if HEAP_DEBUG
        printf("  %08d: 0x%8x\n", bnode->num, bnode->data);
#endif

        heap_insert(&bench_root, &bnode->node, bench_cmp);
    }
    stop = times(&stop_tms);
    time_dump(ticks, start, stop, &start_tms, &stop_tms);

    count = test_deepth(bench_root.node);
    printf("  heap deepth: %u\n", count);

    start = times(&start_tms);
    count = 0;
    printf("Levelorder Iteration:\n");
    heap_for_each_entry(bnode, &index, &bench_root, node) {
        node_dump(bnode);
        count++;
    }
    stop = times(&stop_tms);
    printf("  total num: %u\n", count);
    time_dump(ticks, start, stop, &start_tms, &stop_tms);

    printf("Deletion All bnode...\n");
error:
    while (bench_root.count) {
        bnode = heap_to_bench(bench_root.node);
        node_dump(bnode);
        heap_delete(&bench_root, &bnode->node, bench_cmp);
    }

    return ret;
}
