#include "parser.h"
#include "token.h"

#include <stdio.h>
#include <string.h>

typedef enum State_t {
  GLOBAL,
  FUNCTION
} State;

void print_error(const char*);
FunctionNode* construct_function(TokenListNode*);
StatementNode* construct_statement(TokenListNode*);
ExpressionNode* construct_expression(TokenListNode*);

ProgramNode parse(TokenList* tokens)
{
  ProgramNode prgm;
  prgm.main = NULL;
  TokenListNode* curr = tokens->first;
  State state = GLOBAL;
  int depth = 0;
  int pdepth = 0;
  FunctionNode* current_func;
  current_func = NULL;

  while(curr) {
    Token tok = curr->tok;
    switch(tok.type) {
    case LEFT_BRACE:
      depth++;
      break;
    case RIGHT_BRACE:
      depth--;
      if(state == FUNCTION && depth == 0) {
        state = GLOBAL;
      }
      break;
    case LEFT_PAREN:
      pdepth++;
      break;
    case RIGHT_PAREN:
      pdepth--;
      break;
    case SEMICOLON:
      break;
    case INT:
      if(state == GLOBAL) {
        if(curr->next->tok.type != IDENTIFIER) {
          print_error("Expected name next to int.");
        }
        if(curr->next->next->tok.type != LEFT_PAREN) {
          print_error("Cannot handle global vars.");
        }
        if(strncmp(curr->next->tok.value, "main", 5)) {
          print_error("Can only declare main function for now.");
        }
        if(prgm.main != NULL) {
          print_error("More than one main function.");
        }
        prgm.main = construct_function(curr);
        current_func = prgm.main;
        state = FUNCTION;
        while(curr->tok.type != LEFT_BRACE) {
          curr = curr->next;
          if(!curr) {
            print_error("Missing brace.");
          }
        }
        depth++;
      } else {
        print_error("Cannot handle non-global use of int keyword.");
      }
      break;
    case RETURN:
      if(state == GLOBAL) {
        print_error("Cannot have return in global scope.");
      }
      if(current_func->num_statements == current_func->capacity) {
        current_func->body = realloc(current_func->body,
                                     sizeof(StatementNode*)*2*current_func->capacity);
        current_func->capacity *= 2;
      }
      current_func->body[current_func->num_statements++] = construct_statement(curr);
      while(curr->tok.type != SEMICOLON) {
        curr = curr->next;
        if(!curr) {
          print_error("Missing semicolon.");
        }
      }
      break;
    case IDENTIFIER:
      print_error("Cannot handle most statements right now.");
      break;
    case INT_LITERAL:
    case HEX_LITERAL:
    case OCT_LITERAL:
      break;
    default:
      print_error("Cannot handle this token.");
      break;
    }
    curr = curr->next;
  }
  if (state != GLOBAL) {
    print_error("Still in function at end of token list. Missing brace?");
  }
  return prgm;
}

void print_error(const char * msg)
{
  puts(msg);
  exit(1);
}

FunctionNode* construct_function(TokenListNode* curr)
{
  FunctionNode* func = malloc(sizeof(FunctionNode));
  if(!func) {
    perror("Error");
    exit(1);
  }
  func->name = curr->next->tok.value;
  switch(curr->tok.type) {
  case INT:
    func->type = INT_RET;
    break;
  default:
    print_error("Can only have funcs that return int.");
  }
  func->body = NULL;
  func->num_statements = 0;
  curr = curr->next->next;
  int left_paren = 0;
  int right_paren = 0;
  while(curr->tok.type != LEFT_BRACE) {
    switch(curr->tok.type) {
    case LEFT_PAREN:
      left_paren++;
      break;
    case RIGHT_PAREN:
      right_paren++;
      break;
    default:
      print_error("Cannot handle arguments right now.");
      break;
    }
    curr = curr->next;
  }
  if (left_paren != 1 || right_paren != 1) {
    print_error("Ill formed function declaration. Check parenthesis.");
  }
  func->capacity = 1;
  func->body = calloc(1, sizeof(StatementNode*));
  return func;
}

StatementNode* construct_statement(TokenListNode* curr)
{
  StatementNode* stmt = malloc(sizeof(StatementNode));
  if(!stmt) {
    perror("Error");
    exit(1);
  }
  switch(curr->tok.type) {
  case RETURN:
    stmt->type = RETURN_STATEMENT;
    stmt->return_value = construct_expression(curr->next);
    break;
  default:
    print_error("Can only handle returns statements right now.");
    break;
  }
  while(curr->tok.type != SEMICOLON) {
    if(curr->next == NULL) {
      print_error("Missing semicolon?");
    }
    curr = curr->next;
  }
  return stmt;
}

ExpressionNode* construct_expression(TokenListNode* curr)
{
  ExpressionNode* exp = malloc(sizeof(ExpressionNode));
  if(!exp) {
    perror("Error");
    exit(1);
  }
  int value_set = 0;
  if(curr->tok.type == SEMICOLON) {
    print_error("Invalid expression");
  }
  while(curr->tok.type != SEMICOLON) {
    char* dummy;
    if(curr->next == NULL) {
      print_error("Missing semicolon?");
    }
    if(value_set) {
      curr = curr->next;
      continue;
    }
    switch(curr->tok.type) {
    case INT_LITERAL:
      exp->type = INT_EXP;
      exp->int_value = atoi(curr->tok.value);
      value_set = 1;
      break;
    case HEX_LITERAL:
      exp->type = INT_EXP;
      exp->int_value = strtol(curr->tok.value, &dummy, 16);
      value_set = 1;
      break;
    case OCT_LITERAL:
      exp->type = INT_EXP;
      exp->int_value = strtol(curr->tok.value, &dummy, 8);
      value_set = 1;
      break;
    case LOGICAL_NOT:
      exp->type = LOG_NOT;
      exp->unary_operand = construct_expression(curr->next);
      value_set = 1;
      break;
    case BITWISE_NOT:
      exp->type = BIT_NOT;
      exp->unary_operand = construct_expression(curr->next);
      value_set = 1;
      break;
    case MINUS:
      exp->type = NEGATE;
      exp->unary_operand = construct_expression(curr->next);
      value_set = 1;
      break;
    default:
      print_error("Can only handle int literals right now.");
    }
    curr = curr->next;
  }
  return exp;
}
