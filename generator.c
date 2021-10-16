#include "generator.h"
#include "parser.h"
#include "symbol.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef enum Register_t {
  X0,  X1,  X2,  X3,  X4,  X5,  X6,  X7,
  X8,  X9,  X10, X11, X12, X13, X14, X15,
  X16, X17, X18, X19, X20, X21, X22, X23,
  X24, X25, X26, X27, X28, X29, X30, X31
} Register;

void write_ast_assembly(ProgramNode, FILE*);
void write_block_assembly(BlockNode*, FILE*, int);
void write_declaration_assembly(DeclarationNode*, FILE*);
void write_statement_assembly(StatementNode*, FILE*, int);
void write_expression_assembly(Register, ExpressionNode*, FILE*);
void check_next_reg(Register);
int count_local_vars(FunctionNode*);
size_t get_symbol_offset(char*, FILE*);

int tag_counter = 0;

SymbolTable* main_st;
SymbolTable* top_st;

int func_stack_offset;

char* assembly_filename;

void generate_assembly(ProgramNode prgm, const char* filename)
{
  unsigned long len = strlen(filename);
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
    func_stack_offset = 4*count_local_vars(prgm.main);
    if(func_stack_offset % 16) {
      func_stack_offset += (16 - func_stack_offset % 16);
    }
    fprintf(as_file, "  sub sp, sp, #%i\n", func_stack_offset);
    int ret_tag = tag_counter++;
    top_st = NULL;
    write_block_assembly(prgm.main->body, as_file, ret_tag);
    fprintf(as_file, "  add sp, sp, #%i\n", func_stack_offset);
    fprintf(as_file, ".L%i:\n", ret_tag);
    fputs("  ret\n", as_file);
  }
}

void write_block_assembly(BlockNode* block, FILE* as_file, int ret_tag)
{
  SymbolTable* block_st = malloc(sizeof(SymbolTable));
  block_st->top = NULL;
  block_st->next = top_st;
  top_st = block_st;
  // Why am I doing this?
  push_constructed_symbol(NULL, 0, block_st);
  for(unsigned int i = 0; i < block->count; i++) {
    BlockItem* item = block->body[i];
    if(item->type == STATEMENT_ITEM) {
      write_statement_assembly(item->stmt, as_file, ret_tag);
    } else {
      write_declaration_assembly(item->decl, as_file);
    }
  }
  top_st = block_st->next;
  delete_symbol_table(block_st);
}

void write_declaration_assembly(DeclarationNode* decl, FILE* as_file)
{
  static int next_offset = 4;
  if(find_symbol(decl->var_name, top_st).name) {
    puts("Error: duplicate declaration of variable:");
    puts(decl->var_name);
    fclose(as_file);
    remove(assembly_filename);
    exit(1);
  }
  push_constructed_symbol(decl->var_name, next_offset, top_st);
  if(decl->assignment_expression) {
    write_expression_assembly(X0, decl->assignment_expression, as_file);
    //fprintf(as_file, "  str w0, [sp, %i]\n", func_stack_offset - next_offset);
  }
  next_offset += 4;
}

