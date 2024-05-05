#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  vsnprintf(formatted_string, buffer_size, format, args);

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
  SET,         // set...
  PROJECTION,  // *      "a", "b"
  TABLENAME,   // "users"
  COLNAME,     // "name"
  COLNAME_PK,  // "name..."
  CONDITION,   // "a" >= 2 AND "b" <= 3
  REL,         // "a" >= 2
  COMP,        // >=
  TYPE,        // int, float, varchar(32)
  LITERAL,     // 32 'super'
  INT,         // 0xff 32 -23
  FLOAT,       // -14.56
  STRING,      // 'bla'
  L_PAREN,
} ast_kind;

const char* ast_kind_names[] = {
    [SELECT] = "SELECT",
    [INSERT] = "INSERT",
    [UPDATE] = "UPDATE",
    [DELETE] = "DELETE",
    [CREATE] = "CREATE",
    [DROP] = "DROP",
    [SET] = "SET",
    [PROJECTION] = "PROJECTION",
    [TABLENAME] = "TABLENAME",
    [COLNAME] = "COLNAME",
    [COLNAME_PK] = "COLNAME_PK",
    [CONDITION] = "CONDITION",
    [REL] = "REL",
    [COMP] = "COMP",
    [TYPE] = "TYPE",
    [LITERAL] = "LITERAL",
    [INT] = "INT",
    [FLOAT] = "FLOAT",
    [STRING] = "STRING",
    [L_PAREN] = "LEFT_PAREN",
};

void print_ask_kind(ast_kind kind) {
  printf("kind: (%s) - ", ast_kind_names[kind]);
}

typedef struct ASTNode {
  ast_kind kind;
  size_t nb_tokens;
  char* value;
  long i_value;
  double f_value;
  struct ASTNode* left;
  struct ASTNode* right;
} ast_node;

void set_leaf(ast_node* leaf) {
  leaf->left = NULL;
  leaf->right = NULL;
}

bool is_leaf(ast_node* node) {
  return node->left == NULL && node->right == NULL;
}

void print_node(ast_node* node) {
  print_ask_kind(node->kind);
  printf("value (%s) - nb tokens (%lu)\n", node->value, node->nb_tokens);
}

