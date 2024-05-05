#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

#define MAXFORMAT 128

#define DEBUG 0

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
  vsnprintf(formatted_string, buffer_size, format, args);

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
  size_t nb_attr;
  attr_desc_size** descs;
} table_desc;

attr_desc_size* desc_int(char* name) {
  attr_desc_size* d_int = (attr_desc_size*)malloc(sizeof(attr_desc_size));
  assert(d_int != NULL);
  size_t len = strlen(name);
  d_int->name = (char*)malloc(sizeof(char) * (len + 1));
  assert(d_int->name != NULL);
  strncpy(d_int->name, name, len);
  d_int->name[len] = '\0';
  d_int->desc = D_INT;
  d_int->size = sizeof(long);

  return d_int;
}

attr_desc_size* desc_float(char* name) {
  attr_desc_size* d_float = (attr_desc_size*)malloc(sizeof(attr_desc_size));
  assert(d_float != NULL);
  size_t len = strlen(name);
  d_float->name = (char*)malloc(sizeof(char) * (len + 1));
  assert(d_float->name != NULL);
  strncpy(d_float->name, name, len);
  d_float->name[len] = '\0';
  d_float->desc = D_FLT;
  d_float->size = sizeof(double);

  return d_float;
}

attr_desc_size* desc_varchar(char* name, long unsigned int nb_char) {
  if (nb_char <= 0) {
    runtime_error(
        "Expected a positive number of chars for varchar description");
    return NULL;
  }
  attr_desc_size* d_varchar = (attr_desc_size*)malloc(sizeof(attr_desc_size));
  assert(d_varchar != NULL);
  size_t len = strlen(name);
  d_varchar->name = (char*)malloc(sizeof(char) * (len + 1));
  assert(d_varchar->name != NULL);
  strncpy(d_varchar->name, name, len);
  d_varchar->name[len] = '\0';
  d_varchar->desc = D_CHR;
  d_varchar->size = sizeof(char) * nb_char;

  return d_varchar;
}

void print_schema(table_desc* td) {
  for (size_t i = 0; i < td->nb_attr; i++) {
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
  if (n_tablename->i_value <= 0) {
    runtime_error("Expected a > 0 number of attributes, got %ld",
                  n_tablename->i_value);
    return NULL;
  }
  size_t nb_attr = (size_t)n_tablename->i_value;

  ast_node* colname = n_tablename->left;
  ast_node* type;
  ast_node* varchar_size;
  attr_desc_size* desc;

  table_desc* table = (table_desc*)malloc(sizeof(table_desc));
  assert(table != NULL);
  size_t tablename_len = strlen(tablename);
  table->name = (char*)malloc(sizeof(char) * (tablename_len + 1));
  assert(table->name != NULL);
  strncpy(table->name, tablename, tablename_len);
  table->name[tablename_len] = '\0';

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
        size_t nb_char = (size_t)varchar_size->i_value;
        desc = desc_varchar(colname->value, nb_char);
        if (desc == NULL) {
          return NULL;
        }
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
  for (size_t i = 0; i < table->nb_attr; i++) {
    data->row_size += table->descs[i]->size;
  }
  data->values = (void*)malloc(data->row_size * data->capacity);
  assert(data->values != NULL);

  return data;
}

void print_page_desc(table_data* data) {
  printf("Table: %s | ", data->schema->name);
  printf("Capacity: %ld rows, used rows: %ld, row size: %ld B\n",
         data->capacity, data->nb_rows, data->row_size);
  print_schema(data->schema);
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
  for (size_t i = 0; i < table->schema->nb_attr; i++) {
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
        data = (void*)curr_row->value;
        if (DEBUG)
          printf("string data in buffer %s - %s\n", (char*)data,
                 curr_row->value);
        break;
      default:
        runtime_error("Unknown value kind");
        return false;
    }
    size_t data_size = table->schema->descs[i]->size;
    memcpy((char*)table->values + row_offset + curr_offset, data, data_size);
    curr_offset += data_size;

    curr_row = curr_row->left;
  }
  table->nb_rows += 1;

  return true;
}

typedef struct ExtractedValue {
  char* colname;
  attr_kind kind;
  union {
    long i;
    double f;
    char* s;
  };
} extracted_value;

bool is_node_comp(ast_node* node) {
  return node->kind == COMP && (strncmp(node->value, "AND", 3) != 0 ||
                                strncmp(node->value, "OR", 2) != 0);
}

bool is_node_and(ast_node* node) {
  return node->kind == COMP && strncmp(node->value, "AND", 3) == 0;
}

bool is_node_or(ast_node* node) {
  return node->kind == COMP && strncmp(node->value, "OR", 2) == 0;
}

#define MAXCOLNAME 256

