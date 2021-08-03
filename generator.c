#include "generator.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

typedef enum Register_t {
  X0,  X1,  X2,  X3,  X4,  X5,  X6,  X7,
  X8,  X9,  X10, X11, X12, X13, X14, X15,
  X16, X17, X18, X19, X20, X21, X22, X23,
  X24, X25, X26, X27, X28, X29, X30, X31
} Register;

void write_ast_assembly(ProgramNode, FILE*);
void write_statement_assembly(StatementNode*, FILE*);
void write_expression_assembly(Register, ExpressionNode*, FILE*);

int tag_counter = 0;

void generate_assembly(ProgramNode prgm, const char* filename)
{
  int len = strlen(filename);
  char* assembly_filename = calloc(len+1, sizeof(char));
  strncpy(assembly_filename, filename, len);
  assembly_filename[len-1] = 's';
  
  FILE* as_file = fopen(assembly_filename, "w");
  if(!as_file) {
    perror("Error");
    exit(1);
  }

  fputs(".global _main\n", as_file);
  fputs(".align 2\n", as_file);


  write_ast_assembly(prgm, as_file);

  fclose(as_file);
}

void write_ast_assembly(ProgramNode prgm, FILE* as_file)
{
  if(prgm.main) {
    fputs("_main:\n", as_file);
    FunctionNode* main = prgm.main;
    for(unsigned int i = 0; i < main->num_statements; i++) {
      write_statement_assembly(main->body[i], as_file);
    }
  }
}

void write_statement_assembly(StatementNode* stmt, FILE* as_file)
{
  switch(stmt->type) {
  case RETURN_STATEMENT:
    write_expression_assembly(X0, stmt->return_value, as_file); 
    fputs("  ret\n", as_file);
    break;
  default:
    break;
  }
}

void write_expression_assembly(Register reg, ExpressionNode* exp, FILE* as_file)
{
  switch(exp->type) {
  case INT_VALUE:
    fprintf(as_file, "  mov w%i, #%i\n", reg, exp->int_value);
    break;
  case NEGATE:
    write_expression_assembly(reg, exp->unary_operand, as_file);
    fprintf(as_file, "  neg w%i, w%i\n", reg, reg);
    break;
  case BITWISE_COMP:
    write_expression_assembly(reg, exp->unary_operand, as_file);
    fprintf(as_file, "  mvn w%i, w%i\n", reg, reg);
    break;
  case LOG_NOT:
    write_expression_assembly(reg, exp->unary_operand, as_file);
    // From GCC
    // cmp w0, 0
    // cset w0, eq
    // and w0, w0, 255 # probably not necessary
    fprintf(as_file, "  cbz w%i, zero%i\n", reg, tag_counter);
    fprintf(as_file, "  mov w%i, #0\n", reg);
    fprintf(as_file, "  b endcmp%i\n", tag_counter);
    fprintf(as_file, "zero%i:\n", tag_counter);
    fprintf(as_file, "  mov w%i, #1\n", reg);
    fprintf(as_file, "  b endcmp%i\n", tag_counter);
    fprintf(as_file, "endcmp%i:\n", tag_counter);
    tag_counter++;
    break;
  default:
    break;
  }
}
