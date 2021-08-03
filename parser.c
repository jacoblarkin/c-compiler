#include "parser.h"
#include "token.h"

#include <stdio.h>
#include <string.h>

typedef enum State_t {
  GLOBAL,
  FUNCTION
} State;

void print_error(const char*);
FunctionNode* construct_function(void);
StatementNode* construct_statement(void);
ExpressionNode* construct_expression(void);
int calculate_num_tokens(ExpressionNode*);
ExpressionNode* parse_primary_expression(void);
ExpressionNode* parse_operators(void);
ExpressionNode* parse_operators_impl(ExpressionNode*, int);
int operator_precedence(Token);
ExpressionNode* construct_binary_expression(Token, ExpressionNode*, ExpressionNode*);
ExpressionNode* parse_number(Token);
ExpressionNode* parse_unary_operator(void);
int find_right_paren(void);

// Current Node in token list
TokenListNode* curr;

ProgramNode parse(TokenList* tokens)
{
  ProgramNode prgm;
  prgm.main = NULL;
  curr = tokens->first;
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
        prgm.main = construct_function();
        current_func = prgm.main;
        state = FUNCTION;
      } else {
        print_error("Cannot handle non-global use of int keyword.");
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

FunctionNode* construct_function()
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
  curr = curr->next;
  while(curr->tok.type != RIGHT_BRACE) {
    if(func->num_statements == func->capacity) {
      func->capacity *= 2;
      func->body = realloc(func->body,
                           sizeof(StatementNode*) * func->capacity);
    }
    func->body[func->num_statements++] = construct_statement();
  }
  return func;
}

StatementNode* construct_statement()
{
  StatementNode* stmt = malloc(sizeof(StatementNode));
  if(!stmt) {
    perror("Error");
    exit(1);
  }
  switch(curr->tok.type) {
  case RETURN:
    stmt->type = RETURN_STATEMENT;
    curr = curr->next;
    stmt->return_value = construct_expression();
    break;
  default:
    print_error("Can only handle returns statements right now.");
    break;
  }
  if (curr->tok.type != SEMICOLON) {
    print_error("Invalid statement.");
  }
  return stmt;
}

ExpressionNode* construct_expression()
{
  ExpressionNode* exp = malloc(sizeof(ExpressionNode));
  if(!exp) {
    perror("Error");
    exit(1);
  }
  if(curr->tok.type == SEMICOLON) {
    print_error("Invalid expression");
  }
  switch(curr->tok.type) {
  case INT_LITERAL:
  case HEX_LITERAL:
  case OCT_LITERAL:
  case LOGICAL_NOT:
  case BITWISE_NOT:
  case MINUS:
  case LEFT_PAREN:
    exp = parse_operators();
    break;
  default:
    print_error("Can only handle some expressions right now.");
  }
  //curr = curr->next;
  return exp;
}

ExpressionNode* parse_number(Token num)
{
  ExpressionNode* number = malloc(sizeof(ExpressionNode));
  if(!number) {
    perror("Error");
    exit(1);
  }
  char* dummy;
  switch(num.type) {
  case INT_LITERAL:
    number->type = INT_VALUE;
    number->int_value = atoi(num.value);
    break;
  case HEX_LITERAL:
    number->type = INT_VALUE;
    number->int_value = strtol(num.value, &dummy, 16);
    break;
  case OCT_LITERAL:
    number->type = INT_VALUE;
    number->int_value = strtol(num.value, &dummy, 8);
    break;
  default:
    print_error("This is not a number");
  }
  curr = curr->next;
  return number;
}

ExpressionNode* parse_unary_operator()
{
  ExpressionNode* unary_op = malloc(sizeof(ExpressionNode));
  if(!unary_op) {
    perror("Error");
    exit(1);
  }
  switch(curr->tok.type) {
  case LOGICAL_NOT:
    unary_op->type = LOG_NOT;
    curr = curr->next;
    unary_op->unary_operand = parse_primary_expression();
    break;
  case BITWISE_NOT:
    unary_op->type = BITWISE_COMP;
    curr = curr->next;
    unary_op->unary_operand = parse_primary_expression();
    break;
  case MINUS:
    unary_op->type = NEGATE;
    curr = curr->next;
    unary_op->unary_operand = parse_primary_expression();
    break;
  default:
    print_error("This is not a unary operator.");
  }
  return unary_op;
}

ExpressionNode* parse_primary_expression()
{
  Token tok = curr->tok;
  switch(tok.type) {
  case INT_LITERAL:
  case HEX_LITERAL:
  case OCT_LITERAL:
    return parse_number(tok);
  case LOGICAL_NOT:
  case BITWISE_NOT:
  case MINUS:
    return parse_unary_operator();
  case LEFT_PAREN:
    if(!find_right_paren()) {
      print_error("Missing parenthesis");
    }
    curr = curr->next;
    return parse_operators();
  default: 
    print_error("Cannot parse this primary expression.");
  }
  return NULL;
}

int find_right_paren()
{
  TokenListNode* tmp = curr;
  int paren_depth = 1;
  while(paren_depth > 0) {
    tmp = tmp->next;
    if(tmp->tok.type == RIGHT_PAREN) {
      paren_depth--;
    } else if (tmp->tok.type == LEFT_PAREN) {
      paren_depth++;
    }
    if(tmp->tok.type == SEMICOLON) {
      return 0;
    }
  }
  return 1;
}

ExpressionNode* parse_operators()
{
  ExpressionNode* lhs = parse_primary_expression();
  return parse_operators_impl(lhs, 0);
}

ExpressionNode* parse_operators_impl(ExpressionNode* lhs, int min_precedence)
{
  Token lookahead = curr->tok;
  while(operator_precedence(lookahead) >= min_precedence) {
    Token op = lookahead;
    curr = curr->next;
    // Expect parse_primary_expression to advance curr for us
    ExpressionNode* rhs = parse_primary_expression();
    lookahead = curr->tok;
    while(operator_precedence(lookahead) > operator_precedence(op)) {
      // Again, this will advance curr for us
      rhs = parse_operators_impl(rhs, min_precedence + 1);
      lookahead = curr->tok;
    }
    lhs = construct_binary_expression(op, lhs, rhs);
  }
  return lhs;
}

int operator_precedence(Token op)
{
  switch(op.type) {
  case STAR:
  case SLASH:
    return 2;
  case PLUS:
  case MINUS:
    return 1;
  case RIGHT_PAREN:
  case SEMICOLON:
    return -1;
  default: 
    return 0;
  }
}

ExpressionNode* construct_binary_expression(Token op, ExpressionNode* lhs,
                                            ExpressionNode* rhs)
{
  ExpressionNode* binary_exp = malloc(sizeof(ExpressionNode));
  if(!binary_exp) {
    perror("Error");
    exit(1);
  }
  switch(op.type) {
    case PLUS:
      binary_exp->type = ADD_BINEXP;
      break;
    case MINUS:
      binary_exp->type = SUB_BINEXP;
      break;
    case STAR:
      binary_exp->type = MUL_BINEXP;
      break;
    case SLASH:
      binary_exp->type = DIV_BINEXP;
      break;
    default:
      print_error("Unknown operator.");
  }
  binary_exp->left_operand = lhs;
  binary_exp->right_operand = rhs;
  return binary_exp;
}

int calculate_num_tokens(ExpressionNode* exp)
{
  switch(exp->type)
  {
  case INT_VALUE:
    return 1;
  case LOG_NOT:
  case BITWISE_COMP:
  case NEGATE:
    return 1 + calculate_num_tokens(exp->unary_operand);
  default:
    print_error("Unrecognized expression type");
  }
  return 0;
}
