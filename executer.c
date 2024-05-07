#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

#define MAXFORMAT 128

#define DEBUG false

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
  printf("\n\n");
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
  data->capacity = 16;
  data->row_size = 0;
  for (size_t i = 0; i < table->nb_attr; i++) {
    data->row_size += table->descs[i]->size;
  }
  data->values = (void*)malloc(data->row_size * data->capacity);
  assert(data->values != NULL);

  return data;
}

table_data* execute_create_table(ast_node* root) {
  return create_page_for_table(create_table_desc_from_ast(root));
}

void print_table(table_data* data) {
  printf("\nSchema of table: %s\n", data->schema->name);
  printf("Capacity: %ld rows, used rows: %ld, row size: %ld B\n\n",
         data->capacity, data->nb_rows, data->row_size);
  print_schema(data->schema);
}

#define MAXTABLES 128

table_data* find_table_from_name(table_data** tables, char* name) {
  for (size_t i = 0; i < MAXTABLES; i++) {
    if (tables[i] == NULL) {
      break;
    }
    if (strncmp(tables[i]->schema->name, name, strlen(name)) == 0) {
      return tables[i];
    }
    // TODO why ???
    /* tables[i] += 1; */
  }

  return NULL;
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

bool compare_extracted_value(extracted_value* a, ast_node* b) {
  switch (a->kind) {
    case D_INT:
      return a->i == b->i_value;
      break;
    case D_FLT:
      return a->f == b->f_value;
      break;
    case D_CHR:
      return strcmp(a->s, b->value) == 0;
      break;
  }
  return false;
}

extracted_value** get_row_values(table_data* table,
                                 size_t row_index,
                                 size_t nb_attr) {
  extracted_value** values =
      (extracted_value**)malloc(sizeof(extracted_value*) * nb_attr);
  assert(values != NULL);

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
    if (DEBUG) {
      printf("%s ", value->colname);
    }
    switch (kinds[col_index]) {
      case D_INT:
        value->i = *(long*)read_value;
        if (DEBUG) {
          printf("integer value %ld\n", value->i);
        }
        break;
      case D_FLT:
        value->f = *(double*)read_value;
        break;
      case D_CHR:
        value->s =
            (char*)malloc(sizeof(char) * (strlen((char*)read_value) + 1));
        strcpy(value->s, (char*)read_value);
        if (DEBUG) {
          printf("string value %s\n", value->s);
        }
        break;
    }
    values[col_index] = value;
  }

  return values;
}

bool execute_insert_into_table(table_data** tables, ast_node* root) {
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

  // increase capacity & realloc
  if (table->nb_rows + 1 >= table->capacity) {
    table->capacity *= 2;
    table->values =
        (void*)realloc(table->values, table->row_size * table->capacity);
    assert(table->values != NULL);
    if (DEBUG) {
      runtime_error("Capacity reached for %s, expanding capacity", tablename);
      print_table(table);
    }
  }

  // allocate for a new row
  void* row_dest = (void*)malloc(table->row_size);
  assert(row_dest != NULL);

  size_t row_offset = table->nb_rows * table->row_size;
  size_t curr_offset = 0;
  ast_node* curr_col = n_tablename->left;

  for (size_t col_index = 0; col_index < table->schema->nb_attr; col_index++) {
    if (curr_col == NULL ||
        !(curr_col->kind == INT || curr_col->kind == FLOAT ||
          curr_col->kind == STRING)) {
      runtime_error("Expected a value (INT, FLOAT, STRING) node");
      return false;
    }

    void* data;
    switch (curr_col->kind) {
      case INT:
        data = (void*)&curr_col->i_value;
        break;
      case FLOAT:
        data = (void*)&curr_col->f_value;
        break;
      case STRING:
        data = (void*)curr_col->value;
        if (DEBUG) {
          printf("string data in buffer %s - %s\n", (char*)data,
                 curr_col->value);
        }
        break;
      default:
        runtime_error("Unknown value kind");
        return false;
    }
    // enforce unicity of Primary key
    if (col_index == 0) {
      if (strlen(curr_col->value) == 0) {
        runtime_error("Primary key can't be null");
        return false;
      }
      for (size_t row_index = 0; row_index < table->nb_rows; row_index++) {
        extracted_value** values =
            get_row_values(table, row_index, table->schema->nb_attr);
        if (compare_extracted_value(*values, curr_col)) {
          runtime_error("Primary key must be unique");
          return false;
        }
      }
    }
    // write the data in the table
    size_t data_size = table->schema->descs[col_index]->size;
    memcpy((char*)table->values + row_offset + curr_offset, data, data_size);
    curr_offset += data_size;

    curr_col = curr_col->left;
  }
  table->nb_rows += 1;

  return true;
}

