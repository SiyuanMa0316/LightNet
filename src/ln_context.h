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

#ifndef _LN_CONTEXT_H_
#define _LN_CONTEXT_H_

#include "ln_list.h"
#include "ln_hash.h"
#include "ln_mem.h"
#include "ln_tensor.h"
#include "ln_op.h"
#include "ln_dfg.h"

struct ln_context {
    ln_hash *tensor_table;
    ln_hash *op_table;
    ln_dfg  *dfg;
    ln_list *ops;
    ln_hash *mem_pool_table;
};
typedef struct ln_context ln_context;

#ifdef __cplusplus
LN_CPPSTART
#endif

ln_context *ln_context_create(void);
void ln_context_free(ln_context *ctx);
void ln_context_init_ops(ln_context *ctx);
void ln_context_cleanup_ops(ln_context *ctx);
void ln_context_replace_ops(ln_context *ctx, ln_list **start_p, size_t len,
                            ln_list *new_ops);
int ln_context_check(ln_context *ctx);
void ln_context_run(ln_context *ctx);
void ln_context_static_run(ln_context *ctx);

#ifdef __cplusplus
LN_CPPEND
#endif

#endif  /* _LN_CONTEXT_H_ */
