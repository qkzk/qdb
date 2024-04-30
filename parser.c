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
  COLNAME_PK,  // "name..."
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
    [COLNAME_PK] = "COLNAME_PK",
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

bool is_token_keyword_create(token* tokens) {
  return is_token_keyword_something(tokens, "CREATE");
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

ast_node* create_node_create(void) {
  return create_node_root(CREATE, "create_table");
}

ast_node* parse_identifier(token** tokens, int* nb_tokens) {
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

ast_node* parse_tablename(token** tokens, int* nb_tokens) {
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
        ast_node* table = parse_tablename(tokens + 2, nb_tokens);
        root->next = table;
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

ast_node* parse_insert(token** tokens, int* nb_tokens) {
  if (expect(KEYWORD, *tokens) && is_keyword_this(*tokens, "INSERT") &&
      expect(KEYWORD, *(tokens + 1)) &&
      is_keyword_this(*(tokens + 1), "INTO")) {
    ast_node* root = create_node_insert();
    tokens = tokens + 2;
    *nb_tokens -= 2;
    ast_node* table;
    if (expect(IDENTIFIER, *tokens)) {
      table = parse_tablename(tokens, nb_tokens);
      root->next = table;
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

ast_node* parse_colname(token** tokens, int* nb_tokens) {
  ast_node* col = parse_identifier(tokens, nb_tokens);
  if (col == NULL) {
    parser_error("Couldn't parse the colname.");
    return NULL;
  }
  col->kind = COLNAME;
  return col;
}

ast_node* parse_type(token** tokens, int* nb_tokens) {
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
    printf("found ##varchar## token\n");
    ast_node* node = create_node_root(TYPE, "varchar");
    node->nb_tokens = 4;
    *nb_tokens -= 1;
    tokens += 1;
    if (!expect(LEFT_PAREN, *tokens)) {
      parser_error("Expected left parenthesis. Got %s", (*tokens)->kind);
      return NULL;
    }
    printf("found varchar ##(## token\n");
    *nb_tokens -= 1;
    tokens += 1;
    ast_node* nb_char = parse_literal_postive_integer(tokens, nb_tokens);
    if (nb_char == NULL) {
      parser_error("Expected a positive integer got %s", (*tokens)->kind);
      return NULL;
    }
    printf("found varchar ( ##int## token\n");
    node->next = nb_char;
    *nb_tokens -= 1;
    tokens += 1;
    if (!expect(RIGHT_PAREN, *tokens)) {
      parser_error("Expected rigt parenthesis. Got %s", (*tokens)->kind);
      return NULL;
    }
    printf("found varchar ( int ##)## token\n");
    *nb_tokens -= 1;
    tokens += 1;
    return node;
  } else {
    parser_error("Couldn't parse type. Expected a valid type got %s",
                 (*tokens)->value);
    return NULL;
  }
}

ast_node* parse_normal_col_desc(token** tokens, int* nb_tokens) {
  /*
  normal-col-desc    ::=     colname, type
  */
  if (*nb_tokens < 2) {
    parser_error("Not enough tokens");
    return NULL;
  }
  ast_node* col = parse_colname(tokens, nb_tokens);
  if (col == NULL) {
    parser_error("Couldn't parse the column name");
    return NULL;
  }
  printf("parse_normal_col_desc: got value %s\n", col->value);
  ast_node* type = parse_type(tokens + 1, nb_tokens);
  if (type == NULL) {
    return NULL;
  }
  col->next = type;
  *nb_tokens -= 1;
  return col;
}

ast_node* parse_primary_column(token** tokens, int* nb_tokens) {
  ast_node* col = create_node_root(COLNAME_PK, (*tokens)->value);
  if (col == NULL) {
    parser_error(
        "Couldn't parse primary column. Impossible to create the colname");
    return NULL;
  }
  col->nb_tokens = 3;
  tokens += 1;
  *nb_tokens -= 1;
  // type 
  ast_node* type = col->next;
  tokens += 2;
  if (is_keyword_this(*tokens, "pk")) {
    col->kind = COLNAME_PK;
    col->nb_tokens = 4;
    type->nb_tokens = 0;
    tokens += 1;
    return col;
  }
  parser_error("Couldn't parse primary column");
  return NULL;
}

ast_node* parse_create(token** tokens, int* nb_tokens) {
  int received_tokens = *nb_tokens;
  printf("parse_create received %d tokens\n", *nb_tokens);
  // clang-format off
  /*
  ( token creates an array of  element 
  ) token closes the array which is attached to the parent as "children", not next
  create-clause ::= 'CREATE', 'TABLE', tablename, '(', colname, type, 'PK', (','
  colname, type )* ')'.


  TODO BUG! wrong number of consumed tokens
  input = "CREATE TABLE \"user\" ( \"a\" int pk , \"b\" float  , \"c\" varchar (   32  )  )";
           1      2     3        4 5     6   7  8  9     10   11 12    13      14  15  16 17     
  17 tokens ;
  === AST START ===

  2     kind: (CREATE) - value (create_table) - nb tokens (2)
  1       kind: (TABLENAME) - value ("user") - nb tokens (1)
  4         kind: (COLNAME_PK) - value ("a") - nb tokens (3)          <----- may be in an array of nodes ? [colpk, col, col]
  0           kind: (TYPE) - value (int) - nb tokens (2)
  3             kind: (TABLENAME) - value ("b") - nb tokens (1)       <----- should be colname 
  0               kind: (TYPE) - value (float) - nb tokens (2)
  7                 kind: (TABLENAME) - value ("c") - nb tokens (1)   <----- should be colname
  0                   kind: (TYPE) - value (varchar) - nb tokens (4)
  0                     kind: (INT) - value (32) - nb tokens (1)

  === AST END  ===
  */
  // clang-format on
  if (expect(KEYWORD, *tokens) && is_token_keyword_create(*tokens) &&
      expect(KEYWORD, *(tokens + 1)) &&
      is_keyword_this(*(tokens + 1), "TABLE")) {
    ast_node* root = create_node_create();
    tokens = tokens + 2;
    *nb_tokens -= 2;
    // tablename
    printf("after 'create table' tokens left %d\n", *nb_tokens);
    assert(*nb_tokens == received_tokens - 2);
    if (!expect(IDENTIFIER, *tokens)) {
      parser_error("Expected a tablename, got %s", (*tokens)->value);
      return NULL;
    }
    ast_node* table = parse_tablename(tokens, nb_tokens);
    root->next = table;
    if (*nb_tokens <= 3) {
      parser_error("not enough tokens");
      return NULL;
    }
    tokens = tokens + 1;
    // (
    if (!expect(LEFT_PAREN, *tokens)) {
      parser_error("expected left parenthesis, got: ", (*tokens)->value);
      return NULL;
    }
    tokens = tokens + 1;
    *nb_tokens -= 1;
    printf("after 'create table tablename (' tokens left %d\n", *nb_tokens);
    assert(*nb_tokens == received_tokens - 4);
    // first column : colname type 'PK'
    ast_node* first_col = parse_primary_column(tokens, nb_tokens);
    tokens = tokens + first_col->nb_tokens;
    table->next = first_col;
    ast_node* current = first_col->next;
    // do we have other columns ?
    if (expect(RIGHT_PAREN, *tokens)) {
      // no other column, advance
      set_leaf(first_col);
      print_ast(root);
      if (*nb_tokens != 1) {
        if ((*tokens) != NULL) {
          printf("last token: %s\n", (*tokens)->value);
        }
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
    ast_node* next;
    while (true) {
      printf("\nnew coldesc: %d tokens left\n", *nb_tokens);
      print_token(*tokens);
      next = parse_identifier(tokens, nb_tokens);
      if (next == NULL) {
        parser_error("Couldn't parse the colname");
        return NULL;
      }
      next->kind = COLNAME;
      tokens += 1;
      current->next = next;
      current = next;
      // current is the colname, which will hold the nb of used tokens for this
      // column.
      if (is_keyword_this(*tokens, "varchar")) {
        // 7 tokens will be used
        // varchar
        current->nb_tokens = 7;
        next = create_node_root(TYPE, (*tokens)->value);
        if (next == NULL) {
          parser_error("Couldn't parse the type");
          return NULL;
        }
        tokens += 1;
        next->nb_tokens = 0;
        current->next = next;
        current = next;
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
        current->next = next;
        next = current;
        // right parent
        if (!expect(RIGHT_PAREN, *tokens)) {
          parser_error("Expected a right parenthesis");
          return NULL;
        }
        tokens += 1;
        *nb_tokens -= 1;
      } else if (is_keyword_this(*tokens, "int") ||
                 is_keyword_this(*tokens, "float")) {
        // int or float
        current->nb_tokens = 3;
        next = create_node_root(TYPE, (*tokens)->value);
        if (next == NULL) {
          parser_error("Couldn't create the node");
          return NULL;
        }
        tokens += 1;
        next->nb_tokens = 0;
        current->next = next;
        next = current;
      } else {
        parser_error("expected a type : int, float or varchar, got %s",
                     (*tokens)->value);
        return NULL;
      }

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
      printf("After parsing a whole column desc. there's %d tokens left\n",
             *nb_tokens);
      print_ast(root);
    }
    set_leaf(current);
    return root;
  }
  return NULL;
}

ast_node* parse_statement(token** tokens, int* nb_tokens) {
  if (is_token_keyword_drop(*tokens)) {
    return parse_drop(tokens, nb_tokens);
  }
  if (is_token_keyword_insert(*tokens)) {
    return parse_insert(tokens, nb_tokens);
  }
  if (is_token_keyword_create(*tokens)) {
    return parse_create(tokens, nb_tokens);
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
  // clang-format off
  input = "INSERT INTO \"user\" (0xff,'abc',123.45)";                                 // OKAY success
  input = "INSERT INTO \"user\" ()";                                                  // OKAY failure
  input = "INSERT INTO \"user\" (-19, 'abc', -1.23, 67, 89.01)";                      // OKAY success
  input = "DROP TABLE";                                                               // OKAY failure
  input = "DROP TABLE \"user\"";                                                      // OKAY success
  input = "CREATE TABLE \"user\" (\"a\" int pk )";                                    // OKAY success
  input = "CREATE TABLE \"user\" (\"a\" int pk, \"b\" float, \"c\" varchar ( 32 ) )"; // OKAY success
  input = "CREATE TABLE \"user\" (\"a\" varchar(32) pk, \"b\" float, \"c\" varchar ( 32 ) )"; // OKAY success
  // clang-format on

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
