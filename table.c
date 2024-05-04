#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

#define MAXFORMAT 128

void runtime_error(const char* format, ...) {
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

  fprintf(stderr, "Runtime error: ");
  fprintf(stderr, "%s\n", formatted_string);
  free(formatted_string);  // Free the allocated memory

  va_end(args);
}
typedef enum AttrKind {
  D_INT,  // long
  D_FLT,  // double
  D_CHR,  // char*
} attr_kind;

static char* repr_attr_kind[3] =
    {[D_INT] = "INT", [D_FLT] = "FLOAT", [D_CHR] = "VARCHAR"};

typedef struct {
  char* name;
  attr_kind desc;
  size_t size;
} attr_desc_size;

typedef struct TableDesc {
  char* name;
  int nb_attr;
  attr_desc_size** descs;
} table_desc;

attr_desc_size* desc_int(char* name) {
  attr_desc_size* d_int = (attr_desc_size*)malloc(sizeof(attr_desc_size));
  assert(d_int != NULL);
  int len = strlen(name);
  d_int->name = (char*)malloc(sizeof(char) * len);
  assert(d_int->name != NULL);
  strncpy(d_int->name, name, len);
  d_int->desc = D_INT;
  d_int->size = sizeof(long);

  return d_int;
}

attr_desc_size* desc_float(char* name) {
  attr_desc_size* d_float = (attr_desc_size*)malloc(sizeof(attr_desc_size));
  assert(d_float != NULL);
  int len = strlen(name);
  d_float->name = (char*)malloc(sizeof(char) * len);
  assert(d_float->name != NULL);
  strncpy(d_float->name, name, len);
  d_float->desc = D_FLT;
  d_float->size = sizeof(double);

  return d_float;
}

attr_desc_size* desc_varchar(char* name, unsigned int nb_char) {
  attr_desc_size* d_varchar = (attr_desc_size*)malloc(sizeof(attr_desc_size));
  assert(d_varchar != NULL);
  int len = strlen(name);
  d_varchar->name = (char*)malloc(sizeof(char) * len);
  assert(d_varchar->name != NULL);
  strncpy(d_varchar->name, name, len);
  d_varchar->desc = D_CHR;
  d_varchar->size = sizeof(char) * nb_char;

  return d_varchar;
}

void print_schema(table_desc* td) {
  printf("Table: %s | ", td->name);
  for (int i = 0; i < td->nb_attr; i++) {
    if (td->descs[i]->desc == D_CHR) {
      printf("%s: %s(%ld) - ", td->descs[i]->name,
             repr_attr_kind[td->descs[i]->desc], td->descs[i]->size);
    } else {
      printf("%s: %s - ", td->descs[i]->name,
             repr_attr_kind[td->descs[i]->desc]);
    }
  }
  printf("\n");
}

table_desc* example_create_table_desc(void) {
  table_desc* td = (table_desc*)malloc(sizeof(table_desc));
  assert(td != NULL);

  td->name = "users";

  td->descs = (attr_desc_size**)malloc(sizeof(attr_desc_size*) * 3);

  // int, float, varchar(32)
  attr_desc_size* d_int = desc_int("id");
  attr_desc_size* d_flt = desc_float("size");
  attr_desc_size* d_chr = desc_varchar("city", 32);

  td->nb_attr = 3;

  td->descs[0] = d_int;
  td->descs[1] = d_flt;
  td->descs[2] = d_chr;

  return td;
}

