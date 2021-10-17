#include "symbol.h"

#include <string.h>

SymbolTable global_symbol_table = {.top = NULL, .next = NULL};

void push_symbol(Symbol s, SymbolTable* st)
{
  if(!st) {
    st = &global_symbol_table;
  }
  SymbolTableNode* new = malloc(sizeof(SymbolTableNode));
  new->symbol = s;
  new->next = st->top;
  st->top = new;
}

void push_constructed_symbol(char* name, size_t address, SymbolTable* st)
{
  if(!st) {
    st = &global_symbol_table;
  }
  SymbolTableNode* new = malloc(sizeof(SymbolTableNode));
  Symbol s = {.name = name, .address = address};
  new->symbol = s;
  new->next = st->top;
  st->top = new;
}

Symbol find_symbol(char* name, SymbolTable* st)
{
  if(!st) {
    st = &global_symbol_table;
  }
  if(!name) {
    return (Symbol){.name = NULL, .offset = 0};
  }
  SymbolTableNode* curr = st->top;
  while(curr->next) {
    // Not sure why this would ever happen.
    // Most likely an error if it does, so this should
    // probably cause an exit.
    if(!(curr->symbol.name)) {
        curr = curr->next;
        continue;
    }
    if(!strcmp(name, curr->symbol.name)) {
        return curr->symbol;
    }
    curr = curr->next;
  }
  Symbol s = {.name = NULL, .offset = 0};
  return s;
}

void remove_symbol(char* name, SymbolTable* st)
{
  if(!st) {
    st = &global_symbol_table;
  }
  if(!name) {
    return ;
  }
  SymbolTableNode* curr = st->top;
  SymbolTableNode* prev = st->top;
  while(curr->next) {
    // Not sure why this would ever happen.
    // Most likely an error if it does, so this should
    // probably cause an exit.
    if(!(curr->symbol.name)) {
        prev = curr;
        curr = curr->next;
        continue;
    }
    if(!strcmp(name, curr->symbol.name)) {
        prev->next = curr->next;
        free(curr);
    }
    prev = curr;
    curr = curr->next;
  }
}

void delete_symbol_table(SymbolTable* st)
{
  if(!st) {
    return ;
  }
  SymbolTableNode* curr = st->top;
  while(curr) {
    SymbolTableNode* next = curr->next;
    free(curr);
    curr = next;
  }
  free(st);
}
