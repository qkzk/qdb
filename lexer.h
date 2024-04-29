#ifndef _LEXER__
#define _LEXER__

typedef enum Token_kind {
  KEYWORD,         // select insert update, create drop, into set where
  IDENTIFIER,      // variable name ()
  LITERAL_STRING,  // 'abc' "abc"
  NUMBER,          // 123 0123 0x123
  OPERATOR,        // = != < > <= >= + - * / %
  LEFT_PAREN,      // (
  RIGHT_PAREN,     // )
  PUNCTUATION,     // , .
  UNKNOWN,         //
} token_kind;

typedef struct Token {
  char* value;
  int len;
  token_kind kind;
} token;

#define MAXTOKEN 1000
char* repr_kind(token_kind kind) ;
void syntax_error(char* line, int position);
int lexer(char* line, token** tokens) ;
void print_token(token* tok);
void destroy_tokens(token** tokens);
int example_lexer(void);

#endif