bool is_node_comp(ast_node* node) {
  return node->kind == COMP && strncmp(node->value, "AND", 3) != 0 &&
         strncmp(node->value, "OR", 2) != 0;
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
    if (DEBUG) {
      printf("Node is a condition\n");
    }
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
      if (DEBUG) {
        printf("found colname on left & value on right\n");
      }
    } else {
      strcpy(colname, condition->right->value);
      colkind = condition->left->kind;
      is_value_right = false;
      if (DEBUG) {
        printf("found value on left & colname on right\n");
      }
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
        if (DEBUG) {
          printf("value is an integer\n");
        }
        literal_int_value = (is_value_right) ? condition->right->i_value
                                             : condition->left->i_value;
        if (DEBUG) {
          printf("literal value %ld\n", literal_int_value);
        }
        for (size_t i = 0; i < nb_attr; i++) {
          if (strcmp(values[i]->colname, colname) == 0) {
            value = values[i];
            if (DEBUG) {
              printf("table value %ld\n", value->i);
            }
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
        if (DEBUG) {
          printf("value is a float\n");
        }
        literal_float_value = (is_value_right) ? condition->right->f_value
                                               : condition->left->f_value;
        if (DEBUG) {
          printf("literal value %f\n", literal_float_value);
        }

        for (size_t i = 0; i < nb_attr; i++) {
          if (strcmp(values[i]->colname, colname) == 0) {
            value = values[i];
            if (DEBUG) {
              printf("table value %f\n", value->f);
            }
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
        if (DEBUG) {
          printf("value is a string\n");
        }
        literal_string_value =
            (is_value_right) ? condition->right->value : condition->left->value;
        if (DEBUG) {
          printf("literal value %s\n", literal_string_value);
        }

        for (size_t i = 0; i < nb_attr; i++) {
          if (strcmp(values[i]->colname, colname) == 0) {
            value = values[i];
            if (DEBUG) {
              printf("table value %s\n", value->s);
            }
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
  size_t nb_attr = table->schema->nb_attr;
  extracted_value** values = get_row_values(table, row_index, nb_attr);
  return run_where(right->left->left, nb_attr, values, error);
}

bool execute_select_from_table(table_data** tables, ast_node* root) {
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
  if (DEBUG) {
    print_table(table);
  }
  if (table == NULL) {
    runtime_error("Unknown table %s", tablename);
    return false;
  }

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

  if (DEBUG) {
    for (size_t i = 0; i < nb_projection; i++) {
      printf("column %s, size %ld, offset %ld, kind %d\n",
             projection_colnames[i], sizes[i], offsets[i], kinds[i]);
    }
  }

  // print the columns names
  printf("\n");
  for (size_t i = 0; i < nb_projection; i++) {
    printf("+--------------");
  }
  printf("+\n");
  for (size_t i = 0; i < nb_projection; i++) {
    printf("|   %8s   ", projection_colnames[i]);
  }
  printf("|\n");
  for (size_t i = 0; i < nb_projection; i++) {
    printf("+--------------");
  }
  printf("+\n");

  // print the values
  bool* error = (bool*)malloc(sizeof(bool));
  assert(error != NULL);
  *error = false;
  for (size_t row_index = 0; row_index < table->nb_rows; row_index++) {
    // WHERE CONDITION
    if (*error) {
      break;
    }
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
          printf("  %8s    |", (char*)read_value);
          break;
      }
    }
    printf("\n");
  }
  for (size_t i = 0; i < nb_projection; i++) {
    printf("+--------------");
  }
  printf("+\n");

  return true;
}

bool execute_drop_table(table_data** tables, ast_node* root, size_t nb_tables) {
  if (DEBUG) {
    print_ast(root);
  }
  if (nb_tables == 0) {
    runtime_error("No table to drop");
    return false;
  }
  if (root->kind != DROP) {
    runtime_error("Expected a DROP node");
    return false;
  }
  ast_node* n_tablename = root->left;
  if (n_tablename == NULL || n_tablename->kind != TABLENAME) {
    runtime_error("Expected a TABLENAME node");
    return false;
  }
  char* tablename = n_tablename->value;

  table_data* table = NULL;
  size_t i = 0;

  // Find the index of the table -- can't use find by name which doesn't return
  // an index
  for (; i < nb_tables; i++) {
    if (tables[i] == NULL) {
      continue;
    }
    if (strncmp(tables[i]->schema->name, tablename, strlen(tablename)) == 0) {
      table = tables[i];
      break;
    }
  }

  if (table == NULL) {
    runtime_error("Can't find table %s", tablename);
    return false;
  }
  if (DEBUG) {
    printf("found table %s index %ld\n", tablename, i);
    print_table(table);
  }
  if (nb_tables > 1) {
    for (size_t j = nb_tables - 2; j >= i; j--) {
      if (tables[j + 1] != NULL) {
        // TODO
        if (DEBUG) {
          printf("moving into %ld from %ld\n", j, j + 1);
        }
        memmove(tables[j], tables[j + 1], sizeof(&tables[j + 1]));
        tables[j]->nb_rows = tables[j + 1]->nb_rows;
        tables[j]->capacity = tables[j + 1]->capacity;
        tables[j]->row_size = tables[j + 1]->row_size;
        memmove(tables[j]->schema, tables[j + 1]->schema,
                sizeof(&(tables[j + 1]->schema)));
        memmove(tables[j]->values, tables[j + 1]->values,
                tables[j + 1]->row_size * tables[j + 1]->capacity);
      }
      if (j == 0) {
        break;
      }
    }
  }
  return true;
}

bool execute_delete_from_table(table_data** tables, ast_node* root) {
  if (DEBUG) {
    print_ast(root);
  }
  if (root->kind != DELETE) {
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
  if (table->nb_rows == 0) {
    runtime_error("Table %s is empty", tablename);
    return false;
  }

  ast_node* where = n_tablename->left;

  // when no where clause, clear the table completely
  if (where == NULL) {
    table->nb_rows = 0;
    return true;
  }
  ast_node* condition = where->left;
  if (condition == NULL) {
    runtime_error("Expected a CONDITION node");
    return false;
  }

  bool* error = (bool*)malloc(sizeof(bool));
  *error = false;

  // find row to delete
  size_t nb_rows = table->nb_rows;
  size_t nb_attr = table->schema->nb_attr;
  for (size_t row_index = table->nb_rows - 1;; row_index--) {
    extracted_value** value = get_row_values(table, row_index, nb_attr);
    if (run_where(condition, nb_attr, value, error)) {
      if (*error) {
        runtime_error("Error while exploring the condition");
        return false;
      }
      if (DEBUG) {
        printf("found row to delete %ld\n", row_index);
      }
      // move every row after j to j
      memmove((char*)table->values + table->row_size * row_index,
              (char*)table->values + table->row_size * (row_index + 1),
              table->row_size * (table->nb_rows - row_index + 1));
      nb_rows -= 1;
      if (nb_rows == 0) {
        break;
      }
    }
    if (row_index == 0) {
      break;
    }
    table->nb_rows = nb_rows;
  }

  return true;
}

bool execute_update_table(table_data** tables, ast_node* root) {
  if (DEBUG) {
    print_ast(root);
  }

  if (root->kind != UPDATE) {
    runtime_error("Expected a, update node");
    return false;
  }
  ast_node* n_tablename = root->left;
  if (n_tablename == NULL || n_tablename->kind != TABLENAME) {
    runtime_error("Expected a tablename node");
    return false;
  }
  char* tablename = n_tablename->value;

  table_data* table = find_table_from_name(tables, tablename);
  if (DEBUG) {
    print_table(table);
  }
  if (table == NULL) {
    runtime_error("Unknown table %s", tablename);
    return false;
  }
  ast_node* set = n_tablename->left;
  if (set == NULL || set->kind != SET) {
    runtime_error("Expected a SET node");
    return false;
  }

  // extract colnames & values from ast
  char* set_colnames[table->schema->nb_attr];
  ast_node* set_values[table->schema->nb_attr];
  ast_node* col = set->left;

  size_t nb_set = 0;

  for (; nb_set < table->schema->nb_attr; nb_set++) {
    if (col == NULL || col->right == NULL) {
      break;
    }
    size_t colname_len = strlen(col->value);
    set_colnames[nb_set] = (char*)malloc(sizeof(char) * (colname_len + 1));
    assert(set_colnames[nb_set] != NULL);
    strncpy(set_colnames[nb_set], col->value, colname_len);
    set_colnames[nb_set][colname_len] = '\0';

    size_t value_len = strlen(col->right->value);
    set_values[nb_set] = (ast_node*)malloc(sizeof(ast_node));
    assert(set_values[nb_set] != NULL);
    set_values[nb_set]->kind = col->right->kind;
    set_values[nb_set]->f_value = col->right->f_value;
    set_values[nb_set]->i_value = col->right->i_value;
    set_values[nb_set]->value = (char*)malloc(sizeof(char) * (value_len + 1));
    assert(set_values[nb_set]->value != NULL);
    strcpy(set_values[nb_set]->value, col->right->value);
    if (DEBUG) {
      printf("update got:\n");
      print_ast(set_values[nb_set]);
    }

    col = col->left;
  }

  // search in the schema for sizes, offsets & kinds

  size_t offsets[nb_set];
  size_t sizes[nb_set];
  attr_kind kinds[nb_set];

  for (size_t i = 0; i < nb_set; i++) {
    char* colname = set_colnames[i];
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

  if (DEBUG) {
    for (size_t i = 0; i < nb_set; i++) {
      printf("column %s, size %ld, offset %ld, kind %d\n", set_colnames[i],
             sizes[i], offsets[i], kinds[i]);
    }
  }

  // set the new values
  bool* error = (bool*)malloc(sizeof(bool));
  assert(error != NULL);
  *error = false;
  for (size_t row_index = 0; row_index < table->nb_rows; row_index++) {
    // WHERE CONDITION
    if (*error) {
      break;
    }
    if (!keep_row(table, root->right, row_index, error)) {
      continue;
    }
    if (*error) {
      break;
    }
    size_t row_offset = table->row_size * row_index;
    for (size_t col_index = 0; col_index < nb_set; col_index++) {
      // enforce unicity of Primary key
      ast_node* set_value = set_values[col_index];

      if (col_index == 0) {
        if (strlen(set_value->value) == 0) {
          runtime_error("Primary key can't be null");
          return false;
        }
        for (size_t row_index = 0; row_index < table->nb_rows; row_index++) {
          extracted_value** values =
              get_row_values(table, row_index, table->schema->nb_attr);
          if (compare_extracted_value(*values, set_value)) {
            runtime_error("Primary key must be unique");
            return false;
          }
        }
      }

      void* data;
      switch (set_value->kind) {
        case INT:
          data = (void*)&set_value->i_value;
          break;
        case FLOAT:
          data = (void*)&set_value->f_value;
          break;
        case STRING:
          data = (void*)set_value->value;
          if (DEBUG) {
            printf("string data in buffer %s - %s\n", (char*)data,
                   set_value->value);
          }
          break;
        default:
          runtime_error("Unknown value kind");
          return false;
      }

      size_t data_size = table->schema->descs[col_index]->size;
      memcpy((char*)table->values + row_offset + offsets[col_index], data,
             data_size);
    }
  }

  return true;
}

static table_data** tables;
static size_t nb_tables;

bool save_tables(char* command) {
  runtime_error(".save : not yet implemented");
  return false;

  const char s[] = " ";
  char* filename;
  strtok(command, s);          // first string
  filename = strtok(NULL, s);  // second string
  printf(".save: saving to ##%s##\n", filename);
  if (strlen(filename) == 0) {
    runtime_error(".save requires a filename: .save data.qdb");
    return false;
  }

  FILE* save_file = fopen(filename, "wb");
  assert(save_file != NULL);

  fwrite(&nb_tables, sizeof(size_t), 1, save_file);
  for (size_t index_table = 0; index_table < nb_tables; index_table++) {
    fwrite(&(tables[index_table]), sizeof(table_data), 1, save_file);
  }

  fclose(save_file);
  return true;
}

bool open_tables(char* command) {
  runtime_error(".open : not yet implemented");
  return false;

  const char s[] = " ";
  char* filename;
  strtok(command, s);          // first string
  filename = strtok(NULL, s);  // second string
  printf(".open: reading from ##%s##\n", filename);
  if (strlen(filename) == 0) {
    runtime_error(".open requires a filename: .open data.qdb");
    return false;
  }
  FILE* save_file = fopen(filename, "rb");
  assert(save_file != NULL);

  fread(&nb_tables, sizeof(size_t), 1, save_file);
  printf("nb_tables = %ld\n", nb_tables);
  size_t index_table = 0;
  for (; index_table < nb_tables; index_table++) {
    tables[index_table] = (table_data*)malloc(sizeof(table_data));
    assert(tables[index_table] != NULL);
    fread(&(tables[index_table]), sizeof(table_data), 1, save_file);
  }
  printf("read %ld tables\n", index_table);

  fclose(save_file);
  return true;
}

bool execute_command(char* command) {
  if (DEBUG) {
    printf("Command: %s\n", command);
  }
  if (strcmp(command, ".exit") == 0) {
    exit(0);
  } else if (strcmp(command, ".tables") == 0) {
    if (nb_tables == 0) {
      printf("No table set.\n");
    }
    for (size_t index_table = 0; index_table < nb_tables; index_table++) {
      print_table(tables[index_table]);
    }
  } else if (strncmp(command, ".save", strlen(".save")) == 0) {
    return save_tables(command);
  } else if (strncmp(command, ".open", strlen(".save")) == 0) {
    return open_tables(command);
  } else {
    printf("Unknown command %s\n", command);
  }
  return true;
}

bool execute_request(char* request) {
  if (tables == NULL) {
    tables = (table_data**)malloc(sizeof(table_data) * MAXTABLES);
  }
  assert(tables != NULL);
  if (strlen(request) == 0) {
    return false;
  }
  if (request[0] == '.') {
    return execute_command(request);
  }
  printf("execute_request: %s\n", request);

  if (DEBUG) {
    printf("\n%s\n", request);
  }

  token** tokens = (token**)malloc(sizeof(token) * MAXTOKEN);
  assert(tokens != NULL);
  size_t nb_tokens = lexer(request, tokens);
  if (nb_tokens == 0) {
    runtime_error("Lexer failed to tokenize the request");
    return false;
  }

  /* if (DEBUG) { */
  printf("\ntokens\n");
  for (int i = 0; i < MAXTOKEN; i++) {
    if (tokens[i] == NULL) {
      break;
    }
    print_token(tokens[i]);
  }
  printf("\n");
  /* } */

  ast_node* root = parse_statement(tokens, &nb_tokens);
  if (root == NULL) {
    runtime_error("Parser failed to analyse the tokens.");
    return false;
  }

  if (DEBUG) {
    print_ast(root);
  }
  table_data* data_from_ast;
  switch (root->kind) {
    case CREATE:
      data_from_ast = execute_create_table(root);
      if (data_from_ast != NULL) {
        tables[nb_tables] = data_from_ast;
        nb_tables++;
        if (DEBUG) {
          printf("done creating table\n");
        }
        return true;
      } else {
        runtime_error("Couldn't create the table");
        return false;
      }
      break;
    case INSERT:
      return execute_insert_into_table(tables, root);
      break;
    case SELECT:
      return execute_select_from_table(tables, root);
      break;
    case DROP:
      if (nb_tables == 0) {
        runtime_error("No table to drop");
        return false;
      }
      bool ret = execute_drop_table(tables, root, nb_tables);
      if (ret) {
        nb_tables -= 1;
      }
      return ret;
      break;
    case DELETE:
      return execute_delete_from_table(tables, root);
      break;
    case UPDATE:
      return execute_update_table(tables, root);
      break;
    default:
      runtime_error("Request %s cannot be ran", root->value);
      return false;
      break;
  }

  return true;
}

int example_executer(void) {
  if (DEBUG) {
    table_desc* td = example_create_table_desc();
    print_schema(td);
    printf("done creating example table\n");
  }

  // clang-format off
  char* request_create_1 = "CREATE TABLE \"to_drop\" (\"a\" int pk, \"b\" int, \"c\" varchar ( 32 ) );";
  char* request_create_2 = "CREATE TABLE \"user\" (\"a\" int pk, \"b\" int, \"c\" varchar ( 32 ) );";
  char* request_insert_1 = "INSERT INTO \"user\" (123, 456, 'abc');";
  char* request_insert_2 = "INSERT INTO \"user\" (789, 123, 'defgh');";
  char* request_insert_3 = "INSERT INTO \"user\" (789, 333, 'xyz');";
  char* request_insert_4 = "INSERT INTO \"user\" (102, 123, 'tuv');";
  /* char* request_select = "SELECT \"b\", \"c\", \"a\"  FROM \"user\" WHERE ( \"c\" = 'abc' );"; */
  char* request_select_1 = "SELECT \"b\", \"c\", \"a\"  FROM \"user\" WHERE (( \"c\" = 'abc' ) OR ( \"b\" = 123 ));";
  char* request_drop     = "DROP TABLE \"to_drop\";";
  char* request_delete_1 = "DELETE FROM \"user\" WHERE ( \"b\" = 123 );";
  char* request_delete_2 = "DELETE FROM \"user\";";
  char* request_select_2 = "SELECT \"b\", \"c\", \"a\"  FROM \"user\" WHERE (\"a\" = 123 );";
  char* request_select_3 = "SELECT \"b\", \"c\", \"a\"  FROM \"user\";";
  char* request_update_1 = "UPDATE  \"user\" SET \"a\" = 999, \"b\" = 3  WHERE (\"a\" = 123);";
  char* request_update_2 = "update  \"user\" SET \"a\" = 999  WHERE (\"a\" = 789);";
  // clang-format on

  assert(execute_request(request_create_1));
  assert(execute_request(request_create_2));
  assert(execute_request(request_insert_1));
  assert(execute_request(request_insert_2));
  assert(!execute_request(request_insert_3));  // duplicate primary key
  assert(execute_request(request_insert_4));
  assert(execute_request(request_drop));
  assert(!execute_request(request_drop));  // already dropped
  assert(execute_request(request_select_1));
  assert(execute_request(request_delete_1));
  assert(execute_request(request_select_1));
  assert(execute_request(request_delete_2));
  assert(execute_request(request_select_2));
  assert(execute_request(request_insert_1));
  assert(execute_request(request_insert_2));
  assert(execute_request(request_insert_4));
  assert(execute_request(request_select_3));
  assert(execute_request(request_update_1));
  assert(execute_request(request_select_3));
  assert(!execute_request(request_update_2));  // duplicate primary key
  assert(execute_request(request_select_3));

  return 0;
}
