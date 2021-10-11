#include "lexer.h"
#include "token.h"
#include "c_lang.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum TokenState_e {
  UNKNOWN_TOK = 0,
  OPERATOR_TOK,
  NUMBER_TOK,
  WORD_TOK
} TokenState;

const char* whitespace_delimiters = " \t\r\f\v\n";
const char* operators_str = "&|^~!(){}[]+-*/%=?:;.,<>";

void lex_impl(TokenList*, FILE*);
char* get_next_token(FILE*);
TokenType get_token_type(const char*);
int is_operator_str(char*);

TokenList* lex(const char* filename)
{
  FILE* file = fopen(filename, "r");
  if(!file) {
    perror("Error");
    exit(1);
  }
  TokenList* token_list = calloc(1, sizeof(TokenList));
  if(!token_list) {
    perror("Error");
    fclose(file);
    exit(1);
  }
  lex_impl(token_list, file);
  fclose(file);
  return token_list;
}

void lex_impl(TokenList* list, FILE* file)
{
  char* tok;
  Token new_token;

  tok = get_next_token(file);
  while(tok) {
    TokenType tt = get_token_type(tok);
    new_token.type = tt;    
    if(token_structs[tt].syntax == IDENTIFIER_ST
        || token_structs[tt].syntax == LITERAL_ST) {
      new_token.value = tok;
    } else {
      new_token.value = NULL;
      free(tok);
    }

    token_list_push(list, new_token);
    tok = get_next_token(file);
  }
}

char* get_next_token(FILE* file)
{
  int capacity = 256;
  int char_count = 0;
  TokenState st = UNKNOWN_TOK;
  char* ret = malloc(sizeof(char)*capacity);
  while(1) {
    fpos_t prev;
    fgetpos(file, &prev);
    char c = fgetc(file);
    if(c == EOF) {
      if(st != UNKNOWN_TOK) {
        fsetpos(file, &prev);
        break;
      }
      free(ret);
      return NULL;
    }
    if(strchr(whitespace_delimiters, c)) {
      if(st == UNKNOWN_TOK) {
        continue;
      }
      break;
    }
    if(strchr(operators_str, c)) {
      if(st != UNKNOWN_TOK && st != OPERATOR_TOK) {
        fsetpos(file, &prev);
        break;
      }
      st = OPERATOR_TOK;
      ret[char_count++] = c;
      ret[char_count] = '\0';
      if(char_count > 1 && !is_operator_str(ret)) {
        fsetpos(file, &prev);
        char_count--;
        break;
      }
      if(char_count == 3) { // No operators >3 chars
        break;
      }
      continue;
    }
    if(st == OPERATOR_TOK) {
      fsetpos(file, &prev);
      break;
    }
    ret[char_count++] = c;
    st = WORD_TOK;
    if(char_count == capacity) {
      capacity += 256;
      ret = realloc(ret, sizeof(char)*capacity);
    }
  }
  ret[char_count] = '\0';
  return ret;
}

TokenType get_token_type(const char* tok)
{
  if(isdigit(tok[0])) {
    if(tok[0] == '0') {
      if( tok[1] == 'x') {
        return HEX_LITERAL;
      }
      return OCT_LITERAL;
    }
    return INT_LITERAL;
  }
  for(int i = 1; token_structs[i].tok_type; i++) {
    if(strcmp(tok, token_structs[i].name) == 0) {
      return token_structs[i].tok_type;
    }
  }
  return IDENTIFIER;
}

int is_operator_str(char* str)
{
  for(int i = 1; token_structs[i].tok_type; i++) {
    if(token_structs[i].syntax == OPERATOR_ST
        && strcmp(str, token_structs[i].name) == 0) {
      return 1;
    }
  }
  return 0;
}