void print_ast_rec(ast_node* node, int indent) {
  for (int i = 0; i < indent; i++) {
    printf(" ");
  }
  print_node(node);
  if (node->left != NULL) {
    print_ast_rec(node->left, indent + 2);
  }
  if (node->right != NULL) {
    print_ast_rec(node->right, indent + 2);
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

ast_node* parse_statement(token** tokens, size_t* nb_tokens);
ast_node* parse_drop(token** tokens, size_t* nb_tokens);
ast_node* parse_insert(token** tokens, size_t* nb_tokens);
ast_node* parse_tablename(token** tokens, size_t* nb_tokens);
ast_node* parse_literal(token** tokens, size_t* nb_tokens);
ast_node* parse_literal_string(token** tokens, size_t* nb_tokens);
ast_node* parse_literal_postive_integer(token** tokens, size_t* nb_tokens);
ast_node* parse_literal_negative_integer(token** tokens, size_t* nb_tokens);
ast_node* parse_literal_positive_float(token** tokens, size_t* nb_tokens);
ast_node* parse_literal_negative_integer(token** tokens, size_t* nb_tokens);

/* ast_node* parse_select(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_update(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_delete(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_create(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_projection(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_colname(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_condition(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_rel(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_comp(token* tokens, size_t* nb_tokens); */
/* ast_node* parse_type(token* tokens, size_t* nb_tokens); */

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

bool is_token_keyword_create(token* tokens) {
  return is_token_keyword_something(tokens, "CREATE");
}

bool is_token_keyword_delete(token* tokens) {
  return is_token_keyword_something(tokens, "DELETE");
}

bool is_token_keyword_select(token* tokens) {
  return is_token_keyword_something(tokens, "SELECT");
}

bool is_token_keyword_update(token* tokens) {
  return is_token_keyword_something(tokens, "UPDATE");
}

ast_node* create_node_root(ast_kind kind, char* description) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = kind;
  node->nb_tokens = 2;
  char* value = (char*)malloc(sizeof(char) * strlen(description));
  strcpy(value, description);
  node->value = value;
  node->left = NULL;
  node->right = NULL;
  return node;
}

ast_node* create_node_drop(void) {
  return create_node_root(DROP, "drop_table");
}

ast_node* create_node_insert(void) {
  return create_node_root(INSERT, "insert_into");
}

ast_node* create_node_create(void) {
  return create_node_root(CREATE, "create_table");
}

ast_node* parse_identifier(token** tokens, size_t* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = TABLENAME;
  node->nb_tokens = 1;
  size_t len = (*tokens)->len;
  node->value = (char*)malloc(sizeof(char) * (len + 1));
  assert(node->value != NULL);
  strncpy(node->value, (*tokens)->value, len + 1);
  node->value[len] = '\0';
  *nb_tokens = *nb_tokens - 1;
  return node;
}

ast_node* parse_tablename(token** tokens, size_t* nb_tokens) {
  return parse_identifier(tokens, nb_tokens);
}

bool is_token_punctuation(token* tok, char* value) {
  return tok->kind == PUNCTUATION &&
         strncmp(tok->value, value, strlen(value)) == 0;
}

bool is_token_operator(token* tok, char* value) {
  return tok->kind == OPERATOR &&
         strncmp(tok->value, value, strlen(value)) == 0;
}

ast_node* parse_literal_string(token** tokens, size_t* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = STRING;
  node->nb_tokens = 1;
  size_t len = (*tokens)->len;
  char* value = (char*)malloc(sizeof(char) * len);
  strncpy(value, (*tokens)->value, len);
  node->value = value;
  *nb_tokens -= 1;
  return node;
}

ast_node* parse_literal_negative_integer(token** tokens, size_t* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = INT;
  node->nb_tokens = 2;
  char* value =
      (char*)malloc(sizeof(char) * (strlen((*(tokens + 1))->value) + 1));
  value[0] = '-';
  strcat(value, (*(tokens + 1))->value);
  node->value = value;
  node->i_value = atol(value);
  *nb_tokens -= 2;
  return node;
}

ast_node* parse_literal_postive_integer(token** tokens, size_t* nb_tokens) {
  ast_node* node = (ast_node*)malloc(sizeof(ast_node));
  assert(node != NULL);
  node->kind = INT;
  node->nb_tokens = 1;
  char* value = (char*)malloc(sizeof(char) * strlen((*tokens)->value));
  strcpy(value, (*tokens)->value);
  node->value = value;
  node->i_value = atol(value);
  *nb_tokens -= 1;
  return node;
}

ast_node* parse_literal_positive_float(token** tokens, size_t* nb_tokens) {
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
  node->f_value = atof(value);
  printf("value: char %s float %f\n", node->value, node->f_value);
  *nb_tokens -= 3;
  return node;
}

ast_node* parse_literal_negative_float(token** tokens, size_t* nb_tokens) {
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
  node->f_value = atof(value);
  printf("value: char %s float %f\n", node->value, node->f_value);
  *nb_tokens -= 4;
  return node;
}

// string | int | float
ast_node* parse_literal(token** tokens, size_t* nb_tokens) {
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

ast_node* parse_drop(token** tokens, size_t* nb_tokens) {
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
        ast_node* table = parse_tablename(tokens + 2, nb_tokens);
        root->left = table;
        set_leaf(table);
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

ast_node* parse_insert(token** tokens, size_t* nb_tokens) {
  if (expect(KEYWORD, *tokens) && is_keyword_this(*tokens, "INSERT") &&
      expect(KEYWORD, *(tokens + 1)) &&
      is_keyword_this(*(tokens + 1), "INTO")) {
    ast_node* root = create_node_insert();
    tokens = tokens + 2;
    *nb_tokens -= 2;
    ast_node* table;
    if (expect(IDENTIFIER, *tokens)) {
      table = parse_tablename(tokens, nb_tokens);
      root->left = table;
      *nb_tokens -= 1;
      tokens++;
    } else {
      parser_error("Expected tablename after INSERT INTO got %s",
                   (*tokens)->value);
    }
    if (expect(LEFT_PAREN, *tokens)) {
      tokens += 1;
      *nb_tokens -= 1;
    } else {
      parser_error("Expected ( after \"insert into 'tablename'\" got %s",
                   (*tokens)->value);
      return NULL;
    }
    // ##literal##, ##','## loop
    ast_node* current = table;
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
      current->left = next;
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

ast_node* parse_colname(token** tokens, size_t* nb_tokens) {
  ast_node* col = parse_identifier(tokens, nb_tokens);
  if (col == NULL) {
    parser_error("Couldn't parse the colname.");
    return NULL;
  }
  col->kind = COLNAME;
  return col;
}

ast_node* parse_type(token** tokens, size_t* nb_tokens) {
  // type ::= 'varchar', '(', int, ')' | 'int' | 'float'.
  if (is_keyword_this(*tokens, "int")) {
    ast_node* node = create_node_root(TYPE, "int");
    *nb_tokens -= 1;
    tokens += 1;
    return node;
  } else if (is_keyword_this(*tokens, "float")) {
    ast_node* node = create_node_root(TYPE, "float");
    *nb_tokens -= 1;
    tokens += 1;
    return node;
  } else if (is_keyword_this(*tokens, "varchar")) {
    ast_node* node = create_node_root(TYPE, "varchar");
    node->nb_tokens = 4;
    *nb_tokens -= 1;
    tokens += 1;
    if (!expect(LEFT_PAREN, *tokens)) {
      parser_error("Expected left parenthesis. Got %s", (*tokens)->kind);
      return NULL;
    }
    *nb_tokens -= 1;
    tokens += 1;
    ast_node* nb_char = parse_literal_postive_integer(tokens, nb_tokens);
    if (nb_char == NULL) {
      parser_error("Expected a positive integer got %s", (*tokens)->kind);
      return NULL;
    }
    node->left = nb_char;
    *nb_tokens -= 1;
    tokens += 1;
    if (!expect(RIGHT_PAREN, *tokens)) {
      parser_error("Expected rigt parenthesis. Got %s", (*tokens)->kind);
      return NULL;
    }
    *nb_tokens -= 1;
    tokens += 1;
    return node;
  } else {
    parser_error("Couldn't parse type. Expected a valid type got %s",
                 (*tokens)->value);
    return NULL;
  }
}

ast_node* parse_int_float(token** tokens, size_t* nb_tokens) {
  // consumes 1 token
  ast_node* type = create_node_root(TYPE, (*tokens)->value);
  if (type == NULL) {
    parser_error("Couldn't create the node");
    return NULL;
  }
  tokens += 1;
  type->nb_tokens = 0;
  *nb_tokens -= 1;
  return type;
}

ast_node* parse_varchar(token** tokens, size_t* nb_tokens) {
  // consumes 4 tokens

  // 7 tokens will be used
  // varchar
  ast_node* next;
  ast_node* type = create_node_root(TYPE, (*tokens)->value);
  if (type == NULL) {
    parser_error("Couldn't parse the type");
    return NULL;
  }
  tokens += 1;
  type->nb_tokens = 0;
  // left parenthesis
  if (!expect(LEFT_PAREN, *tokens)) {
    parser_error("Expected left parenthesis, got %s", (*tokens)->value);
    return NULL;
  }
  tokens += 1;
  *nb_tokens -= 1;
  // integer
  next = parse_literal_postive_integer(tokens, nb_tokens);
  if (next == NULL) {
    parser_error("Couldn't parse varchar column width");
    return NULL;
  }
  tokens += 1;
  next->nb_tokens = 0;
  type->left = next;
  // right parent
  if (!expect(RIGHT_PAREN, *tokens)) {
    parser_error("Expected a right parenthesis");
    return NULL;
  }
  tokens += 1;
  *nb_tokens -= 1;
  return type;
}

ast_node* parse_primary_column(token** tokens, size_t* nb_tokens) {
  ast_node* col = create_node_root(COLNAME_PK, (*tokens)->value);
  if (col == NULL) {
    parser_error(
        "Couldn't parse primary column. Impossible to create the colname");
    return NULL;
  }
  // actual column name
  if (!expect(IDENTIFIER, *tokens)) {
    parser_error("Expected a column name got %s", (*tokens)->value);
  }
  col->value = (char*)malloc(sizeof(char) * (*tokens)->len);
  strncpy(col->value, (*tokens)->value, (*tokens)->len);
  *nb_tokens -= 1;
  return col;
}

ast_node* parse_create(token** tokens, size_t* nb_tokens) {
  if (!is_token_keyword_create(*tokens) ||
      !is_keyword_this(*(tokens + 1), "TABLE")) {
    parser_error("Expected CREATE TABLE.");
    return NULL;
  }
  // create table
  ast_node* root = create_node_create();
  tokens = tokens + 2;
  *nb_tokens -= 2;

  // tablename
  if (!expect(IDENTIFIER, *tokens)) {
    parser_error("Expected a tablename, got %s", (*tokens)->value);
    return NULL;
  }
  ast_node* table = parse_tablename(tokens, nb_tokens);
  root->left = table;
  if (*nb_tokens <= 3) {
    parser_error("not enough tokens");
    return NULL;
  }
  tokens += 1;

  // (
  if (!expect(LEFT_PAREN, *tokens)) {
    parser_error("expected left parenthesis, got: ", (*tokens)->value);
    return NULL;
  }
  tokens += 1;
  *nb_tokens -= 1;

  int nb_attr = 0;

  // first column : colname type 'PK'
  ast_node* first_col = parse_primary_column(tokens, nb_tokens);
  tokens += 1;
  table->left = first_col;
  nb_attr++;

  ast_node* current = first_col;
  ast_node* next;
  // type of first column
  if (is_keyword_this(*tokens, "varchar")) {
    next = parse_varchar(tokens, nb_tokens);
    current->left = next;
    current->nb_tokens = 7;
    tokens += 4;
    current = current->left->left;
  } else if (is_keyword_this(*tokens, "int") ||
             is_keyword_this(*tokens, "float")) {
    // int or float
    next = parse_int_float(tokens, nb_tokens);
    current->nb_tokens = 3;
    tokens += 1;
    current->left = next;
    current = next;
  } else {
    parser_error("expected a type : int, float or varchar, got %s",
                 (*tokens)->value);
    return NULL;
  }

  // pk
  if (!is_keyword_this(*tokens, "pk")) {
    parser_error("Expected pk, got %s", (*tokens)->value);
    return NULL;
  }
  // account for PK token :
  tokens += 1;
  *nb_tokens -= 1;
  first_col->nb_tokens += 1;

  // do we have other columns ?
  if (expect(RIGHT_PAREN, *tokens)) {
    // no other column, advance
    set_leaf(first_col);
    print_ast(root);
    if (*nb_tokens != 1) {
      parser_error("too much tokens left expected 1 got %d", *nb_tokens);
      return NULL;
    }
    return root;
  }
  if (!is_token_punctuation(*tokens, ",")) {
    parser_error("Expected , got %s", (*tokens)->value);
    return NULL;
  }
  tokens += 1;
  *nb_tokens -= 1;
  // yes, other columns, advance
  /* ast_node* next; */
  while (true) {
    next = parse_identifier(tokens, nb_tokens);
    if (next == NULL) {
      parser_error("Couldn't parse the colname");
      return NULL;
    }
    next->kind = COLNAME;
    tokens += 1;
    current->left = next;
    current = next;
    // current is the colname, which will hold the nb of used tokens for this
    // column.
    if (is_keyword_this(*tokens, "varchar")) {
      next = parse_varchar(tokens, nb_tokens);
      current->left = next;
      current->nb_tokens = 7;
      tokens += 4;
      current = current->left->left;
    } else if (is_keyword_this(*tokens, "int") ||
               is_keyword_this(*tokens, "float")) {
      // int or float
      next = parse_int_float(tokens, nb_tokens);
      current->nb_tokens = 3;
      tokens += 1;
      current->left = next;
      current = next;
    } else {
      parser_error("expected a type : int, float or varchar, got %s",
                   (*tokens)->value);
      return NULL;
    }
    nb_attr++;

    // is the list of column finished ?
    if (expect(RIGHT_PAREN, *tokens)) {
      // yes, break the look
      break;
    } else if (!is_token_punctuation(*tokens, ",")) {
      parser_error("expected a right parenthesis or a comma, got %s",
                   (*tokens)->value);
      return NULL;
    }
    tokens += 1;
    *nb_tokens -= 1;
  }
  set_leaf(current);
  table->i_value = nb_attr;

  return root;
}

#define MAXSTACK 1024

typedef struct StackNode {
  int sp;
  ast_node* nodes[MAXSTACK];
} stack_node;

void push(stack_node* stack, ast_node* node) {
  if (stack->sp + 1 >= MAXSTACK) {
    parser_error("stack is full");
    return;
  }
  stack->nodes[stack->sp++] = node;
}

bool stack_is_empty(stack_node* stack) {
  return stack->sp <= 0;
}

bool stack_is_full(stack_node* stack) {
  return stack->sp >= MAXSTACK;
}

ast_node* pop(stack_node* stack) {
  if (stack->sp <= 0) {
    parser_error("Cannot pop from empty stack");
    return NULL;
  }
  return stack->nodes[--(stack->sp)];
}

ast_node* peek(stack_node* stack) {
  if (stack_is_empty(stack)) {
    parser_error("Cannot peek from empty stack");
    exit(3);
  }
  return stack->nodes[stack->sp - 1];
}

bool is_token_comparison(token* tok) {
  return expect(COMPARISON, tok);
}

bool is_token_where_leaf(token* tok) {
  return expect(IDENTIFIER, tok) || expect(NUMBER, tok) ||
         expect(LITERAL_STRING, tok);
}

ast_node* create_where_leaf(token** tokens, size_t* nb_tokens) {
  ast_node* leaf;
  switch ((*tokens)->kind) {
    case IDENTIFIER:
      leaf = parse_identifier(tokens, nb_tokens);
      leaf->kind = COLNAME;
      break;
    case NUMBER:
      leaf = parse_literal(tokens, nb_tokens);
      break;
    case LITERAL_STRING:
      leaf = parse_literal(tokens, nb_tokens);
      break;
    default:
      parser_error("token isn't a leaf.");
      return NULL;
      break;
  }
  return leaf;
}

ast_node* create_left_paren(void) {
  return create_node_root(L_PAREN, "(");
}

ast_node* create_comparison(token** tokens, size_t* nb_tokens) {
  /* comparison can use 1 or 2 tokens */
  ast_node* comp = create_node_root(COMP, (*tokens)->value);
  if (comp == NULL) {
    parser_error("Couldn't create COMP node");
    return NULL;
  }
  comp->nb_tokens = 1;
  // if the next token is a comparison...
  if (*nb_tokens > 1 && expect(COMPARISON, *(tokens + 1))) {
    if (comp->value[0] == (*(tokens + 1))->value[0]) {
      parser_error("Cannot merge 2 identical comparisons.");
      return NULL;
    }
    comp->value = (char*)realloc(comp->value, sizeof(char) * 3);
    strncat(comp->value, (*(tokens + 1))->value, 1);
    comp->nb_tokens = 2;
    *nb_tokens -= 1;
  }
  return comp;
}

ast_node* parse_where(token** tokens, size_t* nb_tokens) {
  /*
  https://gist.github.com/tomdaley92/507c3a99c56b779144d9c79c0a3900be
  */
  print_token(*tokens);
  ast_node* where = create_node_root(CONDITION, "where");
  if (where == NULL) {
    parser_error("Couldn't create where node");
    return NULL;
  }
  where->nb_tokens = 1;
  tokens += 1;
  *nb_tokens -= 1;

  // create stacks
  stack_node* output;
  output = (stack_node*)malloc(sizeof(stack_node));
  assert(output != NULL);
  output->sp = 0;

  stack_node* comps;
  comps = (stack_node*)malloc(sizeof(stack_node));
  assert(comps != NULL);
  comps->sp = 0;

  // main loop
  while (*nb_tokens > 0) {
    if (expect(LEFT_PAREN, *tokens)) {
      // left parenthesis
      ast_node* left_paren = create_left_paren();
      if (left_paren == NULL) {
        parser_error("Couldn't create left parent");
        return NULL;
      }
      push(comps, left_paren);

    } else if (is_token_where_leaf(*tokens)) {
      // leaf
      ast_node* leaf = create_where_leaf(tokens, nb_tokens);
      if (leaf == NULL) {
        return NULL;
      }
      // a leaf node decrement the number of tokens, it has to be set back
      *nb_tokens += leaf->nb_tokens;
      push(output, leaf);

    } else if (is_token_comparison(*tokens)) {
      // comparison
      while (!stack_is_empty(comps)) {
        if (peek(comps)->kind == L_PAREN) {
          break;
        }
        ast_node* comp = pop(comps);
        if (comp == NULL) {
          return NULL;
        }
        ast_node* right = pop(output);
        if (right == NULL) {
          return NULL;
        }
        ast_node* left = pop(output);
        if (left == NULL) {
          return NULL;
        }
        comp->left = left;
        comp->right = right;
        push(output, comp);
      }
      ast_node* comp = create_comparison(tokens, nb_tokens);
      if (comp == NULL) {
        return NULL;
      }
      if (comp->nb_tokens > 1) {
        tokens += (comp->nb_tokens - 1);
      }
      push(comps, comp);

    } else if (expect(RIGHT_PAREN, *tokens)) {
      // right parenthesis
      while (!stack_is_empty(comps)) {
        if (peek(comps)->kind == L_PAREN) {
          break;
        }
        ast_node* comp = pop(comps);
        if (comp == NULL) {
          return NULL;
        }
        ast_node* right = pop(output);
        if (right == NULL) {
          return NULL;
        }
        ast_node* left = pop(output);
        if (left == NULL) {
          return NULL;
        }
        comp->left = left;
        comp->right = right;
        push(output, comp);
      }
      ast_node* lparen = pop(comps);
      if (lparen == NULL) {
        return NULL;
      }
      if (lparen->kind != L_PAREN) {
        parser_error("Expected a ( from stack, got %s", lparen->value);
        return NULL;
      }

    } else {
      parser_error("Unexpected token in where statement: %s", (*tokens)->kind);
      return NULL;
    }

    // next token
    tokens += 1;
    *nb_tokens -= 1;
  }

  ast_node* left = pop(output);
  if (left == NULL) {
    return NULL;
  }
  where->left = left;
  if (!stack_is_empty(output)) {
    parser_error("Output stack should be empty");
    print_ast(where);
    return NULL;
  }
  return where;
}

ast_node* parse_delete(token** tokens, size_t* nb_tokens) {
  // DELETE FROM tablename
  // DELETE FROM tablename where condition
  if (*nb_tokens < 3) {
    parser_error(
        "Too few tokens for 'delete from \"tablename\"': expected 3 got %d",
        *nb_tokens);
    return NULL;
  }
  if (!is_keyword_this(*tokens, "DELETE")) {
    parser_error("Expected DELETE token");
    return NULL;
  }
  *nb_tokens = *nb_tokens - 1;
  tokens += 1;
  if (!is_keyword_this(*tokens, "FROM")) {
    parser_error("Expected FROM token");
    return NULL;
  }
  tokens += 1;
  ast_node* root = create_node_root(DELETE, "delete");
  if (!expect(IDENTIFIER, *tokens)) {
    parser_error("Expected FROM token");
    return NULL;
  }
  ast_node* table = parse_tablename(tokens, nb_tokens);
  root->left = table;
  if (*nb_tokens == 0) {
    set_leaf(table);
    return root;
  } else {
    *nb_tokens = *nb_tokens - 1;
    tokens += 1;
    ast_node* where = parse_where(tokens, nb_tokens);
    if (where == NULL) {
      return NULL;
    }
    table->left = where;
    return root;
  }

  return NULL;
}

ast_node* parse_select(token** tokens, size_t* nb_tokens) {
  // SELECT "a", "b", "c" FROM tablename (WHERE condition)
  if (*nb_tokens < 3) {
    parser_error("Too few tokens for 'select clause': expected 3 got %d",
                 *nb_tokens);
    return NULL;
  }
  if (!is_keyword_this(*tokens, "SELECT")) {
    parser_error("Expected DELETE token");
    return NULL;
  }
  ast_node* root = create_node_root(SELECT, "select");
  if (root == NULL) {
    parser_error("Couldn't create select node");
    return NULL;
  }
  root->nb_tokens = 1;
  *nb_tokens -= 1;
  tokens += 1;

  ast_node* tablename_left = create_node_root(TABLENAME, "TODO");
  tablename_left->nb_tokens = 0;

  root->left = tablename_left;
  ast_node* current = tablename_left;
  ast_node* next;
  // while identifier loop
  while (true) {
    next = parse_colname(tokens, nb_tokens);
    current->left = next;
    current = next;
    tokens += 1;
    if (is_token_punctuation(*tokens, ",")) {
      *nb_tokens -= 1;
      tokens += 1;
    } else {
      break;
    }
  }
  if (!is_token_keyword_something(*tokens, "FROM")) {
    parser_error("Expected FROM keyword");
    return NULL;
  }
  tokens += 1;
  // FROM tablename
  if (!expect(IDENTIFIER, *tokens)) {
    parser_error("Expected FROM token");
    return NULL;
  }
  ast_node* tablename_right = parse_tablename(tokens, nb_tokens);
  root->right = tablename_right;
  tablename_left->value =
      (char*)malloc(sizeof(char) * strlen(tablename_right->value));
  strncpy(tablename_left->value, tablename_right->value,
          strlen(tablename_right->value));
  /* print_ast(root); */
  if (*nb_tokens <= 1) {
    root->right = NULL;
    return root;
  } else {
    *nb_tokens = *nb_tokens - 1;
    tokens += 1;
    ast_node* where = parse_where(tokens, nb_tokens);
    if (where == NULL) {
      return NULL;
    }
    tablename_right->left = where;
    return root;
  }

  return NULL;
}

ast_node* parse_update(token** tokens, size_t* nb_tokens) {
  if (*nb_tokens < 3) {
    parser_error("Too few tokens for 'update clause': expected 3 got %d",
                 *nb_tokens);
    return NULL;
  }
  if (!is_keyword_this(*tokens, "UPDATE")) {
    parser_error("Expected UPDATE token");
    return NULL;
  }
  ast_node* root = create_node_root(UPDATE, "udpate");
  if (root == NULL) {
    parser_error("Couldn't create update node");
    return NULL;
  }
  root->nb_tokens = 1;
  *nb_tokens -= 1;
  tokens += 1;

  // tablename
  if (!expect(IDENTIFIER, *tokens)) {
    parser_error("Expected tablename token");
    return NULL;
  }
  ast_node* tablename_left = parse_tablename(tokens, nb_tokens);
  root->left = tablename_left;

  if (!expect(IDENTIFIER, *tokens)) {
    parser_error("Expected FROM token");
    return NULL;
  }
  ast_node* tablename_right = parse_tablename(tokens, nb_tokens);
  root->right = tablename_right;

  *nb_tokens += 1;
  tokens += 1;

  // set
  ast_node* set = create_node_root(SET, "set");
  if (set == NULL) {
    parser_error("Couldn't create set node");
    return NULL;
  }
  set->nb_tokens = 1;
  tokens += 1;
  *nb_tokens -= 1;
  tablename_left->left = set;

  print_ast(root);
  ast_node* current = set;
  /* ast_node *next; */
  // while colname identifier loop
  while (true) {
    print_token(*tokens);
    ast_node* next = parse_colname(tokens, nb_tokens);
    if (next == NULL) {
      parser_error("Couldn't parse colname");
      return NULL;
    }
    tokens += 1;
    print_token(*tokens);
    if (!is_token_comparison(*tokens) || (*tokens)->value[0] != '=' ||
        (*tokens)->len != 1) {
      parser_error("Expected =");
      print_token(*tokens);
      return NULL;
    }
    *nb_tokens -= 1;
    tokens += 1;
    print_token(*tokens);
    ast_node* value = parse_literal(tokens, nb_tokens);
    if (value == NULL) {
      parser_error("Expected a literal");
      return NULL;
    }
    next->right = value;
    print_ast(next);
    tokens += value->nb_tokens;
    print_token(*tokens);

    current->left = next;
    current = next;
    if (*tokens == NULL || !is_token_punctuation(*tokens, ",")) {
      break;
    }
    tokens += 1;
    *nb_tokens -= 1;
  }
  print_ast(root);
  print_token(*tokens);
  if (*nb_tokens <= 1) {
    return root;
  } else {
    ast_node* where = parse_where(tokens, nb_tokens);
    if (where == NULL) {
      return NULL;
    }
    tablename_right->left = where;
    return root;
  }

  return NULL;
}

ast_node* parse_statement(token** tokens, size_t* nb_tokens) {
  if (is_token_keyword_drop(*tokens)) {
    return parse_drop(tokens, nb_tokens);
  }
  if (is_token_keyword_insert(*tokens)) {
    return parse_insert(tokens, nb_tokens);
  }
  if (is_token_keyword_create(*tokens)) {
    return parse_create(tokens, nb_tokens);
  }
  if (is_token_keyword_delete(*tokens)) {
    return parse_delete(tokens, nb_tokens);
  }
  if (is_token_keyword_select(*tokens)) {
    return parse_select(tokens, nb_tokens);
  }
  if (is_token_keyword_update(*tokens)) {
    return parse_update(tokens, nb_tokens);
  }
  parser_error("Couldn't parse statement");
  return NULL;
}

void destroy_ast(ast_node* node) {
  if (node == NULL) {
    return;
  }
  /* if (node->value != NULL) { */
  /*   free(node->value); */
  /* } */
  destroy_ast(node->left);
  destroy_ast(node->right);
  free(node);
  /* free(node); */
  /* ast_node* current = node; */
  /* ast_node* next; */
  /* while (current != NULL) { */
  /*   if (current->value != NULL) { */
  /*     free(current->value); */
  /*   } */
  /*   next = current->left; */
  /*   free(current); */
  /*   current = next; */
  /* } */
}

int example_parser(void) {
  char** input;
  input = (char**)malloc(sizeof(char) * 32 * 1999);
  // clang-format off
  // comparison statement
  // input[8] = "WHERE ( \"a\" >= 3 )";                                                             // OKAY success
  // input[9] = "WHERE ( \"a\" = 2 )";                                                              // OKAY success
  // complex condition statement
  // input[10] = "WHERE ( (\"a\" > 1) OR (\"b\" < 2) ) AND (  (\"c\" = 3) OR (\"d\" > 4) )";         // OKAY failure (wrong parenthesis)
  // input[11] = "WHERE (\"a\" >= 3) OR (\"b\" < 3)";                                                // OKAY failure (wrong parenthesis)
  // input[12] = "WHERE ( ( (\"a\" > 1) OR (\"b\" < 2) ) AND ( (\"c\" > 3) OR (\"d\" = 4) ))";       // OKAY success
  // input[13] = "WHERE ( ( (\"a\" <= 1) OR (\"b\" >= 2) ) AND ( (\"c\" != 3) OR (\"d\" == 4) ))";   // OKAY failure (== isn't a valid token)
  // input[14] = "WHERE ( ( (\"a\" <= 1) OR (\"b\" >= 2) ) AND ( (\"c\" != 3) OR (\"d\" = 4) ))";    // OKAY success
  // drop table clause 
  input[13] = "DROP TABLE";                                                                       // OKAY failure
  input[1] = "DROP TABLE \"user\"";                                                              // OKAY success
  // create table clause 
  input[2] = "CREATE TABLE \"user\" (\"a\" int pk )";                                            // OKAY success
  input[3] = "CREATE TABLE \"user\" (\"a\" int pk, \"b\" float, \"c\" varchar ( 32 ) )";         // OKAY success
  input[4] = "CREATE TABLE \"user\" (\"a\" varchar(32) pk, \"b\" float, \"c\" varchar ( 32 ) )"; // OKAY success
  // delete from table 
  input[5] = "DELETE FROM \"user\"";                                                             // OKAY success
  input[6] = "DELETE FROM \"user\" WHERE ( \"a\" = 2 )";                                         // OKAY success
  
  // select 
  input[7] = "SELECT \"a\" FROM \"users\" WHERE ( \"a\" = 2 )";                                                                          // OKAY success
  input[8] = "SELECT \"a\" FROM \"users\"";                                                                                              // OKAY success
  input[9] = "SELECT \"a\", \"b\"  FROM \"users\"";                                                                                      // OKAY success 
  input[10] = "SELECT \"a\" FROM \"users\" WHERE ( ( (\"a\" > 1) OR (\"b\" < 2) ) AND ( (\"c\" > 3) OR (\"d\" = 4) ))";                   // OKAY success
  input[11] = "SELECT \"a\" FROM \"users\" WHERE ( ( (\"a\" <= 1) OR (\"b\" >= 2) ) AND ( (\"c\" != 3) OR (\"d\" = 4) ))";                // OKAY success
  input[12] = "SELECT \"a\", \"b\", \"c\" FROM \"users\" WHERE ( ( (\"a\" <= 1) OR (\"b\" >= 2) ) AND ( (\"c\" != 3) OR (\"d\" = 4) ))";  // OKAY success
  
  // update 
  input[0] = "UPDATE \"users\" set \"a\" = 1";                                                                                           // OKAY success
  input[14] = "UPDATE \"users\" set \"a\" = 1, \"b\" = 'abc', \"c\" = 2.3";                                                               // OKAY success
  input[15] = "UPDATE \"users\" set \"a\" = 1 WHERE ( \"a\" = 2 )";                                                                       // OKAY success
  input[16] = "UPDATE \"users\" set \"a\" = 1, \"b\" = 'abc', \"c\" = 2.3 WHERE ( \"a\" = 2 )";                                           // OKAY success
  input[17] = "UPDATE \"users\" set \"a\" = 1 WHERE ( ( (\"a\" <= 1) OR (\"b\" >= 2) ) AND ( (\"c\" != 3) OR (\"d\" = 4) ) )";            // OKAY success

  // input
  input[18] = "INSERT INTO \"user\" ( 0 , 'abc' , 123.45 )";                                         // OKAY success
  input[19] = "INSERT into";                                                          // OKAY failure
  input[20] = "INSERT INTO \"user\" (0xff,'abc',123.45)";                                         // OKAY success
  input[21] = "INSERT INTO \"user\" ()";                                                          // OKAY failure
  /* input[22] = "INSERT INTO \"user\" (-19, 'abc', -1.23, 67, 123.45)";                              // OKAY success */
  input[22] = "INSERT INTO \"user\" (456, -123.45, 'abc')";
  // clang-format on

  for (int j = 0; j < 23; j++) {
    printf("\n%s\n", input[j]);
    token** tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
    assert(tokens != NULL);
    size_t nb_tokens = lexer(input[j], tokens);
    for (int i = 0; i < MAXTOKEN; i++) {
      if (tokens[i] == NULL) {
        break;
      }
      print_token(tokens[i]);
    }
    ast_node* root = parse_statement(tokens, &nb_tokens);

    print_ast(root);
    /* destroy_ast(root); */
    destroy_tokens(tokens);
    /* free(tokens); */
  }
  free(input);
  printf("done\n");

  return 0;
}