bool run_where(ast_node* condition,
               size_t nb_attr,
               extracted_value* values[],
               bool* error) {
  // base case
  if (condition == NULL) {
    runtime_error("Condition shouldn't be NULL");
    *error = true;
    return false;
  }
  if (DEBUG) {
    printf("run_where received node:\n");
    print_ast(condition);
  }
  if (is_node_comp(condition)) {
    if (DEBUG)
      printf("Node is a condition\n");
    // switch about condition
    if (condition->left == NULL || condition->right == NULL) {
      runtime_error("Condition should have both children set.");
      *error = true;
      return false;
    }
    // a = 2
    // at least one colname
    if (condition->left->kind != COLNAME && condition->right->kind != COLNAME) {
      runtime_error("Condition should have at least one COLNAME as children");
      *error = true;
      return false;
    }
    // get the colname and its kind (INT, FLOAT, VARCHAR)
    char* colname = (char*)malloc(sizeof(char) * MAXCOLNAME);
    assert(colname != NULL);
    ast_kind colkind;
    bool is_value_right;
    if (condition->left->kind == COLNAME) {
      strcpy(colname, condition->left->value);
      colkind = condition->right->kind;
      is_value_right = true;
      if (DEBUG)
        printf("found colname on left & value on right\n");
    } else {
      strcpy(colname, condition->right->value);
      colkind = condition->left->kind;
      is_value_right = false;
      if (DEBUG)
        printf("found value on left & colname on right\n");
    }
    // return the comparison
    extracted_value* value = NULL;
    long literal_int_value;
    double literal_float_value;
    char* literal_string_value;

    // invalid comparisons :
    //
    // type left != type right
    // literal vs literal
    // float: ==
    // char*: < <= > >=

    switch (colkind) {
      case INT:
        if (DEBUG)
          printf("value is an integer\n");
        literal_int_value = (is_value_right) ? condition->right->i_value
                                             : condition->left->i_value;
        if (DEBUG)
          printf("literal value %ld\n", literal_int_value);

        for (size_t i = 0; i < nb_attr; i++) {
          if (strcmp(values[i]->colname, colname) == 0) {
            value = values[i];
            if (DEBUG)
              printf("table value %ld\n", value->i);
            break;
          }
        }
        if (value == NULL) {
          runtime_error("Couldn't find the colname %s in the table", colname);
          *error = true;
          return false;
        }
        if (strncmp(condition->value, "=", 1) == 0) {
          return value->i == literal_int_value;
        } else if (strncmp(condition->value, "!=", 2) == 0) {
          return value->i != literal_int_value;
        } else if (strncmp(condition->value, "<=", 2) == 0) {
          return value->i <= literal_int_value;
        } else if (strncmp(condition->value, ">=", 2) == 0) {
          return value->i >= literal_int_value;
        } else if (strncmp(condition->value, "<", 1) == 0) {
          return value->i < literal_int_value;
        } else if (strncmp(condition->value, ">", 1) == 0) {
          return value->i > literal_int_value;
        } else {
          runtime_error("invalid comparison between integer %s",
                        condition->value);
          *error = true;
          return false;
        }
        break;
      case FLOAT:
        if (DEBUG)
          printf("value is a float\n");
        literal_float_value = (is_value_right) ? condition->right->f_value
                                               : condition->left->f_value;
        if (DEBUG)
          printf("literal value %f\n", literal_float_value);

        for (size_t i = 0; i < nb_attr; i++) {
          if (strcmp(values[i]->colname, colname) == 0) {
            value = values[i];
            if (DEBUG)
              printf("table value %f\n", value->f);
            break;
          }
        }
        if (value == NULL) {
          runtime_error("Couldn't find the colname %s in the table", colname);
          *error = true;
          return false;
        }
        if (strncmp(condition->value, "<", 1) == 0) {
          return value->f < literal_float_value;
        } else if (strncmp(condition->value, ">", 1) == 0) {
          return value->f > literal_float_value;
        } else {
          runtime_error("invalid comparison between floats %s",
                        condition->value);
          *error = true;
          return false;
        }
        break;
      case STRING:
        if (DEBUG)
          printf("value is a string\n");
        literal_string_value =
            (is_value_right) ? condition->right->value : condition->left->value;
        if (DEBUG)
          printf("literal value %s\n", literal_string_value);

        for (size_t i = 0; i < nb_attr; i++) {
          if (strcmp(values[i]->colname, colname) == 0) {
            value = values[i];
            if (DEBUG)
              printf("table value %s\n", value->s);
            break;
          }
        }
        if (value == NULL) {
          runtime_error("Couldn't find the colname %s in the table", colname);
          *error = true;
          return false;
        }
        if (strncmp(condition->value, "=", 1) == 0) {
          return strcmp(value->s, literal_string_value) == 0;
        } else {
          runtime_error("invalid comparison between strings %s",
                        condition->value);
          *error = true;
          return false;
        }
        break;
        break;
      default:
        runtime_error("Invalid node kind for a comparison value");
        *error = true;
        return false;
        break;
    }

  } else if (is_node_and(condition)) {
    return run_where(condition->left, nb_attr, values, error) &&
           run_where(condition->right, nb_attr, values, error);
  } else if (is_node_or(condition)) {
    return run_where(condition->left, nb_attr, values, error) ||
           run_where(condition->right, nb_attr, values, error);
  } else {
    runtime_error("Expected a condition node");
    return false;
  }
  return true;
}

