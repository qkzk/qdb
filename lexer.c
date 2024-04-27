#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum Token_kind {
  KEYWORD,    // select insert update, create drop, into set where
  TABLENAME,  // parser : exists in table list ?
  ATTRNAME,   // parser : exists in tablename attr
  STRING,     // 'abc' "abc"
  NUMBER,     // 123 123.4 0123 0x123
  REL,        // = != < > <= >=
  LPAREN,     // (
  RPAREN,     // )
} token_kind;

typedef enum Keyword {
  SELECT,
  INSERT,
  UPDATE,
  DELETE,
  CREATE,
  DROP,
  INTO,
  SET,
  WHERE,
} keyword;

#define NBKEYWORDS 18
// clang-format off
const char skeywords[NBKEYWORDS][6] = {
  "drop",   "DROP",
  "select", "SELECT", 
  "create", "CREATE",
  "insert", "INSERT", 
  "into",   "INTO", 
  "set",    "SET", 
  "update", "UPDATE", 
  "where",  "WHERE",
  "delete", "DELETE", 
};
// clang-format on

int is_keyword(char* word) {
  for (int i = 0; i < NBKEYWORDS; i++) {
    if (strcmp(word, skeywords[i]) == 0) {
      return i / 2;
    }
  }
  return -1;
}

typedef struct Token {
  char* data;
  int len;
  token_kind kind;
  struct Token* next;
  struct Token* prev;
} token;

token* create_token(char* data, int len, token* prev) {
  token* t = (token*)malloc(sizeof(token));
  assert(t != NULL);
  char* d = (char*)malloc(sizeof(char) * len);
  t->data = d;
  strcpy(t->data, data);
  t->len = len;
  t->prev = prev;
  t->next = NULL;
  return t;
}

#define MAXTOKEN 1000
#define MAXWORD 100
#define MAXWORDS 100

token* split_line(char* line) {
  int line_index = 0;
  int len_word = 0;
  int nb_words = 0;
  char word[MAXWORD];
  char c;

  token* current = (token*)malloc(sizeof(token));
  assert(current != NULL);
  token* next;

  token* root = current;

  while ((c = line[line_index]) != '\0') {
    if (c == ' ' || c == '\t' || c == '\n') {
      if (len_word == 0) {
        continue;
      }
      if (len_word > 0 && word[len_word - 1] == ',') {
        len_word--;
      }
      word[len_word] = '\0';
      next = create_token(word, len_word, current);
      current->next = next;
      current = next;

      nb_words++;
      len_word = 0;
    } else {
      word[len_word] = c;
      len_word++;
    }
    line_index++;
  }
  if (len_word > 0) {
    if (line_index > 0 && line[line_index - 1] == ';') {
      len_word--;
    }
    word[len_word] = '\0';
    next = create_token(word, len_word, current);
    current->next = next;
    current = next;
  }

  return root;
}

int main(void) {
  char* line;
  line = "select a, b, c from tablename where a=2;";
  line = "insert into tablename (x=1, y = 2, z = 3);";
  token* tok = split_line(line);
  getchar();
  while (tok != NULL) {
    printf("token: data %s\n", tok->data);
    tok = tok->next;
  }

  printf("done\n");

  return 0;
}
