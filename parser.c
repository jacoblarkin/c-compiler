#include "parser.h"
#include "token.h"
#include "c_lang.h"
#include "symbol.h"

#include <stdio.h>
#include <string.h>

typedef enum State_t {
  GLOBAL,
  FUNCTION
} State;

void print_error(const char*);
FunctionNode* construct_function(Type);
BlockNode* construct_block();
DeclarationNode* construct_declaration(Token);
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
Type parse_type(Token);
int operator_precedence(Token);
int right_assoc_operator(Token);
int find_right_paren(void);
void handle_ternary(void);

TokenList* tokens;

SymbolTable* main_st;
SymbolTable* top_st;

ProgramNode parse(TokenList* _tokens)
{
  tokens = _tokens;
  ProgramNode prgm;
  prgm.main = NULL;

  while(!token_list_empty(tokens)) {
    Token tok = token_list_pop_front(tokens);
    switch(tok.type) {
    case SEMICOLON:
      break;
    case INT_TOK:
      if(token_list_peek_n(tokens, 1).type != LEFT_PAREN) {
        print_error("Cannot handle global vars.");
      }
      if(strcmp(token_list_peek_front(tokens).value, "main") != 0) {
        print_error("Can only declare main function for now.");
      }
      if(prgm.main != NULL) {
        print_error("More than one main function.");
      }
      Type main_type = {.base = INT_VAR, .cvr = 0, .storage = 0, .signed_ = 1};
      top_st = NULL;
      prgm.main = construct_function(main_type);
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

FunctionNode* construct_function(Type fn_type)
{
  FunctionNode* func = malloc(sizeof(FunctionNode));
  if(!func) {
    perror("Error");
    exit(1);
  }
  Token fn_name = token_list_pop_front(tokens);
  func->name = fn_name.value;
  func->type = fn_type;
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
  int returned = 0;
  func->body = construct_block();
  BlockItem* last_item = NULL;
  if(func->body->count > 0) {
    last_item = func->body->body[func->body->count - 1];
  }
  if(last_item && last_item->type == STATEMENT_ITEM 
      && last_item->stmt->type == RETURN_STATEMENT) {
    returned = 1;
  }
  if(!returned) {
    BlockItem* blck_item = malloc(sizeof(BlockItem));
    StatementNode* stmt = malloc(sizeof(StatementNode));
    ExpressionNode* zero = malloc(sizeof(ExpressionNode));
    blck_item->type = STATEMENT_ITEM;
    stmt->type = RETURN_STATEMENT;
    zero->type = INT_VALUE;
    zero->int_value = 0;
    stmt->expression = zero;
    blck_item->stmt = stmt;
    if(func->body->count == func->body->capacity) {
      func->body->capacity++;
      func->body->body = realloc(func->body->body,
                           sizeof(BlockItem*) * func->body->capacity);
    }
    func->body->body[func->body->count++] = blck_item;
  }
  return func;
}

BlockNode* construct_block()
{
  BlockNode* blck = malloc(sizeof(BlockNode));
  if(!blck) {
    perror("Error");
    exit(1);
  }
  blck->count = 0;
  blck->capacity = 1;
  blck->body = calloc(1, sizeof(BlockItem*));
  SymbolTable* block_st = malloc(sizeof(SymbolTable));
  block_st->top = NULL;
  block_st->next = top_st;
  top_st = block_st;
  push_constructed_symbol(NULL, 0, block_st);
  Token st_begin = token_list_pop_front(tokens);
  while(st_begin.type != RIGHT_BRACE) {
    if(blck->count == blck->capacity) {
      blck->capacity *= 2;
      blck->body = realloc(blck->body,
                           sizeof(BlockItem*) * blck->capacity);
    }
    BlockItem* item = malloc(sizeof(BlockItem));
    if(token_structs[st_begin.type].declaration) {
      item->decl = construct_declaration(st_begin);
      item->type = DECLARATION_ITEM;
    } else {
      item->stmt = construct_statement(st_begin);
      item->type = STATEMENT_ITEM;
    }
    blck->body[blck->count++] = item;
    st_begin = token_list_pop_front(tokens);
  }
  top_st = block_st->next;
  delete_symbol_table(block_st);
  return blck;
}

Type parse_type(Token first_tok)
{
  Type var_type = (Type){.base = UNKNOWN_VAR, .cvr = NO_CVRQUAL,
                           .storage = NO_STORAGEQUAL, .signed_ = 1};
  Token curr_tok = first_tok;
  int signedness_set = 0;
  while(token_structs[curr_tok.type].syntax == KEYWORD_ST) {
    switch(curr_tok.type) {
    case CHAR_TOK:
      if(var_type.base != UNKNOWN_VAR) {
        print_error("Multiple types in var declaration");
      }
      var_type.base = CHAR_VAR;
      break;
    case SHORT_TOK:
      if(var_type.base != UNKNOWN_VAR && var_type.base != INT_VAR) {
        print_error("Multiple types in var declaration");
      }
      var_type.base = SHORT_VAR;
      break;
    case INT_TOK:
      if(var_type.base != UNKNOWN_VAR && (var_type.base != SHORT_VAR
            && var_type.base != LONG_VAR)) {
        print_error("Multiple types in var declaration");
      } else if(var_type.base == UNKNOWN_VAR) {
        var_type.base = INT_VAR; 
      }
      break;
    case LONG_TOK:
      if(var_type.base != UNKNOWN_VAR && (var_type.base != INT_VAR 
            && var_type.base != LONG_VAR)) {
        print_error("Multiple types in var declaration");
      } else if(var_type.base == LONG_VAR) {
        var_type.base = LONG_LONG_VAR;
      } else {
        var_type.base = LONG_VAR;
      }
      break;
    case AUTO_TOK:
      if(var_type.storage != NO_STORAGEQUAL) {
        print_error("Multiple storage qualifiers in var declaration");
      }
      var_type.storage = AUTO_STORAGEQUAL;
      break;
    case EXTERN_TOK:
      if(var_type.storage != NO_STORAGEQUAL) {
        print_error("Multiple storage qualifiers in var declaration");
      }
      var_type.storage = EXTERN_STORAGEQUAL;
      break;
    case STATIC_TOK:
      if(var_type.storage != NO_STORAGEQUAL) {
        print_error("Multiple storage qualifiers in var declaration");
      }
      var_type.storage = STATIC_STORAGEQUAL;
      break;
    case REGISTER_TOK:
      if(var_type.storage != NO_STORAGEQUAL) {
        print_error("Multiple storage qualifiers in var declaration");
      }
      var_type.storage = REGISTER_STORAGEQUAL;
      break;
    case CONST_TOK:
      var_type.cvr |= CONST_CVRQUAL;
      break;
    case VOLATILE_TOK:
      var_type.cvr |= VOLATILE_CVRQUAL;
      break;
    case RESTRICT_TOK:
      var_type.cvr |= RESTRICT_CVRQUAL;
      break;
    case SIGNED_TOK:
      if(signedness_set) {
        print_error("Multiple signededness qualifiers in declaration");
      }
      signedness_set = 1;
      var_type.signed_ = 1;
      break;
    case UNSIGNED_TOK:
      if(signedness_set) {
        print_error("Multiple signededness qualifiers in declaration");
      }
      signedness_set = 1;
      var_type.signed_ = 0;
      break;
    default:
      print_error("Uknown Type");
    }
    curr_tok = token_list_pop_front(tokens);
  }
  if(var_type.base == UNKNOWN_VAR && signedness_set) {
    var_type.base = INT_VAR;
  }
  token_list_push_front(tokens, curr_tok);
  return var_type;
}

int compatible_types(Type from, Type to)
{
  if(from.signed_ != to.signed_) {
    return 0;
  }
  if(from.base > to.base){
    return 0;
  }
  return 1;
}

DeclarationNode* construct_declaration(Token first_tok)
{
  DeclarationNode* decl = malloc(sizeof(DeclarationNode));
  if(!decl) {
    perror("Error");
    exit(1);
  } 
  decl->var_type = parse_type(first_tok);
  Token name = token_list_pop_front(tokens);
  if(name.type != IDENTIFIER) {
    print_error("Expected identifier to declar var.");
  }
  decl->var_name = name.value;
  push_constructed_typed_symbol(name.value, decl->var_type, top_st);
  decl->assignment_expression = NULL;
  Token assign = token_list_peek_front(tokens);
  if(assign.type == ASSIGN) {
    token_list_push_front(tokens, name);
    decl->assignment_expression = construct_expression();
  }
  Token semicolon = token_list_pop_front(tokens);
  if (semicolon.type != SEMICOLON) {
    print_error("Invalid statement.");
  }
  return decl;
}

StatementNode* construct_statement(Token first_tok)
{
  StatementNode* stmt = malloc(sizeof(StatementNode));
  if(!stmt) {
    perror("Error");
    exit(1);
  }
  Token semicolon;
  Token next;
  SymbolTable* for_st = NULL;
  static int in_switch = 0;
  static int switch_signed = 1;
  switch(first_tok.type) {
  case RETURN_TOK:
    stmt->type = RETURN_STATEMENT;
    stmt->expression = construct_expression();
    if(stmt->expression->type == EMPTY_EXP) {
      print_error("Must have return value right now.");
    }
    semicolon = token_list_pop_front(tokens);
    if (semicolon.type != SEMICOLON) {
      print_error("Invalid statement.");
    }
    break;
  case IF_TOK:
    stmt->type = CONDITIONAL;
    if(token_list_peek_front(tokens).type != LEFT_PAREN) {
      print_error("Invalid if statement.");
    }
    stmt->condition = parse_primary_expression();
    next = token_list_pop_front(tokens);
    stmt->if_stmt = construct_statement(next);
    stmt->else_stmt = NULL;
    next = token_list_peek_front(tokens);
    if(next.type == ELSE_TOK) {
      token_list_pop_front(tokens); // else
      next = token_list_pop_front(tokens);
      stmt->else_stmt = construct_statement(next);
    }
    break;
  case WHILE_TOK:
    stmt->type = WHILE_LOOP;
    if(token_list_peek_front(tokens).type != LEFT_PAREN) {
      print_error("Invalid while statement.");
    }
    stmt->loop_condition = parse_primary_expression();
    next = token_list_pop_front(tokens);
    stmt->loop_stmt = construct_statement(next);
    break;
  case DO_TOK:
    stmt->type = DO_LOOP;
    next = token_list_pop_front(tokens);
    stmt->loop_stmt = construct_statement(next);
    next = token_list_pop_front(tokens);
    if(next.type != WHILE_TOK) {
      print_error("Invalid do statement.");
    }
    if(token_list_peek_front(tokens).type != LEFT_PAREN) {
      print_error("Invalid while statement.");
    }
    stmt->loop_condition = parse_primary_expression();
    next = token_list_pop_front(tokens);
    if(next.type != SEMICOLON) {
      print_error("Invalid while statement. Missing ; at end.");
    }
    break;
  case FOR_TOK:
    next = token_list_pop_front(tokens);
    if(next.type != LEFT_PAREN) {
      print_error("Invalid for statement. Missing left paren.");
    }
    next = token_list_pop_front(tokens);
    if(next.type == INT_TOK) {
      stmt->type = FORDECL_LOOP;
      for_st = malloc(sizeof(SymbolTable));
      for_st->top = NULL;
      for_st->next = top_st;
      top_st = for_st;
      push_constructed_symbol(NULL, 0, for_st);
      stmt->init_decl = construct_declaration(next);
    } else {
      stmt->type = FOR_LOOP;
      token_list_push_front(tokens, next);
      stmt->init_exp = construct_expression();
      next = token_list_pop_front(tokens);
      if(next.type != SEMICOLON) {
        print_error("Invalid for statement. Missing first ;.");
      }
    }
    stmt->loop_condition = construct_expression();
    next = token_list_pop_front(tokens);
    if(next.type != SEMICOLON) {
      print_error("Invalid for statement. Missing second ;.");
    }
    if(token_list_peek_front(tokens).type == RIGHT_PAREN) {
      stmt->post_exp = malloc(sizeof(ExpressionNode));
      stmt->post_exp->type = EMPTY_EXP;
    } else {
      stmt->post_exp = construct_expression();
    }
    next = token_list_pop_front(tokens);
    if(next.type != RIGHT_PAREN) {
      print_error("Invalid for statement. Missing right paren.");
    }
    next = token_list_pop_front(tokens);
    stmt->loop_stmt = construct_statement(next);
    break;
  case BREAK_TOK:
    stmt->type = BREAK_STATEMENT;
    semicolon = token_list_pop_front(tokens);
    if (semicolon.type != SEMICOLON) {
      print_error("Break statement missing ;.");
    }
    break;
  case CONTINUE_TOK:
    stmt->type = CONTINUE_STATEMENT;
    semicolon = token_list_pop_front(tokens);
    if (semicolon.type != SEMICOLON) {
      print_error("Continue statement missin ;.");
    }
    break;
  case LEFT_BRACE:
    stmt->type = BLOCK_STATEMENT;
    stmt->block = construct_block();
    break;
  case SWITCH_TOK:
    stmt->type = SWITCH_STATEMENT;
    if(token_list_peek_front(tokens).type != LEFT_PAREN) {
      print_error("Invalid switch statement. Missing paren.");
    }
    stmt->switch_exp = parse_primary_expression();
    next = token_list_pop_front(tokens);
    if(next.type != LEFT_BRACE) {
      print_error("Invalid switch statement. Missing brace.");
    }
    in_switch = 1;
    stmt->switch_block = construct_block();
    in_switch = 0;
    break;
  case CASE_TOK:
    stmt->type = CASE_STATEMENT;
    if(!in_switch) {
      print_error("Use of case outside of switch statement.");
    }
    next = token_list_pop_front(tokens);
    if(token_structs[next.type].syntax != LITERAL_ST) {
      print_error("Expected literal convertable to long.");
    }
    if(switch_signed) {
      ExpressionNode* tmp = parse_number(next);
      stmt->val = tmp->int_value;
      free(tmp);
    } else {
      ExpressionNode* tmp = parse_number(next);
      stmt->unsigned_val = tmp->int_value;
      free(tmp);
    }
    next = token_list_pop_front(tokens);
    if(next.type != COLON) {
      print_error("Expected : after case.");
    }
    break;
  case DEFAULT_TOK:
    stmt->type = DEFAULT_STATEMENT;
    next = token_list_pop_front(tokens);
    if(next.type != COLON) {
      print_error("Expected : after default.");
    }
    if(!in_switch) {
      print_error("Use of default tag outside of switch statement.");
    }
    break;
  case GOTO_TOK:
    stmt->type = GOTO_STATEMENT;
    next = token_list_pop_front(tokens);
    if(next.type != IDENTIFIER) {
      print_error("Expected label for goto.");
    }
    stmt->label_name = next.value;
    semicolon = token_list_pop_front(tokens);
    if (semicolon.type != SEMICOLON) {
      print_error("Goto statement missing ;.");
    }
    break;
  case IDENTIFIER:
    if(token_list_peek_front(tokens).type == COLON) {
      stmt->type = LABEL;
      stmt->label_name = first_tok.value;
      token_list_pop_front(tokens);
      break;
    }
    // Intentional fallthrough to handle else case
  default:
    stmt->type = EXPRESSION;
    token_list_push_front(tokens, first_tok);
    stmt->expression = construct_expression();
    semicolon = token_list_pop_front(tokens);
    if (semicolon.type != SEMICOLON) {
      print_error("Invalid statement.");
    }
    break;
  }
  if(for_st) {
    top_st = for_st->next;
    delete_symbol_table(for_st);
  }
  return stmt;
}

ExpressionNode* construct_expression()
{
  Token first = token_list_peek_front(tokens);
  ExpressionNode* exp = NULL;
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
  case SEMICOLON:
    exp = malloc(sizeof(ExpressionNode));
    exp->type = EMPTY_EXP;
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
    number->value_type.base = INT_VAR;
    number->value_type.signed_ = 1;
    break;
  case HEX_LITERAL:
    number->type = INT_VALUE;
    number->int_value = (int)strtoul(num.value, &dummy, 16);
    number->value_type.base = INT_VAR;
    number->value_type.signed_ = 1;
    break;
  case OCT_LITERAL:
    number->type = INT_VALUE;
    number->int_value = (int)strtoul(num.value, &dummy, 8);
    number->value_type.base = INT_VAR;
    number->value_type.signed_ = 1;
    break;
  case CHAR_LITERAL:
    number->type = CHAR_VALUE;
    number->char_value = num.value[1];
    number->value_type.base = INT_VAR;
    number->value_type.signed_ = 1;
    break;
  case UINT_LITERAL:
    number->type = UINT_VALUE;
    number->uint_value = (int)strtoul(num.value, &dummy, 10);
    number->value_type.base = INT_VAR;
    number->value_type.signed_ = 0;
    break;
  case LONG_LITERAL:
    number->type = LONG_VALUE;
    printf("Long literal %s\n", num.value);
    if(num.value[strlen(num.value)-1] == 'l' ||
        num.value[strlen(num.value)-1] == 'L') {
      num.value[strlen(num.value)-1] = '\0';
    }
    number->long_value = atol(num.value);
    number->value_type.base = LONG_VAR;
    number->value_type.signed_ = 1;
    break;
  case ULONG_LITERAL:
    number->type = ULONG_VALUE;
    number->ulong_value = strtoul(num.value, &dummy, 10);
    number->value_type.base = LONG_VAR;
    number->value_type.signed_ = 0;
    break;
  case LONGLONG_LITERAL:
    number->type = LONGLONG_VALUE;
    number->longlong_value = atoll(num.value);
    number->value_type.base = LONG_LONG_VAR;
    number->value_type.signed_ = 1;
    break;
  case ULONGLONG_LITERAL:
    number->type = ULONGLONG_VALUE;
    number->ulonglong_value = strtoull(num.value, &dummy, 10);
    number->value_type.base = LONG_LONG_VAR;
    number->value_type.signed_ = 0;
    break;
  default:
    print_error("This is not a number");
  }
  free(num.value);
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
  unary_op->value_type = unary_op->unary_operand->value_type;
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
  Symbol sym = {.name = NULL};
  SymbolTable* st = top_st;
  while(!sym.name) {
    sym = find_symbol(var.value, st);
    if(!st->next && !sym.name) {
      print_error("Var not found!");
    }
    st = st->next;
  }
  variable->value_type = sym.type;
  Token next = token_list_peek_front(tokens);
  if(next.type == PLUSPLUS) {
    ExpressionNode* pp = malloc(sizeof(ExpressionNode));
    pp->type = POSTINC_EXP;
    pp->unary_operand = variable;
    pp->value_type = variable->value_type;
    token_list_pop_front(tokens);
    return pp;
  } else if(next.type == MINUSMINUS) {
    ExpressionNode* mm = malloc(sizeof(ExpressionNode));
    mm->type = POSTDEC_EXP;
    mm->unary_operand = variable;
    mm->value_type = variable->value_type;
    token_list_pop_front(tokens);
    return mm;
  }
  return variable;
}

ExpressionNode* parse_primary_expression()
{
  Token tok = token_list_pop_front(tokens);
  TokenType type = tok.type;
  switch(token_structs[type].syntax) {
  case LITERAL_ST:
    return parse_number(tok);
  case OPERATOR_ST:
    return parse_unary_operator(tok);
  case IDENTIFIER_ST:
    return parse_var(tok);
  case BRACE_ST:
    if (type != LEFT_PAREN) {
      print_error("Invalid Expression. Expected (.");
    }
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
  if(lookahead.type == QMARK) {
    handle_ternary();
  }
  while(operator_precedence(lookahead) >= min_precedence) {
    Token op = token_list_pop_front(tokens);
    ExpressionNode* rhs = parse_primary_expression();
    lookahead = token_list_peek_front(tokens);
    while(operator_precedence(lookahead) > operator_precedence(op)
          || (right_assoc_operator(lookahead) 
              && operator_precedence(lookahead) == operator_precedence(op))) {
      int next_prec = operator_precedence(op);
      if(!right_assoc_operator(lookahead)) {
        next_prec++;
      }
      rhs = parse_operators_impl(rhs, next_prec);
      lookahead = token_list_peek_front(tokens);
    }
    lhs = construct_binary_expression(op, lhs, rhs);
  }
  return lhs;
}

void handle_ternary()
{
  TokenListNode* curr = tokens->first;
  Token left = {.type = LEFT_PAREN, .value = NULL};
  Token right = {.type = RIGHT_PAREN, .value = NULL};
  TokenList mock;
  mock.first = tokens->first->next;
  mock.last = tokens->first->next;
  token_list_push_front(&mock, left);
  mock.first->prev = curr;
  curr->next = mock.first;
  int numQ = 1;
  int numC = 0;
  while(numQ > numC) {
    switch(mock.last->tok.type) {
    case QMARK:
      numQ++;
      break;
    case COLON:
      numC++;
      break;
    case SEMICOLON:
      print_error("Incomplete ternary operator expression.");
      break;
    default:
      break;
    }
    if(!mock.last->next) {
      print_error("Incomplete ternary operator expression.");
    }
    mock.last = mock.last->next;
  }
  curr = mock.last->prev;
  mock.last = mock.last->prev->prev;
  token_list_push(&mock, right);
  curr->prev = mock.last;
  mock.last->next = curr;
}

int right_assoc_operator(Token op)
{
  return token_structs[op.type].right_assoc;
}

int operator_precedence(Token op)
{
  return token_structs[op.type].precedence;
}

Type larger_type(Type lhs, Type rhs)
{
  if(lhs.base > rhs.base) return lhs;
  if(rhs.base > lhs.base) return rhs;
  if(lhs.signed_) return rhs;
  return lhs;
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
  if(binary_exp->type == COND_EXP && op.type == QMARK) {
    binary_exp->condition = lhs;
    binary_exp->if_exp = rhs;
    binary_exp->else_exp = NULL;
    return binary_exp;
  } else if(binary_exp->type == COND_EXP && op.type == COLON) {
    free(binary_exp);
    if(lhs->type != COND_EXP || lhs->else_exp) {
      print_error("Invalid conditional expression.");
    }
    lhs->else_exp = rhs;
    lhs->value_type = larger_type(lhs->if_exp->value_type, rhs->value_type);
    return lhs;
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
  binary_exp->value_type = larger_type(lhs->value_type, rhs->value_type);
  return binary_exp;
}
