#ifndef CONTEXT_H
#define CONTEXT_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum cc_stage {
    STAGE_LEXER,
    STAGE_PARSER,
    STAGE_AST,
};

typedef struct cc_context {
    FILE* fp;
    FILE* out;
    char* top_file;

    struct cc_lexer_token* tokens;
    size_t n_tokens;
    size_t c_token; /* Current token index */

    struct cc_ast_node* root;

    const char* cbuf; /* Current logical line buffer */
    const char* cptr; /* Line pointer for diagnostics */

    struct cc_diag_info* diag_infos;
    size_t n_diag_infos;
    enum cc_stage stage;
    const struct cc_ast_node* diag_node; /* Node for diagnostic */

    void* asgen_data; /* Opaque pointer for assembly generation */
    struct cc_backend_context* backend_data;

    struct cc_ast_node* continue_node; /* Node to jump to in continue */
    struct cc_ast_node* break_node; /* Node to jump to in break */
} cc_context;

#endif
