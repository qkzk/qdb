#ifndef __PARSER_H__
#define __PARSER_H__

#include "lexer.h"

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
typedef struct ASTNode {
  ast_kind kind;
  int nb_tokens;
  char* value;
  long i_value;
  double f_value;
  struct ASTNode* left;
  struct ASTNode* right;
} ast_node;
ast_node *parse_statement(token **tokens, int *nb_tokens);
void print_ast(ast_node* root);
void destroy_ast(ast_node *node);

#endif // __PARSER_H__
