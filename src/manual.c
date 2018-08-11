/* Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file manualc
 *
 * Manual Structure, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "manual.h"

/**
 * Create a new manual structure.
 *
 * \param *node		Pointer to the top-level node for the structure.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual *manual_create(struct manual_data *node)
{
	struct manual *document;

	document = malloc(sizeof(struct manual));
	if (document == NULL)
		return NULL;

	document->manual = node;
	document->id_index = NULL;

	return document;
}

