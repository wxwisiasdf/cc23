/* mf370.c - Assembly code generation for i370 machines */
#include "mf370.h"
#include "ast.h"
#include "backend.h"
#include "context.h"
#include "diag.h"
#include "parser.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum cc_mf370_reg {
    MF370_NONE = -1,
    MF370_R0 = 0,
    MF370_R1,
    MF370_R2,
    MF370_R3,
    MF370_R4,
    MF370_R5,
    MF370_R6,
    MF370_R7,
    MF370_R8,
    MF370_R9,
    MF370_R10,
    MF370_R11,
    MF370_R12,
    MF370_R13,
    MF370_R14,
    MF370_R15,
    MF370_NUM_REGS,
};

typedef struct cc_mf370_context {
    cc_backend_varmap latest_varmap; /* Latest relevant varmap */
} cc_mf370_context;

static const char* reg_names[MF370_NUM_REGS] = { "R0", "R1", "R2", "R3", "R4",
    "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };

unsigned int cc_mf370_get_sizeof(cc_context* ctx, const cc_ast_type* type)
{
    size_t sizeof_ptr = 4;
    if (type->n_cv_qual > 0) /* Pointer types */
        return sizeof_ptr;

    switch (type->mode) {
    case TYPE_MODE_CHAR:
        return 1;
    case TYPE_MODE_BOOL:
        return 1;
    case TYPE_MODE_INT:
        return 4;
    case TYPE_MODE_SHORT:
        return 2;
    case TYPE_MODE_LONG:
        return 8;
    case TYPE_MODE_FUNCTION:
        return sizeof_ptr;
    default:
        break;
    }
    cc_diag_error(ctx, "Unknown sizeof");
    return 0;
}

_Bool cc_mf370_is_reserved_reg(unsigned int regno)
{
    return regno == MF370_R14 || regno == MF370_R13;
}

static void cc_mf370_print_varmap(
    cc_context* ctx, const cc_backend_varmap* vmap)
{
    switch (vmap->flags) {
    case VARMAP_REGISTER:
        fprintf(ctx->out, "%s", reg_names[vmap->regno]);
        break;
    case VARMAP_STACK:
        fprintf(ctx->out, "%u(R1)", vmap->offset);
        break;
    case VARMAP_CONSTANT:
        fprintf(ctx->out, "=F'%08lu'", vmap->constant);
        break;
    default:
        cc_diag_error(ctx, "Invalid varmap %i", vmap->flags);
        break;
    }
}

_Bool cc_mf370_gen_mov(cc_context* ctx, const cc_backend_varmap* lvmap,
    const cc_backend_varmap* rvmap)
{
    assert(
        !(lvmap->flags == VARMAP_CONSTANT && rvmap->flags == VARMAP_CONSTANT));

    /* No need for move */
    if (lvmap->flags == VARMAP_REGISTER && rvmap->flags == VARMAP_REGISTER
        && lvmap->regno == rvmap->regno)
        return true;

    /* Constant 0 */
    if (lvmap->flags == VARMAP_REGISTER && rvmap->flags == VARMAP_CONSTANT
        && rvmap->constant == 0) {
        fprintf(ctx->out, "\tL\t%s,=F'0'\n", reg_names[lvmap->regno]);
        return true;
    }

    /* General mov operand modes */
    if (lvmap->flags == VARMAP_REGISTER && rvmap->flags == VARMAP_REGISTER) {
        fprintf(ctx->out, "\tLR\t");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, ",");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, "\n");
        return true;
    }
    if ((lvmap->flags == VARMAP_STACK || lvmap->flags == VARMAP_THREAD_LOCAL)
        && rvmap->flags == VARMAP_REGISTER) {
        fprintf(ctx->out, "\tST\t");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, ",");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, "\n");
        return true;
    } else if (lvmap->flags == VARMAP_REGISTER
        && (rvmap->flags == VARMAP_STACK || rvmap->flags == VARMAP_THREAD_LOCAL
            || rvmap->flags == VARMAP_CONSTANT)) {
        fprintf(ctx->out, "\tL\t");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, ",");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, "\n");
        return true;
    } else if (lvmap->flags == VARMAP_STACK
        && (rvmap->flags == VARMAP_REGISTER
            || rvmap->flags == VARMAP_CONSTANT)) {
        fprintf(ctx->out, "\tST\t");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, ",");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, "\n");
        return true;
    }

    if (lvmap->flags == VARMAP_LITERAL || rvmap->flags == VARMAP_LITERAL) {
        unsigned int branch_label_id = cc_backend_get_labelnum(ctx);
        unsigned int literal_label_id = cc_backend_get_labelnum(ctx);

        fprintf(ctx->out, "\tBR\tL%i\n", branch_label_id);
        fprintf(ctx->out, "L%i\tDS\t0H\n", literal_label_id);
        if (lvmap->flags == VARMAP_LITERAL)
            fprintf(ctx->out, "\tDC\tC'%s'\n", lvmap->data);
        if (rvmap->flags == VARMAP_LITERAL)
            fprintf(ctx->out, "\tDC\tC'%s'\n", rvmap->data);
        fprintf(ctx->out, "L%i\tDS\t0H\n", branch_label_id);

        if (lvmap->flags == VARMAP_REGISTER && rvmap->flags == VARMAP_LITERAL) {
            fprintf(ctx->out, "\tL\t%s,L%i\n", reg_names[lvmap->regno],
                literal_label_id);
        } else if (lvmap->flags == VARMAP_LITERAL
            && rvmap->flags == VARMAP_REGISTER) {
            fprintf(ctx->out, "\tL\t%s, L%i\n", reg_names[rvmap->regno],
                literal_label_id);
        } else if (lvmap->flags == VARMAP_STACK
            && rvmap->flags == VARMAP_LITERAL) {
            fprintf(ctx->out, "\tL\tL%i,%u(R13)\n", literal_label_id,
                lvmap->offset);
        } else if (lvmap->flags == VARMAP_LITERAL
            && rvmap->flags == VARMAP_STACK) {
            cc_backend_varmap mvmap = {};
            cc_backend_spill(ctx, 1);
            mvmap.regno = cc_backend_alloc_register(ctx);
            mvmap.flags = VARMAP_REGISTER;
            fprintf(
                ctx->out, "\tLA\t%s,%u(R13)\n", reg_names[mvmap.regno], rvmap->offset);
            fprintf(
                ctx->out, "\tST\t%s,=V(L%i)\n", reg_names[mvmap.regno], literal_label_id);
            cc_backend_free_register(ctx, mvmap.regno);
        }
        return true;
    }

    if (lvmap->flags == VARMAP_STACK) {
        cc_backend_varmap mvmap = {};
        cc_backend_spill(ctx, 1);
        mvmap.regno = cc_backend_alloc_register(ctx);
        mvmap.flags = VARMAP_REGISTER;
        fprintf(ctx->out, "\tL\t%s,%u(R13)\n", reg_names[mvmap.regno],
            rvmap->offset);
        fprintf(ctx->out, "\tST\t%s,%u(R13)\n", reg_names[mvmap.regno],
            lvmap->offset);
        cc_backend_free_register(ctx, mvmap.regno);
        return true;
    }

    cc_diag_error(ctx, "Impossible constraints for move from %i->%i",
        rvmap->flags, lvmap->flags);
    return false;
}

