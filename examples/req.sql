CREATE TABLE "t1" ("a" int pk, "b" int);
CREATE TABLE "t2" ("c" int pk, "d" int);
CREATE TABLE "t3" ("e" int pk, "f" int);

.tables

INSERT INTO "t2" VALUES (1, 2);
INSERT INTO "t3" VALUES (3, 4);

DROP TABLE "t1";

.tables

SELECT * FROM "t2";
SELECT * FROM "t3";


