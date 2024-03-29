#include "token.h"

#include <stdlib.h>

Token token_list_pop_back(TokenList* list)
{
  Token tok = {UNKNOWN, NULL};
  if(list && list->last) {
    tok = list->last->tok;
    TokenListNode* new_back = list->last->prev;
    if(new_back) {
      new_back->next = NULL;
    } else { // Clear first if we are at front of list
      list->first = NULL;
    }
    free(list->last);
    list->last = new_back;
  }
  return tok;
}

Token token_list_pop_front(TokenList* list)
{
  Token tok = {UNKNOWN, NULL};
  if(list && list->first) {
    tok = list->first->tok;
    TokenListNode* new_front = list->first->next;
    if(new_front) {
      new_front->prev = NULL;
    } else { // Clear last if we are at end of list
      list->last = NULL;
    }
    free(list->first);
    list->first = new_front;
  }
  return tok;
}

Token token_list_peek_front(TokenList* list)
{
  Token tok = {UNKNOWN, NULL};
  if(list && list->first) {
    tok = list->first->tok;
  }
  return tok;
}

Token token_list_peek_n(TokenList* list, size_t n)
{
  Token tok = {UNKNOWN, NULL};
  if(list && list->first) {
    TokenListNode *node = list->first;
    for(size_t i = 0; i < n; i++) {
      node = node->next;
    }
    tok = node->tok;
  }
  return tok;
}

int token_list_push(TokenList* list, Token tok)
{
  if(!list) {
    return 1;
  }

  TokenListNode* node = malloc(sizeof(TokenListNode));
  if(!node) {
   return 1; 
  }

  node->tok = tok;
  node->prev = list->last;
  node->next = NULL;

  if(list->last) {
    list->last->next = node;
  }
  if(!list->first) {
    list->first = node;
  }

  list->last = node;

  return 0;
}

int token_list_push_front(TokenList* list, Token tok)
{
  if(!list) {
    return 1;
  }

  TokenListNode* node = malloc(sizeof(TokenListNode));
  if(!node) {
   return 1; 
  }

  node->tok = tok;
  node->prev = NULL;
  node->next = list->first;

  if(!list->last) {
    list->last = node;
  }
  if(list->first) {
    list->first->prev = node;
  }

  list->first = node;

  return 0;
}

int token_list_empty(TokenList* list)
{
  if(!list->first && !list->last) {
    return 1;
  }
  return 0;
}
