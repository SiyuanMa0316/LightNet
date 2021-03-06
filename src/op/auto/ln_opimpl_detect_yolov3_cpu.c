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

#include <assert.h>
#include "ln_op.h"
#include "ln_arch.h"

struct priv_s {
    ln_tensor_entry *feature_entry;
    ln_tensor_entry *anchors_entry;
    ln_tensor_entry *box_centers_entry;
    ln_tensor_entry *box_sizes_entry;
    ln_tensor_entry *boxes_entry;
    ln_tensor_entry *confs_entry;
    ln_tensor_entry *probs_entry;
    ln_param_entry  *img_width_entry;
    ln_param_entry  *img_height_entry;
};

/* This function should do the parameter checking and tensor shape inference. */
static void detect_yolov3_cpu_pre_run(ln_op_arg *op_arg)
{
    char                 *feature_name;
    ln_tensor_list_entry *feature_list_entry;
    ln_tensor_entry      *feature_entry;
    tl_tensor            *feature;
    char                 *anchors_name;
    ln_tensor_list_entry *anchors_list_entry;
    ln_tensor_entry      *anchors_entry;
    tl_tensor            *anchors;
    char                 *box_centers_name;
    ln_tensor_list_entry *box_centers_list_entry;
    ln_tensor_entry      *box_centers_entry;
    tl_tensor            *box_centers;
    int                   box_centers_ndim;
    int                  *box_centers_dims;
    tl_dtype              box_centers_dtype;
    char                 *box_sizes_name;
    ln_tensor_list_entry *box_sizes_list_entry;
    ln_tensor_entry      *box_sizes_entry;
    tl_tensor            *box_sizes;
    int                   box_sizes_ndim;
    int                  *box_sizes_dims;
    tl_dtype              box_sizes_dtype;
    char                 *boxes_name;
    ln_tensor_list_entry *boxes_list_entry;
    ln_tensor_entry      *boxes_entry;
    tl_tensor            *boxes;
    int                   boxes_ndim;
    int                  *boxes_dims;
    tl_dtype              boxes_dtype;
    char                 *confs_name;
    ln_tensor_list_entry *confs_list_entry;
    ln_tensor_entry      *confs_entry;
    tl_tensor            *confs;
    int                   confs_ndim;
    int                  *confs_dims;
    tl_dtype              confs_dtype;
    char                 *probs_name;
    ln_tensor_list_entry *probs_list_entry;
    ln_tensor_entry      *probs_entry;
    tl_tensor            *probs;
    int                   probs_ndim;
    int                  *probs_dims;
    tl_dtype              probs_dtype;
    int                   img_width;
    ln_param_entry       *img_width_entry;
    int                   img_height;
    ln_param_entry       *img_height_entry;
    int                   tensors_in_n;
    int                   tensors_out_n;
    int                   params_n;
    struct priv_s        *priv;

    /* check tensors and parameters */
    tensors_in_n = ln_tensor_list_length(op_arg->tensors_in);
    ln_opck_tensors_in_len_eq(tensors_in_n, 2);

    feature_list_entry = ln_tensor_list_find_by_arg_name(op_arg->tensors_in, "feature");
    ln_opck_tensor_in_exist(feature_list_entry, "feature");
    feature_name = feature_list_entry->name;
    feature_entry = ln_tensor_table_find(op_arg->tensor_table, feature_name);
    ln_opck_tensor_defined(feature_entry, feature_name);
    feature = feature_entry->tensor;
    feature = feature;
    ln_opck_tensor_mtype_eq(feature_entry, LN_MEM_CPU);
    ln_opck_tensor_dtype_eq(feature_entry, TL_FLOAT);
    ln_opck_tensor_ndim(feature_entry, 4);
    ln_opck_satisfy(feature->dims[0] == 1);

    anchors_list_entry = ln_tensor_list_find_by_arg_name(op_arg->tensors_in, "anchors");
    ln_opck_tensor_in_exist(anchors_list_entry, "anchors");
    anchors_name = anchors_list_entry->name;
    anchors_entry = ln_tensor_table_find(op_arg->tensor_table, anchors_name);
    ln_opck_tensor_defined(anchors_entry, anchors_name);
    anchors = anchors_entry->tensor;
    anchors = anchors;
    ln_opck_tensor_mtype_eq(anchors_entry, LN_MEM_CPU);
    ln_opck_tensor_ndim(anchors_entry, 2);
    ln_opck_tensor_issametype(anchors_entry, feature_entry);

    tensors_out_n = ln_tensor_list_length(op_arg->tensors_out);
    ln_opck_tensors_out_len_eq(tensors_out_n, 5);

    box_centers_list_entry = ln_tensor_list_find_by_arg_name(op_arg->tensors_out, "box_centers");
    ln_opck_tensor_out_exist(box_centers_list_entry, "box_centers");
    box_centers_name = box_centers_list_entry->name;
    box_centers_entry = ln_tensor_table_find(op_arg->tensor_table, box_centers_name);
    ln_opck_tensor_not_defined(box_centers_entry, box_centers_name);

    box_sizes_list_entry = ln_tensor_list_find_by_arg_name(op_arg->tensors_out, "box_sizes");
    ln_opck_tensor_out_exist(box_sizes_list_entry, "box_sizes");
    box_sizes_name = box_sizes_list_entry->name;
    box_sizes_entry = ln_tensor_table_find(op_arg->tensor_table, box_sizes_name);
    ln_opck_tensor_not_defined(box_sizes_entry, box_sizes_name);

    boxes_list_entry = ln_tensor_list_find_by_arg_name(op_arg->tensors_out, "boxes");
    ln_opck_tensor_out_exist(boxes_list_entry, "boxes");
    boxes_name = boxes_list_entry->name;
    boxes_entry = ln_tensor_table_find(op_arg->tensor_table, boxes_name);
    ln_opck_tensor_not_defined(boxes_entry, boxes_name);

    confs_list_entry = ln_tensor_list_find_by_arg_name(op_arg->tensors_out, "confs");
    ln_opck_tensor_out_exist(confs_list_entry, "confs");
    confs_name = confs_list_entry->name;
    confs_entry = ln_tensor_table_find(op_arg->tensor_table, confs_name);
    ln_opck_tensor_not_defined(confs_entry, confs_name);

    probs_list_entry = ln_tensor_list_find_by_arg_name(op_arg->tensors_out, "probs");
    ln_opck_tensor_out_exist(probs_list_entry, "probs");
    probs_name = probs_list_entry->name;
    probs_entry = ln_tensor_table_find(op_arg->tensor_table, probs_name);
    ln_opck_tensor_not_defined(probs_entry, probs_name);

    params_n = ln_param_list_length(op_arg->params);
    ln_opck_params_len_eq(params_n, 2);

    img_width_entry = ln_param_list_find(op_arg->params, "img_width");
    ln_opck_param_exist(img_width_entry, "img_width");
    ln_opck_param_type(img_width_entry, LN_PARAM_NUMBER);
    img_width = img_width_entry->value_int;
    ln_opck_param_int_gt(img_width_entry, 0);
    img_width = img_width;

    img_height_entry = ln_param_list_find(op_arg->params, "img_height");
    ln_opck_param_exist(img_height_entry, "img_height");
    ln_opck_param_type(img_height_entry, LN_PARAM_NUMBER);
    img_height = img_height_entry->value_int;
    ln_opck_param_int_gt(img_height_entry, 0);
    img_height = img_height;

    /* define output tensor shape, tensor data should be NULL */
    box_centers_ndim = 5;
    box_centers_dtype = feature->dtype;
    {
        int dims[] = {1, anchors->dims[0], 2, feature->dims[2], feature->dims[3]};
        box_centers_dims = ln_clone(dims, sizeof(int)*5);
    }
    box_centers = tl_tensor_create(NULL, box_centers_ndim, box_centers_dims, box_centers_dtype);
    box_centers_entry = ln_tensor_entry_create(box_centers_name, box_centers);
    box_centers_entry->offset = box_centers_list_entry->offset;
    ln_tensor_entry_set_creater(box_centers_entry, op_arg->name);
    box_centers_entry->mtype = LN_MEM_CPU;
    ln_tensor_table_insert(op_arg->tensor_table, box_centers_entry);
    {
        ln_free(box_centers_dims);
    }

    box_sizes_ndim = 5;
    box_sizes_dtype = feature->dtype;
    {
        int dims[] = {1, anchors->dims[0], 2, feature->dims[2], feature->dims[3]};
        box_sizes_dims = ln_clone(dims, sizeof(int)*5);
    }
    box_sizes = tl_tensor_create(NULL, box_sizes_ndim, box_sizes_dims, box_sizes_dtype);
    box_sizes_entry = ln_tensor_entry_create(box_sizes_name, box_sizes);
    box_sizes_entry->offset = box_sizes_list_entry->offset;
    ln_tensor_entry_set_creater(box_sizes_entry, op_arg->name);
    box_sizes_entry->mtype = LN_MEM_CPU;
    ln_tensor_table_insert(op_arg->tensor_table, box_sizes_entry);
    {
        ln_free(box_sizes_dims);
    }

    boxes_ndim = 5;
    boxes_dtype = feature->dtype;
    {
        int dims[] = {1, anchors->dims[0], 4, feature->dims[2], feature->dims[3]};
        boxes_dims = ln_clone(dims, sizeof(int)*5);
    }
    boxes = tl_tensor_create(NULL, boxes_ndim, boxes_dims, boxes_dtype);
    boxes_entry = ln_tensor_entry_create(boxes_name, boxes);
    boxes_entry->offset = boxes_list_entry->offset;
    ln_tensor_entry_set_creater(boxes_entry, op_arg->name);
    boxes_entry->mtype = LN_MEM_CPU;
    ln_tensor_table_insert(op_arg->tensor_table, boxes_entry);
    {
        ln_free(boxes_dims);
    }

    confs_ndim = 5;
    confs_dtype = feature->dtype;
    {
        int dims[] = {1, anchors->dims[0], 1, feature->dims[2], feature->dims[3]};
        confs_dims = ln_clone(dims, sizeof(int)*5);
    }
    confs = tl_tensor_create(NULL, confs_ndim, confs_dims, confs_dtype);
    confs_entry = ln_tensor_entry_create(confs_name, confs);
    confs_entry->offset = confs_list_entry->offset;
    ln_tensor_entry_set_creater(confs_entry, op_arg->name);
    confs_entry->mtype = LN_MEM_CPU;
    ln_tensor_table_insert(op_arg->tensor_table, confs_entry);
    {
        ln_free(confs_dims);
    }

    probs_ndim = 5;
    probs_dtype = feature->dtype;
    {
        int dims[] = {1, anchors->dims[0], feature->dims[1]/anchors->dims[0]-5,
                     feature->dims[2], feature->dims[3]};
        probs_dims = ln_clone(dims, sizeof(int)*5);
    }
    probs = tl_tensor_create(NULL, probs_ndim, probs_dims, probs_dtype);
    probs_entry = ln_tensor_entry_create(probs_name, probs);
    probs_entry->offset = probs_list_entry->offset;
    ln_tensor_entry_set_creater(probs_entry, op_arg->name);
    probs_entry->mtype = LN_MEM_CPU;
    ln_tensor_table_insert(op_arg->tensor_table, probs_entry);
    {
        ln_free(probs_dims);
    }

    /* use op_arg->priv to store private data to be used in other functions */
    priv = ln_alloc(sizeof(struct priv_s));
    priv->feature_entry = feature_entry;
    priv->anchors_entry = anchors_entry;
    priv->box_centers_entry = box_centers_entry;
    priv->box_sizes_entry = box_sizes_entry;
    priv->boxes_entry = boxes_entry;
    priv->confs_entry = confs_entry;
    priv->probs_entry = probs_entry;
    priv->img_width_entry = img_width_entry;
    priv->img_height_entry = img_height_entry;
    op_arg->priv = priv;
}

