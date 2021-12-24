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

typedef struct CaseLabelTableNode_s {
  int def;
  long val;
  size_t tag;
  struct CaseLabelTableNode_s* next;
} CaseLabelTableNode;

typedef struct CaseLabelTable_s {
  CaseLabelTableNode* first;
  struct CaseLabelTable_s* parent;
} CaseLabelTable;

void write_ast_assembly(ProgramNode, FILE*);
void write_block_assembly(BlockNode*, FILE*, int);
void write_declaration_assembly(DeclarationNode*, FILE*);
void write_statement_assembly(StatementNode*, FILE*, int);
void write_expression_assembly(Register, ExpressionNode*, FILE*);
void write_switch_case_table(StatementNode*, FILE*);
void check_next_reg(Register);
int count_local_vars(BlockNode*);
void construct_label_table(SymbolTable*, BlockNode*);
size_t get_symbol_offset(char*, FILE*);
char reg_prefix_for_type(Type type);
char suffix_for_type(Type type);

int tag_counter = 0;

SymbolTable* main_st;
SymbolTable* top_st;
SymbolTable* labels;
CaseLabelTable* curr_switch_table;

int func_stack_offset;

char* assembly_filename;

void generate_assembly(ProgramNode prgm, const char* filename)
{
  unsigned long len = strlen(filename);
  assembly_filename = calloc(len+1, sizeof(char));
  strncpy(assembly_filename, filename, len);
  assembly_filename[len-1] = 's';
  labels = NULL;
  
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
    func_stack_offset = 4*count_local_vars(prgm.main->body);
    if(func_stack_offset % 16) {
      func_stack_offset += (16 - func_stack_offset % 16);
    }
    fprintf(as_file, "  sub sp, sp, #%i\n", func_stack_offset);
    int ret_tag = tag_counter++;
    top_st = NULL;
    SymbolTable* label_st = malloc(sizeof(SymbolTable));
    label_st->top = NULL;
    label_st->next = labels;
    labels = label_st;
    curr_switch_table = NULL;
    push_constructed_symbol(NULL, 0, labels);
    construct_label_table(labels, prgm.main->body);
    write_block_assembly(prgm.main->body, as_file, ret_tag);
    fprintf(as_file, ".L%i:\n", ret_tag);
    fprintf(as_file, "  add sp, sp, #%i\n", func_stack_offset);
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
  }
  next_offset += 4;
}

