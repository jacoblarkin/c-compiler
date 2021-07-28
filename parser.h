#ifndef __PARSER_H__
#define __PARSER_H__

#include "token.h"

#include <stdlib.h>

typedef enum ExpressionType_t {
  INT_EXP
} ExpressionType;

typedef enum StatementType_t {
  RETURN_STATEMENT
} StatementType;

typedef enum ReturnType_t {
  INT_RET
} ReturnType;

typedef struct ExpressionNode_t {
  ExpressionType type;
  union {
    int int_value;
  };
} ExpressionNode;

typedef struct StatementNode_t {
  StatementType type;
  union {
    ExpressionNode* return_value;
  };
} StatementNode;

typedef struct FunctionNode_t {
  char* name;
  ReturnType type;
  StatementNode** body;
  size_t num_statements;
  size_t capacity;
} FunctionNode;

typedef struct ProgramNode_t {
  FunctionNode* main;
} ProgramNode;

ProgramNode parse(TokenList*);

#endif
