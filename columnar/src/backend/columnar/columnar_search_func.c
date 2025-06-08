/*-------------------------------------------------------------------------
 *
 * columnar_search_func.c
 *
 * Copyright (c)  
 * 
 *
 * add search functions for full-text index
 *
 *-------------------------------------------------------------------------
 */


#include "postgres.h"

#include "safe_lib.h"
#include "funcapi.h"

#include "columnar/columnar.h"
#include "columnar/columnar_version_compat.h"


typedef struct InvertedIndexQueryField
{
	char* field;
	float4 boost;
} InvertedIndexQueryField;


typedef struct MatchFunctionInfo
{
	List* queryField;
	char* queryText;
	uint32 fuzziness;
} MatchFunctionInfo;


int
ParseQueryField(char *fieldstr, List* queryFields)
{
	int	nf = 0;
	char *token;
	char *p;
	char *q; 

	if(fieldstr == NULL) return 0;
	p = fieldstr;
	token = strtok(fieldstr, ",");

	while (token != NULL) 
	{
		p = token;
		q = p;
		while (*p !='\0')
		{
			if(*p =='^') 
			{
				*p = '\0'; 
				p++;
				q = p;
			}else{
              	p++;
			}
		}
		p++;
		InvertedIndexQueryField *queryField = palloc(sizeof(InvertedIndexQueryField));
		queryField->field = pstrdup(token);
		queryField->boost = 1.0;
		if(*q == '\0')
		{
			float4 boost = atof(q); 
			queryField->boost = boost;
		} 		
		queryFields = lappend(queryFields, queryField);
		nf++;
		token = strtok(NULL, ","); 
    }

	// the last  one field
	if(token == NULL)
	{
		q = p;
		while (*p !='\0')
		{
			if(*p =='^') 
			{
				*p = '\0'; // set ^ to string end
				p++;
				break;
			} else {
				p++;
			}
		}
		InvertedIndexQueryField *queryField = palloc(sizeof(InvertedIndexQueryField));
		queryField->field = pstrdup(q);
		queryField->boost = 0.0;
		if(*p != '\0')
		{
			float4 boost = atof(p); 
			queryField->boost = boost;
		} 		
		queryFields = lappend(queryFields, queryField);
		nf++;
	}
	return nf;
}

PG_FUNCTION_INFO_V1(InvertedIndexFuncMatch);

Datum
InvertedIndexFuncMatch(PG_FUNCTION_ARGS)
{
	#define MATCH_INFO_NATTS 4
	char *fieldStr = PG_GETARG_CSTRING(0);
	char *queryText = PG_GETARG_CSTRING(1);
	char *queryOption = PG_GETARG_CSTRING(2);
	List *queryFields = NIL;
	TupleDesc tupdesc;
	int	 dterr;

	
	tupdesc = CreateTemplateTupleDesc(MATCH_INFO_NATTS);

	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "field",
					   TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "boost",
					   FLOAT4OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3, "query",
					   TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 4, "option",
					   TEXTOID, -1, 0);

	dterr = ParseQueryField(fieldStr, queryFields);
	if (dterr == 0) 
	{
		ereport(ERROR, (errmsg("cannot parse query field"),
						errdetail("parse invert index query field error: %s", fieldStr)));
	}

	Datum values[MATCH_INFO_NATTS] = { 0 };
	bool nulls[MATCH_INFO_NATTS] = { 0 };

	InvertedIndexQueryField* queryField = (InvertedIndexQueryField*) linitial(queryFields);

	values[0] = CStringGetDatum(queryField->field);
	values[1] = Float4GetDatum(queryField->boost);
	values[2] = CStringGetDatum(queryText);
	values[3] = CStringGetDatum(queryOption);


	HeapTuple tuple = heap_form_tuple(tupdesc, values, nulls);

	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
	
}
