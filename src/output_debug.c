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

static void output_debug_write_node(struct manual_data *parent, struct manual_data *node, int depth, bool *indent);
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

	output_debug_write_node(NULL, document->manual, 0, NULL);

	return true;
}


static void output_debug_write_node(struct manual_data *parent, struct manual_data *node, int depth, bool *indent)
{
	int i;
	bool *new_indent = NULL;
	struct manual_data *previous = NULL;

	/* Copy the depth info. */

	new_indent = malloc(depth + 1);
	if (new_indent == NULL)
		return;

	for (i = 0; i < depth; i++)
		new_indent[i] = indent[i];


	/* Output the child nodes at depth + 1. */

	while (node != NULL) {
		if (depth > 0) {
			for (i = 0; i < depth - 1; i++)
				printf(" %c ", (indent[i] == true) ? '|' : ' ');

			printf(" %c ", (previous == NULL) ? '*' : '+');
		}

		printf("\033[1;36m%s\033[0m (%d) [Parent %s, Previous %s]\n",
				manual_data_find_object_name(node->type),
				node->index,
				(node->parent == parent) ? "\033[1;32mOK\033[0m" : "\033[1;31mBad\033[0m",
				(node->previous == previous) ? "\033[1;32mOK\033[0m" : "\033[1;31mBad\033[0m");

		if (depth > 0)
			new_indent[depth - 1] = (node->next != NULL) ? true : false;

		if (node->title != NULL)
			output_debug_write_node(node, node->title, depth + 1, new_indent);

		if ((node->type == MANUAL_DATA_OBJECT_TYPE_MANUAL || node->type == MANUAL_DATA_OBJECT_TYPE_CHAPTER ||
				node->type == MANUAL_DATA_OBJECT_TYPE_INDEX || node->type == MANUAL_DATA_OBJECT_TYPE_SECTION) &&
				node->chapter.resources != NULL && node->chapter.resources->summary != NULL)
			output_debug_write_node(node, node->chapter.resources->summary, depth + 1, new_indent);

		if (node->type == MANUAL_DATA_OBJECT_TYPE_LINK && node->chunk.link != NULL)
			output_debug_write_node(node, node->chunk.link, depth + 1, new_indent);

		if (node->type == MANUAL_DATA_OBJECT_TYPE_TABLE && node->chapter.columns != NULL)
			output_debug_write_node(node, node->chapter.columns, depth + 1, new_indent);

		if (node->first_child != NULL)
			output_debug_write_node(node, node->first_child, depth + 1, new_indent);

		previous = node;
		node = node->next;
	}

	free(new_indent);
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
