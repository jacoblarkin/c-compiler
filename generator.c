#include "generator.h"
#include "parser.h"
#include "symbol.h"

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
void check_next_reg(Register);
int count_local_vars(FunctionNode*);

int tag_counter = 0;

SymbolTable* main_st;

int func_stack_offset;

char* assembly_filename;

void generate_assembly(ProgramNode prgm, const char* filename)
{
  int len = strlen(filename);
  assembly_filename = calloc(len+1, sizeof(char));
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
    main_st = malloc(sizeof(SymbolTable));
    main_st->top = NULL;
    // Why am I doing this?
    push_constructed_symbol(NULL, 0, main_st);
    func_stack_offset = 4*count_local_vars(prgm.main);
    if(func_stack_offset % 16) {
      func_stack_offset += (16 - func_stack_offset % 16);
    }
    fprintf(as_file, "  sub sp, sp, #%i\n", func_stack_offset);
    FunctionNode* main = prgm.main;
    for(unsigned int i = 0; i < main->num_statements; i++) {
      write_statement_assembly(main->body[i], as_file);
    }
    fprintf(as_file, "  add sp, sp, #%i\n", func_stack_offset);
  }
}

void write_statement_assembly(StatementNode* stmt, FILE* as_file)
{
  static int next_offset = 4;
  switch(stmt->type) {
  case RETURN_STATEMENT:
    write_expression_assembly(X0, stmt->return_value, as_file); 
    fputs("  ret\n", as_file);
    break;
  case DECLARATION:
    if(find_symbol(stmt->var_name, main_st).name) {
      puts("Error: duplicate declaration of variable:");
      puts(stmt->var_name);
      fclose(as_file);
      remove(assembly_filename);
      exit(1);
    }
    push_constructed_symbol(stmt->var_name, next_offset, main_st);
    if(stmt->assignment_expression) {
      write_expression_assembly(X0, stmt->assignment_expression, as_file);
      //fprintf(as_file, "  str w0, [sp, %i]\n", func_stack_offset - next_offset);
    }
    next_offset += 4;
    break;
  case EXPRESSION:
    write_expression_assembly(X0, stmt->expression, as_file);
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
    fprintf(as_file, "  cmp w%i, 0\n  cset w%i, eq\n", reg, reg);
//    fprintf(as_file, "  cbz w%i, zero%i\n", reg, tag_counter);
//    fprintf(as_file, "  mov w%i, #0\n", reg);
//    fprintf(as_file, "  b endcmp%i\n", tag_counter);
//    fprintf(as_file, "zero%i:\n", tag_counter);
//    fprintf(as_file, "  mov w%i, #1\n", reg);
//    fprintf(as_file, "  b endcmp%i\n", tag_counter);
//    fprintf(as_file, "endcmp%i:\n", tag_counter);
//    tag_counter++;
    break;
  case ADD_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  add w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case SUB_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  sub w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case MUL_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  mul w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case DIV_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  sdiv w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case MOD_BINEXP:
    check_next_reg(reg);
    check_next_reg(reg+1);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  sdiv w%i, w%i, w%i\n", reg+2, reg, reg+1);
    fprintf(as_file, "  msub w%i, w%i, w%i, w%i\n", reg, reg+1, reg+2, reg);
    break;
  case EQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, w%i\n", reg, reg+1);
    fprintf(as_file, "  cset w%i, eq\n", reg);
    break;
  case NEQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, w%i\n", reg, reg+1);
    fprintf(as_file, "  cset w%i, ne\n", reg);
    break;
  case GT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, w%i\n", reg, reg+1);
    fprintf(as_file, "  cset w%i, gt\n", reg);
    break;
  case GEQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, w%i\n", reg, reg+1);
    fprintf(as_file, "  cset w%i, ge\n", reg);
    break;
  case LT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, w%i\n", reg, reg+1);
    fprintf(as_file, "  cset w%i, lt\n", reg);
    break;
  case LEQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, w%i\n", reg, reg+1);
    fprintf(as_file, "  cset w%i, le\n", reg);
    break;
  case AND_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, 0\n", reg);
    fprintf(as_file, "  ccmp w%i, 0, 4, ne\n", reg+1);
    fprintf(as_file, "  cset w%i, ne\n", reg);
    break;
  case OR_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  orr w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  cmp w%i, 0\n", reg);
    fprintf(as_file, "  cset w%i, ne\n", reg);
    break;
  case BITAND_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  and w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case BITOR_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  orr w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case BITXOR_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  eor w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case LSHIFT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  lsl w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case RSHIFT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  asr w%i, w%i, w%i\n", reg, reg, reg+1);
    break;
  case ASSIGN_EXP:
    if(!find_symbol(exp->left_operand->var_name, main_st).name) {
      puts("Error: Symbol not found:");
      puts(exp->var_name);
      fclose(as_file);
      remove(assembly_filename);
      exit(1);
    }
    write_expression_assembly(reg, exp->right_operand, as_file);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, func_stack_offset - find_symbol(exp->left_operand->var_name, main_st).offset);
    break;
  case VAR_EXP:
    if(!find_symbol(exp->var_name, main_st).name) {
      puts("Error: Symbol not found:");
      puts(exp->var_name);
      fclose(as_file);
      remove(assembly_filename);
      exit(1);
    }
    fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - find_symbol(exp->var_name, main_st).offset);
    break;
  default:
    break;
  }
}

void check_next_reg(Register reg)
{
  if(reg+1 > 18) {
    puts("Error:  Can only handle up to 18 registers right now");
    remove(assembly_filename);
    exit(1);
  }
}

int count_local_vars(FunctionNode* func)
{
  int local_vars = 0;
  for(unsigned int i = 0; i < func->num_statements; i++) {
    if(func->body[i]->type == DECLARATION) {
      local_vars++;
    }
  }
  return local_vars;
}
