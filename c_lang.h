#ifndef C_LANG_H_
#define C_LANG_H_

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
    int declaration;
    int precedence;
    int right_assoc;
    int need_lvalue;
    const char* name;
    const char* tok_out;
};

extern struct TokenStruct_s token_structs[];

#endif
