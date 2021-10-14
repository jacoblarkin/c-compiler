#include "pprint.h"
#include "token.h"
#include "parser.h"
#include "c_lang.h"

#include <stdio.h>

void print_expression(ExpressionNode*);

void print_lexemes(TokenList* lexemes)
{
  TokenListNode* curr = lexemes->first;
  while(curr) {
    TokenType tt = curr->tok.type;
    if(token_structs[tt].syntax == IDENTIFIER_ST
       || token_structs[tt].syntax == LITERAL_ST) {
      printf("%s: %s\n", token_structs[tt].tok_out, curr->tok.value);
    } else {
      puts(token_structs[tt].tok_out);
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
  case PLUSEQ_EXP:
    printf("%s <- ( %s + (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case MINUSEQ_EXP:
    printf("%s <- ( %s - (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case TIMESEQ_EXP:
    printf("%s <- ( %s * (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case DIVEQ_EXP:
    printf("%s <- ( %s / (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case MODEQ_EXP:
    printf("%s <- ( %s MOD (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case LSHEQ_EXP:
    printf("%s <- ( %s << (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case RSHEQ_EXP:
    printf("%s <- ( %s >> (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case ANDEQ_EXP:
    printf("%s <- ( %s & (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case OREQ_EXP:
    printf("%s <- ( %s | (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case XOREQ_EXP:
    printf("%s <- ( %s ^ (", exp->left_operand->var_name, exp->left_operand->var_name);
    print_expression(exp->right_operand);
    printf("))");
    break;
  case VAR_EXP:
    printf("(%s)", exp->var_name);
    break;
  case PREINC_EXP:
    printf("++(");
    print_expression(exp->unary_operand);
    printf(")");
    break;
  case PREDEC_EXP:
    printf("--(");
    print_expression(exp->unary_operand);
    printf(")");
    break;
  case POSTINC_EXP:
    printf("(");
    print_expression(exp->unary_operand);
    printf(")++");
    break;
  case POSTDEC_EXP:
    printf("(");
    print_expression(exp->unary_operand);
    printf(")--");
    break;
  default:
    break;
  }
}