bool keep_row(table_data* table,
              ast_node* right,
              size_t row_index,
              bool* error) {
  // no where condition, all rows should be kept
  if (right == NULL) {
    return true;
  }
  if (right->left == NULL || right->left->kind != CONDITION) {
    runtime_error("Expected a where condition");
    return false;
  }
  // extract all row values sequentially, store them in union
  size_t nb_attr = table->schema->nb_attr;
  extracted_value* values[nb_attr];

  char* colnames[nb_attr];
  size_t sizes[nb_attr];
  attr_kind kinds[nb_attr];
  size_t offsets[nb_attr];

  size_t offset = 0;
  size_t col_index = 0;
  for (; col_index < nb_attr; col_index++) {
    char* colname = table->schema->descs[col_index]->name;
    colnames[col_index] = (char*)malloc(sizeof(char) * (strlen(colname) + 1));
    assert(colnames[col_index] != NULL);

    strncpy(colnames[col_index], colname, strlen(colname) + 1);
    sizes[col_index] = table->schema->descs[col_index]->size;
    kinds[col_index] = table->schema->descs[col_index]->desc;
    offsets[col_index] = offset;
    offset += table->schema->descs[col_index]->size;
  }
  offsets[col_index] = offset;

  // extract the values

  for (size_t col_index = 0; col_index < nb_attr; col_index++) {
    void* read_value = (void*)malloc(sizes[col_index]);
    assert(read_value != NULL);
    /* printf("offset : %ld, size read: %ld\n", */
    /*        line_index * table->row_size + offsets[col_index], */
    /*        sizes[col_index]); */
    extracted_value* value = (extracted_value*)malloc(sizeof(extracted_value));
    assert(value != NULL);
    value->kind = kinds[col_index];
    value->colname =
        (char*)malloc(sizeof(char*) * (strlen(colnames[col_index]) + 1));
    assert(value->colname != NULL);
    strcpy(value->colname, colnames[col_index]);

    memcpy(
        read_value,
        (char*)table->values + row_index * table->row_size + offsets[col_index],
        sizes[col_index]);
    if (DEBUG)
      printf("%s ", value->colname);
    switch (kinds[col_index]) {
      case D_INT:
        value->i = *(long*)read_value;
        if (DEBUG)
          printf("integer value %ld\n", value->i);
        break;
      case D_FLT:
        value->f = *(double*)read_value;
        break;
      case D_CHR:
        value->s =
            (char*)malloc(sizeof(char) * (strlen((char*)read_value) + 1));
        strcpy(value->s, (char*)read_value);
        if (DEBUG)
          printf("string value %s\n", value->s);
        break;
    }
    values[col_index] = value;
  }
  // explore the tree...
  return run_where(right->left->left, nb_attr, values, error);
}

