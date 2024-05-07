# QDB

Attempt to remake a simple SQL like database in C

## Syntax

- keywords can be capitalized or not
- one statement at a time no need for semicolon ;

select suivi d'identifieurs de champs séparés par des virgules suivi de from suivi d'un identifieur de table
suivi de where suivi d'un identifieur d'une condition

```sql
select "a", "b", "c" from "tablename" where "a" = 1
```

```sql
select * from "tablename"
```

insert suivi de into suivi d'un identifieur de table suivi de ( suivi de toutes les valeurs dans l'ordre suivi de )

```sql
insert into "tablename" (1, 2, 3)
```

update suivi d'un identifieur de table suivi de set suivi de

```sql
update "tablename" set "a" = 1, "c" = 3
```

```sql
delete from "tablename" where "a" = 1
```

```sql
create table "tablename" ("id" int pk, "b" varchar(32), "c" float)
```

```sql
drop table "tablename"
```

litteraux

| truc      | type              | C rep              |
| --------- | ----------------- | ------------------ |
| `32`      | NUMBER            | long               |
| `32.3`    | NUMBER            | double             |
| `0x33`    | NUMBER            | long               |
| `bonjour` | str[unsigned int] | char[unsigned int] |

conditions

```sql
"identifieur de champ" rel val
```

## Steps

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

## Lexer

1. keyword (select, insert into, update, create, drop)

1. select a, b, c from tablename where condition
1. update tablename set a=x where condition
1. insert (a, b, c, d) into tablename
1. create table tablename (
   attrname type PK,
   attrname type,
   ...
   )

# Syntax

```ebnf
statement          ::=     select-clause | insert-clause | update-clause | delete-clause | create-clause | drop-clause.


select-clause      ::=     'SELECT', projection, 'FROM', tablename ( 'WHERE' condition );.
insert-clause      ::=     'INSERT', 'INTO', tablename, '(', literal (',' literal)*, ')';.
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

1. [lexer](./lexer.c)
2. [parser](./parser.c)
3. [executer](./executer.c)
4. [repl](./repl.c)

# Changelog

1. compare tokens without case
2. request should end with ;
3. repl navigation up, down (in memory only)
4. FIX: table name should be unique
5. .read requests from file
6. FIX: tokens keyword should be converted to uppercase
7. FIX: wrong colname for primary key when lenght 1
8. FIX: selecting a wrong table segfault
9. comments: every thing after # is ignored
10. FIX: create table with only 1 col segfautls
11. FIX: some tables can't be found in repl... (wrong copying of node value...)
12. select \* from table
13. BUG. drop moves the tables in wrong order. Must run through the table in ascending order and move right to left.

## BUGS & TODO

1. .save & .open [binn](https://github.com/liteserver/binn?tab=readme-ov-file#usage-example)
