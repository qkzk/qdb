#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  UNKNOWN,         //
} token_kind;

char* repr_kind(token_kind kind) {
  switch (kind) {
    case KEYWORD:
      return "keyword";
    case IDENTIFIER:
      return "identifier";
    case LITERAL_STRING:
      return "literal_string";
    case NUMBER:
      return "number";
    case OPERATOR:
      return "operator";
    case COMPARISON:
      return "comparison";
    case LEFT_PAREN:
      return "left_paren";
    case RIGHT_PAREN:
      return "right_paren";
    case PUNCTUATION:
      return "punctuation";
    case END:
      return "end";
    case UNKNOWN:
      return "unknown";
    default:
      return "";
  }
}

#define NBKEYWORDS 34
// clang-format off
const char *skeywords[NBKEYWORDS] = {
  "and",      "AND",
  "create",   "CREATE",
  "delete",   "DELETE",
  "drop",     "DROP",
  "float",    "FLOAT",
  "from",     "FROM",
  "insert",   "INSERT",
  "int",      "INT",
  "into",     "INTO",
  "or",       "OR",
  "pk",       "PK",
  "select",   "SELECT",
  "set",      "SET",
  "table",    "TABLE",
  "update",   "UPDATE",
  "varchar",  "VARCHAR",
  "where",    "WHERE",
};
// clang-format on

int strncicmp(char const* a, char const* b, size_t n) {
  for (size_t i = 0; i < n; i++, a++, b++) {
    int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
    if (d != 0 || !*a)
      return d;
  }
  return 0;
}

bool is_keyword(char* word, size_t len) {
  if (len < 2 || len > 7) {
    return false;
  }

  for (int i = 0; i < NBKEYWORDS; i++) {
    size_t len_keyword = strlen(skeywords[i]);
    if (len_keyword == len && strncicmp(word, skeywords[i], len_keyword) == 0) {
      return true;
    }
  }
  return false;
}

// identifier in sql https://sqlite.org/lang_keywords.html
// identifier are enclosed between double quote "identifier"
bool is_identifier(char* word, size_t len) {
  if (word[0] != '"') {
    return false;
  }
  unsigned long i = 1;
  while (i < len) {
    if (word[i] == '"') {
      break;
    }
    i++;
  }
  return i + 1 == len;
}

// literal strings are enclosed between 'literal_string'
bool is_literal_string(char* word, size_t len) {
  if (word[0] != '\'') {
    return false;
  }
  unsigned long i = 1;
  while (i < len) {
    if (word[i] == '\'') {
      break;
    }
    if (word[i] == '\\') {
      i++;
    }
    i++;
  }
  return i + 1 == len;
}

// https://koor.fr/C/cstdlib/strtol.wp
bool is_number(char* word, size_t len) {
  char* p_end;
  strtod(word, &p_end);
  if (word + len == p_end) {
    return true;
  }
  strtol(word, &p_end, 0);
  if (word + len == p_end) {
    return true;
  }
  return false;
}

#define LEN1COMPARISON 3
const char len_1_comparison[LEN1COMPARISON] = "=<>";

#define LEN2COMPARISON 4
const char len_2_comparison[LEN2COMPARISON][2] = {"!=", "<=", ">=", "OR"};

const char* len_3_comparison = "AND";

#define LEN1OPERATORS 5
const char len_1_operators[LEN1OPERATORS] = "+-*/%";

bool is_comparison(char* word, size_t len) {
  switch (len) {
    case 3:
      return strncmp(word, len_3_comparison, 3) == 0;
    default:
      return false;
      break;
    case 2:
      for (int i = 0; i < LEN2COMPARISON; i++) {
        if (strncmp(word, len_2_comparison[i], 2) == 0) {
          return true;
        }
      }
      return false;
      break;
    case 1:
      for (int i = 0; i < LEN1OPERATORS; i++) {
        if (*word == len_1_comparison[i]) {
          return true;
        }
      }
      return false;
      break;
  }

  return false;
}

bool is_operator(char* word, size_t len) {
  if (len == 1) {
    for (int i = 0; i < LEN1OPERATORS; i++) {
      if (*word == len_1_operators[i]) {
        return true;
      }
    }
  }
  return false;
}

bool is_punctuation(char* word, size_t len) {
  if (len != 1) {
    return false;
  }
  switch (word[0]) {
    case ',':
    case '.':
      return true;
      break;
    default:
      return false;
      break;
  }
}

bool is_left_paren(char* word, size_t len) {
  if (len != 1) {
    return false;
  }
  return word[0] == '(';
}

bool is_right_paren(char* word, size_t len) {
  if (len != 1) {
    return false;
  }
  return word[0] == ')';
}

bool is_end(char* word, size_t len) {
  if (len != 1) {
    return false;
  }
  return word[0] == ';';
}

typedef struct Token {
  char* value;
  size_t len;
  token_kind kind;
} token;

