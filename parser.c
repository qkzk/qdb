#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <stdio.h>
#include "lexer.h"

#define MAXFORMAT 128

void parser_error(const char* format, ...) {
  va_list args;
  va_start(args, format);

  // Allocate memory for formatted string (estimate initial size)
  size_t buffer_size = MAXFORMAT;
  char* formatted_string = (char*)malloc(sizeof(char) * (buffer_size + 13));
  if (formatted_string == NULL) {
    // Handle allocation failure (e.g., print error message)
    return;
  }

  // Use vsnprintf for safe formatting into dynamically allocated buffer
  int bytes_written = vsnprintf(formatted_string, buffer_size, format, args);

  // Check for potential truncation (if bytes_written >= buffer_size)
  // You might need to reallocate a larger buffer and retry if necessary

  fprintf(stderr, "Parser error: ");
  fprintf(stderr, "%s\n", formatted_string);
  free(formatted_string);  // Free the allocated memory

  va_end(args);
}

typedef enum Ast_kind {
  SELECT,      // select
  INSERT,      // insert into
  UPDATE,      // update
  DELETE,      // delete
  CREATE,      // create table
  DROP,        // drop table
  PROJECTION,  // *      "a", "b"
  TABLENAME,   // "users"
  COLNAME,     // "name"
  CONDITION,   // "a" >= 2 AND "b" <= 3
  REL,         // "a" >= 2
  COMP,        // >=
  TYPE,        // int, float, varchar(32)
  LITERAL,     // 32 'super'
  INT,         // 0xff 32 -23
  FLOAT,       // -14.56
  STRING,      // 'bla'
} ast_kind;

const char* ast_kind_names[] = {
    [SELECT] = "SELECT",
    [INSERT] = "INSERT",
    [UPDATE] = "UPDATE",
    [DELETE] = "DELETE",
    [CREATE] = "CREATE",
    [DROP] = "DROP",
    [PROJECTION] = "PROJECTION",
    [TABLENAME] = "TABLENAME",
    [COLNAME] = "COLNAME",
    [CONDITION] = "CONDITION",
    [REL] = "REL",
    [COMP] = "COMP",
    [TYPE] = "TYPE",
    [LITERAL] = "LITERAL",
    [INT] = "INT",
    [FLOAT] = "FLOAT",
    [STRING] = "STRING",
};

void print_ask_kind(ast_kind kind) {
  printf("kind: (%s) - ", ast_kind_names[kind]);
}

typedef struct ASTNode {
  ast_kind kind;
  int nb_tokens;
  char* value;
  struct ASTNode* next;
} ast_node;

void set_leaf(ast_node* leaf) {
  leaf->next = NULL;
}

bool is_leaf(ast_node* node) {
  return node->next == NULL;
}

void print_node(ast_node* node) {
  print_ask_kind(node->kind);
  printf("value (%s) - nb tokens (%d)\n", node->value, node->nb_tokens);
}

void print_ast_rec(ast_node* node, int indent) {
  for (int i = 0; i < indent; i++) {
    printf(" ");
  }
  print_node(node);
  if (!is_leaf(node)) {
    print_ast_rec(node->next, indent + 2);
  }
}

void print_ast(ast_node* root) {
  if (root == NULL) {
    printf("ast is NULL\n");
    return;
  }
  printf("\n\n=== AST START ===\n\n");
  print_ast_rec(root, 0);
  printf("\n=== AST END  ===\n\n");
}

ast_node* parse_statement(token** tokens, int* nb_tokens);
ast_node* parse_drop(token** tokens, int* nb_tokens);
ast_node* parse_insert(token** tokens, int* nb_tokens);
ast_node* parse_tablename(token** tokens, int* nb_tokens);
ast_node* parse_literal(token** tokens, int* nb_tokens);
ast_node* parse_literal_string(token** tokens, int* nb_tokens);
ast_node* parse_literal_postive_integer(token** tokens, int* nb_tokens);
ast_node* parse_literal_negative_integer(token** tokens, int* nb_tokens);
ast_node* parse_literal_positive_float(token** tokens, int* nb_tokens);
ast_node* parse_literal_negative_integer(token** tokens, int* nb_tokens);

/* ast_node* parse_select(token* tokens, int* nb_tokens); */
/* ast_node* parse_update(token* tokens, int* nb_tokens); */
/* ast_node* parse_delete(token* tokens, int* nb_tokens); */
/* ast_node* parse_create(token* tokens, int* nb_tokens); */
/* ast_node* parse_projection(token* tokens, int* nb_tokens); */
/* ast_node* parse_colname(token* tokens, int* nb_tokens); */
/* ast_node* parse_condition(token* tokens, int* nb_tokens); */
/* ast_node* parse_rel(token* tokens, int* nb_tokens); */
/* ast_node* parse_comp(token* tokens, int* nb_tokens); */
/* ast_node* parse_type(token* tokens, int* nb_tokens); */

