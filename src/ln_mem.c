/*
 * Copyright (c) 2018 Zhao Zhixu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>
#include "ln_mem.h"
#include "ln_error.h"

typedef enum mem_flag mem_flag;
enum mem_flag {
    HOLE,
    SYMBOL
};

typedef struct mem_info mem_info;
struct mem_info {
    mem_flag flag;
    size_t   start;
    size_t   size;
};

static const char *mtype_name[] = {
    "LN_MEM_NONE",
    "LN_MEM_DIFF",
    "LN_MEM_CPU",
    "LN_MEM_CUDA"
};

#define ln_check_mem_type(mtype)                        \
    assert(mtype >= 0 && mtype < LN_MEM_TYPE_SIZE)

const char *ln_mem_type_name(ln_mem_type mtype)
{
    ln_check_mem_type(mtype);
    return mtype_name[mtype];
}

static mem_info *mem_info_create(mem_flag flag, size_t start, size_t size)
{
    mem_info *minfo;
    minfo = ln_alloc(sizeof(mem_info));
    minfo->flag = flag;
    minfo->start = start;
    minfo->size = size;

    return minfo;
}

static void mem_info_free(mem_info *minfo)
{
    ln_free(minfo);
}

ln_mem_pool *ln_mem_pool_create(size_t size, size_t align_size)
{
    ln_mem_pool *mem_pool;
    mem_info *minfo;

    assert(size > 0 && align_size > 0);
    mem_pool = ln_alloc(sizeof(ln_mem_pool));
    mem_pool->size = size;
    mem_pool->align_size = align_size;
    minfo = mem_info_create(HOLE, 1, size);
    mem_pool->mem_blocks = ln_list_append(NULL, minfo);

    return mem_pool;
}

static void mem_info_free_wrapper(void *minfo)
{
    mem_info_free((mem_info *)minfo);
}

void ln_mem_pool_free(ln_mem_pool *mem_pool)
{
    ln_list_free_deep(mem_pool->mem_blocks, mem_info_free_wrapper);
    ln_free(mem_pool);
}

static int best_fit(ln_mem_pool *mem_pool, size_t size)
{
    int first_hole = 0;
    size_t min_size;
    size_t align_size = mem_pool->align_size;
    int min_idx;
    ln_list *l;
    int i;

    for (l = mem_pool->mem_blocks, i = 0; l; l = l->next, i++) {
        mem_info *minfo = l->data;
        size_t align_start = minfo->start % align_size == 0 ? minfo->start :
            align_size - minfo->start % align_size + minfo->start;
        size_t mem_end = minfo->start + minfo->size - 1;
        if (minfo->flag != HOLE || align_start + size - 1 > mem_end)
            continue;
        size_t alignable_size = mem_end - align_start + 1;
        if (!first_hole) {
            first_hole = 1;
            min_size = alignable_size;
            min_idx = i;
            continue;
        }
        if (alignable_size < min_size) {
            min_size = alignable_size;
            min_idx = i;
        }
    }
    if (!first_hole)
        return -1;
    return min_idx;
}

size_t ln_mem_alloc(ln_mem_pool *mem_pool, size_t size)
{
    assert(size > 0);
    int fit_idx = best_fit(mem_pool, size);
    if (fit_idx < 0) {
        ln_error *error = ln_error_create(LN_ERROR,
                                          "ln_mem_alloc(): out of virtual memory pool when allocating %ld bytes", size);
        ln_error_handle(&error);
    }

    mem_info *minfo = ln_list_nth_data(mem_pool->mem_blocks, fit_idx);
    size_t align_size = mem_pool->align_size;
    size_t align_start = minfo->start % align_size == 0 ? minfo->start :
        align_size - minfo->start % align_size + minfo->start;
    size_t mem_size = size + align_start - minfo->start;
    mem_info *new_minfo = mem_info_create(SYMBOL, minfo->start, mem_size);
    mem_pool->mem_blocks = ln_list_insert_nth(mem_pool->mem_blocks,
                                              new_minfo, fit_idx);
    minfo->start += mem_size;
    minfo->size -= mem_size;
    if (minfo->size == 0)
        mem_pool->mem_blocks = ln_list_remove_nth_deep(mem_pool->mem_blocks,
                                                       fit_idx + 1,
                                                       mem_info_free_wrapper);

    size_t hole_size = align_start - new_minfo->start;
    if (hole_size == 0)
        return align_start;
    mem_info *hole_minfo = mem_info_create(HOLE, new_minfo->start, hole_size);
    mem_pool->mem_blocks = ln_list_insert_nth(mem_pool->mem_blocks,
                                              hole_minfo, fit_idx);
    new_minfo->start = align_start;
    new_minfo->size -= hole_size;
    return align_start;
}

void ln_mem_free(ln_mem_pool *mem_pool, size_t addr)
{
    int i;
    ln_list *l, *l_next, *l_before = NULL;
    mem_info *minfo;
    for (l = mem_pool->mem_blocks, i = 0; l; l_before = l, l = l->next, i++) {
        minfo = l->data;
        if (minfo->flag != SYMBOL || minfo->start != addr)
            continue;
        l_next = l->next;
        if (l_next && ((mem_info *)l_next->data)->flag == HOLE) {
            minfo->size += ((mem_info *)l_next->data)->size;
            mem_pool->mem_blocks = ln_list_remove_nth_deep(mem_pool->mem_blocks,
                                                           i + 1,
                                                           mem_info_free_wrapper);
        }
        if (l_before && ((mem_info *)l_before->data)->flag == HOLE) {
            minfo->start -= ((mem_info *)l_before->data)->size;
            minfo->size += ((mem_info *)l_before->data)->size;
            mem_pool->mem_blocks = ln_list_remove_nth_deep(mem_pool->mem_blocks,
                                                           i - 1,
                                                           mem_info_free_wrapper);
        }
        minfo->flag = HOLE;
        break;
    }
    if (!l) {
        ln_error *error = ln_error_create(LN_ERROR,
                                          "ln_mem_free(): invalid address: 0x%012lx",
                                          addr);
        ln_error_handle(&error);
    }
}

int ln_mem_exist(ln_mem_pool *mem_pool, size_t addr)
{
    mem_info *minfo;

    LN_LIST_FOREACH(minfo, mem_pool->mem_blocks) {
        if (minfo->flag == SYMBOL && minfo->start == addr)
            return 1;
    }
    return 0;
}

void ln_mem_dump(ln_mem_pool *mem_pool, FILE *fp)
{
    mem_info *minfo;

    fprintf(fp, "======= Lightnet Memory map: =======\n");
    LN_LIST_FOREACH(minfo, mem_pool->mem_blocks) {
        fprintf(fp, "0x%012lx-0x%012lx %s\n", minfo->start,
                minfo->start+minfo->size-1, minfo->flag==HOLE?"H":"S");
    }
}

static void mem_pool_free_wrapper(void *p)
{
    ln_mem_pool_free(p);
}

/* TODO: custom size and align size */
ln_hash *ln_mem_pool_table_create(void)
{
    ln_hash *mpt;
    ln_mem_pool *mp;
    int i;

    mpt = ln_hash_create(ln_direct_hash, ln_direct_cmp, NULL,
                         mem_pool_free_wrapper);
    for (i = LN_MEM_NONE; i < LN_MEM_TYPE_SIZE; i++) {
        mp = ln_mem_pool_create(1073741824, 1);
        ln_hash_insert(mpt, (void *)(size_t)i, mp);
    }
    return mpt;
}

