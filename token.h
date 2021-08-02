#ifndef __TOKEN_H__
#define __TOKEN_H__

typedef enum TokenType_t {
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
  SLASH
} TokenType;

typedef struct Token_t {
  TokenType type;
  char* value;
} Token;

typedef struct TokenListNode_t {
  Token tok;
  struct TokenListNode_t* prev;
  struct TokenListNode_t* next;
} TokenListNode;

typedef struct TokenList_t {
  TokenListNode* first;
  TokenListNode* last;
} TokenList;

Token token_list_pop_back(TokenList*);
Token token_list_pop_front(TokenList*);
int token_list_push(TokenList*, Token);

#endif