void write_statement_assembly(StatementNode* stmt, FILE* as_file, int ret_tag)
{
  static int current_continue_tag = -1;
  static int current_break_tag = -1;
  int last_continue_tag; 
  int last_break_tag; 
  int tag0;
  int tag1;
  int tag2;
  Symbol label;
  CaseLabelTableNode* node;
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
    fprintf(as_file, "  cmp %c%i, 0\n", 
        reg_prefix_for_type(stmt->condition->value_type), X0);
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
  case WHILE_LOOP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    last_continue_tag = current_continue_tag;
    last_break_tag = current_break_tag;
    current_continue_tag = tag0;
    current_break_tag = tag1;
    write_expression_assembly(X0, stmt->loop_condition, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", 
        reg_prefix_for_type(stmt->loop_condition->value_type), X0);
    fprintf(as_file, "  beq .L%i\n", tag1);
    fprintf(as_file, ".L%i:\n", tag0);
    write_statement_assembly(stmt->loop_stmt, as_file, ret_tag);
    write_expression_assembly(X0, stmt->loop_condition, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", 
        reg_prefix_for_type(stmt->loop_condition->value_type), X0);
    fprintf(as_file, "  bne .L%i\n", tag0);
    fprintf(as_file, ".L%i:\n", tag1);
    current_continue_tag = last_continue_tag;
    current_break_tag = last_break_tag;
    break;
  case DO_LOOP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    last_continue_tag = current_continue_tag;
    last_break_tag = current_break_tag;
    current_continue_tag = tag0;
    current_break_tag = tag1;
    fprintf(as_file, ".L%i:\n", tag0);
    write_statement_assembly(stmt->loop_stmt, as_file, ret_tag);
    write_expression_assembly(X0, stmt->loop_condition, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", 
        reg_prefix_for_type(stmt->loop_condition->value_type), X0);
    fprintf(as_file, "  bne .L%i\n", tag0);
    fprintf(as_file, ".L%i:\n", tag1);
    current_continue_tag = last_continue_tag;
    current_break_tag = last_break_tag;
    break;
  case FOR_LOOP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    tag2 = tag_counter++;
    last_continue_tag = current_continue_tag;
    last_break_tag = current_break_tag;
    current_continue_tag = tag2;
    current_break_tag = tag1;
    write_expression_assembly(X0, stmt->init_exp, as_file);
    fprintf(as_file, ".L%i:\n", tag0);
    if(stmt->loop_condition->type != EMPTY_EXP) {
      write_expression_assembly(X0, stmt->loop_condition, as_file);
      fprintf(as_file, "  cmp %c%i, 0\n", 
          reg_prefix_for_type(stmt->loop_condition->value_type), X0);
      fprintf(as_file, "  beq .L%i\n", tag1);
    }
    write_statement_assembly(stmt->loop_stmt, as_file, ret_tag);
    fprintf(as_file, ".L%i:\n", tag2);
    write_expression_assembly(X0, stmt->post_exp, as_file);
    fprintf(as_file, "  b .L%i\n", tag0);
    fprintf(as_file, ".L%i:\n", tag1);
    current_continue_tag = last_continue_tag;
    current_break_tag = last_break_tag;
    break;
  case FORDECL_LOOP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    tag2 = tag_counter++;
    last_continue_tag = current_continue_tag;
    last_break_tag = current_break_tag;
    current_continue_tag = tag2;
    current_break_tag = tag1;
    SymbolTable* for_st = malloc(sizeof(SymbolTable));
    for_st->top = NULL;
    for_st->next = top_st;
    top_st = for_st;
    push_constructed_symbol(NULL, 0, for_st);
    write_declaration_assembly(stmt->init_decl, as_file);
    fprintf(as_file, ".L%i:\n", tag0);
    if(stmt->loop_condition->type != EMPTY_EXP) {
      write_expression_assembly(X0, stmt->loop_condition, as_file);
      fprintf(as_file, "  cmp %c%i, 0\n", 
          reg_prefix_for_type(stmt->loop_condition->value_type), X0);
      fprintf(as_file, "  beq .L%i\n", tag1);
    }
    write_statement_assembly(stmt->loop_stmt, as_file, ret_tag);
    fprintf(as_file, ".L%i:\n", tag2);
    write_expression_assembly(X0, stmt->post_exp, as_file);
    fprintf(as_file, "  b .L%i\n", tag0);
    fprintf(as_file, ".L%i:\n", tag1);
    top_st = for_st->next;
    delete_symbol_table(for_st);
    current_continue_tag = last_continue_tag;
    current_break_tag = last_break_tag;
    break;
  case CONTINUE_STATEMENT:
    if(current_continue_tag < 0) {
      puts("Error: continue not in loop.");
      fclose(as_file);
      remove(assembly_filename);
      exit(1);  
    }
    fprintf(as_file, "  b .L%i\n", current_continue_tag);
    break;
  case BREAK_STATEMENT:
    if(current_break_tag < 0) {
      puts("Error: break not in loop.");
      fclose(as_file);
      remove(assembly_filename);
      exit(1);
    }
    fprintf(as_file, "  b .L%i\n", current_break_tag);
    break;
  case BLOCK_STATEMENT:
    write_block_assembly(stmt->block, as_file, ret_tag);
    break;
  case SWITCH_STATEMENT:
    tag0 = tag_counter++;
    last_break_tag = current_break_tag;
    current_break_tag = tag0;
    write_expression_assembly(X0, stmt->switch_exp, as_file);
    write_switch_case_table(stmt, as_file);
    write_block_assembly(stmt->switch_block, as_file, ret_tag);
    fprintf(as_file, ".L%i:\n", tag0);
    current_break_tag = last_break_tag;
    curr_switch_table = curr_switch_table->parent;
    break;
  case CASE_STATEMENT:
    node = curr_switch_table->first;
    while(node) {
      if(!node->def && node->val == stmt->val) {
        fprintf(as_file, ".L%zu:\n", node->tag);
        break;
      }
      node = node->next;
    }
    break;
  case DEFAULT_STATEMENT:
    node = curr_switch_table->first;
    while(node) {
      if(node->def) {
        fprintf(as_file, ".L%zu:\n", node->tag);
        break;
      }
      node = node->next;
    }
    break;
  case GOTO_STATEMENT:
    label = find_symbol(stmt->label_name, labels);
    if(!label.name) {
      puts("Error: Could not find label for goto");
      puts(stmt->label_name);
      fclose(as_file);
      remove(assembly_filename);
      exit(1);
    }
    fprintf(as_file, "  b .L%zu\n", label.address);
    break;
  case LABEL:
    label = find_symbol(stmt->label_name, labels);
    if(!label.name) {
      puts("Error: Could not find label");
      puts(stmt->label_name);
      fclose(as_file);
      remove(assembly_filename);
      exit(1);
    }
    fprintf(as_file, ".L%zu:\n", label.address);
    break;
  case EXPRESSION:
    write_expression_assembly(X0, stmt->expression, as_file);
    break;
  default:
    break;
  }
}

