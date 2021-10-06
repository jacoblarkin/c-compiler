#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <stddef.h>

typedef enum TokenType_e {
  UNKNOWN = 0,
  LEFT_BRACE,
  RIGHT_BRACE,
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_SQUARE,
  RIGHT_SQUARE,
  SEMICOLON,
  BREAK_TOK,
  CASE_TOK,
  CHAR_TOK,
  CONST_TOK,
  CONTINUE_TOK,
  DEFAULT_TOK,
  DO_TOK,
  DOUBLE_TOK,
  ELSE_TOK,
  ENUM_TOK,
  EXTERN_TOK,
  FLOAT_TOK,
  FOR_TOK,
  GOTO_TOK,
  IF_TOK,
  INLINE_TOK,
  INT_TOK,
  LONG_TOK,
  REGISTER_TOK,
  RESTRICT_TOK,
  RETURN_TOK,
  SHORT_TOK,
  SIGNED_TOK,
  SIZEOF_TOK,
  STATIC_TOK,
  STRUCT_TOK,
  SWITCH_TOK,
  TYPEDEF_TOK,
  UNION_TOK,
  UNSIGNED_TOK,
  VOID_TOK,
  VOLATILE_TOK,
  WHILE_TOK,
  IDENTIFIER,
  INT_LITERAL,
  HEX_LITERAL,
  OCT_LITERAL,
  LOGICAL_NOT,
  BITWISE_NOT,
  MINUS,
  PLUS,
  STAR,
  SLASH,
  EQUAL,
  NOTEQUAL,
  AND,
  OR,
  LESSTHAN,
  LEQ,
  GREATERTHAN,
  GEQ,
  MOD,
  BIT_AND,
  BIT_OR,
  BIT_XOR,
  LSHIFT,
  RSHIFT,
  ASSIGN,
  PLUSEQ,
  MINUSEQ,
  TIMESEQ,
  DIVEQ,
  MODEQ,
  LSHEQ,
  RSHEQ,
  ANDEQ,
  OREQ,
  XOREQ,
  COMMA,
  PLUSPLUS,
  MINUSMINUS,
  QMARK,
  COLON,
  DOT,
  ARROW
} TokenType;

typedef struct Token_s {
  TokenType type;
  char* value;
} Token;

typedef struct TokenListNode_s {
  Token tok;
  struct TokenListNode_s* prev;
  struct TokenListNode_s* next;
} TokenListNode;

typedef struct TokenList_s {
  TokenListNode* first;
  TokenListNode* last;
} TokenList;

Token token_list_pop_back(TokenList*);
Token token_list_pop_front(TokenList*);
Token token_list_peek_front(TokenList*);
Token token_list_peek_n(TokenList*, size_t);
int token_list_push(TokenList*, Token);

#endif
