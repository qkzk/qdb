#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1000

typedef struct Input_Buffer {
  char* buffer;
  size_t capacity;
  ssize_t bytes_read;
} input_buffer;

input_buffer* create_input_buffer(size_t capacity) {
  input_buffer* input = (input_buffer*)malloc(sizeof(input_buffer));
  assert(input != NULL);

  input->capacity = capacity;
  input->bytes_read = EOF;
  input->buffer = NULL;

  return input;
}

void destroy_input(input_buffer* input) {
  free(input->buffer);
  free(input);
}

#define COLUMN_USERNAME 32
#define COLUMN_EMAIL 255

typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME];
  char email[COLUMN_EMAIL];
} Row;

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
  uint32_t num_rows;
  void* pages[TABLE_MAX_PAGES];
} Table;

void serialize_row(Row* source, void* destination) {
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

void* row_slot(Table* table, uint32_t row_num) {
  uint32_t page_num = row_num / ROWS_PER_PAGE;
  void* page = table->pages[page_num];
  if (page == NULL) {
    page = table->pages[page_num] = malloc(PAGE_SIZE);
    assert(page != NULL);
  }
  uint32_t row_offset = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset = row_offset * ROW_SIZE;
  return page + byte_offset;
}

Table* new_table(void) {
  Table* table = (Table*)malloc(sizeof(Table));
  assert(table != NULL);
  table->num_rows = 0;
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    table->pages[i] = NULL;
  }
  return table;
}

void free_table(Table* table) {
  for (int i = 0; table->pages[i] != NULL; i++) {
    free(table->pages[i]);
  }
  free(table);
}

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementType;

typedef struct {
  StatementType type;
  Row row_to_insert;
} Statement;

typedef enum {
  PREPARE_SUCCESS,
  PREPARE_UNRECOGNIZED_COMMAND,
  PREPARE_SYNTAX_ERROR
} PrepareResult;

PrepareResult prepare_statement(input_buffer* input, Statement* statement) {
  if (strncmp(input->buffer, "insert", 6) == 0) {
    statement->type = STATEMENT_INSERT;
    int args_assigned = sscanf(
        input->buffer, "insert %d %s %s", &(statement->row_to_insert.id),
        statement->row_to_insert.username, statement->row_to_insert.email);
    if (args_assigned < 3) {
      return PREPARE_SYNTAX_ERROR;
    }
    return PREPARE_SUCCESS;
  }
  if (strncmp(input->buffer, "select", 6) == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_COMMAND;
}

typedef enum ExecuteResult {
  EXECUTE_SUCCESS,
  EXECUTE_TABLE_FULL
} ExecuteResult;

void print_row(Row* row) {
  printf("%d %s %s\n", row->id, row->username, row->email);
}

ExecuteResult execute_insert(Statement* statement, Table* table) {
  if (table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;
  }
  Row* row_to_insert = &(statement->row_to_insert);

  serialize_row(row_to_insert, row_slot(table, table->num_rows));
  table->num_rows += 1;

  return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table) {
  Row row;
  for (uint32_t i = 0; i < table->num_rows; i++) {
    deserialize_row(row_slot(table, i), &row);
    print_row(&row);
  }
  return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table) {
  switch (statement->type) {
    case (STATEMENT_INSERT):
      return execute_insert(statement, table);
    case (STATEMENT_SELECT):
      return execute_select(statement, table);
  }
}

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

MetaCommandResult meta_command_result(input_buffer* input) {
  if (strcmp(input->buffer, ".exit") == 0) {
    destroy_input(input);
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

void print_prompt(void) {
  printf("db> ");
}

void read_input(input_buffer* input) {
  input->bytes_read = getline(&input->buffer, &input->capacity, stdin);
}

void error_check_input(input_buffer* input) {
  if (input->bytes_read <= 0) {
    fprintf(stderr, "Error reading input\n");
    destroy_input(input);
    exit(EXIT_FAILURE);
  }
  input->bytes_read -= 1;  // remove trailing newlines.
  input->buffer[input->bytes_read] = '\0';
}

bool parse_input(input_buffer* input) {
  if (input->buffer[0] == '.') {
    switch (meta_command_result(input)) {
      case (META_COMMAND_SUCCESS):
        return false;
      case (META_COMMAND_UNRECOGNIZED_COMMAND):
        printf("Unknown command %s\n", input->buffer);
        return false;
    }
  }
  return true;
}

int main(void) {
  Table* table = new_table();
  input_buffer* input = create_input_buffer(MAXLINE);

  while (true) {
    print_prompt();
    read_input(input);
    error_check_input(input);
    if (!parse_input(input)) {
      continue;
    };
    Statement statement;
    switch (prepare_statement(input, &statement)) {
      case (PREPARE_SUCCESS):
        break;
      case (PREPARE_UNRECOGNIZED_COMMAND):
        printf("Unrecognized keyword at start of %s\n", input->buffer);
        continue;
      case (PREPARE_SYNTAX_ERROR):
        printf("Syntax error. Could not parse statement %s\n", input->buffer);
        continue;
    }
    switch (execute_statement(&statement, table)) {
      case (EXECUTE_SUCCESS):
        printf("Executed.\n");
        break;
      case (EXECUTE_TABLE_FULL):
        printf("Error: Table full\n");
        break;
    }
  }

  destroy_input(input);

  exit(EXIT_SUCCESS);
}
