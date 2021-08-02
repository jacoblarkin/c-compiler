#include "lexer.h"
#include "token.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct keyword_pair {
  const char* key;
  TokenType type;
};

struct operator_pair {
  char key;
  TokenType type;
};

struct keyword_pair keywords[] = 
{
  {"return", RETURN},
  {"int", INT},
  {NULL, UNKNOWN}
};

struct operator_pair operators[] = 
{
  {'{', LEFT_BRACE},
  {'}', RIGHT_BRACE},
  {'(', LEFT_PAREN},
  {')', RIGHT_PAREN},
  {';', SEMICOLON},
  {'!', LOGICAL_NOT},
  {'~', BITWISE_NOT},
  {'-', MINUS},
  {'+', PLUS},
  {'*', STAR},
  {'/', SLASH},
  { 0 , UNKNOWN}
};

const char* whitespace_delimiters = " \t\r\f\v\n";

void lex_impl(TokenList*, FILE*);
char* get_next_token(FILE*);
TokenType get_token_type(const char*);
int is_operator(char);

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
    new_token.type = UNKNOWN;
    new_token.value = NULL;

    TokenType tt = get_token_type(tok);
    switch(tt) {
    case LEFT_BRACE:
    case RIGHT_BRACE:
    case LEFT_PAREN:
    case RIGHT_PAREN:
    case SEMICOLON:
    case LOGICAL_NOT:
    case BITWISE_NOT:
    case MINUS:
    case PLUS:
    case STAR:
    case SLASH:
    case INT:
    case RETURN:
      new_token.type = tt;
      free(tok);
      break;
    case IDENTIFIER:
    case INT_LITERAL:
    case HEX_LITERAL:
    case OCT_LITERAL:
      new_token.type = tt;
      new_token.value = tok;
      break;
    default:
      free(tok);
      break;
    }

    token_list_push(list, new_token);
    tok = get_next_token(file);
  }
}

char* get_next_token(FILE* file)
{
  int capacity = 256;
  int char_count = 0;
  char* ret = malloc(sizeof(char)*capacity);
  while(1) {
    fpos_t prev;
    fgetpos(file, &prev);
    char c = fgetc(file);
    if(c == EOF) {
      return NULL;
      break;
    }
    if(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
      if(char_count == 0) {
        continue;
      }
      break;
    }
    if(is_operator(c)) {
      if(char_count == 0) {
        ret[0] = c;
        char_count++;
        break;
      }
      fsetpos(file, &prev);
      break;
    }
    ret[char_count] = c;
    char_count++;
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
  for(int i = 0; operators[i].key; i++) {
    if(tok[0] == operators[i].key) {
      return operators[i].type;
    }
  }
  for(int i = 0; keywords[i].key != NULL; i++) {
    int len = strlen(keywords[i].key);
    if(strncmp(tok, keywords[i].key, len+1) == 0) {
      return keywords[i].type;
    }
  }
  return IDENTIFIER;
}

int is_operator(char c)
{
  for(int i = 0; operators[i].key; i++) {
    if(c == operators[i].key) {
      return 1;
    }
  }
  return 0;
}
