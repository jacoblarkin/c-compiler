#include "parser.h"
#include "token.h"
#include "c_lang.h"

#include <stdio.h>
#include <string.h>

typedef enum State_t {
  GLOBAL,
  FUNCTION
} State;

void print_error(const char*);
FunctionNode* construct_function(CType);
StatementNode* construct_statement(Token);
ExpressionNode* construct_expression(void);
ExpressionNode* parse_primary_expression(void);
ExpressionNode* parse_operators(void);
ExpressionNode* parse_operators_impl(ExpressionNode*, int);
ExpressionNode* construct_binary_expression(Token, ExpressionNode*, ExpressionNode*);
ExpressionNode* parse_number(Token);
ExpressionNode* parse_unary_operator(Token);
ExpressionNode* parse_assignment(void);
ExpressionNode* parse_var(Token);
int operator_precedence(Token);
int right_to_left_operator(Token);
int find_right_paren(void);

TokenList* tokens;

ProgramNode parse(TokenList* _tokens)
{
  tokens = _tokens;
  ProgramNode prgm;
  prgm.main = NULL;
  FunctionNode* current_func;
  current_func = NULL;

  while(!token_list_empty(tokens)) {
    Token tok = token_list_pop_front(tokens);
    switch(tok.type) {
    case SEMICOLON:
      break;
    case INT_TOK:
      if(token_list_peek_n(tokens, 1).type != LEFT_PAREN) {
        print_error("Cannot handle global vars.");
      }
      if(strncmp(token_list_peek_front(tokens).value, "main", 5)) {
        print_error("Can only declare main function for now.");
      }
      if(prgm.main != NULL) {
        print_error("More than one main function.");
      }
      CType main_type = {.base = INT_VAR, .mods = NO_MOD};
      prgm.main = construct_function(main_type);
      current_func = prgm.main;
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
  }
  return prgm;
}

void print_error(const char * msg)
{
  puts(msg);
  exit(1);
}

FunctionNode* construct_function(CType fn_type)
{
  FunctionNode* func = malloc(sizeof(FunctionNode));
  if(!func) {
    perror("Error");
    exit(1);
  }
  Token fn_name = token_list_pop_front(tokens);
  func->name = fn_name.value;
  switch(fn_type.base) {
  case INT_VAR:
    func->type = INT_RET;
    break;
  default:
    print_error("Can only have funcs that return int.");
  }
  func->body = NULL;
  func->num_statements = 0;
  int left_paren = 0;
  int right_paren = 0;
  Token plist = token_list_pop_front(tokens);
  while(plist.type != LEFT_BRACE) {
    switch(plist.type) {
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
    plist = token_list_pop_front(tokens);
  }
  if (left_paren != 1 || right_paren != 1) {
    print_error("Ill formed function declaration. Check parenthesis.");
  }
  func->capacity = 1;
  func->body = calloc(1, sizeof(StatementNode*));
  Token st_begin = token_list_pop_front(tokens);
  int returned = 0;
  while(st_begin.type != RIGHT_BRACE) {
    if(func->num_statements == func->capacity) {
      func->capacity *= 2;
      func->body = realloc(func->body,
                           sizeof(StatementNode*) * func->capacity);
    }
    if(st_begin.type == RETURN_TOK) {
      returned = 1;
    }
    func->body[func->num_statements++] = construct_statement(st_begin);
    st_begin = token_list_pop_front(tokens);
  }
  if(!returned) {
    StatementNode* stmt = malloc(sizeof(StatementNode));
    ExpressionNode* zero = malloc(sizeof(ExpressionNode));
    stmt->type = RETURN_STATEMENT;
    zero->type = INT_VALUE;
    zero->int_value = 0;
    stmt->return_value = zero;
    if(func->num_statements == func->capacity) {
      func->capacity++;
      func->body = realloc(func->body,
                           sizeof(StatementNode*) * func->capacity);
    }
    func->body[func->num_statements++] = stmt;
  }
  return func;
}

StatementNode* construct_statement(Token first_tok)
{
  StatementNode* stmt = malloc(sizeof(StatementNode));
  if(!stmt) {
    perror("Error");
    exit(1);
  }
  switch(first_tok.type) {
  case RETURN_TOK:
    stmt->type = RETURN_STATEMENT;
    stmt->return_value = construct_expression();
    break;
  case INT_TOK:
    stmt->type = DECLARATION;
    stmt->var_type = INT_VAR;
    Token name = token_list_peek_front(tokens);
    if(name.type != IDENTIFIER) {
      print_error("Expected identifier to declar var.");
    }
    stmt->var_name = name.value;
    stmt->assignment_expression = NULL;
    Token assign = token_list_peek_n(tokens, 1);
    if(assign.type == ASSIGN) {
      stmt->assignment_expression = construct_expression();
    } else {
      token_list_pop_front(tokens);
    }
    break;
  default:
    stmt->type = EXPRESSION;
    token_list_push_front(tokens, first_tok);
    stmt->expression = construct_expression();
    break;
  }
  Token semicolon = token_list_pop_front(tokens);
  if (semicolon.type != SEMICOLON) {
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
  Token first = token_list_peek_front(tokens);
  if(first.type == SEMICOLON) {
    print_error("Invalid expression");
  }
  switch(first.type) {
  case INT_LITERAL:
  case HEX_LITERAL:
  case OCT_LITERAL:
  case LOGICAL_NOT:
  case BITWISE_NOT:
  case MINUS:
  case LEFT_PAREN:
  case IDENTIFIER:
    exp = parse_operators();
    break;
  default:
    print_error("Can only handle some expressions right now.");
  }
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
  return number;
}

ExpressionNode* parse_unary_operator(Token op)
{
  ExpressionNode* unary_op = malloc(sizeof(ExpressionNode));
  if(!unary_op) {
    perror("Error");
    exit(1);
  }
  unary_op->type = token_structs[op.type].primary_type;
  if(unary_op->type == UNKNOWN_EXP) {
    print_error("Not a unary operator.");
  }
  unary_op->unary_operand = parse_primary_expression();
  if((unary_op->type == PREINC_EXP || unary_op->type == PREDEC_EXP)
      && unary_op->unary_operand->type != VAR_EXP) {
    print_error("Pre Inc/Dec must act on variable.");
  }
  return unary_op;
}

ExpressionNode* parse_var(Token var)
{
  ExpressionNode* variable = malloc(sizeof(ExpressionNode));
  if(!variable) {
    perror("Error");
    exit(1);
  }
  variable->type = VAR_EXP;
  variable->var_name = var.value;
  Token next = token_list_peek_front(tokens);
  if(next.type == PLUSPLUS) {
    ExpressionNode* pp = malloc(sizeof(ExpressionNode));
    pp->type = POSTINC_EXP;
    pp->unary_operand = variable;
    token_list_pop_front(tokens);
    return pp;
  } else if(next.type == MINUSMINUS) {
    ExpressionNode* mm = malloc(sizeof(ExpressionNode));
    mm->type = POSTDEC_EXP;
    mm->unary_operand = variable;
    token_list_pop_front(tokens);
    return mm;
  }
  return variable;
}

ExpressionNode* parse_primary_expression()
{
  Token tok = token_list_pop_front(tokens);
  switch(tok.type) {
  case INT_LITERAL:
  case HEX_LITERAL:
  case OCT_LITERAL:
    return parse_number(tok);
  case LOGICAL_NOT:
  case BITWISE_NOT:
  case MINUS:
  case PLUSPLUS:
  case MINUSMINUS:
    return parse_unary_operator(tok);
  case IDENTIFIER:
    return parse_var(tok);
  case LEFT_PAREN:
    if(!find_right_paren()) {
      print_error("Missing parenthesis");
    }
    ExpressionNode* tmp = parse_operators();
    token_list_pop_front(tokens); // Pop right paren
    return tmp;
  default: 
    print_error("Cannot parse this primary expression.");
  }
  return NULL;
}

int find_right_paren()
{
  TokenListNode* tmp = tokens->first;
  int paren_depth = 1;
  while(paren_depth > 0) {
    if(tmp->tok.type == RIGHT_PAREN) {
      paren_depth--;
    } else if (tmp->tok.type == LEFT_PAREN) {
      paren_depth++;
    }
    if(tmp->tok.type == SEMICOLON) {
      return 0;
    }
    tmp = tmp->next;
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
  Token lookahead = token_list_peek_front(tokens);
  while(operator_precedence(lookahead) >= min_precedence) {
    Token op = token_list_pop_front(tokens);
    ExpressionNode* rhs = parse_primary_expression();
    lookahead = token_list_peek_front(tokens);
    while(operator_precedence(lookahead) > operator_precedence(op)
          || (right_to_left_operator(lookahead) 
              && operator_precedence(lookahead) == operator_precedence(op))) {
      int next_prec = operator_precedence(op);
      if(!right_to_left_operator(lookahead)) {
        next_prec++;
      }
      rhs = parse_operators_impl(rhs, next_prec);
      lookahead = token_list_peek_front(tokens);
    }
    lhs = construct_binary_expression(op, lhs, rhs);
  }
  return lhs;
}

int right_to_left_operator(Token op)
{
  return token_structs[op.type].right_assoc;
}

int operator_precedence(Token op)
{
  return token_structs[op.type].precedence;
}

ExpressionNode* construct_binary_expression(Token op, ExpressionNode* lhs,
                                            ExpressionNode* rhs)
{
  ExpressionNode* binary_exp = malloc(sizeof(ExpressionNode));
  if(!binary_exp) {
    perror("Error");
    exit(1);
  }
  binary_exp->type = token_structs[op.type].binary_type;
  if(binary_exp->type == UNKNOWN_EXP) {
    print_error("Unknown binary expression.");
  }
  if(token_structs[op.type].need_lvalue 
      && lhs->type != VAR_EXP) {
    print_error("Invalid lvalue");
  }
  if(!lhs || !rhs) {
    print_error("Error constructing binary expression");
  }
  binary_exp->left_operand = lhs;
  binary_exp->right_operand = rhs;
  return binary_exp;
}
