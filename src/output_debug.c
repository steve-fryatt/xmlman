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

#include <libxml/xmlstring.h>

#include "output_debug.h"

/* Static Function Prototypes. */

static char *output_debug_get_text(xmlChar *text);

/**
 * Output a manual in debug form.
 *
 * \param *manual	The manual to be output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_debug(struct manual_data *manual)
{
	struct manual_data_chapter	*chapter;

	if (manual == NULL)
		return false;

	/* Output the manual heading and title. */

	printf("*** Found Manual ***\n");

	if (manual->title != NULL)
		printf("Manual Title: '%s'\n", manual->title);
	else
		printf("<No Title>\n");

	/* Output the chapter details. */

	chapter = manual->first_chapter;

	while (chapter != NULL) {
		printf("* Found Chapter '%s'*\n", output_debug_get_text(chapter->title));

		printf("Processed: %d\n", chapter->processed);

		if (chapter->filename != NULL)
			printf("Associated with file '%s'\n", chapter->filename);

		chapter = chapter->next_chapter;
	}

	return true;
}



static char *output_debug_get_text(xmlChar *text)
{
	/* \TODO -- This needs to de-UTF8-ify the text! */

	if (text == NULL)
		return "<none>";

	return text;
}
