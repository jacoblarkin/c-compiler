#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

#include <stdio.h>

void pretty_print(ProgramNode);
void print_lexemes(TokenList*);
void usage(void);

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
        printf("\treturn %i", stmt->return_value->int_value);
        break;
      default:
        break;
      }
    }
  }
}
