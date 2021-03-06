/*
 * Copyright (c) 2018-2019 Zhao Zhixu
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

#ifndef _LN_UTIL_H_
#define _LN_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "tl_tensor.h"

enum ln_bool {
    LN_FALSE = 0,
    LN_TRUE = 1
};
typedef enum ln_bool ln_bool;

typedef int (*ln_cmp_func)(const void *p1, const void *p2);
typedef void (*ln_free_func)(void *p);
typedef void (*ln_fprint_func) (FILE *fp, const void *p);
typedef uint32_t (*ln_hash_func)(const void *p);
typedef void *(*ln_copy_func)(void *dst, const void *src, size_t n);

#define ln_free free

#define LN_MAXLINE 4096

#define LN_TIMEIT_START { double _t1, _t2; _t1 = ln_clock();
#define LN_TIMEIT_END(msg, varg...)                             \
    _t2 = ln_clock(); ln_msg_info(msg"%.6fs", ##varg, _t2 - _t1); }

#ifdef __cplusplus
#define LN_CPPSTART extern "C" {
#define LN_CPPEND }
LN_CPPSTART
#endif

char *ln_sprint_version(char *buf);
void *ln_alloc(size_t size);
void *ln_realloc(void *ptr, size_t size);
char *ln_strdup(const char *s);
char *ln_path_alloc(size_t *sizep);
void *ln_clone(const void *src, size_t size);
void *ln_repeat(void *data, size_t size, int times);
char *ln_read_text(const char *path);
char *ln_read_stdin(void);
char *ln_next_token(const char *s, int c);
char *ln_strcat_alloc(const char *s1, const char *s2);
char *ln_strcat_delim_alloc(const char *s1, const char *s2, char delim);
int ln_streq(const char *s1, const char *s2);
int ln_streqn(const char *s1, const char *s2, size_t n);
int ln_is_prefix_plus_number(const char *str, const char *prefix);
int ln_digit_num(ssize_t num);
int ln_compute_output_dim(int input_dim, int size, int stride, int padding);
int ln_compute_length(int ndim, const int *dims);
int *ln_autopading(int *padding, const int *input_shape, const int *size,
                   const int *stride, int ndim, const char *mode);
void ln_print_shape(int ndim, int *dims);
char *ln_sprint_shape(char *buf, int ndim, int *dims);
uint32_t ln_direct_hash(const void *key);
int ln_direct_cmp(const void *p1, const void *p2);
uint32_t ln_str_hash(const void *key);
int ln_str_cmp(const void *p1, const void *p2);
double ln_clock(void);
void ln_img_submean(const unsigned char *data, const float *mean, float *out,
                    int H, int W, int C);

void ln_err_msg(const char *fmt, ...);
void ln_err_cont(int error, const char *fmt, ...);
void ln_err_ret(const char *fmt, ...);
void ln_err_quit(const char *fmt, ...);
void ln_err_bt(const char *fmt, ...);
void ln_err_exit(int error, const char *fmt, ...);
void ln_err_sys(const char *fmt, ...);
void ln_err_dump(const char *fmt, ...);

#ifdef __cplusplus
LN_CPPEND
#endif

#endif	/* _LN_UTIL_H_ */
