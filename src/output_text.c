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
 * \file output_text.c
 *
 * Text Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

#include "output_text.h"

#include "encoding.h"
#include "manual_data.h"
#include "msg.h"
#include "output_text_line.h"

/* Global Variables. */

static int output_text_page_width = 77;

/* Static Function Prototypes. */

static bool output_text_write_chapter(struct manual_data *chapter);
static bool output_text_write_heading(struct manual_data *title, int indent);
static bool output_text_write_text(struct output_text_line *line, int column, enum manual_data_object_type type, struct manual_data *text);

/**
 * Output a manual in text form.
 *
 * \param *manual	The manual to be output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_text(struct manual_data *manual)
{
	struct manual_data *chapter, *section, *block;
	struct output_text_line	*line;

	if (manual == NULL)
		return false;

	encoding_select_table(ENCODING_TARGET_UTF8);

	output_text_write_heading(manual->title, 0);

	/* Output the chapter details. */

	chapter = manual->first_child;

	while (chapter != NULL) {
		output_text_write_chapter(chapter);

#if 0


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

			/* Output the block details. */

			block = section->first_child;

			while (block != NULL) {
				printf("*** Found Block **\n");

				output_debug_write_text(MANUAL_DATA_OBJECT_TYPE_PARAGRAPH, block);

				block = block->next;
			}

			section = section->next;
		}

#endif

		chapter = chapter->next;
	}

	return true;
}

static bool output_text_write_chapter(struct manual_data *chapter)
{
	if (chapter == NULL || chapter->first_child == NULL)
		return true;

	if (!output_text_write_heading(chapter->title, 0))
		return false;

	return true;
}

static bool output_text_write_heading(struct manual_data *title, int indent)
{
	struct output_text_line *line;

	line = output_text_line_create();
	if (line == NULL)
		return false;

	if (!output_text_line_add_column(line, indent, output_text_page_width - indent)) {
		output_text_line_destroy(line);
		return false;
	}

	if (!output_text_line_reset(line)) {
		output_text_line_destroy(line);
		return false;
	}

	if (!output_text_write_text(line, 0, MANUAL_DATA_OBJECT_TYPE_TITLE, title)) {
		output_text_line_destroy(line);
		return false;
	}

	if (!output_text_line_write(line, true)) {
		output_text_line_destroy(line);
		return false;
	}

	output_text_line_destroy(line);

	return true;
}


static bool output_text_write_text(struct output_text_line *line, int column, enum manual_data_object_type type, struct manual_data *text)
{
	struct manual_data *chunk;

	/* An empty block doesn't require any output. */

	if (text == NULL)
		return true;

	if (text->type != type) {
		msg_report(MSG_UNEXPECTED_BLOCK, type, text->type);
		return;
	}

	chunk = text->first_child;

	while (chunk != NULL) {
		switch (chunk->type) {
		case MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS:
			output_text_line_add_text(line, column, "/");
			output_text_write_text(line, column, MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, chunk);
			output_text_line_add_text(line, column, "/");
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			output_text_line_add_text(line, column, "*");
			output_text_write_text(line, column, MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, chunk);
			output_text_line_add_text(line, column, "*");
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			output_text_line_add_text(line, column, chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			output_text_line_add_text(line, column, manual_entity_find_name(chunk->chunk.entity));
			break;
		default:
			msg_report(MSG_UNEXPECTED_CHUNK, chunk->type);
			break;
		}

		chunk = chunk->next;
	}
}