token* new_token(char* value, size_t len, token_kind kind) {
  token* t = (token*)malloc(sizeof(token));
  assert(t != NULL);
  char* v = (char*)malloc(sizeof(char) * (len + 2));
  assert(v != NULL);
  t->value = v;
  strncpy(t->value, value, len + 1);
  t->value[len + 1] = '\0';
  t->len = len + 1;
  t->kind = kind;
  return t;
}

void print_token(token* tok) {
  if (tok == NULL) {
    printf("Token is NULL\n");
    return;
  }
  printf("Token kind %s, data: %s, len %ld\n", repr_kind(tok->kind), tok->value,
         tok->len);
}

token* get_next_token(char* line, size_t* position, size_t line_len) {
  size_t i = 0;
  char c;
  token_kind kind;
  token* tok = NULL;
  while (true) {
    c = line[*position + i];
    if (c == ' ' || c == '\t' || c == '\n') {
      *position = *position + 1;
      continue;
    }
    if (c == '\0' || c == EOF) {
      tok = NULL;
      break;
    }
    if (is_comparison(line + *position, i + 1)) {
      kind = COMPARISON;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_keyword(line + *position, i + 1)) {
      if (*position + i + 1 >= line_len ||
          !is_keyword(line + *position, i + 2)) {
        kind = KEYWORD;
        tok = new_token(line + *position, i, kind);
        break;
      }
    }
    if (is_identifier(line + *position, i + 1)) {
      kind = IDENTIFIER;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_literal_string(line + *position, i + 1)) {
      kind = LITERAL_STRING;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_number(line + *position, i + 1)) {
      kind = NUMBER;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_operator(line + *position, i + 1)) {
      kind = OPERATOR;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_punctuation(line + *position, i + 1)) {
      kind = PUNCTUATION;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_left_paren(line + *position, i + 1)) {
      kind = LEFT_PAREN;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_right_paren(line + *position, i + 1)) {
      kind = RIGHT_PAREN;
      tok = new_token(line + *position, i, kind);
      break;
    }
    if (is_end(line + *position, i + 1)) {
      kind = END;
      tok = new_token(line + *position, i, kind);
      break;
    }
    i++;
  }
  *position += i + 1;
  return tok;
}

#define MAXTOKEN 1000

void syntax_error(char* line, size_t position) {
  fprintf(stderr, "Syntax error column %ld. Unknown token.\n", position);
  fprintf(stderr, "%s\n", line);
  for (size_t i = 0; i + 2 < position; i++) {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "^\n");
}

size_t lexer(char* line, token** tokens) {
  size_t position = 0;
  size_t line_len = strlen(line);
  size_t nb_tokens = 0;
  while (position < line_len) {
    token* tok = get_next_token(line, &position, line_len);
    if (tok == NULL) {
      syntax_error(line, position);
      tokens = NULL;
      return 0;
    }
    tokens[nb_tokens++] = tok;
  }
  if (nb_tokens == 0) {
    perror("Empty request");
    tokens = NULL;
    return 0;
  }
  if (tokens[nb_tokens - 1]->kind != END) {
    perror("Requests must end with ;");
    tokens = NULL;
    return 0;
  }
  return nb_tokens - 1;
}

void destroy_tokens(token** tokens) {
  /* for (int i = 0; i < MAXTOKEN; i++) { */
  /*   if (tokens[i] == NULL) { */
  /*     break; */
  /*   } */
  /*   if (tokens[i]->value != NULL) { */
  /*     free(tokens[i]->value); */
  /*   } */
  /*   free(tokens[i]); */
  /* } */
  free(tokens);
}

void test_comparison_functions(void) {
  assert(is_keyword("SELECT", 6));
  assert(is_keyword("select", 6));
  assert(!is_keyword("selezt", 6));
  assert(!is_keyword("selec", 5));
  assert(is_identifier("\"abc\"", 5));
  assert(is_literal_string("'abc'", 5));
  assert(is_literal_string("'a\\'c'", 6));
  assert(is_number("123", 3));
  assert(is_number("12.3", 4));
  assert(is_number("0xff22aa", 8));
  assert(is_number("00", 2));
  assert(is_operator("*", 1));
  assert(is_comparison("<=", 2));
  assert(is_comparison("!=", 2));
  assert(!is_comparison("<=!", 3));
  assert(is_left_paren("(", 1));
  assert(is_right_paren(")", 1));
  assert(!is_left_paren(")", 1));
  assert(!is_right_paren("(", 1));
}

int example_lexer(void) {
  test_comparison_functions();

  char* line;
  line = "select";
  line = "select * from 'tablename'";
  line = "insert tablename x = 1, y = 2, z = 3";
  line = "select \"a\", \"b\", \"c\" from \"tablename\" where \"a\"=2";
  line = "drop table 'user'";
  line = "WHERE ( \"a\" <= 2 )";
  line = "into";
  /* line = "aze"; */

  token** tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
  assert(tokens != NULL);
  lexer(line, tokens);
  for (int i = 0; i < MAXTOKEN; i++) {
    if (tokens[i] == NULL) {
      break;
    }
    print_token(tokens[i]);
  }

  destroy_tokens(tokens);

  printf("done\n");

  return 0;
}