static const char* cc_mf370_logical_label(const char* name)
{
    static char buf[8];
    size_t n = strlen(name) >= sizeof(buf) - 1 ? sizeof(buf) : strlen(name) + 1;
    memcpy(buf, name, n);
    buf[n - 1] = '\0';

    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] == '_')
            buf[i] = '@';
        buf[i] = toupper(buf[i]);
    }
    return buf;
}

_Bool cc_mf370_gen_epilogue(
    cc_context* ctx, const cc_ast_node* node, const cc_ast_variable* var)
{
    assert(var != NULL);
    if (node->type == AST_NODE_BLOCK) {
        for (size_t i = 0; i < node->data.block.n_vars; i++) {
            const cc_ast_variable* bvar = &node->data.block.vars[i];
            ctx->backend_data->stack_frame_size
                += cc_mf370_get_sizeof(ctx, &bvar->type);
            cc_backend_add_stack_var(ctx, bvar);
        }
    }
    /* I forgot how you're supposed to do alloc/drop on hlasm */
    fprintf(ctx->out, "* X-epilogue\n");
    fprintf(ctx->out, "\tPUSH\tUSING\n");
    fprintf(ctx->out, "\tDROP\t,\n");
    fprintf(ctx->out, "\tENTRY\t%-7s\n", cc_mf370_logical_label(var->name));
    fprintf(ctx->out, "%-7s\tDS\t0H\n", cc_mf370_logical_label(var->name));
    fprintf(ctx->out, "\tSAVE\t(R14,R12),,%-7s\n",
        cc_mf370_logical_label(var->name));
    fprintf(ctx->out, "\tLR\tR12, R15\n");
    fprintf(ctx->out, "\tUSING\tR13,R14\n");
    assert(ctx->backend_data->min_stack_alignment == 0);
    return true;
}

cc_backend_varmap cc_mf370_get_call_retval(
    cc_context* ctx, const cc_ast_node* node)
{
    cc_backend_varmap vmap = {};
    cc_backend_reserve_reg(ctx, MF370_R1);
    vmap.regno = MF370_R1;
    vmap.flags = VARMAP_REGISTER;
    return vmap;
}

