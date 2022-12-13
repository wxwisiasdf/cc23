#ifndef BACKEND_H
#define BACKEND_H 1

#include "ast.h"
#include "context.h"
#include <stddef.h>

enum cc_backend_varmap_flags {
    VARMAP_REGISTER = 0x01,
    VARMAP_CONSTANT = 0x02,
    VARMAP_STACK = 0x04,
    VARMAP_STATIC = 0x08,
    VARMAP_THREAD_LOCAL = 0x10,
    VARMAP_LITERAL = 0x20,
};

typedef struct cc_backend_varmap {
    const cc_ast_variable* restrict var;
    enum cc_backend_varmap_flags flags;
    unsigned int regno; /* Register number */
    unsigned int offset; /* Stack offset (if stack based) */
    cc_ast_literal literal;
    char* data;
    size_t n_data;
    unsigned int depth; /* Depth at which the varmap was made */
} cc_backend_varmap;

typedef struct cc_backend_reginfo {
    bool used;
    bool spilled;
} cc_backend_reginfo;

typedef struct cc_backend_context {
    const cc_ast_node* outermost_stmt; /* For scheduling ++/--, &&, ||, etc */
    const cc_ast_node* if_outermost_stmt; /* If without a tail else will
                                             require scheduling too */
    cc_backend_varmap* varmaps;
    size_t n_varmaps;
    unsigned int varmap_depth; /* Depth for moving/removing varmaps */
    const char** reg_names;
    cc_backend_reginfo* regs;
    size_t n_regs;
    unsigned int stack_frame_size; /* Size of the frame for the current
                                      function */
    unsigned int min_stack_alignment; /* Minimum alignment for stack */
    bool (*is_reserved)(unsigned int regno);
    bool (*gen_mov)(cc_context* ctx, const cc_backend_varmap* lvmap,
        const cc_backend_varmap* rvmap);
    bool (*gen_prologue)(
        cc_context* ctx, const cc_ast_node* node, const cc_ast_variable* var);
    bool (*gen_epilogue)(
        cc_context* ctx, const cc_ast_node* node, const cc_ast_variable* var);
    bool (*gen_call)(cc_context* ctx, const cc_ast_node* node);
    unsigned int (*get_sizeof)(cc_context* ctx, const cc_ast_type* type);
    cc_backend_varmap (*get_call_retval)(
        cc_context* ctx, const cc_ast_node* node);
    bool (*gen_jump)(cc_context* ctx, const cc_ast_node* node);
    bool (*gen_binop)(cc_context* ctx, const cc_backend_varmap* lvmap,
        const cc_backend_varmap* rvmap, enum cc_ast_binop_type type,
        const cc_backend_varmap *ovmap);
    bool (*gen_unop)(cc_context* ctx, const cc_backend_varmap* lvmap,
        const cc_backend_varmap* rvmap, enum cc_ast_unop_type type);
    bool (*map_variable)(cc_context* ctx, const cc_ast_variable* var);
    bool (*gen_branch)(cc_context* ctx, const cc_ast_node* node,
        const cc_backend_varmap* lvmap, const cc_backend_varmap* rvmap,
        enum cc_ast_binop_type type);
    void (*deinit)(cc_context* ctx);
    const cc_ast_variable* current_func_var;
} cc_backend_context;

unsigned short cc_ast_alloc_label_id(cc_context* ctx);
void cc_backend_spill_reg(cc_context* ctx, unsigned int regno);
void cc_backend_unspill_reg(cc_context* ctx, unsigned int regno);
void cc_backend_spill(cc_context* ctx, unsigned int num);
void cc_backend_unspill(cc_context* ctx);
void cc_backend_reserve_reg(cc_context* ctx, unsigned int regno);
unsigned int cc_backend_alloc_register(cc_context* ctx);
void cc_backend_free_register(cc_context* ctx, int regno);
void cc_backend_add_varmap(
    cc_context* ctx, const cc_ast_variable* restrict var);
cc_backend_varmap* cc_backend_find_var_varmap(
    cc_context* ctx, const cc_ast_variable* restrict var);
cc_backend_varmap cc_backend_get_node_varmap(
    cc_context* ctx, const cc_ast_node* node);
void cc_backend_map_variables(cc_context* ctx, const cc_ast_node* node);
void cc_backend_process_binop(
    cc_context* ctx, const cc_ast_node* node, const cc_backend_varmap* ovmap);
void cc_backend_process_node(
    cc_context* ctx, const cc_ast_node* node, const cc_backend_varmap* ovmap);
void cc_backend_init(
    cc_context* ctx, const char* reg_names[], unsigned int n_regs);
void cc_backend_deinit(cc_context* ctx);

#define PATMAT_NODE_BASE(t)                                                    \
    .type = (t), .parent = NULL, .info = { 0 }, .label_id = 0, .ref_count = 0

#define PATMAT_NODE_BINOP(t, l, r, b)                                          \
    &((cc_ast_node) { PATMAT_NODE_BASE(AST_NODE_BINOP),                        \
        .data.binop = { .op = (t), .left = (l), .right = (r), .bits = (b) } })

#define PATMAT_NODE_UNOP(t, c, b)                                              \
    &((cc_ast_node) { PATMAT_NODE_BASE(AST_NODE_UNOP),                         \
        .data.unop = { .op = (t), .child = (c), .bits = (b) } })

#define PATMAT_NODE_VARIABLE()                                                 \
    &((cc_ast_node) { PATMAT_NODE_BASE(AST_NODE_VARIABLE), .data.var = { 0 } })

#define PATMAT_NODE_REGISTER(r)                                                \
    &((cc_ast_node) {                                                          \
        PATMAT_NODE_BASE(AST_NODE_REGISTER), .data.reg_group = (r) })

#define PATMAT_NODE_CALL(e)                                                    \
    &((cc_ast_node) { PATMAT_NODE_BASE(AST_NODE_CALL),                         \
        .data.call = { .call_expr = (e), .params = NULL, .n_params = 0 } })

#define PATMAT_

typedef struct cc_backend_pattern {
    const char* insn_fmt;
    cc_ast_node* node;
} cc_backend_pattern;

#endif