bool select_from_table(table_data** tables, ast_node* root) {
  if (root->kind != SELECT) {
    runtime_error("Expected a select node");
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
  print_page_desc(table);

  char* projection_colnames[table->schema->nb_attr];
  ast_node* col = n_tablename->left;

  size_t nb_projection = 0;

  for (; nb_projection < table->schema->nb_attr; nb_projection++) {
    if (col == NULL) {
      break;
    }
    size_t colname_len = strlen(col->value);
    projection_colnames[nb_projection] =
        (char*)malloc(sizeof(char) * (colname_len + 1));
    assert(projection_colnames[nb_projection] != NULL);
    strncpy(projection_colnames[nb_projection], col->value, colname_len);
    projection_colnames[nb_projection][colname_len] = '\0';
    col = col->left;
  }

  // search in the schema for sizes, offsets & kinds

  size_t offsets[nb_projection];
  size_t sizes[nb_projection];
  attr_kind kinds[nb_projection];

  for (size_t i = 0; i < nb_projection; i++) {
    char* colname = projection_colnames[i];
    size_t offset = 0;
    for (size_t j = 0; j < table->schema->nb_attr; j++) {
      if (strcmp(colname, table->schema->descs[j]->name) == 0) {
        sizes[i] = table->schema->descs[j]->size;
        kinds[i] = table->schema->descs[j]->desc;
        break;
      }
      offset += table->schema->descs[j]->size;
    }
    offsets[i] = offset;
  }

  // TODO remove, debugging
  for (size_t i = 0; i < nb_projection; i++) {
    printf("column %s, size %ld, offset %ld, kind %d\n", projection_colnames[i],
           sizes[i], offsets[i], kinds[i]);
  }

  // print the columns names
  for (size_t i = 0; i < nb_projection; i++) {
    printf("|   %8s   ", projection_colnames[i]);
  }
  printf("|\n");

  // print the values
  bool* error = (bool*)malloc(sizeof(bool));
  assert(error != NULL);
  *error = false;
  for (size_t row_index = 0; row_index < table->nb_rows; row_index++) {
    // WHERE CONDITION
    if (!keep_row(table, root->right, row_index, error)) {
      continue;
    }
    if (*error) {
      break;
    }
    printf("|");
    for (size_t col_index = 0; col_index < nb_projection; col_index++) {
      void* read_value = (void*)malloc(sizes[col_index]);
      assert(read_value != NULL);
      /* printf("offset : %ld, size read: %ld\n", */
      /*        line_index * table->row_size + offsets[col_index], */
      /*        sizes[col_index]); */
      memcpy(read_value,
             (char*)table->values + row_index * table->row_size +
                 offsets[col_index],
             sizes[col_index]);
      switch (kinds[col_index]) {
        case D_INT:
          printf("  %8ld    |", *(long*)read_value);
          break;
        case D_FLT:
          printf("  %8.3f    |", *(double*)read_value);
          break;
        case D_CHR:
          printf("  %8s     |", (char*)read_value);
          break;
      }
    }
    printf("\n");
  }

  return true;
}

int main(void) {
  table_desc* td = example_create_table_desc();
  print_schema(td);
  printf("done\n");

  // CREATE TABLE

  // clang-format off
  char* input = "CREATE TABLE \"user\" (\"a\" int pk, \"b\" int, \"c\" varchar ( 32 ) )";
  // clang-format on
  printf("\n%s\n", input);
  token** tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
  assert(tokens != NULL);
  size_t nb_tokens = lexer(input, tokens);
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

  input = "INSERT INTO \"user\" (123, 456, 'abc')";
  printf("\n%s\n\n", input);
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

  input = "INSERT INTO \"user\" (789, 123, 'defgh')";
  printf("\n%s\n\n", input);
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

  success = insert_into_table(tables, root);
  assert(success);
  printf("done inserting into existing table\n");

  long* i_data = (long*)malloc(sizeof(long));
  assert(i_data != NULL);
  memcpy(i_data, (char*)tables[0]->values, sizeof(long));
  printf("read long data %ld\n", *i_data);

  long* ii_data = (long*)malloc(sizeof(double));
  assert(ii_data != NULL);
  memcpy(ii_data, (char*)tables[0]->values + sizeof(long), sizeof(long));
  printf("read long data %ld\n", *ii_data);

  char* s_data =
      (char*)malloc(sizeof(char) * tables[0]->schema->descs[2]->size);
  assert(s_data != NULL);
  memcpy(s_data, (char*)tables[0]->values + sizeof(long) + sizeof(long),
         sizeof(char) * tables[0]->schema->descs[2]->size);
  printf("read char data %s\n", s_data);

  input =
      "SELECT \"b\", \"c\", \"a\"  FROM \"user\" WHERE ( \"c\" = 'abc' )";  // OKAY
                                                                            // success

  printf("\n%s\n\n", input);
  tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
  assert(tokens != NULL);
  nb_tokens = lexer(input, tokens);
  for (int i = 0; i < MAXTOKEN; i++) {
    if (tokens[i] == NULL) {
      break;

      return 1;
    }
    print_token(tokens[i]);
  }
  root = parse_statement(tokens, &nb_tokens);

  print_ast(root);

  success = select_from_table(tables, root);
  assert(success);
  printf("done selecting from existing table\n");
  // char* origin = "content";
  // char* copy_1 = (char*)malloc(sizeof(char) * (strlen(origin) + 1));
  // assert(copy_1 != NULL);
  // strcpy(copy_1, origin);
  // assert(strcmp(origin, copy_1) == 0);

  // void* buffer = (void*)malloc(sizeof(char) * (strlen(origin) + 1) +
  //                              sizeof(long) + sizeof(double));
  // assert(buffer != NULL);

  // memcpy(buffer + sizeof(long) + sizeof(double), origin,
  //        sizeof(char) * (strlen(origin) + 1));
  // printf("%s %s %s\n", origin, copy_1,
  //        (char*)(buffer + sizeof(long) + sizeof(double)));

  // char* dest = (char*)malloc(sizeof(char) * 1000);
  // assert(dest != NULL);

  // memcpy(dest, buffer + sizeof(long) + sizeof(double),
  //        sizeof(char) * (strlen(origin) + 1));

  // printf("dest %s\n", dest);

  return 0;
}