void write_switch_case_table(StatementNode* switch_stmt, FILE* as_file)
{
  BlockNode* block = switch_stmt->switch_block;
  CaseLabelTable* table = malloc(sizeof(CaseLabelTable));
  table->parent = curr_switch_table;
  table->first = NULL;
  curr_switch_table = table;
  int def_tag = -1;
  for(size_t i = 0; i < block->count; i++) {
    if(block->body[i]->type == STATEMENT_ITEM
       && block->body[i]->stmt->type == CASE_STATEMENT) {
      CaseLabelTableNode* node = malloc(sizeof(CaseLabelTableNode));
      node->tag = tag_counter++;
      node->val = block->body[i]->stmt->val;
      node->def = 0;
      node->next = table->first;
      table->first = node;
      fprintf(as_file, "  cmp x%i, %ld\n", X0, node->val);
      fprintf(as_file, "  beq .L%zu\n", node->tag);
    } else if(block->body[i]->type == STATEMENT_ITEM
       && block->body[i]->stmt->type == DEFAULT_STATEMENT) {
      CaseLabelTableNode* node = malloc(sizeof(CaseLabelTableNode));
      node->tag = tag_counter++;
      node->val = 0;
      node->def = 1;
      node->next = table->first;
      table->first = node;
      def_tag = node->tag;
    }
  }
  if(def_tag >= 0) {
    fprintf(as_file, "  b .L%i\n", def_tag);
  }
}