table_desc* create_table_desc_from_ast(ast_node* root) {
  if (root->kind != CREATE) {
    runtime_error("Expected a create node");
    return NULL;
  }
  ast_node* n_tablename = root->left;
  char* tablename = n_tablename->value;
  int nb_attr = n_tablename->i_value;

  ast_node* colname = n_tablename->left;
  ast_node* type;
  ast_node* varchar_size;
  attr_desc_size* desc;

  table_desc* table = (table_desc*)malloc(sizeof(table_desc));
  assert(table != NULL);
  table->name = (char*)malloc(sizeof(char) * strlen(tablename));
  assert(table->name != NULL);
  strcpy(table->name, tablename);

  table->descs = (attr_desc_size**)malloc(sizeof(attr_desc_size*) * nb_attr);
  assert(table->descs != NULL);
  table->nb_attr = nb_attr;

  int attr_index = 0;

  while (colname != NULL) {
    type = colname->left;
    if (type == NULL) {
      printf("%s\n", colname->value);
      runtime_error("Expected a type node");
      return NULL;
    }
    switch (type->value[0]) {
      case 'i':
        desc = desc_int(colname->value);
        break;
      case 'f':
        desc = desc_float(colname->value);
        break;
      case 'v':
        varchar_size = type->left;
        if (varchar_size == NULL) {
          runtime_error("Expected an int node after varchar");
          return NULL;
        }
        desc = desc_varchar(colname->value, varchar_size->i_value);
        type = varchar_size;
        break;
      default:
        runtime_error("Expected a known type, got %s", type->value);
        return NULL;
    }
    table->descs[attr_index] = desc;

    colname = type->left;
    attr_index++;
  }

  return table;
}

typedef struct TableData {
  table_desc* schema;
  size_t nb_rows;
  size_t capacity;
  size_t row_size;
  void* values;
} table_data;

table_data* create_page_for_table(table_desc* table) {
  table_data* data = (table_data*)malloc(sizeof(table_data));
  assert(data != NULL);
  data->schema = table;
  data->nb_rows = 0;
  data->capacity = 128;
  data->row_size = 0;
  for (int i = 0; i < table->nb_attr; i++) {
    data->row_size += table->descs[i]->size;
  }
  data->values = (void*)malloc(data->row_size * data->capacity);
  assert(data->values != NULL);

  return data;
}

void print_page_desc(table_data* data) {
  print_schema(data->schema);
  printf("Capacity: %ld rows, used rows: %ld, row size: %ld B\n",
         data->capacity, data->nb_rows, data->row_size);
}

#define MAXTABLE 128

table_data* find_table_from_name(table_data** tables, char* name) {
  for (int i = 0; i < MAXTABLE; i++) {
    if (tables[i] == NULL) {
      break;
    }
    if (strncmp(tables[i]->schema->name, name, strlen(name)) == 0) {
      return tables[i];
    }
    tables[i] += 1;
  }

  return NULL;
}

bool insert_into_table(table_data** tables, ast_node* root) {
  if (root->kind != INSERT) {
    runtime_error("Expected an insert node");
    return false;
  }
  ast_node* n_tablename = root->left;
  if (n_tablename == NULL || n_tablename->kind != TABLENAME) {
    runtime_error("Expected a tablename node");
    return false;
  }
  char* tablename = n_tablename->value;

  table_data* table = find_table_from_name(tables, tablename);
  if (table == NULL) {
    runtime_error("Unknown table %s", tablename);
    return false;
  }

  // TODO realloc
  if (table->nb_rows + 1 >= table->capacity) {
    runtime_error("Table %s is full", tablename);
    return false;
  }

  // bla
  void* row_dest = (void*)malloc(table->row_size);
  assert(row_dest != NULL);

  size_t row_offset = table->nb_rows * table->row_size;
  size_t curr_offset = 0;

  ast_node* curr_row = n_tablename->left;
  for (int i = 0; i < table->schema->nb_attr; i++) {
    if (curr_row == NULL ||
        !(curr_row->kind == INT || curr_row->kind == FLOAT ||
          curr_row->kind == STRING)) {
      runtime_error("Expected a value (INT, FLOAT, STRING) node");
      return false;
    }

    void* data;
    switch (curr_row->kind) {
      case INT:
        data = (void*)&curr_row->i_value;
        break;
      case FLOAT:
        data = (void*)&curr_row->f_value;
        break;
      case STRING:
        data = (void*)&curr_row->value;
        break;
      default:
        runtime_error("Unknown value kind");
        return false;
    }
    size_t data_size = table->schema->descs[i]->size;
    memcpy(table->values + row_offset + curr_offset, data, data_size);
    curr_offset += data_size;

    curr_row = curr_row->left;
  }
  table->nb_rows += 1;

  return true;
}

