#include "ln_arch.h"

extern ln_op  ln_opimpl_conv_dpu;

tatic ln_op *ops_dpu[] {
    &ln_opimpl_conv_dpu,
    NULL
};

ln_arch ln_archimpl_dpu = {
    .init_func = init_dpu,
    .cleanup_func = cleanup_dpu,
    .reg_ops = ops_dpu,
    .ep_funcs = ep_funcs_dpu,
    .cb_funcs = cb_funcs_dpu,
    .arch_name = "dpu",
};