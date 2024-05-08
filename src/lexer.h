#ifndef _LEXER__
#define _LEXER__
#include <stdio.h>

typedef enum Token_kind {
  KEYWORD,         // select insert update, create drop, into set where
  IDENTIFIER,      // variable name ()
  LITERAL_STRING,  // 'abc' "abc"
  NUMBER,          // 123 0123 0x123
  OPERATOR,        // + - * / %
  COMPARISON,      // = != < > <= >=
  LEFT_PAREN,      // (
  RIGHT_PAREN,     // )
  PUNCTUATION,     // , .
  END,             // ; 
  COMMENT,         // //
  UNKNOWN,         //
} token_kind;

typedef struct Token {
  char* value;
  size_t len;
  token_kind kind;
} token;

#define MAXTOKEN 1000
char* repr_kind(token_kind kind) ;
void syntax_error(char* line, size_t position);
size_t lexer(char* line, token** tokens) ;
void print_token(token* tok);
void destroy_tokens(token** tokens);
int example_lexer(void);
void print_tokens(token** tokens, size_t nb_tokens);

#endif // _LEXER__