void write_statement_assembly(StatementNode* stmt, FILE* as_file, int ret_tag)
{
  int tag0;
  int tag1;
  switch(stmt->type) {
  case RETURN_STATEMENT:
    write_expression_assembly(X0, stmt->expression, as_file); 
    fprintf(as_file, "  b .L%i\n", ret_tag);
    break;
  case CONDITIONAL:
    tag0 = tag_counter++;
    if(stmt->else_stmt) {
      tag1 = tag_counter++;
    }
    write_expression_assembly(X0, stmt->condition, as_file);
    fprintf(as_file, "  cmp w%i, 0\n", X0);
    fprintf(as_file, "  beq .L%i\n", tag0);
    write_statement_assembly(stmt->if_stmt, as_file, ret_tag);
    if(stmt->else_stmt){
      fprintf(as_file, "  b .L%i\n", tag1);
    }
    fprintf(as_file, ".L%i:\n", tag0);
    if(stmt->else_stmt) {
      write_statement_assembly(stmt->else_stmt, as_file, ret_tag);
      fprintf(as_file, ".L%i:\n", tag1);
    }
    break;
  case BLOCK_STATEMENT:
    write_block_assembly(stmt->block, as_file, ret_tag);
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
  size_t offset;
  int tag0;
  int tag1;
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
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  cmp w%i, 0\n", reg);
    fprintf(as_file, "  beq .L%i\n", tag0);
    write_expression_assembly(reg, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, 0\n", reg);
    fprintf(as_file, "  beq .L%i\n", tag0);
    fprintf(as_file, "  mov w%i, 1\n", reg);
    fprintf(as_file, "  b .L%i\n", tag1);
    fprintf(as_file, ".L%i:\n  mov w%i, 0\n", tag0, reg);
    fprintf(as_file, ".L%i:\n", tag1);
    //fprintf(as_file, "  ccmp w%i, 0, 4, ne\n", reg+1);
    //fprintf(as_file, "  cset w%i, ne\n", reg);
    break;
  case OR_BINEXP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  cmp w%i, 0\n", reg);
    fprintf(as_file, "  bne .L%i\n", tag0);
    write_expression_assembly(reg, exp->right_operand, as_file);
    fprintf(as_file, "  cmp w%i, 0\n", reg);
    fprintf(as_file, "  bne .L%i\n", tag0);
    fprintf(as_file, "  mov w%i, 0\n", reg);
    fprintf(as_file, "  b .L%i\n", tag1);
    fprintf(as_file, ".L%i:\n  mov w%i, 1\n", tag0, reg);
    fprintf(as_file, ".L%i:\n", tag1);
    //fprintf(as_file, "  orr w%i, w%i, w%i\n", reg, reg, reg+1);
    //fprintf(as_file, "  cmp w%i, 0\n", reg);
    //fprintf(as_file, "  cset w%i, ne\n", reg);
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
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    write_expression_assembly(reg, exp->right_operand, as_file);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case PLUSEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  add w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case MINUSEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  sub w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case TIMESEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  mul w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case DIVEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  sdiv w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case MODEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    check_next_reg(reg+1);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  sdiv w%i, w%i, w%i\n", reg+2, reg, reg+1);
    fprintf(as_file, "  msub w%i, w%i, w%i, w%i\n", reg, reg+1, reg+2, reg);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case LSHEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  lsl w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case RSHEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  asr w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case ANDEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  and w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case OREQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  orr w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case XOREQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, func_stack_offset - sym.offset);
    fprintf(as_file, "  eor w%i, w%i, w%i\n", reg, reg, reg+1);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case VAR_EXP:
    offset = get_symbol_offset(exp->var_name, as_file);
    fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, offset);
    break;
  case COMMA_EXP:
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg, exp->right_operand, as_file);
    break;
  case PREINC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, offset);
    fprintf(as_file, "  add w%i, w%i, #1\n", reg, reg);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case PREDEC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, offset);
    fprintf(as_file, "  sub w%i, w%i, #1\n", reg, reg);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg, offset);
    break;
  case POSTINC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, offset);
    fprintf(as_file, "  add w%i, w%i, #1\n", reg+1, reg);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg+1, offset);
    break;
  case POSTDEC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    //fprintf(as_file, "  ldr w%i, [sp, %lu]\n", reg, offset);
    fprintf(as_file, "  sub w%i, w%i, #1\n", reg+1, reg);
    fprintf(as_file, "  str w%i, [sp, %lu]\n", reg+1, offset);
    break;
  case COND_EXP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    write_expression_assembly(reg, exp->condition, as_file);
    fprintf(as_file, "  cmp w%i, 0\n", reg);
    fprintf(as_file, "  beq .L%i\n", tag0);
    write_expression_assembly(reg, exp->if_exp, as_file);
    fprintf(as_file, "  b .L%i\n", tag1);
    fprintf(as_file, ".L%i:\n", tag0);
    write_expression_assembly(reg, exp->else_exp, as_file);
    fprintf(as_file, ".L%i:\n", tag1);
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
  for(unsigned int i = 0; i < func->body->count; i++) {
    if(func->body->body[i]->type == DECLARATION_ITEM) {
      local_vars++;
    }
  }
  return local_vars;
}

size_t get_symbol_offset(char* name, FILE* as_file)
{
  Symbol sym = {.name = NULL, .offset = 0};
  SymbolTable* st = top_st;
  assert(st);
  while(!sym.name) {
    sym = find_symbol(name, st);
    if(!st->next && !sym.name) {
      puts("Error: Symbol not found:");
      puts(name);
      fclose(as_file);
      remove(assembly_filename);
      exit(1);
    }
    st = st->next;
  }
  return func_stack_offset - sym.offset;
}
