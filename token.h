#ifndef __TOKEN_H__
#define __TOKEN_H__

typedef enum TokenType_e {
  UNKNOWN = 0,
  LEFT_BRACE,
  RIGHT_BRACE,
  LEFT_PAREN,
  RIGHT_PAREN,
  SEMICOLON,
  INT,
  RETURN,
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
  XOREQ
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
int token_list_push(TokenList*, Token);

#endif
