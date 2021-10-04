#ifndef __PARSER_H__
#define __PARSER_H__

#include "token.h"

#include <stdlib.h>

typedef enum ExpressionType_t {
  INT_VALUE,
  NEGATE,
  LOG_NOT,
  BITWISE_COMP,
  ADD_BINEXP,
  SUB_BINEXP,
  MUL_BINEXP,
  DIV_BINEXP,
  EQ_BINEXP,
  NEQ_BINEXP,
  GT_BINEXP,
  GEQ_BINEXP,
  LT_BINEXP,
  LEQ_BINEXP,
  AND_BINEXP,
  OR_BINEXP,
  MOD_BINEXP,
  BITAND_BINEXP,
  BITOR_BINEXP,
  BITXOR_BINEXP,
  LSHIFT_BINEXP,
  RSHIFT_BINEXP,
  ASSIGN_EXP,
  VAR_EXP,
  PLUSEQ_EXP,
  MINUSEQ_EXP,
  TIMESEQ_EXP,
  DIVEQ_EXP,
  MODEQ_EXP,
  LSHEQ_EXP,
  RSHEQ_EXP,
  ANDEQ_EXP,
  OREQ_EXP,
  XOREQ_EXP,
  COMMA_EXP,
  PREINC_EXP,
  POSTINC_EXP,
  PREDEC_EXP,
  POSTDEC_EXP
} ExpressionType;

typedef enum VarType_t {
  INT_VAR
} VarType;

typedef enum StatementType_t {
  RETURN_STATEMENT,
  DECLARATION,
  EXPRESSION
} StatementType;

typedef enum ReturnType_t {
  INT_RET
} ReturnType;

typedef struct ExpressionNode_t {
  ExpressionType type;
  union {
    int int_value;
    struct ExpressionNode_t *unary_operand;
    struct { // For binary operators
      struct ExpressionNode_t *left_operand;
      struct ExpressionNode_t *right_operand;
    };
    char* var_name; // For VAR
  };
} ExpressionNode;

typedef struct StatementNode_t {
  StatementType type;
  union {
    ExpressionNode* return_value;
    struct {  // Declare var
      VarType var_type;
      char* var_name;
      ExpressionNode* assignment_expression;
    };
    ExpressionNode* expression;
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