/* Generate a jump to the given node */
_Bool cc_mf370_gen_jump(cc_context* ctx, const cc_ast_node* node)
{
    fprintf(ctx->out, "\tB\tL%i\n", node->label_id);
    return true;
}

_Bool cc_mf370_gen_call(cc_context* ctx, const cc_ast_node* node)
{
    switch (node->type) {
    case AST_NODE_VARIABLE: {
        const cc_ast_variable* var
            = cc_ast_find_variable(node->data.var.name, node);
        fprintf(ctx->out, "\tCALL\t%s\n", cc_mf370_logical_label(var->name));
    }
        return true;
    default:
        return false;
    }
    return false;
}

_Bool cc_mf370_gen_unop(cc_context* ctx, const cc_backend_varmap* lvmap,
    const cc_backend_varmap* rvmap, enum cc_ast_unop_type type)
{
    if ((lvmap->flags == VARMAP_STACK || lvmap->flags == VARMAP_STATIC
            || lvmap->flags == VARMAP_THREAD_LOCAL)
        && (rvmap->flags == VARMAP_STACK || rvmap->flags == VARMAP_STATIC
            || rvmap->flags == VARMAP_THREAD_LOCAL))
        return false;

    switch (type) {
    case AST_UNOP_DEREF: {
        cc_backend_spill(ctx, 1);
        cc_backend_varmap nlvmap = {};
        nlvmap.regno = cc_backend_alloc_register(ctx);
        nlvmap.flags = VARMAP_REGISTER;
        fprintf(ctx->out, "\tLA\t");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, ", ");
        cc_mf370_print_varmap(ctx, &nlvmap);
        fprintf(ctx->out, "\n");

        fprintf(ctx->out, "\tL\t(");
        cc_mf370_print_varmap(ctx, &nlvmap);
        fprintf(ctx->out, "), ");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, "\n");
    } break;
    case AST_UNOP_REF:
        break;
    default:
        cc_diag_error(ctx, "Unrecognized unop type %u", type);
        break;
    }
    return true;
}

_Bool cc_mf370_gen_binop(cc_context* ctx, const cc_backend_varmap* lvmap,
    const cc_backend_varmap* rvmap, enum cc_ast_binop_type type)
{
    if ((lvmap->flags == VARMAP_STACK || lvmap->flags == VARMAP_STATIC
            || lvmap->flags == VARMAP_THREAD_LOCAL)
        && (rvmap->flags == VARMAP_STACK || rvmap->flags == VARMAP_STATIC
            || rvmap->flags == VARMAP_THREAD_LOCAL))
        return false;

    switch (type) {
    case AST_BINOP_PLUS:
    case AST_BINOP_MINUS:
    case AST_BINOP_AND:
    case AST_BINOP_OR:
    case AST_BINOP_XOR: {
        const char* insn = NULL;
        switch (type) {
        case AST_BINOP_PLUS:
            insn = "A";
            break;
        case AST_BINOP_MINUS:
            insn = "S";
            break;
        case AST_BINOP_AND:
            insn = "N";
            break;
        case AST_BINOP_OR:
            insn = "O";
            break;
        case AST_BINOP_XOR:
            insn = "X";
            break;
        default:
            break;
        }
        fprintf(ctx->out, "\t%sl\t", insn);
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, ", ");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, "\n");
    } break;
    case AST_BINOP_MUL:
        fprintf(ctx->out, "\tMR\t");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, ", ");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, "\n");
        break;
    case AST_BINOP_DIV:
        fprintf(ctx->out, "\tDR\t");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, ", ");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, "\n");
        break;
    case AST_BINOP_LSHIFT:
        fprintf(ctx->out, "\tSLR\t");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, ", ");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, "\n");
        break;
    case AST_BINOP_RSHIFT:
        fprintf(ctx->out, "\tSRR\t");
        cc_mf370_print_varmap(ctx, rvmap);
        fprintf(ctx->out, ", ");
        cc_mf370_print_varmap(ctx, lvmap);
        fprintf(ctx->out, "\n");
        break;
    case AST_BINOP_GT:
    case AST_BINOP_GTE:
    case AST_BINOP_LT:
    case AST_BINOP_LTE:
    case AST_BINOP_COND_EQ:
    case AST_BINOP_COND_NEQ: {
        unsigned int branch_lnum = cc_backend_get_labelnum(ctx);
        unsigned int finish_lnum = cc_backend_get_labelnum(ctx);
        cc_backend_varmap constant = {};
        constant.flags = VARMAP_CONSTANT;

        const char* jmp_insn = "BR";
        switch (type) {
        case AST_BINOP_GT:
            jmp_insn = "BGT";
            break;
        case AST_BINOP_GTE:
            jmp_insn = "BGE";
            break;
        case AST_BINOP_LT:
            jmp_insn = "BLT";
            break;
        case AST_BINOP_LTE:
            jmp_insn = "BLE";
            break;
        case AST_BINOP_COND_EQ:
            jmp_insn = "BEQ";
            break;
        case AST_BINOP_COND_NEQ:
            jmp_insn = "BNE";
            break;
        default:
            break;
        }

        fprintf(ctx->out, "\tCMP\t%s, %s\n", reg_names[lvmap->regno],
            reg_names[rvmap->regno]);
        fprintf(ctx->out, "\t%s\tL%u\n", jmp_insn, branch_lnum);
        constant.constant = 1ul;
        ctx->backend_data->gen_mov(ctx, lvmap, &constant);
        fprintf(ctx->out, "\tBR\tL%u\n", finish_lnum);

        fprintf(ctx->out, "L%u\tDS\t0H\n", branch_lnum);
        constant.constant = 0ul;
        ctx->backend_data->gen_mov(ctx, lvmap, &constant);

        fprintf(ctx->out, "L%u\tDS\t0H\n", finish_lnum);
    } break;
    default:
        cc_diag_error(ctx, "Unrecognized binop type %u", type);
        break;
    }
    return true;
}

