#ifndef __C_LANG_H__
#define __C_LANG_H__

#include "token.h"
#include "parser.h"

typedef enum SyntaxType_e {
  UNKNOWN_ST,
  BRACE_ST,    // {} and ()
  OPERATOR_ST, // +, -, *, /, etc.
  LITERAL_ST,  // numbers, chars, and strings
  KEYWORD_ST,  // if, for, while, enum, struct, etc.
  SEMICOLON_ST,
  IDENTIFIER_ST
} SyntaxType;

struct TokenStruct_s {
    TokenType tok_type;
    SyntaxType syntax;
    ExpressionType primary_type;
    ExpressionType binary_type;
    int precedence;
    const char* name;
    const char* tok_out;
};

struct TokenStruct_s token_structs[84] = {
  {UNKNOWN,      UNKNOWN_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, NULL,       NULL},
  {LEFT_BRACE,   BRACE_ST,      UNKNOWN_EXP, UNKNOWN_EXP,    0, "{",        "LEFT BRACE"},
  {RIGHT_BRACE,  BRACE_ST,      UNKNOWN_EXP, UNKNOWN_EXP,    0, "}",        "RIGHT BRACE"},
  {LEFT_PAREN,   BRACE_ST,      UNKNOWN_EXP, UNKNOWN_EXP,    0, "(",        "LEFT BRACE"},
  {RIGHT_PAREN,  BRACE_ST,      UNKNOWN_EXP, UNKNOWN_EXP,   -1, ")",       "RIGHT BRACE"},
  {LEFT_SQUARE,  BRACE_ST,      UNKNOWN_EXP, UNKNOWN_EXP,    0, "[",        "LEFT SQUARE"},
  {RIGHT_SQUARE, BRACE_ST,      UNKNOWN_EXP, UNKNOWN_EXP,    0, "]",        "RIGHT SQUARE"},
  {SEMICOLON,    SEMICOLON_ST,  UNKNOWN_EXP, UNKNOWN_EXP,   -1, ";",       "SEMICOLON"},
  {BREAK_TOK,    KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "break",    "BREAK"},
  {CASE_TOK,     KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "case",     "CASE"},
  {CHAR_TOK,     KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "char",     "CHAR"},
  {CONST_TOK,    KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "const",    "CONST"},
  {CONTINUE_TOK, KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "continue", "CONTINUE"},
  {DEFAULT_TOK,  KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "default",  "DEFAULT"},
  {DO_TOK,       KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "do",       "DO"},
  {DOUBLE_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "double",   "DOUBLE"},
  {ELSE_TOK,     KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "else",     "ELSE"},
  {ENUM_TOK,     KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "enum",     "ENUM"},
  {EXTERN_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "extern",   "EXTERN"},
  {FLOAT_TOK,    KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "float",    "FLOAT"},
  {FOR_TOK,      KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "for",      "FOR"},
  {GOTO_TOK,     KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "goto",     "GOTO"},
  {IF_TOK,       KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "if",       "IF"},
  {INLINE_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "inline",   "INLINE"},
  {INT_TOK,      KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "int",      "INT"},
  {LONG_TOK,     KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "long",     "LONG"},
  {REGISTER_TOK, KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "register", "REGISTER"},
  {RESTRICT_TOK, KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "restrict", "RESTRICT"},
  {RETURN_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "return",   "RETURN"},
  {SHORT_TOK,    KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "short",    "SHORT"},
  {SIGNED_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "signed",   "SIGNED"},
  {SIZEOF_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "sizeof",   "SIZEOF"},
  {STATIC_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "static",   "STATIC"},
  {STRUCT_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "struct",   "STRUCT"},
  {SWITCH_TOK,   KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "switch",   "SWITCH"},
  {TYPEDEF_TOK,  KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "typedef",  "TYPEDEF"},
  {UNION_TOK,    KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "union",    "UNION"},
  {UNSIGNED_TOK, KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "unsigned", "UNSIGNED"},
  {VOID_TOK,     KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "void",     "VOID"},
  {VOLATILE_TOK, KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "volatile", "VOLATILE"},
  {WHILE_TOK,    KEYWORD_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, "while",    "WHILE"},
  {IDENTIFIER,   IDENTIFIER_ST, VAR_EXP,     UNKNOWN_EXP,    0, "",         "IDENT"},
  {INT_LITERAL,  LITERAL_ST,    INT_VALUE,   UNKNOWN_EXP,    0, "",         "INT LITERAL"},
  {HEX_LITERAL,  LITERAL_ST,    INT_VALUE,   UNKNOWN_EXP,    0, "",         "HEX LITERAL"},
  {OCT_LITERAL,  LITERAL_ST,    INT_VALUE,   UNKNOWN_EXP,    0, "",         "OCT LITERAL"},
  {LOGICAL_NOT,  OPERATOR_ST,   LOG_NOT,     UNKNOWN_EXP,    0, "!",        "NOT"},
  {BITWISE_NOT,  OPERATOR_ST,   BITWISE_COMP,UNKNOWN_EXP,    0, "~",        "NOT"},
  {MINUS,        OPERATOR_ST,   NEGATE,      SUB_BINEXP,    12, "-",        "NOT"},
  {PLUS,         OPERATOR_ST,   UNKNOWN_EXP, ADD_BINEXP,    12, "+",        "NOT"},
  {STAR,         OPERATOR_ST,   UNKNOWN_EXP, MUL_BINEXP,    13, "*",        "NOT"},
  {SLASH,        OPERATOR_ST,   UNKNOWN_EXP, DIV_BINEXP,    13, "/",        "NOT"},
  {EQUAL,        OPERATOR_ST,   UNKNOWN_EXP, EQ_BINEXP,      9, "==",        "NOT"},
  {NOTEQUAL,     OPERATOR_ST,   UNKNOWN_EXP, NEQ_BINEXP,     9, "!=",        "NOT"},
  {AND,          OPERATOR_ST,   UNKNOWN_EXP, AND_BINEXP,     5, "&&",        "NOT"},
  {OR,           OPERATOR_ST,   UNKNOWN_EXP, OR_BINEXP,      4, "||",        "NOT"},
  {LESSTHAN,     OPERATOR_ST,   UNKNOWN_EXP, LT_BINEXP,     10, "<",        "NOT"},
  {LEQ,          OPERATOR_ST,   UNKNOWN_EXP, LEQ_BINEXP,    10, "<=",        "NOT"},
  {GREATERTHAN,  OPERATOR_ST,   UNKNOWN_EXP, GT_BINEXP,     10, ">",        "NOT"},
  {GEQ,          OPERATOR_ST,   UNKNOWN_EXP, GEQ_BINEXP,    10, ">=",        "NOT"},
  {MOD,          OPERATOR_ST,   UNKNOWN_EXP, MOD_BINEXP,    13, "%",        "NOT"},
  {BIT_AND,      OPERATOR_ST,   UNKNOWN_EXP, BITAND_BINEXP,  8, "&",        "NOT"},
  {BIT_OR,       OPERATOR_ST,   UNKNOWN_EXP, BITOR_BINEXP,   6, "|",        "NOT"},
  {BIT_XOR,      OPERATOR_ST,   UNKNOWN_EXP, BITXOR_BINEXP,  7, "^",        "NOT"},
  {LSHIFT,       OPERATOR_ST,   UNKNOWN_EXP, LSHIFT_BINEXP, 11, "<<",        "NOT"},
  {RSHIFT,       OPERATOR_ST,   UNKNOWN_EXP, RSHIFT_BINEXP, 11, ">>",        "NOT"},
  {ASSIGN,       OPERATOR_ST,   UNKNOWN_EXP, ASSIGN_EXP,     2, "=",        "NOT"},
  {PLUSEQ,       OPERATOR_ST,   UNKNOWN_EXP, PLUSEQ_EXP,     2, "+=",        "NOT"},
  {MINUSEQ,      OPERATOR_ST,   UNKNOWN_EXP, MINUSEQ_EXP,    2, "-=",        "NOT"},
  {TIMESEQ,      OPERATOR_ST,   UNKNOWN_EXP, TIMESEQ_EXP,    2, "*=",        "NOT"},
  {DIVEQ,        OPERATOR_ST,   UNKNOWN_EXP, DIVEQ_EXP,      2, "/=",        "NOT"},
  {MODEQ,        OPERATOR_ST,   UNKNOWN_EXP, MODEQ_EXP,      2, "%=",        "NOT"},
  {LSHEQ,        OPERATOR_ST,   UNKNOWN_EXP, LSHEQ_EXP,      2, "<<=",        "NOT"},
  {RSHEQ,        OPERATOR_ST,   UNKNOWN_EXP, RSHEQ_EXP,      2, ">>=",        "NOT"},
  {ANDEQ,        OPERATOR_ST,   UNKNOWN_EXP, ANDEQ_EXP,      2, "&=",        "NOT"},
  {OREQ,         OPERATOR_ST,   UNKNOWN_EXP, OREQ_EXP,       2, "|=",        "NOT"},
  {XOREQ,        OPERATOR_ST,   UNKNOWN_EXP, XOREQ_EXP,      2, "^=",        "NOT"},
  {COMMA,        OPERATOR_ST,   UNKNOWN_EXP, COMMA_EXP,      1, ",",        "NOT"},
  {PLUSPLUS,     OPERATOR_ST,   PREINC_EXP,  UNKNOWN_EXP,   14, "++",        "NOT"},
  {MINUSMINUS,   OPERATOR_ST,   PREDEC_EXP,  UNKNOWN_EXP,   14, "--",        "NOT"},
  {QMARK,        OPERATOR_ST,   UNKNOWN_EXP, UNKNOWN_EXP,    3, "?",        "NOT"},
  {COLON,        OPERATOR_ST,   UNKNOWN_EXP, UNKNOWN_EXP,    3, ":",        "NOT"},
  {DOT,          OPERATOR_ST,   UNKNOWN_EXP, UNKNOWN_EXP,   14, ".",        "NOT"},
  {ARROW,        OPERATOR_ST,   UNKNOWN_EXP, UNKNOWN_EXP,   14, "->",        "NOT"},
  {UNKNOWN,      UNKNOWN_ST,    UNKNOWN_EXP, UNKNOWN_EXP,    0, 0,            0}
};

#endif
