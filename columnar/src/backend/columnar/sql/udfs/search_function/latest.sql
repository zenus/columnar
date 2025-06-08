CREATE OR REPLACE FUNCTION match(cstring,cstring,cstring)
RETURNS setof record
LANGUAGE C PARALLEL
AS 'MODULE_PATHNAME', 'InvertedIndexFuncMatch';
