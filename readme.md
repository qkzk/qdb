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


select-clause      ::=     'SELECT', projection, 'FROM', tablename ( 'WHERE' condition ).
insert-clause      ::=     'INSERT', 'INTO', tablename, '(', literal (',' literal)*, ')'.
update-clause      ::=     'UPDATE', tablename, 'SET', colname, '=', literal (',' colname = literal)* ( 'WHERE', condition ).
delete-clause      ::=     'DELETE', 'FROM', tablename, ( 'WHERE', condition ).
create-clause      ::=     'CREATE', 'TABLE', tablename, '(', colname, type, 'PK', (',' colname, type )* ')'.
drop-clause        ::=     'DROP', 'TABLE', tablename.

projection         ::=     colname (',' colname)* ) | *.

colname            ::=     identifier.
tablename          ::=     identifier.

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
   - [x] `drop table "tablename"`
   - [x] `insert into table "tablename" (123, 'ast', 123.456,-19, -89.012)`
