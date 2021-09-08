#include "symbol.h"

#include <string.h>

SymbolTable global_symbol_table = {{.name = NULL, .address = 0}, .next = NULL};

void push_symbol(Symbol s, SymbolTable* st)
{
  if(!st) {
    st = &global_symbol_table;
  }
  SymbolTable* new = malloc(sizeof(SymbolTable));
  new->symbol = s;
  new->next = NULL;
  while(st->next) {
    st = st->next;
  }
  st->next = new;
}

void push_constructed_symbol(char* name, size_t address, SymbolTable* st)
{
  if(!st) {
    st = &global_symbol_table;
  }
  SymbolTable* new = malloc(sizeof(SymbolTable));
  Symbol s = {.name = name, .address = address};
  new->symbol = s;
  new->next = NULL;
  while(st->next) {
    st = st->next;
  }
  st->next = new;
}

Symbol find_symbol(char* name, SymbolTable* st)
{
  if(!st) {
    st = &global_symbol_table;
  }
  while(st->next) {
    if(!strcmp(name, st->symbol.name)) {
        return st->symbol;
    }
    st = st->next;
  }
  Symbol s = {.name = NULL, .offset = 0};
  return s;
}