_Bool cc_mf370_gen_prologue(
    cc_context* ctx, const cc_ast_node* node, const cc_ast_variable* var)
{
    fprintf(ctx->out, "* X-prologue\n");
    cc_backend_unspill(ctx);
    if (node != NULL) {
        /* TODO: Generate & return on EAX */
        cc_backend_varmap lvmap = {};
        cc_backend_reserve_reg(ctx, MF370_R1);
        lvmap.flags = VARMAP_REGISTER;
        lvmap.regno = MF370_R1;
        cc_backend_process_node(ctx, node, &lvmap);
    }
    fprintf(ctx->out, "\tRETURN\t(14,12),RC=(15)\n");
    fprintf(ctx->out, "\tPOP\tUSING\n");
    fprintf(ctx->out, "\tLTORG\t,\n");
    return true;
}

_Bool cc_mf370_map_variable(cc_context* ctx, const cc_ast_variable* var)
{
    if (var->type.mode == TYPE_MODE_FUNCTION) {
        if (var->type.storage == STORAGE_STATIC) {
            cc_backend_add_static_var(ctx, var);
        } else if (var->type.storage == STORAGE_AUTO) {
            cc_backend_add_static_var(ctx, var);
        } else if (var->type.storage == STORAGE_EXTERN) {
            cc_backend_add_static_var(ctx, var);
            fprintf(
                ctx->out, "\tEXTRN\t%s\n", cc_mf370_logical_label(var->name));
        }
        return true;
    }

    if (var->type.storage == STORAGE_STATIC) {
        cc_backend_add_static_var(ctx, var);
        fprintf(ctx->out, "%s\tDS\t%uH\n", cc_mf370_logical_label(var->name),
            cc_mf370_get_sizeof(ctx, &var->type));
    } else if (var->type.storage == STORAGE_AUTO) {
        cc_backend_add_stack_var(ctx, var);
        fprintf(ctx->out, "* X-stack-var %s\n", var->name);
    }
    return true;
}

int cc_mf370_top(cc_context* ctx)
{
    ctx->asgen_data = cc_zalloc(sizeof(cc_mf370_context));
    cc_backend_init(ctx, reg_names, MF370_NUM_REGS);
    /* No alignment required! */
    ctx->backend_data->min_stack_alignment = 0;
    ctx->backend_data->is_reserved = &cc_mf370_is_reserved_reg;
    ctx->backend_data->get_sizeof = &cc_mf370_get_sizeof;
    ctx->backend_data->gen_mov = &cc_mf370_gen_mov;
    ctx->backend_data->get_call_retval = &cc_mf370_get_call_retval;
    ctx->backend_data->map_variable = &cc_mf370_map_variable;
    ctx->backend_data->gen_epilogue = &cc_mf370_gen_epilogue;
    ctx->backend_data->gen_prologue = &cc_mf370_gen_prologue;
    ctx->backend_data->gen_call = &cc_mf370_gen_call;
    ctx->backend_data->gen_jump = &cc_mf370_gen_jump;
    ctx->backend_data->gen_binop = &cc_mf370_gen_binop;
    ctx->backend_data->gen_unop = &cc_mf370_gen_unop;
    ctx->stage = STAGE_AST;
    cc_backend_process_node(ctx, ctx->root, NULL);
    cc_backend_deinit(ctx);
    cc_free(ctx->asgen_data);
    return 0;
}