// true iff read has same kind as current
bool expect(token_kind expected, token* current) {
  return expected == current->kind;
}

bool is_keyword_this(token* keyword, char* value) {
  return strcmp(keyword->value, value) == 0;
}

bool is_token_keyword_something(token* tokens, char* something) {
  return expect(KEYWORD, tokens) && is_keyword_this(tokens, something);
}

bool is_token_keyword_drop(token* tokens) {
  return is_token_keyword_something(tokens, "DROP");
}

bool is_token_keyword_insert(token* tokens) {
  return is_token_keyword_something(tokens, "INSERT");
}

ast_node* create_node_root(ast_kind kind, char* description) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = kind;
  node->nb_tokens = 2;
  char* value = (char*)malloc(sizeof(char) * strlen(description));
  strcpy(value, description);
  node->value = value;
  return node;
}

ast_node* create_node_drop(void) {
  return create_node_root(DROP, "drop_table");
}

ast_node* create_node_insert(void) {
  return create_node_root(INSERT, "insert_into");
}

ast_node* parse_tablename(token** tokens, int* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = TABLENAME;
  node->nb_tokens = 1;
  char* value = (char*)malloc(sizeof(char) * (*tokens)->len);
  assert(value != NULL);
  node->value = value;
  strncpy(node->value, (*tokens)->value, (*tokens)->len);
  *nb_tokens = *nb_tokens - 1;
  return node;
}

ast_node* parse_drop(token** tokens, int* nb_tokens) {
  if (*nb_tokens != 3) {
    parser_error(
        "Wrong number of tokens for 'drop table \"tablename\"': expected 3 got "
        "%d",
        *nb_tokens);
    return NULL;
  }
  if (expect(KEYWORD, *tokens) && is_keyword_this(*tokens, "DROP")) {
    *nb_tokens = *nb_tokens - 1;
    if (expect(KEYWORD, *(tokens + 1)) &&
        is_keyword_this(*(tokens + 1), "TABLE")) {
      *nb_tokens = *nb_tokens - 1;
      ast_node* root = create_node_drop();
      if (expect(IDENTIFIER, *(tokens + 2))) {
        ast_node* child = parse_tablename(tokens + 2, nb_tokens);
        root->next = child;
        set_leaf(child);
        if (*nb_tokens == 0) {
          return root;
        } else {
          parser_error("remaining tokens: ");
        }
      }
    }
  }
  parser_error("Couldn't parse drop statement");
  return NULL;
}

bool is_token_punctuation(token* tok, char* value) {
  return tok->kind == PUNCTUATION &&
         strncmp(tok->value, value, strlen(value)) == 0;
}

bool is_token_operator(token* tok, char* value) {
  return tok->kind == OPERATOR &&
         strncmp(tok->value, value, strlen(value)) == 0;
}

ast_node* parse_literal_string(token** tokens, int* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = STRING;
  node->nb_tokens = 1;
  char* value = (char*)malloc(sizeof(char) * strlen((*tokens)->value));
  strcpy(value, (*tokens)->value);
  node->value = value;
  *nb_tokens -= 1;
  return node;
}

ast_node* parse_literal_negative_integer(token** tokens, int* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = INT;
  node->nb_tokens = 2;
  char* value =
      (char*)malloc(sizeof(char) * (strlen((*(tokens + 1))->value) + 1));
  value[0] = '-';
  strcat(value, (*(tokens + 1))->value);
  node->value = value;
  *nb_tokens -= 2;
  return node;
}

ast_node* parse_literal_postive_integer(token** tokens, int* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = INT;
  node->nb_tokens = 1;
  char* value = (char*)malloc(sizeof(char) * strlen((*tokens)->value));
  strcpy(value, (*tokens)->value);
  node->value = value;
  *nb_tokens -= 1;
  return node;
}

ast_node* parse_literal_positive_float(token** tokens, int* nb_tokens) {
  token* left = *tokens;
  token* right = *(tokens + 2);
  unsigned long len_left = strlen(left->value);
  unsigned long len_right = strlen(right->value);

  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = FLOAT;
  node->nb_tokens = 3;
  char* value = (char*)malloc(sizeof(char) * (len_left + 1 + len_right));
  strcpy(value, left->value);
  value[len_left] = '.';
  strcat(value, right->value);
  node->value = value;
  *nb_tokens -= 3;
  return node;
}

ast_node* parse_literal_negative_float(token** tokens, int* nb_tokens) {
  token* left = *(tokens + 1);
  token* right = *(tokens + 3);
  unsigned long len_left = strlen(left->value);
  unsigned long len_right = strlen(right->value);

  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = FLOAT;
  node->nb_tokens = 4;
  char* value = (char*)malloc(sizeof(char) * (1 + len_left + 1 + len_right));
  value[0] = '-';
  strcat(value, left->value);
  value[len_left + 1] = '.';
  strcat(value, right->value);
  node->value = value;
  *nb_tokens -= 4;
  return node;
}

