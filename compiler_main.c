#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

#include <stdio.h>

void pretty_print(ProgramNode);
void print_lexemes(TokenList*);
void usage(void);
void print_expression(ExpressionNode*);

int main(int argc, char** argv)
{
  if(argc != 2) {
    usage();
    exit(1);
  }

  char* filename = argv[1];

  TokenList* lexemes = lex(filename);

  print_lexemes(lexemes);

  ProgramNode program = parse(lexemes);

  pretty_print(program);

  generate_assembly(program, filename);

  return 0;
}

void usage()
{
  puts("C Compiler\n----------\n\n");
  puts("Only call with one argument,\n the filename of c file to compile.");
}

void print_lexemes(TokenList* lexemes)
{
  TokenListNode* curr = lexemes->first;
  while(curr) {
    switch(curr->tok.type) {
    case UNKNOWN:
      puts("UNKNOWN");
      break;
    case LEFT_BRACE:
      puts("LEFT_BRACE");
      break;
    case RIGHT_BRACE:
      puts("RIGHT_BRACE");
      break;
    case LEFT_PAREN:
      puts("LEFT_PAREN");
      break;
    case RIGHT_PAREN:
      puts("RIGHT_PAREN");
      break;
    case SEMICOLON:
      puts("SEMICOLON");
      break;
    case INT:
      puts("INT");
      break;
    case RETURN:
      puts("RETURN");
      break;
    case IDENTIFIER:
      puts("IDENTIFIER");
      puts(curr->tok.value);
      break;
    case INT_LITERAL:
      printf("INT_LITERAL: %s\n", curr->tok.value);
      break;
    case HEX_LITERAL:
      printf("HEX_LITERAL: %s\n", curr->tok.value);
      break;
    case OCT_LITERAL:
      printf("OCT_LITERAL: %s\n", curr->tok.value);
      break;
    case MINUS:
      puts("MINUS");
      break;
    case PLUS:
      puts("PLUS");
      break;
    case STAR:
      puts("STAR");
      break;
    case SLASH:
      puts("SLASH");
      break;
    case EQUAL:
      puts("EQUAL");
      break;
    case NOTEQUAL:
      puts("NOTEQUAL");
      break;
    case AND:
      puts("AND");
      break;
    case OR:
      puts("OR");
      break;
    case LESSTHAN:
      puts("LESSTHAN");
      break;
    case LEQ:
      puts("LEQ");
      break;
    case GREATERTHAN:
      puts("GREATERTHAN");
      break;
    case GEQ:
      puts("GEQ");
      break;
    case BITWISE_NOT:
      puts("BITWISE_NOT");
      break;
    case LOGICAL_NOT:
      puts("LOGICAL_NOT");
      break;
    case MOD:
      puts("MOD");
      break;
    case BIT_AND:
      puts("BIT_AND");
      break;
    case BIT_OR:
      puts("BIT_OR");
      break;
    case BIT_XOR:
      puts("BIT_XOR");
      break;
    case LSHIFT:
      puts("LSHIFT");
      break;
    case RSHIFT:
      puts("RSHIFT");
      break;
    case ASSIGN:
      puts("ASSIGN");
      break;
    default:
      break;
    }
    curr = curr->next;
  }
}

void pretty_print(ProgramNode program)
{
  if(program.main) {
    FunctionNode* main = program.main;
    printf("func main -> %s:\n", (main->type == INT_RET ? "int" : "void"));
    for(unsigned int i = 0; i < main->num_statements; i++) {
      StatementNode* stmt = main->body[i];
      switch(stmt->type) {
      case RETURN_STATEMENT:
        printf("\treturn ");
        print_expression(stmt->return_value);
        printf("\n");
        break;
      case DECLARATION:
        printf("\t%s: INTEGER", stmt->var_name);
        if(stmt->assignment_expression) {
          printf(" = ");
          print_expression(stmt->assignment_expression);
        }
        printf("\n");
        break;
      case EXPRESSION:
        printf("\t");
        print_expression(stmt->expression);
        printf("\n");
        break;
      default:
        break;
      }
    }
  }
}

void print_expression(ExpressionNode* exp)
{
  switch(exp->type) {
  case INT_VALUE:
    printf("%i", exp->int_value);
    break;
  case NEGATE:
    printf("-");
    printf("(");
    print_expression(exp->unary_operand);
    printf(")");
    break;
  case LOG_NOT:
    printf("!");
    printf("(");
    print_expression(exp->unary_operand);
    printf(")");
    break;
  case BITWISE_COMP:
    printf("~");
    printf("(");
    print_expression(exp->unary_operand);
    printf(")");
    break;
  case ADD_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") + (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case SUB_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") - (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case MUL_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") * (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case DIV_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") / (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case MOD_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") MOD (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case EQ_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") EQ (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case NEQ_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") NEQ (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case GT_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") > (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case GEQ_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") >= (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case LT_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") < (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case LEQ_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") <= (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case AND_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") AND (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case OR_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") OR (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case BITAND_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") BITAND (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case BITOR_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") BITOR (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case BITXOR_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") BITXOR (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case LSHIFT_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") LSHIFT (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case RSHIFT_BINEXP:
    printf("(");
    print_expression(exp->left_operand);
    printf(") RSHIFT (");
    print_expression(exp->right_operand);
    printf(")");
    break;
  case ASSIGN_EXP:
    printf("%s <- (", exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf(")");
    break;
  case VAR_EXP:
    printf("(%s)", exp->var_name);
    break;
  default:
    break;
  }
}
