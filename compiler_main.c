#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "pprint.h"

#include <stdio.h>

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