// string | int | float
ast_node* parse_literal(token** tokens, int* nb_tokens) {
  if ((*tokens)->kind == LITERAL_STRING) {
    // string - consumes 1 token
    return parse_literal_string(tokens, nb_tokens);
  }
  if (is_token_operator(*tokens, "-") &&
      (*nb_tokens < 3 || !is_token_punctuation(*(tokens + 2), "."))) {
    // negative int consumes 2 tokens
    return parse_literal_negative_integer(tokens, nb_tokens);
  }
  if ((*tokens)->kind == NUMBER &&
      (*nb_tokens == 1 || !is_token_punctuation(*(tokens + 1), "."))) {
    // positive int - consumes 1 token
    return parse_literal_postive_integer(tokens, nb_tokens);
  }
  if ((*tokens)->kind == NUMBER &&
      (*nb_tokens > 2 && is_token_punctuation(*(tokens + 1), "."))) {
    // positive float - consumes 3 tokens
    return parse_literal_positive_float(tokens, nb_tokens);
  }
  if (*nb_tokens > 3 && is_token_operator(*tokens, "-") &&
      (*(tokens + 1))->kind == NUMBER &&
      (is_token_punctuation(*(tokens + 2), ".")) &&
      (*(tokens + 3))->kind == NUMBER) {
    // negative float - consumes 4 tokens
    return parse_literal_negative_float(tokens, nb_tokens);
  }

  parser_error("Couldn't parse literal %s", (*tokens)->value);
  return NULL;
}

ast_node* parse_insert(token** tokens, int* nb_tokens) {
  if (expect(KEYWORD, *tokens) && is_keyword_this(*tokens, "INSERT") &&
      expect(KEYWORD, *(tokens + 1)) &&
      is_keyword_this(*(tokens + 1), "INTO")) {
    ast_node* root = create_node_insert();
    tokens = tokens + 2;
    *nb_tokens -= 2;
    ast_node* user;
    if (expect(IDENTIFIER, *tokens)) {
      user = parse_tablename(tokens, nb_tokens);
      root->next = user;
      *nb_tokens -= 1;
      tokens++;
    } else {
      parser_error("Expected tablename after INSERT INTO got %s",
                   (*tokens)->value);
    }
    if (expect(LEFT_PAREN, *tokens)) {
      tokens = tokens + 1;
      *nb_tokens -= 1;
    } else {
      parser_error("Expected ( after \"insert into 'tablename'\" got %s",
                   (*tokens)->value);
      return NULL;
    }
    // ##literal##, ##','## loop
    ast_node* current = user;
    while (true) {
      if (*nb_tokens <= 0) {
        parser_error("Expected a literal but no token left.");
        return NULL;
      }
      ast_node* next = parse_literal(tokens, nb_tokens);
      if (next == NULL) {
        return NULL;
      }
      tokens = tokens + (next->nb_tokens);
      current->next = next;
      current = next;
      if (!is_token_punctuation(*tokens, ",")) {
        set_leaf(current);
        break;
      }
      // advance ,
      *nb_tokens -= 1;
      tokens += 1;
    }
    if (!expect(RIGHT_PAREN, *tokens) || *nb_tokens != 0) {
      parser_error("Expected ) as final token after values");
      return NULL;
    }
    return root;
  }

  parser_error("Couldn't parse insert statement");
  return NULL;
}

ast_node* parse_statement(token** tokens, int* nb_tokens) {
  if (is_token_keyword_drop(*tokens)) {
    return parse_drop(tokens, nb_tokens);
  }
  if (is_token_keyword_insert(*tokens)) {
    return parse_insert(tokens, nb_tokens);
  }
  parser_error("Couldn't parse statement");
  return NULL;
}

void destroy_ast(ast_node* node) {
  ast_node* current = node;
  ast_node* next;
  while (current != NULL) {
    if (current->value != NULL) {
      free(current->value);
    }
    next = current->next;
    free(current);
    current = next;
  }
}

int main(void) {
  char* input;
  /* input = "DROP TABLE \"user\"";  // OKAY */
  input = "INSERT INTO \"user\" (0xff,'abc',123.45)";              // OKAY
  input = "INSERT INTO \"user\" (-19,'abc',-123.45,67,890.0123)";  // OKAY

  token** tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
  assert(tokens != NULL);
  int nb_tokens = lexer(input, tokens);
  for (int i = 0; i < MAXTOKEN; i++) {
    if (tokens[i] == NULL) {
      break;
    }
    print_token(tokens[i]);
  }
  getchar();
  ast_node* root = parse_statement(tokens, &nb_tokens);

  print_ast(root);
  destroy_ast(root);
  destroy_tokens(tokens);

  printf("done\n");

  return 0;
}
