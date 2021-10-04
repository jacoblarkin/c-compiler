#include <stdlib.h>

typedef struct Symbol_s {
  char* name;
  union {
    size_t address; // Global adress
    size_t offset;  // Stack offset for local vars
  };
} Symbol;

typedef struct SymbolTableNode_s {
  Symbol symbol;
  struct SymbolTableNode_s* next;
} SymbolTableNode;

typedef struct SymbolTable_s {
  SymbolTableNode* top;
} SymbolTable;

extern SymbolTable global_symbol_table;

void push_symbol(Symbol, SymbolTable*);
void push_constructed_symbol(char*, size_t, SymbolTable*);
Symbol find_symbol(char*, SymbolTable*);
