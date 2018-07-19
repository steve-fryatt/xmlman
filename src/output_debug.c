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
 * \file parse_element.c
 *
 * XML Parser Element Decoding, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

#include "output_debug.h"

/* Static Function Prototypes. */

static void output_debug_write_text(enum manual_data_object_type type, struct manual_data *text);
static char *output_debug_get_text(xmlChar *text);

/**
 * Output a manual in debug form.
 *
 * \param *manual	The manual to be output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_debug(struct manual_data *manual)
{
	struct manual_data *chapter, *section;

	if (manual == NULL)
		return false;

	/* Output the manual heading and title. */

	printf("*** Found Manual ***\n");

	output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, manual->title);

	/* Output the chapter details. */

	chapter = manual->first_child;

	while (chapter != NULL) {
		printf("* Found Chapter *\n");

		output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, chapter->title);

		printf("Processed: %d\n", chapter->chapter.processed);

		if (chapter->id != NULL)
			printf("Chapter ID '%s'\n", chapter->id);

		if (chapter->chapter.filename != NULL)
			printf("Associated with file '%s'\n", chapter->chapter.filename);

		/* Output the section details. */

		section = chapter->first_child;
	
		while (section != NULL) {
			printf("** Found Section **\n");

		output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, section->title);

			if (section->id != NULL)
				printf("Section ID '%s'\n", section->id);

			section = section->next;
		}

		chapter = chapter->next;
	}

	return true;
}


static void output_debug_write_text(enum manual_data_object_type type, struct manual_data *text)
{
	if (text == NULL) {
		printf("*** Text Block NULL ***\n");
		return;
	}

	if (text->type != type) {
		printf("*** Text Block not expected type\n");
		return;
	}
}



static char *output_debug_get_text(xmlChar *text)
{
	/* \TODO -- This needs to de-UTF8-ify the text! */

	if (text == NULL)
		return "<none>";

	return (char *)text;
}
