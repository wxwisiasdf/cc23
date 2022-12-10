#ifndef LEXER_H
#define LEXER_H 1

#include "diag.h"
#include "context.h"

#define LEXER_TOKEN_LIST_R(x) LEXER_TOKEN_LIST_1(x, #x)
#define LEXER_TOKEN_LIST                                                       \
    LEXER_TOKEN_LIST_1(NONE, 0), \
    LEXER_TOKEN_LIST_R(typeof_unqual), \
    LEXER_TOKEN_LIST_R(static_assert), \
    LEXER_TOKEN_LIST_R(thread_local), \
    LEXER_TOKEN_LIST_R(_Decimal128), \
    LEXER_TOKEN_LIST_R(_Imaginary), \
    LEXER_TOKEN_LIST_R(_Decimal64), \
    LEXER_TOKEN_LIST_R(_Decimal32), \
    LEXER_TOKEN_LIST_R(_Noreturn), \
    LEXER_TOKEN_LIST_R(constexpr), \
    LEXER_TOKEN_LIST_R(_Generic), \
    LEXER_TOKEN_LIST_R(_Complex), \
    LEXER_TOKEN_LIST_R(volatile), \
    LEXER_TOKEN_LIST_R(unsigned), \
    LEXER_TOKEN_LIST_R(restrict), \
    LEXER_TOKEN_LIST_R(register), \
    LEXER_TOKEN_LIST_R(continue), \
    LEXER_TOKEN_LIST_R(alignas), \
    LEXER_TOKEN_LIST_R(alignof), \
    LEXER_TOKEN_LIST_R(default), \
    LEXER_TOKEN_LIST_R(nullptr), \
    LEXER_TOKEN_LIST_R(_Atomic), \
    LEXER_TOKEN_LIST_R(_BitInt), \
    LEXER_TOKEN_LIST_R(double), \
    LEXER_TOKEN_LIST_R(extern), \
    LEXER_TOKEN_LIST_R(inline), \
    LEXER_TOKEN_LIST_R(return), \
    LEXER_TOKEN_LIST_R(signed), \
    LEXER_TOKEN_LIST_R(sizeof), \
    LEXER_TOKEN_LIST_R(static), \
    LEXER_TOKEN_LIST_R(struct), \
    LEXER_TOKEN_LIST_R(switch), \
    LEXER_TOKEN_LIST_R(typeof), \
    LEXER_TOKEN_LIST_R(break), \
    LEXER_TOKEN_LIST_R(const), \
    LEXER_TOKEN_LIST_R(false), \
    LEXER_TOKEN_LIST_R(float), \
    LEXER_TOKEN_LIST_R(short), \
    LEXER_TOKEN_LIST_R(union), \
    LEXER_TOKEN_LIST_R(while), \
    LEXER_TOKEN_LIST_R(auto), \
    LEXER_TOKEN_LIST_R(bool), \
    LEXER_TOKEN_LIST_R(case), \
    LEXER_TOKEN_LIST_R(char), \
    LEXER_TOKEN_LIST_R(else), \
    LEXER_TOKEN_LIST_R(enum), \
    LEXER_TOKEN_LIST_R(goto), \
    LEXER_TOKEN_LIST_R(long), \
    LEXER_TOKEN_LIST_R(true), \
    LEXER_TOKEN_LIST_R(void), \
    LEXER_TOKEN_LIST_R(for), \
    LEXER_TOKEN_LIST_R(int), \
    LEXER_TOKEN_LIST_1(ASSIGN_LSHIFT, "<<="), \
    LEXER_TOKEN_LIST_1(ASSIGN_RSHIFT, ">>="), \
    LEXER_TOKEN_LIST_1(ELLIPSIS, "..."), \
    LEXER_TOKEN_LIST_R(do), \
    LEXER_TOKEN_LIST_R(if), \
    LEXER_TOKEN_LIST_1(NAMESPACE, "::"), \
    LEXER_TOKEN_LIST_1(LOGICAL_AND, "&&"), \
    LEXER_TOKEN_LIST_1(LOGICAL_OR, "||"), \
    LEXER_TOKEN_LIST_1(ASSIGN_PLUS, "+="), \
    LEXER_TOKEN_LIST_1(ASSIGN_MINUS, "-="), \
    LEXER_TOKEN_LIST_1(ASSIGN_MUL, "*="), \
    LEXER_TOKEN_LIST_1(ASSIGN_DIV, "/="), \
    LEXER_TOKEN_LIST_1(ASSIGN_MOD, "%="), \
    LEXER_TOKEN_LIST_1(ASSIGN_AND, "&="), \
    LEXER_TOKEN_LIST_1(ASSIGN_OR, "|="), \
    LEXER_TOKEN_LIST_1(ASSIGN_XOR, "^="), \
    LEXER_TOKEN_LIST_1(COND_NEQ, "!="), \
    LEXER_TOKEN_LIST_1(COND_EQ, "=="), \
    LEXER_TOKEN_LIST_1(LTE, "<="), \
    LEXER_TOKEN_LIST_1(GTE, ">="), \
    LEXER_TOKEN_LIST_1(LSHIFT, "<<"), \
    LEXER_TOKEN_LIST_1(RSHIFT, ">>"), \
    LEXER_TOKEN_LIST_1(INCREMENT, "++"), \
    LEXER_TOKEN_LIST_1(DECREMENT, "--"), \
    LEXER_TOKEN_LIST_1(ARROW, "->"), \
    LEXER_TOKEN_LIST_1(PLUS, "+"), \
    LEXER_TOKEN_LIST_1(MINUS, "-"), \
    LEXER_TOKEN_LIST_1(DIV, "/"), \
    LEXER_TOKEN_LIST_1(MOD, "%"), \
    LEXER_TOKEN_LIST_1(OR, "|"), \
    LEXER_TOKEN_LIST_1(XOR, "^"), \
    LEXER_TOKEN_LIST_1(ASTERISK, "*"), \
    LEXER_TOKEN_LIST_1(AMPERSAND, "&"), \
    LEXER_TOKEN_LIST_1(DOT, "."), \
    LEXER_TOKEN_LIST_1(COMMA, ","), \
    LEXER_TOKEN_LIST_1(LPAREN, "("), \
    LEXER_TOKEN_LIST_1(RPAREN, ")"), \
    LEXER_TOKEN_LIST_1(LBRACE, "{"), \
    LEXER_TOKEN_LIST_1(RBRACE, "}"), \
    LEXER_TOKEN_LIST_1(LBRACKET, "["), \
    LEXER_TOKEN_LIST_1(RBRACKET, "]"), \
    LEXER_TOKEN_LIST_1(COLON, ":"), \
    LEXER_TOKEN_LIST_1(SEMICOLON, ";"), \
    LEXER_TOKEN_LIST_1(LT, "<"), \
    LEXER_TOKEN_LIST_1(GT, ">"), \
    LEXER_TOKEN_LIST_1(ASSIGN, "="), \
    LEXER_TOKEN_LIST_1(HASHTAG, "#"), \
    LEXER_TOKEN_LIST_1(NOT, "~"), \
    LEXER_TOKEN_LIST_1(COND_NOT, "!"), \
    LEXER_TOKEN_LIST_1(TERNARY, "?"), \
    LEXER_TOKEN_LIST_1(IDENT, NULL), \
    LEXER_TOKEN_LIST_1(NUMBER, NULL), \
    LEXER_TOKEN_LIST_1(CHAR_LITERAL, NULL), \
    LEXER_TOKEN_LIST_1(STRING_LITERAL, NULL)

#define LEXER_TOKEN_LIST_1(x, v) LEXER_TOKEN_##x
enum cc_lexer_token_type { LEXER_TOKEN_LIST, NUM_LEXER_TOKENS };
#undef LEXER_TOKEN_LIST_1

typedef struct cc_lexer_token {
    enum cc_lexer_token_type type;
    char* data; /* Extra data for string literals, char literals, etc */
    cc_diag_info info;
} cc_lexer_token;

cc_lexer_token* cc_lex_token_peek(cc_context* ctx, int offset);
cc_lexer_token* cc_lex_token_consume(cc_context* ctx);
int cc_lex_top(cc_context* ctx);
void cc_lex_print_token(const cc_lexer_token* tok);
const cc_lexer_token* cc_lex_skip_until(
    cc_context* ctx, enum cc_lexer_token_type type);
void cc_lex_deinit(cc_context* ctx);

#endif