/* This function should only do the calculations. */
static void detect_yolov3_cpu_run(ln_op_arg *op_arg)
{
    struct priv_s *priv = op_arg->priv;

    {
    }
}

/* This function should free all the memory allocated by other *_run()s. */
static void detect_yolov3_cpu_post_run(ln_op_arg *op_arg)
{
    struct priv_s *priv = op_arg->priv;

    ln_tensor_table_remove(op_arg->tensor_table, priv->box_centers_entry->name);
    ln_tensor_table_remove(op_arg->tensor_table, priv->box_sizes_entry->name);
    ln_tensor_table_remove(op_arg->tensor_table, priv->boxes_entry->name);
    ln_tensor_table_remove(op_arg->tensor_table, priv->confs_entry->name);
    ln_tensor_table_remove(op_arg->tensor_table, priv->probs_entry->name);
    ln_free(priv);
}

static const char *in_arg_names[] = {
    "feature",
    "anchors",
    NULL
};

static const char *out_arg_names[] = {
    "box_centers",
    "box_sizes",
    "boxes",
    "confs",
    "probs",
    NULL
};

static const char *param_arg_names[] = {
    "img_width",
    "img_height",
    NULL
};

static const ln_param_type param_ptypes[] = {
    LN_PARAM_NUMBER,
    LN_PARAM_NUMBER,
};

/* specify other ln_op_arg fields */
static ln_op_arg op_arg_detect_yolov3_cpu = {
    .optype = "detect_yolov3_cpu",
    .arch = "cpu",
    .in_arg_names = in_arg_names,
    .out_arg_names = out_arg_names,
    .param_arg_names = param_arg_names,
    .param_ptypes = param_ptypes,
};

/* struct used for op registration in ln_oplist.c */
ln_op ln_opimpl_detect_yolov3_cpu = {
    .op_arg = &op_arg_detect_yolov3_cpu,
    .pre_run = detect_yolov3_cpu_pre_run,
    .static_run = NULL,
    .run = detect_yolov3_cpu_run,
    .post_run = detect_yolov3_cpu_post_run
};
