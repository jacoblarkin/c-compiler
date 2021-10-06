#include "lexer.h"
#include "token.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct keyop_pair {
  const char* key;
  TokenType type;
};

struct keyop_pair keywords[] = 
{
  {"break",    BREAK_TOK},
  {"case",     CASE_TOK},
  {"char",     CHAR_TOK},
  {"const",    CONST_TOK},
  {"continue", CONTINUE_TOK},
  {"default",  DEFAULT_TOK},
  {"do",       DO_TOK},
  {"double",   DOUBLE_TOK},
  {"else",     ELSE_TOK},
  {"enum",     ENUM_TOK},
  {"extern",   EXTERN_TOK},
  {"float",    FLOAT_TOK},
  {"for",      FOR_TOK},
  {"goto",     GOTO_TOK},
  {"if",       IF_TOK},
  {"inline",   INLINE_TOK},
  {"int",      INT_TOK},
  {"long",     LONG_TOK},
  {"register", REGISTER_TOK},
  {"restrict", RESTRICT_TOK},
  {"return",   RETURN_TOK},
  {"short",    SHORT_TOK},
  {"signed",   SIGNED_TOK},
  {"sizeof",   SIZEOF_TOK},
  {"static",   STATIC_TOK},
  {"struct",   STRUCT_TOK},
  {"switch",   SWITCH_TOK},
  {"typedef",  TYPEDEF_TOK},
  {"union",    UNION_TOK},
  {"unisnged", UNSIGNED_TOK},
  {"void",     VOID_TOK},
  {"volatile", VOLATILE_TOK},
  {"while",    WHILE_TOK},
  {NULL,       UNKNOWN}
};

struct keyop_pair operators[] = 
{
  {"{", LEFT_BRACE},
  {"}", RIGHT_BRACE},
  {"(", LEFT_PAREN},
  {")", RIGHT_PAREN},
  {"[", LEFT_SQUARE},
  {"]", RIGHT_SQUARE},
  {";", SEMICOLON},
  {"=", ASSIGN},
  {"!", LOGICAL_NOT},
  {"~", BITWISE_NOT},
  {"-", MINUS},
  {"+", PLUS},
  {"*", STAR},
  {"/", SLASH},
  {"&&", AND},
  {"||", OR},
  {"==", EQUAL},
  {"!=", NOTEQUAL},
  {"<", LESSTHAN},
  {"<=", LEQ},
  {">", GREATERTHAN},
  {">=", GEQ},
  {"%", MOD},
  {"&", BIT_AND},
  {"|", BIT_OR},
  {"^", BIT_XOR},
  {"<<", LSHIFT},
  {">>", RSHIFT},
  {"+=", PLUSEQ},
  {"-=", MINUSEQ},
  {"*=", TIMESEQ},
  {"/=", DIVEQ},
  {"%=", MODEQ},
  {"<<=", LSHEQ},
  {">>=", RSHEQ},
  {"&=", ANDEQ},
  {"|=", OREQ},
  {"^=", XOREQ},
  {",", COMMA},
  {"++", PLUSPLUS},
  {"--", MINUSMINUS},
  {"?", QMARK},
  {":", COLON},
  {".", DOT},
  {"->", ARROW},
  { 0 , UNKNOWN}
};

const char* whitespace_delimiters = " \t\r\f\v\n";

void lex_impl(TokenList*, FILE*);
char* get_next_token(FILE*);
TokenType get_token_type(const char*);
int is_operator(char);
int is_keyop_token(TokenType);

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
    if(is_keyop_token(tt)) {
      new_token.type = tt;
      free(tok);
    } else {
      switch(tt){
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
        fgetpos(file, &prev);
        c = fgetc(file);
        if(ret[0] == '=' && c != '=') { // Can't have any op after = other than =
          fsetpos(file, &prev);
        } else if(is_operator(c) && c != ')' && c != '}' 
                                 && c != '(' && c != '{') {
          ret[1] = c;
          char_count++;
          // Check for <<= or >>= (Only possible 3 char operators?)
          if((ret[0]=='<' && ret[1]=='<') || (ret[0]=='>' && ret[0]=='>')) {
            fgetpos(file, &prev);
            c = fgetc(file);
            if(c == '=') { // if <<= or >>=
              ret[2] = c;
              char_count++;
            } else { // if not <<= or >>=
              fsetpos(file, &prev);
            }
          }
        } else { // Only a single char operator
          fsetpos(file, &prev);  
        }
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
    if(strcmp(tok, operators[i].key) == 0) {
      return operators[i].type;
    }
  }
  for(int i = 0; keywords[i].key; i++) {
    if(strcmp(tok, keywords[i].key) == 0) {
      return keywords[i].type;
    }
  }
  return IDENTIFIER;
}

int is_keyop_token(TokenType tt)
{
  for(int i = 0; keywords[i].key; i++) {
    if(tt == keywords[i].type) {
      return 1;
    }
  }
  for(int i = 0; operators[i].key; i++) {
    if(tt == operators[i].type) {
      return 1;
    }
  }
  return 0;
}

int is_operator(char c)
{
  for(int i = 0; operators[i].key; i++) {
    if(c == operators[i].key[0]) {
      return 1;
    }
  }
  return 0;
}