int main(void) {
  table_desc* td = example_create_table_desc();
  print_schema(td);
  printf("done\n");

  // CREATE TABLE

  // clang-format off
  char* input = "CREATE TABLE \"user\" (\"a\" int pk, \"b\" float, \"c\" varchar ( 32 ) )";
  // clang-format on
  printf("\n%s\n", input);
  token** tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
  assert(tokens != NULL);
  int nb_tokens = lexer(input, tokens);
  for (int i = 0; i < MAXTOKEN; i++) {
    if (tokens[i] == NULL) {
      break;
    }
    print_token(tokens[i]);
  }
  ast_node* root = parse_statement(tokens, &nb_tokens);

  print_ast(root);
  table_desc* table_from_ast = create_table_desc_from_ast(root);
  /* print_schema(table_from_ast); */
  table_data* data_from_ast = create_page_for_table(table_from_ast);
  print_page_desc(data_from_ast);
  destroy_tokens(tokens);
  printf("done creating table\n");

  // FIND TABLE BY NAME

  table_data** tables = (table_data**)malloc(sizeof(table_data) * MAXTABLE);
  assert(tables != NULL);

  tables[0] = data_from_ast;
  tables[1] = NULL;

  assert(find_table_from_name(tables, "\"user\"") == data_from_ast);
  printf("done finding table by name\n");

  // INSERT

  input = "INSERT INTO \"user\" ( 456 , 123.45, 'abc' )";  // OKAY success
  printf("\n%s\n", input);
  tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
  assert(tokens != NULL);
  nb_tokens = lexer(input, tokens);
  for (int i = 0; i < MAXTOKEN; i++) {
    if (tokens[i] == NULL) {
      break;
    }
    print_token(tokens[i]);
  }
  root = parse_statement(tokens, &nb_tokens);

  print_ast(root);

  bool success = insert_into_table(tables, root);
  assert(success);
  printf("done inserting into existing table\n");

  long* i_data = (long*)malloc(sizeof(long));
  assert(i_data != NULL);
  memcpy((void*)i_data, tables[0]->values, sizeof(long));
  printf("read long data %ld\n", *i_data);

  double* f_data = (double*)malloc(sizeof(double));
  assert(f_data != NULL);
  memcpy((void*)f_data, tables[0]->values + sizeof(long), sizeof(double));
  printf("read double data %f\n", *f_data);

  char* s_data =
      (char*)malloc(sizeof(char) * tables[0]->schema->descs[2]->size);
  assert(s_data != NULL);
  memcpy(s_data, tables[0]->values + sizeof(long) + sizeof(double),
         sizeof(char) * tables[0]->schema->descs[2]->size);
  printf("read char data %s\n", s_data);

  /* char* origin = "origin"; */
  /* char* copy_1 = (char*)malloc(sizeof(char) * (strlen(origin) + 1)); */
  /* assert(copy_1 != NULL); */
  /* strcpy(copy_1, origin); */
  /* assert(strcmp(origin, copy_1) == 0); */
  /*  */
  /* void* dest = (void*)malloc(sizeof(char) * (strlen(origin) + 1) +
   * sizeof(long) + sizeof(double)); */
  /* assert(dest != NULL); */
  /*  */
  /* memcpy(dest, origin, sizeof(char) * (strlen(origin) + 1)); */
  /* printf("%s %s %s\n", origin, copy_1, (char*)dest); */
  return 0;
}