void write_expression_assembly(Register reg, ExpressionNode* exp, FILE* as_file)
{
  size_t offset;
  int tag0;
  int tag1;
  char reg_prefix = reg_prefix_for_type(exp->value_type);
  char sign_char = exp->value_type.signed_ ? 's' : 'u';
  char suffix = suffix_for_type(exp->value_type);
  switch(exp->type) {
  case INT_VALUE:
    fprintf(as_file, "  mov w%i, #%i\n", reg, exp->int_value);
    break;
  case NEGATE:
    write_expression_assembly(reg, exp->unary_operand, as_file);
    fprintf(as_file, "  neg %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg);
    break;
  case BITWISE_COMP:
    write_expression_assembly(reg, exp->unary_operand, as_file);
    fprintf(as_file, "  mvn %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg);
    break;
  case LOG_NOT:
    write_expression_assembly(reg, exp->unary_operand, as_file);
    // From GCC
    // cmp w0, 0
    // cset w0, eq
    // and w0, w0, 255 # probably not necessary
    fprintf(as_file, "  cmp %c%i, 0\n  cset %c%i, eq\n", 
        reg_prefix, reg, reg_prefix, reg);
    break;
  case ADD_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  add %c%i, %c%i, %c%i\n", 
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case SUB_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  sub %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case MUL_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  mul %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case DIV_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  %cdiv %c%i, %c%i, %c%i\n",
        sign_char, reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case MOD_BINEXP:
    check_next_reg(reg);
    check_next_reg(reg+1);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  %cdiv %c%i, %c%i, %c%i\n",
        sign_char, reg_prefix, reg+2, reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  msub %c%i, %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg+1, reg_prefix, reg+2, reg_prefix, reg);
    break;
  case EQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  cset %c%i, eq\n", reg_prefix, reg);
    break;
  case NEQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  cset %c%i, ne\n", reg_prefix, reg);
    break;
  case GT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  cset %c%i, gt\n", reg_prefix, reg);
    break;
  case GEQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  cset %c%i, ge\n", reg_prefix, reg);
    break;
  case LT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  cset %c%i, lt\n", reg_prefix, reg);
    break;
  case LEQ_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, %c%i\n", reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  cset %c%i, le\n", reg_prefix, reg);
    break;
  case AND_BINEXP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", reg_prefix, reg);
    fprintf(as_file, "  beq .L%i\n", tag0);
    write_expression_assembly(reg, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", reg_prefix, reg);
    fprintf(as_file, "  beq .L%i\n", tag0);
    fprintf(as_file, "  mov %c%i, 1\n", reg_prefix, reg);
    fprintf(as_file, "  b .L%i\n", tag1);
    fprintf(as_file, ".L%i:\n  mov %c%i, 0\n", tag0, reg_prefix, reg);
    fprintf(as_file, ".L%i:\n", tag1);
    //fprintf(as_file, "  ccmp w%i, 0, 4, ne\n", reg+1);
    //fprintf(as_file, "  cset w%i, ne\n", reg);
    break;
  case OR_BINEXP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", reg_prefix, reg);
    fprintf(as_file, "  bne .L%i\n", tag0);
    write_expression_assembly(reg, exp->right_operand, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", reg_prefix, reg);
    fprintf(as_file, "  bne .L%i\n", tag0);
    fprintf(as_file, "  mov %c%i, 0\n", reg_prefix, reg);
    fprintf(as_file, "  b .L%i\n", tag1);
    fprintf(as_file, ".L%i:\n  mov %c%i, 1\n", tag0, reg_prefix, reg);
    fprintf(as_file, ".L%i:\n", tag1);
    //fprintf(as_file, "  orr w%i, w%i, w%i\n", reg, reg, reg+1);
    //fprintf(as_file, "  cmp w%i, 0\n", reg);
    //fprintf(as_file, "  cset w%i, ne\n", reg);
    break;
  case BITAND_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  and %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case BITOR_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  orr %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case BITXOR_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  eor %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case LSHIFT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  lsl %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case RSHIFT_BINEXP:
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    fprintf(as_file, "  asr %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    break;
  case ASSIGN_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    write_expression_assembly(reg, exp->right_operand, as_file);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case PLUSEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  add %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case MINUSEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  sub %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case TIMESEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  mul %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case DIVEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  %cdiv %c%i, %c%i, %c%i\n",
        sign_char, reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case MODEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    check_next_reg(reg+1);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  %cdiv %c%i, %c%i, %c%i\n",
        sign_char, reg_prefix, reg+2, reg_prefix, reg, reg_prefix, reg+1);
    fprintf(as_file, "  msub %c%i, %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg+1, reg_prefix, reg+2, reg_prefix, reg);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case LSHEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  lsl %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case RSHEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  asr %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case ANDEQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  and %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case OREQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  orr %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case XOREQ_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg+1, exp->right_operand, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  eor %c%i, %c%i, %c%i\n",
        reg_prefix, reg, reg_prefix, reg, reg_prefix, reg+1);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case VAR_EXP:
    offset = get_symbol_offset(exp->var_name, as_file);
    if(suffix) {
      fprintf(as_file, "  ldr%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  ldr %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case COMMA_EXP:
    write_expression_assembly(reg, exp->left_operand, as_file);
    write_expression_assembly(reg, exp->right_operand, as_file);
    break;
  case PREINC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  add %c%i, %c%i, #1\n", reg_prefix, reg, reg_prefix, reg);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case PREDEC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  sub %c%i, %c%i, #1\n", reg_prefix, reg, reg_prefix, reg);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg, offset);
    }
    break;
  case POSTINC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  add %c%i, %c%i, #1\n", reg_prefix, reg+1, reg_prefix, reg);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg+1, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg+1, offset);
    }
    break;
  case POSTDEC_EXP:
    offset = get_symbol_offset(exp->left_operand->var_name, as_file);
    check_next_reg(reg);
    write_expression_assembly(reg, exp->left_operand, as_file);
    fprintf(as_file, "  sub %c%i, %c%i, #1\n", reg_prefix, reg+1, reg_prefix, reg);
    if(suffix) {
      fprintf(as_file, "  str%c %c%i, [sp, %lu]\n", suffix, reg_prefix, reg+1, offset);
    } else {
      fprintf(as_file, "  str %c%i, [sp, %lu]\n", reg_prefix, reg+1, offset);
    }
    break;
  case COND_EXP:
    tag0 = tag_counter++;
    tag1 = tag_counter++;
    write_expression_assembly(reg, exp->condition, as_file);
    fprintf(as_file, "  cmp %c%i, 0\n", reg_prefix, reg);
    fprintf(as_file, "  beq .L%i\n", tag0);
    write_expression_assembly(reg, exp->if_exp, as_file);
    fprintf(as_file, "  b .L%i\n", tag1);
    fprintf(as_file, ".L%i:\n", tag0);
    write_expression_assembly(reg, exp->else_exp, as_file);
    fprintf(as_file, ".L%i:\n", tag1);
    break;
  case EMPTY_EXP:
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

int count_local_vars(BlockNode* block)
{
  int local_vars = 0;
  for(unsigned int i = 0; i < block->count; i++) {
    if(block->body[i]->type == DECLARATION_ITEM) {
      local_vars++;
    } else {
      switch(block->body[i]->stmt->type) {
      case FORDECL_LOOP:
        local_vars++;
        if (block->body[i]->stmt->loop_stmt->type == BLOCK_STATEMENT) {
          local_vars += count_local_vars(block->body[i]->stmt->loop_stmt->block);
        }
        break;
      case WHILE_LOOP:
      case DO_LOOP:
      case FOR_LOOP:
        if (block->body[i]->stmt->loop_stmt->type == BLOCK_STATEMENT) {
          local_vars += count_local_vars(block->body[i]->stmt->loop_stmt->block);
        }
        break;
      case CONDITIONAL:
        if (block->body[i]->stmt->if_stmt->type == BLOCK_STATEMENT) {
          local_vars += count_local_vars(block->body[i]->stmt->if_stmt->block);
        }
        if (block->body[i]->stmt->else_stmt 
            && block->body[i]->stmt->else_stmt->type == BLOCK_STATEMENT) {
          local_vars += count_local_vars(block->body[i]->stmt->else_stmt->block);
        }
        break;
      default:
        break;
      }
    }
  }
  return local_vars;
}

void construct_label_table(SymbolTable* st, BlockNode* block)
{
  for(unsigned int i = 0; i < block->count; i++) {
    if(block->body[i]->type == DECLARATION_ITEM) {
      continue;
    }
    StatementNode* stmt = block->body[i]->stmt;
    switch(stmt->type) {
    case LABEL:
      push_constructed_symbol(stmt->label_name, tag_counter++, st);
      break;
    case FORDECL_LOOP:
    case WHILE_LOOP:
    case DO_LOOP:
    case FOR_LOOP:
      if (stmt->loop_stmt->type == BLOCK_STATEMENT) {
        construct_label_table(st, stmt->loop_stmt->block);
      }
      break;
    case CONDITIONAL:
      if (stmt->if_stmt->type == BLOCK_STATEMENT) {
        construct_label_table(st, stmt->if_stmt->block);
      }
      if (stmt->else_stmt 
          && stmt->else_stmt->type == BLOCK_STATEMENT) {
        construct_label_table(st, stmt->else_stmt->block);
      }
      break;
    default:
      break;
    }
  }
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

char reg_prefix_for_type(Type type)
{
  switch(type.base) {
  case LONG_LONG_VAR:
  case LONG_VAR:
    return 'x';
  default:
    return 'w';
  }
  return 'w';
}

char suffix_for_type(Type type)
{
  switch (type.base) {
  case CHAR_VAR:
    return 'b';
  case SHORT_VAR:
    return 'h';
  default:
    return '\0';
  }
  return '\0';
}
