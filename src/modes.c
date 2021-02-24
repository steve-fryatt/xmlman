/* Copyright 2021, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of XmlMan:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file modes.c
 *
 * Mode Lookup, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "modes.h"

#include "msg.h"

/* Constant Declarations. */


/**
 * A mode type definitnion
 */

struct modes_type_definition {
	enum modes_type		type;		/**< The type of mode.		*/
	const char		*name;		/**< The name of the mode.		*/
};

/**
 * The list of known mode definitions.
 */

static struct modes_type_definition modes_type_names[] = {

	{MODES_TYPE_DEBUG,	"debug"},
	{MODES_TYPE_TEXT,	"text"},
	{MODES_TYPE_STRONGHELP,	"strong"},
	{MODES_TYPE_HTML,	"html"},

	{MODES_TYPE_NONE,	""}
};


/**
 * Given a mode name, find the mode type.
 *
 * \param *name		The name to look up.
 * \return		The mode type, or MODES_TYPE_NONE on failure.
 */

enum modes_type modes_find_type(char *name)
{
	int i;

	if (name == NULL)
		return MODES_TYPE_NONE;

	for (i = 0; modes_type_names[i].type != MODES_TYPE_NONE && strcmp(modes_type_names[i].name, name) != 0; i++);

	return modes_type_names[i].type;
}
