CREATE TABLE "user" ("a" int pk, "b" int);
CREATE TABLE "to_drop" ("c" int pk, "d" int);

.tables

INSERT INTO "user" (12, 70);
INSERT INTO "user" (34, 80);
INSERT INTO "user" (56, 90);

SELECT "a", "b" FROM "user";

UPDATE "user" SET "a" = 2 WHERE ( "a" = 12 );

SELECT "a", "b" FROM "user";

DELETE FROM "user" WHERE ("b" = 80);

SELECT "a", "b" FROM "user";
SELECT "c", "d" FROM "to_drop";

DROP TABLE "to_drop"



