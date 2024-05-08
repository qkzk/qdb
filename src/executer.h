#ifndef _EXECUTER_H__

#include "parser.h"

typedef enum AttrKind {
  D_INT,  // long
  D_FLT,  // double
  D_CHR,  // char*
} attr_kind;
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
typedef struct TableData {
  table_desc* schema;
  size_t nb_rows;
  size_t capacity;
  size_t row_size;
  void* values;
} table_data;
bool execute(char* request);
void print_table(table_data* data);
int example_executer(void);

#define _EXECUTER_H__
#endif // _EXECUTER_H__
