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
 * \file output_text_line.c
 *
 * Text Line Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

#include "output_text.h"

/**
 * A text line output instance structure.
 */

struct output_text_line {
	size_t		columns;
};

/**
 * Create a new text line output instance.
 *
 * \return		Pointer to the new line block, or NULL on failure.
 */

struct output_text_line *output_text_line_create(void)
{
	struct output_text_line		*line = NULL;

	line = malloc(sizeof(struct output_text_line));
	if (line == NULL)
		return NULL;

	return line;
}

/**
 * Destroy a text line output instance.
 *
 * \param *line		The line output instance to destroy.
 */

void output_text_line_destroy(struct output_text_line *line)
{
	if (line == NULL)
		return;

	free(line);
}

