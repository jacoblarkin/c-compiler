#include <stdlib.h>

typedef struct Symbol_s {
  char* name;
  union {
    size_t address; // Global adress
    size_t offset;  // Stack offset for local vars
  };
} Symbol;

typedef struct SymbolTable_s {
  Symbol symbol;
  struct SymbolTable_s* next;
} SymbolTable;

extern SymbolTable global_symbol_table;

void push_symbol(Symbol, SymbolTable*);
void push_constructed_symbol(char*, size_t, SymbolTable*);
Symbol find_symbol(char*, SymbolTable*);
