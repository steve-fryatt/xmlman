/* Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of XmlMan:
 *
 *   http://www.stevefryatt.org.uk/risc-os
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file output_debug.c
 *
 * Debug Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "output_debug.h"

#include "encoding.h"
#include "filename.h"
#include "manual_data.h"

/* Static Function Prototypes. */

static void output_debug_write_text(enum manual_data_object_type type, struct manual_data *text);
static char *output_debug_get_text(char *text);

/**
 * Output a manual in debug form.
 *
 * \param *document	The manual to be output.
 * \param *filename	The filename to use to write to.
 * \param encoding	The encoding to use for output.
 * \param line_end	The line ending to use for output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_debug(struct manual *document, struct filename *filename, enum encoding_target encoding, enum encoding_line_end line_end)
{
	struct manual_data *manual, *chapter, *section, *block;

	if (document == NULL || document->manual == NULL)
		return false;

	manual = document->manual;

	/* Output the manual heading and title. */

	printf("*** Found Manual ***\n");

	output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, manual->title);

	/* Output the chapter details. */

	chapter = manual->first_child;

	while (chapter != NULL) {
		printf("* Found Chapter *\n");

		output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, chapter->title);

		printf("Processed: %d\n", chapter->chapter.processed);

		if (chapter->chapter.id != NULL)
			printf("Chapter ID '%s'\n", chapter->chapter.id);

		/* Output the section details. */

		section = chapter->first_child;
	
		while (section != NULL) {
			printf("** Found Section **\n");

			output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, section->title);

			if (section->chapter.id != NULL)
				printf("Section ID '%s'\n", section->chapter.id);

			/* Output the block details. */

			block = section->first_child;

			while (block != NULL) {
				printf("*** Found Block **\n");

				output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_PARAGRAPH, block);

				block = block->next;
			}

			section = section->next;
		}

		chapter = chapter->next;
	}

	return true;
}


static void output_debug_write_text(enum manual_data_object_type type, struct manual_data *text)
{
	struct manual_data *chunk;

	if (text == NULL) {
		printf("*** Text Block NULL ***\n");
		return;
	}

	if (text->type != type) {
		printf("*** Text Block not expected type (expected %d, found %d)\n", type, text->type);
		return;
	}

	chunk = text->first_child;

	while (chunk != NULL) {
		switch (chunk->type) {
		case MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS:
			printf(">>> Light Emphasis...\n");
			output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, chunk);
			printf("<<< end\n");
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			printf(">>> STRONG Emphasis...\n");
			output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, chunk);
			printf("<<< END\n");
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			printf("--- Chunk text: `%s`\n", output_debug_get_text(chunk->chunk.text));
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			printf("--- Chunk entity: %s\n", manual_entity_find_name(chunk->chunk.entity));
			break;
		default:
			printf("*** Unexpected chunk type! ***\n");
		}

		chunk = chunk->next;
	}
}



static char *output_debug_get_text(char *text)
{
	/* \TODO -- This needs to de-UTF8-ify the text! */

	if (text == NULL)
		return "<none>";

	return text;
}
