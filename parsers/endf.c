/*
*   Copyright (c) 2020, Masatake YAMATO
*   Copyright (c) 2020, Red Hat, Inc.
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License version 2 or (at your option) any later version.
*
*   This module contains functions for generating tags for ENDF (Evaluated Nuclear Data Files).
*   https://www.nndc.bnl.gov/csewg/docs/endf-manual.pdf
*/

#include "general.h"	/* must always come first */

#include "entry.h"
#include "kind.h"
#include "parse.h"
#include "read.h"

#include <string.h>


enum ENDFKind {
	K_MAT,
	K_MF,
	K_MT,
};

static scopeSeparator endfSeparators [] = {
	{ KIND_WILDCARD_INDEX, "" },
};

static kindDefinition endfKinds[] = {
	{ true, 'm', "mat", "materials",
	  ATTACH_SEPARATORS (endfSeparators)},
	{ true, 'f', "mf", "material files",
	  ATTACH_SEPARATORS (endfSeparators)},
	{ true, 't', "mt", "material subdivisions",
	  ATTACH_SEPARATORS (endfSeparators)},
};


static int makeENDFTagEntry (const char *mat, const char *mf, const char *mt, int kindIndex, int parentCorkIndex)
{
	tagEntryInfo e;
  char name[30];
  snprintf(name, 20, ">%s/%s/%s", mat, mf, mt);
	initTagEntry (&e, name, kindIndex);
	e.extensionFields.scopeIndex = parentCorkIndex;
	return makeTagEntry (&e);
}

static void setEndLine (int corkIndex, unsigned long endLine)
{
	tagEntryInfo *e = getEntryInCorkQueue (corkIndex);
	if (e)
		e->extensionFields.endLine = endLine;
}

static void findENDFTags(void)
{
	const unsigned char *line;
	char last_mat[5] = { '\0' };
	int last_mat_index = CORK_NIL;
	char last_mf[3]  = { '\0' };
	int last_mf_index = CORK_NIL;
	char last_mt[4]  = { '\0' };

	while ((line = readLineFromInputFile ()) != NULL)
	{

		char mat[5] = { line[66], line[67], line[68], line[69], '\0' };
		char mf[3]  = { line[70], line[71], '\0'};
		char mt[4]  = { line[72], line[73], line[74], '\0'};

    if (!strcmp(mt, "  0")) {
			memcpy (last_mt, mt, 3);
      continue;
    }
		if (strcmp (mat, last_mat) && strcmp(mat, "  -1"))
		{
			if (last_mat_index != CORK_NIL)
				setEndLine (last_mat_index, getInputLineNumber ());
			last_mat_index = makeENDFTagEntry (mat, mf, mt, K_MAT, CORK_NIL);
			memcpy (last_mat, mat, 4);
			last_mf[0] = '\0';
			last_mt[0] = '\0';
		}

		if (strcmp (mf, last_mf) && strcmp(mf, " 0"))
		{
			if (last_mf_index != CORK_NIL)
				setEndLine (last_mf_index, getInputLineNumber ());
			setEndLine (last_mf_index, getInputLineNumber ());
			last_mf_index = makeENDFTagEntry (mat, mf, mt, K_MF, last_mat_index);
			memcpy (last_mf, mf, 2);
			last_mt[0] = '\0';
		}

		if (strcmp (mt, last_mt))
		{
			makeENDFTagEntry (mat, mf, mt, K_MT, last_mf_index);
			memcpy (last_mt, mt, 3);
		}
	}
}

/* Parser definition */
extern parserDefinition * ENDFParser (void)
{
	static const char * const extensions [] = { "endf", "pendf", "rendf", "gendf", NULL };
	parserDefinition * def = parserNew ("ENDF");

	def->kindTable  = endfKinds;
	def->kindCount  = ARRAY_SIZE(endfKinds);
	def->extensions = extensions;
	def->parser     = findENDFTags;
	def->useCork    = CORK_QUEUE;
	return def;
}
