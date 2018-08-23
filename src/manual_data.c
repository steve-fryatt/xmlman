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
 * \file manual_data.c
 *
 * Manual Data Structures, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "manual_data.h"

#include "msg.h"

/* Constant Declarations. */

/**
 * The maximum number of levels that can be numbered.
 */

#define MANUAL_DATA_MAX_NUMBER_DEPTH 8

/**
 * The size of the number buffer.
 */

#define MANUAL_DATA_MAX_NUMBER_BUFFER_LEN 256

/**
 * An chunk type definition structure.
 */

struct manual_data_object_type_definition {
	enum manual_data_object_type	type;		/**< The type of object.		*/
	const char			*name;		/**< The name of the object.		*/
};

/**
 * The list of known object type definitions.
 */

static struct manual_data_object_type_definition manual_data_object_type_names[] = {
	{MANUAL_DATA_OBJECT_TYPE_MANUAL,		"Manual"},
	{MANUAL_DATA_OBJECT_TYPE_INDEX,			"Index"},
	{MANUAL_DATA_OBJECT_TYPE_CHAPTER,		"Chapter"},
	{MANUAL_DATA_OBJECT_TYPE_SECTION,		"Section"},
	{MANUAL_DATA_OBJECT_TYPE_TITLE,			"Title"},
	{MANUAL_DATA_OBJECT_TYPE_PARAGRAPH,		"Paragraph"},
	{MANUAL_DATA_OBJECT_TYPE_CITATION,		"Citation"},
	{MANUAL_DATA_OBJECT_TYPE_CODE,			"Code"},
	{MANUAL_DATA_OBJECT_TYPE_USER_ENTRY,		"User Entry"},
	{MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS,	"Light Emphasis"},
	{MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS,	"Strong Emphasis"},
	{MANUAL_DATA_OBJECT_TYPE_FILENAME,		"Filename"},
	{MANUAL_DATA_OBJECT_TYPE_ICON,			"Icon"},
	{MANUAL_DATA_OBJECT_TYPE_KEY,			"Key"},
	{MANUAL_DATA_OBJECT_TYPE_MOUSE,			"Mouse"},
	{MANUAL_DATA_OBJECT_TYPE_WINDOW,		"Window"},

	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_FILE,		"File Resource"},
	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_FOLDER,	"Folder Resource"},
	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_IMAGE,	"Image Resource"},
	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_DOWNLOAD,	"Download Resource"},


	{MANUAL_DATA_OBJECT_TYPE_TEXT,			"Text"},
	{MANUAL_DATA_OBJECT_TYPE_ENTITY,		"Entity"},
	{MANUAL_DATA_OBJECT_TYPE_NONE,			""}
};

/**
 * Create a new manual_data structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data *manual_data_create(enum manual_data_object_type type)
{
	struct manual_data *data;

	data = malloc(sizeof(struct manual_data));
	if (data == NULL) {
		msg_report(MSG_DATA_MALLOC_FAIL);
		return NULL;
	}

	data->type = type;

	data->id = NULL;
	data->index = 0;
	data->title = NULL;
	data->first_child = NULL;
	data->parent = NULL;
	data->previous = NULL;
	data->next = NULL;

	switch (type) {
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		data->chapter.filename = NULL;
		data->chapter.processed = false;
		break;

	case MANUAL_DATA_OBJECT_TYPE_TEXT:
		data->chunk.text = NULL;
		break;

	case MANUAL_DATA_OBJECT_TYPE_ENTITY:
		data->chunk.entity = MANUAL_ENTITY_NONE;
		break;

	default:
		break;
	}

	return data;
}

/**
 * Given an object type, return the textual object type name.
 *
 * \param type		The object type to look up.
 * \return		Pointer to the object's textual name, or to "" if
 *			the type was not recognised.
 */

const char *manual_data_find_object_name(enum manual_data_object_type type)
{
	int i;

	for (i = 0; manual_data_object_type_names[i].type != MANUAL_DATA_OBJECT_TYPE_NONE && manual_data_object_type_names[i].type != type; i++);

	return manual_data_object_type_names[i].name;
}

/**
 * Given a node, return a pointer to its display number in string format,
 * or NULL if no number is defined.
 *
 * \param *node		The node to return a number for.
 * \return		Pointer to the display number, or NULL.
 */

xmlChar *manual_data_get_node_number(struct manual_data *node)
{
	struct manual_data	*nodes[MANUAL_DATA_MAX_NUMBER_DEPTH], *last = NULL;
	char			*text;
	int			depth = 0, written = 0, position = 0;

	if (node == NULL)
		return NULL;

	/* Identify the nodes which will make up the number. */

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		do {
			nodes[depth++] = node;
			last = node;
			node = node->parent;
		} while ((node != NULL) && (last != NULL) &&
				(last->type != MANUAL_DATA_OBJECT_TYPE_CHAPTER) &&
				(depth < MANUAL_DATA_MAX_NUMBER_DEPTH));

		if (node == NULL)
			return NULL;

		break;
	/* Images, code blocks and so on would just take the chapter
	 * and the node, and not step through.
	 */
	default:
		return NULL;
	}

	/* If there are no nodes, something went wrong! */

	if (depth == 0)
		return NULL;

	/* Claim a memory buffer. */

	text = malloc(MANUAL_DATA_MAX_NUMBER_BUFFER_LEN);
	if (text == NULL)
		return NULL;

	/* Write the number to the buffer. */

	position = 0;

	do {
		written = snprintf(text + position, MANUAL_DATA_MAX_NUMBER_BUFFER_LEN - position, "%d.", nodes[--depth]->index);
		if (written < 0) {
			free(text);
			return NULL;
		}

		position += written;
	} while ((depth > 0) && (position < MANUAL_DATA_MAX_NUMBER_BUFFER_LEN));

	text[MANUAL_DATA_MAX_NUMBER_BUFFER_LEN - 1] = '\0';

	return (xmlChar *) text;
}

