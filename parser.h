#ifndef PARSER_H_
#define PARSER_H_

#include "token.h"

#include <stdlib.h>

typedef enum ExpressionType_t {
  UNKNOWN_EXP,
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
  POSTDEC_EXP,
  COND_EXP
} ExpressionType;

typedef enum VarType_e {
  INT_VAR,
  FLOAT_VAR,
  DOUBLE_VAR,
  CHAR_VAR
} VarType;

typedef enum StatementType_e {
  RETURN_STATEMENT,
  EXPRESSION,
  CONDITIONAL
} StatementType;

typedef enum BlockItemType_e {
  STATEMENT_ITEM,
  DECLARATION_ITEM
} BlockItemType;

typedef enum ReturnType_e {
  INT_RET
} ReturnType;

typedef enum TypeModifier_e {
  NO_MOD = 0,
  CONST_MOD = 1 << 0,
  STATIC_MOD = 1 << 1,
  SHORT_MOD = 1 << 2,
  LONG_MOD = 1 << 3,
  LONGLONG_MOD = 1 << 4,
  SIGNED_MOD = 1 << 5,
  UNSIGNED_MOD = 1 << 6,
  VOLATILE_MOD = 1 << 7,
  REGISTER_MOD = 1 << 8,
  RESTRICT_MOD = 1 << 9,
  AUTO_MOD = 1 << 10,
  EXTERN_MOD = 1 << 11
} TypeModifier;

typedef struct CType_s {
  VarType base;
  TypeModifier mods;
} CType;

typedef struct ExpressionNode_s {
  ExpressionType type;
  union {
    int int_value;
    struct ExpressionNode_s* unary_operand;
    struct { // For binary operators
      struct ExpressionNode_s* left_operand;
      struct ExpressionNode_s* right_operand;
    };
    struct { // For Conditionals
      struct ExpressionNode_s* condition;
      struct ExpressionNode_s* if_exp;
      struct ExpressionNode_s* else_exp;
    };
    char* var_name; // For VAR
  };
} ExpressionNode;

typedef struct DeclarationNode_s {
  VarType var_type;
  char* var_name;
  ExpressionNode* assignment_expression;
} DeclarationNode;

typedef struct StatementNode_s {
  StatementType type;
  union {
    ExpressionNode* expression;
    struct {
      ExpressionNode* condition;
      struct StatementNode_s* if_stmt;
      struct StatementNode_s* else_stmt;
    };
  };
} StatementNode;

typedef struct BlockItem_s {
  BlockItemType type;
  union {
    StatementNode* stmt;
    DeclarationNode* decl;
  };
} BlockItem;

typedef struct BlockNode_s {
  BlockItem** body;
  size_t count;
  size_t capacity;
} BlockNode;

typedef struct FunctionNode_s {
  char* name;
  ReturnType type;
  BlockNode* body;
} FunctionNode;

typedef struct ProgramNode_s {
  FunctionNode* main;
} ProgramNode;

ProgramNode parse(TokenList*);

#endif