void ln_mem_pool_table_free(ln_hash *mpt)
{
    ln_hash_free(mpt);
}

static inline void use_count_zero(ln_hash *use_counts, char *name)
{
    ln_hash_insert(use_counts, name, (void *)0);
}

static inline ssize_t use_count_inc(ln_hash *use_counts, char *name)
{
    int found;
    ssize_t uc;

    found = ln_hash_find_extended(use_counts, name, NULL, (void **)&uc);
    assert(found);
    ln_hash_insert(use_counts, name, (void *)(++uc));
    return uc;
}

static inline ssize_t use_count_dec(ln_hash *use_counts, char *name)
{
    int found;
    ssize_t uc;

    found = ln_hash_find_extended(use_counts, name, NULL, (void **)&uc);
    assert(found);
    ln_hash_insert(use_counts, name, (void *)(--uc));
    assert(uc >= 0);
    return uc;
}

static inline ssize_t use_count_of(ln_hash *use_counts, char *name)
{
    int found;
    ssize_t uc;

    found = ln_hash_find_extended(use_counts, name, NULL, (void **)&uc);
    assert(found);
    return uc;
}

ln_list *ln_pass_mem(ln_list *ops, ln_hash *mem_pools)
{
    ln_op *op;
    ln_op_arg *arg;
    ln_hash *use_counts;
    ln_tensor_entry *te;
    ln_tensor_list_entry *tle;
    ln_mem_pool *mp;
    ln_list *unused_tles;

    use_counts = ln_hash_create(ln_str_hash, ln_str_cmp, NULL, NULL);
    LN_LIST_FOREACH(op, ops) {
        arg = op->op_arg;
        LN_LIST_FOREACH(tle, arg->tensors_out) {
            te = ln_tensor_table_find(arg->tensor_table, tle->name);
            mp = ln_hash_find(mem_pools, (void *)te->mtype);
            if (te->owner)
                continue;
            if (te->isstatic) {
                te->offset = ln_mem_alloc(mp, tl_tensor_size(te->tensor));
                continue;
            }
            if (ln_hash_find_extended(use_counts, te->name, NULL, NULL))
                use_count_inc(use_counts, te->name);
            else
                use_count_zero(use_counts, te->name);
        }
        LN_LIST_FOREACH(tle, arg->tensors_in) {
            te = ln_tensor_table_find(arg->tensor_table, tle->name);
            if (te->owner) {
                use_count_inc(use_counts, te->owner);
                continue;
            }
            if (te->isstatic)
                continue;
            use_count_inc(use_counts, te->name);
        }
    }

    LN_LIST_FOREACH(op, ops) {
        arg = op->op_arg;
        unused_tles = NULL;
        LN_LIST_FOREACH(tle, arg->tensors_out) {
            te = ln_tensor_table_find(arg->tensor_table, tle->name);
            mp = ln_hash_find(mem_pools, (void *)te->mtype);
            if (te->owner)
                continue;
            if (te->isstatic)
                continue;
            if (ln_mem_exist(mp, te->offset)) {
                use_count_dec(use_counts, te->name);
            }
            else {
                te->offset = ln_mem_alloc(mp, tl_tensor_size(te->tensor));
            }
            if (use_count_of(use_counts, te->name) == 0)
                unused_tles = ln_list_prepend(unused_tles, tle);
        }
        LN_LIST_FOREACH(tle, unused_tles) {
            te = ln_tensor_table_find(arg->tensor_table, tle->name);
            mp = ln_hash_find(mem_pools, (void *)te->mtype);
            ln_mem_free(mp, te->offset);
        }
        ln_list_free(unused_tles);
        LN_LIST_FOREACH(tle, arg->tensors_in) {
            te = ln_tensor_table_find(arg->tensor_table, tle->name);
            mp = ln_hash_find(mem_pools, (void *)te->mtype);
            if (te->owner) {
                if (use_count_dec(use_counts, te->owner) == 0) {
                    te = ln_tensor_table_find(arg->tensor_table, te->owner);
                    ln_mem_free(mp, te->offset);
                }
                continue;
            }
            if (te->isstatic)
                continue;
            if (use_count_dec(use_counts, te->name) == 0) {
                ln_mem_free(mp, te->offset);
            }
        }
    }

    ln_hash_free(use_counts);
    return ops;
}
