#ifndef PARSER_H_
#define PARSER_H_

#include "token.h"

#include <stdlib.h>

typedef enum ExpressionType_t {
  UNKNOWN_EXP,
  CHAR_VALUE,
  UCHAR_VALUE,
  SHORT_VALUE,
  USHORT_VALUE,
  INT_VALUE,
  UINT_VALUE,
  LONG_VALUE,
  ULONG_VALUE,
  LONGLONG_VALUE,
  ULONGLONG_VALUE,
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
  COND_EXP,
  EMPTY_EXP
} ExpressionType;

typedef enum VarType_e {
  UNKNOWN_VAR = 0,
  CHAR_VAR,
  SHORT_VAR,
  INT_VAR,
  LONG_VAR,
  LONG_LONG_VAR,
  FLOAT_VAR,
  DOUBLE_VAR
} VarType;

typedef enum StatementType_e {
  RETURN_STATEMENT,
  EXPRESSION,
  CONDITIONAL,
  FOR_LOOP,
  FORDECL_LOOP,
  WHILE_LOOP,
  DO_LOOP,
  BREAK_STATEMENT,
  CONTINUE_STATEMENT,
  BLOCK_STATEMENT,
  SWITCH_STATEMENT,
  CASE_STATEMENT,
  DEFAULT_STATEMENT,
  GOTO_STATEMENT,
  LABEL
} StatementType;

typedef enum BlockItemType_e {
  STATEMENT_ITEM,
  DECLARATION_ITEM
} BlockItemType;

typedef enum CVRQual_e {
  NO_CVRQUAL = 0,
  CONST_CVRQUAL = 1<<0,
  VOLATILE_CVRQUAL = 1<<1,
  RESTRICT_CVRQUAL = 1<<2
} CVRQual;

typedef enum StorageQual_e {
  NO_STORAGEQUAL = 0,
  AUTO_STORAGEQUAL,
  STATIC_STORAGEQUAL,
  EXTERN_STORAGEQUAL,
  REGISTER_STORAGEQUAL
} StorageQual;

typedef struct Type_s {
  VarType base;
  CVRQual cvr;
  StorageQual storage;
  int signed_;
} Type;

typedef struct ExpressionNode_s {
  ExpressionType type;
  Type value_type;
  union {
    char char_value;
    unsigned char uchar_value;
    short short_value;
    unsigned short ushort_value;
    int int_value;
    unsigned int uint_value;
    long long_value;
    unsigned long ulong_value;
    long long longlong_value;
    unsigned long long ulonglong_value;
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
  Type var_type;
  char* var_name;
  ExpressionNode* assignment_expression;
} DeclarationNode;

// Forward declare blocks to use in statements
struct BlockNode_s;

typedef struct StatementNode_s {
  StatementType type;
  union {
    ExpressionNode* expression;
    struct { // Ifs
      ExpressionNode* condition;
      struct StatementNode_s* if_stmt;
      struct StatementNode_s* else_stmt;
    };
    struct { // Loops
      union {
        DeclarationNode* init_decl;
        ExpressionNode* init_exp;
      };
      ExpressionNode* loop_condition;
      ExpressionNode* post_exp;
      struct StatementNode_s* loop_stmt;
    };
    struct { // switch
      ExpressionNode* switch_exp;
      struct BlockNode_s* switch_block;
    };
    struct { // case
      int is_signed;
      union {
        long val;
        unsigned long unsigned_val;
      };
    };
    struct BlockNode_s* block;
    char* label_name;
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
  Type type;
  BlockNode* body;
} FunctionNode;

typedef struct ProgramNode_s {
  FunctionNode* main;
} ProgramNode;

ProgramNode parse(TokenList*);

#endif
