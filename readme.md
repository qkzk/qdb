# QDB

Remake a simple SQLite like database in C

## Ideas from sqlite

### Core

1. interface
2. SQL Command processor
   1. Lexer
   2. Parser
   3. Code generator
3. VM

### Backend

4. B-Tree
5. Pager
6. OS Interface

### Accessories

- Utilities
- Tests

## Sqlite architecture

![Architecture of sqlite](img/img-2024-04-26-18-03.png)

## Sources

- [sqlite tutorial](https://www.sqlitetutorial.net/)
- [sqlite official documentation](https://www.sqlite.org/)
- [cstack db tutorial](https://cstack.github.io/db_tutorial/parts/part1.html)

# Process

ATM not everything is implemented.

1. Start it with `$ .qdb`
   NYI use `$ . repl`
2. `qdb> ` type a command (`.something`) or a request.
3. Request / Command is [read](./repl.c), [tokenzide](./lexer.c), [parsed](./parser.c) and [executed](./executer.c)

   - If the request success, nothing happens except the output of `SELECT...` requests.
   - If the request fails, some error messages are displayed:
     - `Syntax error` from the lexer,
     - `Parser error` from the parser,
     - `Runtime error` from the executer.

   Those errors bubble and only the first message can tell you exactly what happened.

## Commands

Commands aren't sql requests but act on the whole database.

- `.tables` display all tables in memory
- `.read req.sql` read and execute every line from `req.sql`. It stops at first failure. `.read` can't call itself recursivelly.
- NYI `.open database.qdb` loads a database from `database.qdb` file in memory, without wiping the current tables. It may overwrite what is currently in memory.
- NYI `.save database.qdb` saves in memory tables into `database.qdb` file.

## Requests Syntax

This is a simplified version of SQL. Since it's a hobby project I won't do much more.

```ebnf
statement          ::=     select-clause | insert-clause | update-clause | delete-clause | create-clause | drop-clause.


select-clause      ::=     'SELECT', projection, 'FROM', tablename ( 'WHERE' condition );.
insert-clause      ::=     'INSERT', 'INTO', tablename, 'VALUES', '(', literal (',' literal)*, ')';.
update-clause      ::=     'UPDATE', tablename, 'SET', colname, '=', literal (',' colname = literal)* ( 'WHERE', condition );.
delete-clause      ::=     'DELETE', 'FROM', tablename, ( 'WHERE', condition );.
create-clause      ::=     'CREATE', 'TABLE', tablename, '(', pk-description, (',' normal-col-desc )* ')';.
drop-clause        ::=     'DROP', 'TABLE', tablename;.

projection         ::=     colname (',' colname)* ) | *.

colname            ::=     identifier.
tablename          ::=     identifier.
identifier         ::=     "'", name, "'".
name               ::=     char(char)*.

normal-col-desc    ::=     colname, type
pk-description     ::=     normal-col-desc, 'PK'

condition          ::=     rel | '(', rel, ')'  ( 'AND', condition )* ( 'OR', condition )* .
rel                ::=     colname, comp-operator, literal.
comp-operator      ::=     '=' | '<' | '>' | '<=' | '>=' | '!='.

type               ::=     'varchar', '(', int, ')' | 'int' | 'float'.
literal            ::=     string  | int | float.
string             ::=     '"', expr, '"'.
int                ::=     ('-')digit-excl-zero (digit)* | '0x'hexdigit-excl-zero(hexdigit)* | '0'octdigit-exl-zero(octigit)*.
float              ::=     ('-')digit-excl-zero (digit)*'.'digit (digit)*

digit-excl-zero    ::=     '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'.
digit              ::=     '0' | digit-excl-zero.
hexdigit-excl-zero ::=     '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' | 'a' | 'b' | 'c' | 'd' | 'e' | 'f'.
hexdigit           ::=     '0' | hexdigit-excl-zero.
octdigit-excl-zero ::=     '1' | '2' | '3' | '4' | '5' | '6' | '7'.
octdigit           ::=     '0' | octdigit-excl-zero.
```

# DONE

## Changelog

1. [lexer](./lexer.c)
2. [parser](./parser.c)
3. [executer](./executer.c)
4. [repl](./repl.c)
5. compare tokens without case
6. request should end with ;
7. repl navigation up, down (in memory only)
8. FIX: table name should be unique
9. .read requests from file
10. FIX: tokens keyword should be converted to uppercase
11. FIX: wrong colname for primary key when lenght 1
12. FIX: selecting a wrong table segfault
13. comments: everything after # is ignored
14. FIX: create table with only 1 col segfautls
15. FIX: some tables can't be found in repl... (wrong copying of node value...)
16. select \* from table
17. FIX. drop moves the tables in wrong order. Must run through the table in ascending order and move right to left.
18. insert into table **values**

## BUGS & TODO

1. .save & .open [binn](https://github.com/liteserver/binn?tab=readme-ov-file#usage-example)
2. .clear : drop all tables from memory
3. load a database from CLI argument
4. refactor select and store values somewhere _like_ a table

## Ambitions

### Requests & syntax

1. join & foreign key
   many more tokens & parser & table edition... then how to join them properly that ? check while creating, check for every insertion...
2. Agregate functions (sum, avg, max etc.)
   difficult (lexer more keyword, parser those expression, call aggregate on result... so store them)
3. Order by, Distinct, Limit
   - limit easy
   - distinct & order by requires storing, distinct is easier
4. null values when inserting
   change the parser, create a null value which isn't 0 (???)
   - float ? 0 ???
   - int ? store x+1, retrieve x-1, compare x-1, then 0 is NULL
5. autoincrement pk
   easy. requires another table just for that and check after every insertion change schema
6. binary data
   another type, must read from something and specify maxsize
7. something from **select**
   need to create table from select and simply display them... not that hard

### Backend

1. Store the data in B-trees
