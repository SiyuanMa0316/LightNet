#include "ln_arch.h"
#include "ln_context.h"

extern ln_op  ln_opimpl_conv_dpu;

tatic ln_op *ops_dpu[] {
    &ln_opimpl_conv_dpu,
    NULL
};

void cst_funcs_dpu(const ln_context *ctx);
void ln_context remap_for_dpu(const ln_context *ctx);
void ln_context resort_for_dpu(const ln_context *ctx);

void cst_funcs_dpu(const ln_context *ctx)
{
    remap_for_dpu(ctx);
    resort_for_dpu(ctx);
}

void ln_context remap_for_dpu(const ln_context *ctx)
{
    ln_list *l;
    ln_op *op;
    ln_list ops;
    ln_dfg dfg;
    ln_hash optable;

    ln_op *next_op;
    ln_op *prev_op;
    ln_list ldmr_ops;
    ln_list svmr_ops;
    const char *tname;
    ln_list *ll1;
    ln_list *ll2;
    ln_op ldmr_op;
    ln_op svmr_op;
    ln_op link1_op;
    ln_op link2_op;
    size_t i;

    ops = ctx->ops;
    dfg = ctx->dfg;
    op_table = ctx->op_table;
    
    for (i = 0, l = (ln_list *)ops; i < l;) {
        op = l->data;


        if (!ln_streq(op->op_arg->optype, "concat")){
             i++, l = l->next;
             continue;
        }//不是concat，找下一个op

        if (!l->next->data) 
        return;//找到最后一个concat了，不做remap，返回

        next_op = ln_dfg_next(*dfg, *op, *tname); //下一个op
        if (!ln_streq(next_op->op_arg->optype, "slice")){
             i++, l = l->next;
             continue;
        }//不是slice，不做remap，找下一个op //这里需不需要加更多判定条件如ldmr？
        //是slice：
        //op1 -> svmr -> concat ->slice ->ldmr -> op2

        ldmr_ops = ln_dfg_nexts(*dfg, *next_op, *tname); //ldmr的oplist
        svmr_ops = ln_dfg_prevs(*dfg, *op, *tname); //svmr的oplist

        for (i = 0, ll1 = (ln_list *)svmr_ops; ll2 = (ln_list *)ldmr_ops; i < l;){
            svmr_op = ll1->data;
            link1_op = ln_dfg_prev(*dfg, *svmr_op, *tname);

            ldmr_op = ll2->data;
            link2_op = ln_dfg_next(*dfg, *ldmr_op, *tname);

            ln_dfg_link(*dfg, *link1_op, *link2_op, *tname)//what's tname?
            ln_dfg_remove(*dfg, *svmr_op); //remove svmr
            ln_dfg_remove(*dfg, *ldmr_op); //remove ldmr
            
            ll1 = ll1->next;
            ll2 = ll2->next;
        } //逐个链接，删除ldmr,svmr
        ln_dfg_remove(*dfg, *op); //remove concat
        ln_dfg_remove(*dfg, *next_op);//remove slice
        
    }
}

ln_arch ln_archimpl_dpu = {
    .init_func = init_dpu,
    .cleanup_func = cleanup_dpu,
    .reg_ops = ops_dpu,
    .ep_funcs = ep_funcs_dpu,
    .cb_funcs = cb_funcs_dpu,
    .cst_func = cst_funcs_dpu,
    .arch_name = "dpu",
};