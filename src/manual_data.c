/* Copyright 2018-2024, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file manual_data.c
 *
 * Manual Data Structures, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "xmlman.h"
#include "manual_data.h"

#include "modes.h"
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
 * A chunk type definition structure.
 */

struct manual_data_object_type_definition {
	enum manual_data_object_type	type;		/**< The type of object.		*/
	const char			*name;		/**< The name of the object.		*/
};

/**
 * Two manual chunks for use when returning arbitary text items.
 */

struct manual_data manual_data_chunk_list, manual_data_chunk_text;

/**
 * The number of entries in the object type list.
 */

static int manual_data_max_object_types = -1;

/**
 * The list of known object type definitions.
 */

static struct manual_data_object_type_definition manual_data_object_type_names[] = {
	{MANUAL_DATA_OBJECT_TYPE_MANUAL,			"Manual"},
	{MANUAL_DATA_OBJECT_TYPE_INDEX,				"Index"},
	{MANUAL_DATA_OBJECT_TYPE_CHAPTER,			"Chapter"},
	{MANUAL_DATA_OBJECT_TYPE_SECTION,			"Section"},

	{MANUAL_DATA_OBJECT_TYPE_CREDIT,			"Credit"},
	{MANUAL_DATA_OBJECT_TYPE_DATE,				"Date"},
	{MANUAL_DATA_OBJECT_TYPE_TITLE,				"Title"},
	{MANUAL_DATA_OBJECT_TYPE_STRAPLINE,			"Strapline"},
	{MANUAL_DATA_OBJECT_TYPE_SUMMARY,			"Summary"},
	{MANUAL_DATA_OBJECT_TYPE_VERSION,			"Version"},

	{MANUAL_DATA_OBJECT_TYPE_CONTENTS,			"Contents"},
	{MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST,			"Ordered List"},
	{MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST,		"Unordered List"},
	{MANUAL_DATA_OBJECT_TYPE_LIST_ITEM,			"List Item"},
	{MANUAL_DATA_OBJECT_TYPE_TABLE,				"Table"},
	{MANUAL_DATA_OBJECT_TYPE_TABLE_ROW,			"Table Row"},
	{MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN,			"Table Column"},
	{MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET,		"Table Column Set"},
	{MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_DEFINITION,	"Table Column Definition"},
	{MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK,			"Code Block"},
	{MANUAL_DATA_OBJECT_TYPE_CALLOUT,			"Callout"},
	{MANUAL_DATA_OBJECT_TYPE_FOOTNOTE,			"Footnote"},
	{MANUAL_DATA_OBJECT_TYPE_PARAGRAPH,			"Paragraph"},

	{MANUAL_DATA_OBJECT_TYPE_CITATION,			"Citation"},
	{MANUAL_DATA_OBJECT_TYPE_CODE,				"Code"},
	{MANUAL_DATA_OBJECT_TYPE_COMMAND,			"Command"},
	{MANUAL_DATA_OBJECT_TYPE_CONSTANT,			"Constant"},
	{MANUAL_DATA_OBJECT_TYPE_DEFINED_TEXT,			"Defined Text"},
	{MANUAL_DATA_OBJECT_TYPE_EVENT,				"Event"},
	{MANUAL_DATA_OBJECT_TYPE_FILENAME,			"Filename"},
	{MANUAL_DATA_OBJECT_TYPE_FUNCTION,			"Function"},
	{MANUAL_DATA_OBJECT_TYPE_ICON,				"Icon"},
	{MANUAL_DATA_OBJECT_TYPE_INTRO,				"Intro"},
	{MANUAL_DATA_OBJECT_TYPE_KEY,				"Key"},
	{MANUAL_DATA_OBJECT_TYPE_KEYWORD,			"Keyword"},
	{MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS,		"Light Emphasis"},
	{MANUAL_DATA_OBJECT_TYPE_LINK,				"Link"},
	{MANUAL_DATA_OBJECT_TYPE_MATHS,				"Maths"},
	{MANUAL_DATA_OBJECT_TYPE_MENU,				"Menu"},
	{MANUAL_DATA_OBJECT_TYPE_MESSAGE,			"Message"},
	{MANUAL_DATA_OBJECT_TYPE_MOUSE,				"Mouse"},
	{MANUAL_DATA_OBJECT_TYPE_NAME,				"Name"},
	{MANUAL_DATA_OBJECT_TYPE_REFERENCE,			"Reference"},
	{MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS,		"Strong Emphasis"},
	{MANUAL_DATA_OBJECT_TYPE_SWI,				"SWI"},
	{MANUAL_DATA_OBJECT_TYPE_TYPE,				"Type"},
	{MANUAL_DATA_OBJECT_TYPE_USER_ENTRY,			"User Entry"},
	{MANUAL_DATA_OBJECT_TYPE_VARIABLE,			"Variable"},
	{MANUAL_DATA_OBJECT_TYPE_WINDOW,			"Window"},

	{MANUAL_DATA_OBJECT_TYPE_LINE_BREAK,			"Line Break"},
	{MANUAL_DATA_OBJECT_TYPE_TEXT,				"Text"},
	{MANUAL_DATA_OBJECT_TYPE_ENTITY,			"Entity"},

	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_FILE,			"File Resource"},
	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_FOLDER,		"Folder Resource"},
	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_IMAGE,		"Image Resource"},
	{MANUAL_DATA_OBJECT_TYPE_RESOURCE_DOWNLOAD,		"Download Resource"},

	{MANUAL_DATA_OBJECT_TYPE_MULTI_LEVEL_ATTRIBUTE,		"Multi Level Attribute"},
	{MANUAL_DATA_OBJECT_TYPE_SINGLE_LEVEL_ATTRIBUTE,	"Single Level Attribute"},

	{MANUAL_DATA_OBJECT_TYPE_NONE,				"*None*"}
};

/* Static Function Prototypes. */

static void manual_data_initialise_mode_resources(struct manual_data_mode *mode);

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

	data->index = 0;
	data->title = NULL;
	data->first_child = NULL;
	data->parent = NULL;
	data->previous = NULL;
	data->next = NULL;

	switch (type) {
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		data->chapter.id = NULL;
		data->chapter.filename = NULL;
		data->chapter.processed = false;
		break;

	case MANUAL_DATA_OBJECT_TYPE_MANUAL:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		data->chapter.id = NULL;
		data->chapter.resources = NULL;
		break;

	case MANUAL_DATA_OBJECT_TYPE_TABLE:
		data->chapter.id = NULL;
		data->chapter.columns = NULL;
		break;

	case MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_DEFINITION:
		data->chunk.width = 0;
		break;

	case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
		data->chapter.id = NULL;
		break;

	case MANUAL_DATA_OBJECT_TYPE_ENTITY:
		data->chunk.flags = MANUAL_DATA_OBJECT_FLAGS_NONE;
		data->chunk.id = NULL; // .link should also be NULL.
		data->chunk.entity = MANUAL_ENTITY_NONE;
		break;

	case MANUAL_DATA_OBJECT_TYPE_TEXT:
	default:
		data->chunk.flags = MANUAL_DATA_OBJECT_FLAGS_NONE;
		data->chunk.id = NULL; // .link should also be NULL.
		data->chunk.text = NULL;
		break;
	}

	return data;
}

/**
 * Return a pointer to an object's resources structure, if one would be
 * valid, creating it first if required.
 *
 * \param *object	Pointer to the object of interest.
 * \return		Pointer to the resources block, or NULL.
 */

struct manual_data_resources *manual_data_get_resources(struct manual_data *object)
{
	if (object == NULL)
		return NULL;

	/* Only the following nodes can take resources. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_MANUAL:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		break;
	default:
		msg_report(MSG_BAD_RESOURCE_BLOCK);
		return NULL;
	}

	if (object->chapter.resources == NULL) {
		object->chapter.resources = malloc(sizeof(struct manual_data_resources));
		if (object->chapter.resources == NULL) {
			msg_report(MSG_DATA_MALLOC_FAIL);
			return NULL;
		}

		object->chapter.resources->images = NULL;
		object->chapter.resources->downloads = NULL;
		object->chapter.resources->summary = NULL;
		object->chapter.resources->strapline = NULL;
		object->chapter.resources->credit = NULL;
		object->chapter.resources->version = NULL;
		object->chapter.resources->date = NULL;

		manual_data_initialise_mode_resources(&(object->chapter.resources->text));
		manual_data_initialise_mode_resources(&(object->chapter.resources->html));
		manual_data_initialise_mode_resources(&(object->chapter.resources->strong));
	}

	return object->chapter.resources;
}

/**
 * Initialise the resources block for a mode.
 *
 * \param *mode		Pointer to the mode resources block to initialise.
 */

static void manual_data_initialise_mode_resources(struct manual_data_mode *mode)
{
	if (mode == NULL)
		return;

	mode->filename = NULL;
	mode->folder = NULL;
	mode->stylesheet = NULL;
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

	if (manual_data_max_object_types <= 0) {
		for (i = 0; manual_data_object_type_names[i].type != MANUAL_DATA_OBJECT_TYPE_NONE; i++) {
			if (manual_data_object_type_names[i].type != i) {
				msg_report(MSG_ELEMENT_OUT_OF_SEQ);
				return "*error*";
			}
		}

		manual_data_max_object_types = i;
	}

	if (type < 0 || type >= manual_data_max_object_types)
		return "*none*";

	/* Look up the object type name.
	 *
	 * WARNING: This relies on the array indices matching the values of
	 * the element enum entries.
	 */

	return manual_data_object_type_names[type].name;
}

/**
 * Given a node and a current nesting level for the parent node,
 * determine the nesting level if the node is descended into.
 *
 * \param *node		The node of interest.
 * \param current_level	The nesting level of the parent node.
 * \return		The new nesting level.
 */

int manual_data_get_nesting_level(struct manual_data *node, int current_level)
{
	if (node == NULL)
		return current_level;

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
		/* Chapters and Indexes bump the level, so long as they are
		 * nested in a manual, which they should always be!
		 *
		 * We don't worry about the title, as they should all have titles!
		 */
		if (node->parent != NULL && node->parent->type == MANUAL_DATA_OBJECT_TYPE_MANUAL)
			current_level++;
		break;

	case MANUAL_DATA_OBJECT_TYPE_SECTION:
	case MANUAL_DATA_OBJECT_TYPE_CONTENTS:
		if (node->parent == NULL)
			break;

		/* A section with no parent shouldn't happen! */

		if (node->parent->type == MANUAL_DATA_OBJECT_TYPE_MANUAL)
			/* A section in a manual is always a level bump. */
			current_level++;
	//	else if (node->parent->type == MANUAL_DATA_OBJECT_TYPE_SECTION)
	//		/* A section in a section is always a level bump. */
	//		current_level++;
		else if (node->title != NULL)
			/* A section in a chapter or index is only a level bump
			 * if it has a title. Otherwise it's text at the parent's
			 * level.
			 */
			current_level++;
		break;

	default:
		break;
	}

	return current_level;
}

/**
 * Given a node, return a pointer to its display number in string format,
 * or NULL if no number is defined.
 *
 * This is the full number, including the numbers of any parent sections.
 *
 * \param *node		The node to return a number for.
 * \param include_name	Should we prefix the number with the object name?
 * \return		Pointer to the display number, or NULL.
 */

char *manual_data_get_node_number(struct manual_data *node, bool include_name)
{
	struct manual_data	*nodes[MANUAL_DATA_MAX_NUMBER_DEPTH], *last = NULL;
	char			*text, *separator = NULL, *name = NULL;
	int			depth = 0, written = 0, position = 0;
	bool			include_sections = false;

	if (node == NULL)
		return NULL;

	/* Identify a node type name. */

	// TODO -- these should come from the manual, so that they can be internationalised.

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		name = "Chapter";
		break;
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		name = "Section";
		break;
	case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
		name = "Listing";
		break;
	case MANUAL_DATA_OBJECT_TYPE_TABLE:
		name = "Table";
		break;
	case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
		name = "Note";
		break;
	default:
		break;	
	}

	/* Identify the nodes which will make up the number. */

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		include_sections = true;
		/* Chapters and sections are the same as block objects,
		 * except that we include the numbers for all intermediate
		 * sections on the way up.
		 */

	case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
	case MANUAL_DATA_OBJECT_TYPE_TABLE:
		do {
			if ((node->type == MANUAL_DATA_OBJECT_TYPE_SECTION && include_sections == true) ||
					(node->type == MANUAL_DATA_OBJECT_TYPE_CHAPTER) ||
					last == NULL /* Always include the target node. */)
				nodes[depth++] = node;

			last = node;
			node = node->parent;
		} while ((node != NULL) && (last != NULL) &&
				(last->type != MANUAL_DATA_OBJECT_TYPE_CHAPTER) &&
				(depth < MANUAL_DATA_MAX_NUMBER_DEPTH));

		if (node == NULL)
			return NULL;

		separator = ".";
		break;

	case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
		nodes[depth++] = node;
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

	position = 0;

	/* Put the object name in the buffer. */

	if (include_name == true && name != NULL) {
		written = snprintf(text + position, MANUAL_DATA_MAX_NUMBER_BUFFER_LEN - position, "%s ", name);
		if (written < 0) {
			free(text);
			return NULL;
		}

		position += written;
	}

	/* Write the number to the buffer. */

	do {
		written = snprintf(text + position, MANUAL_DATA_MAX_NUMBER_BUFFER_LEN - position, "%d%s",
				nodes[--depth]->index, (separator == NULL) ? "" : separator);
		if (written < 0) {
			free(text);
			return NULL;
		}

		position += written;
	} while ((depth > 0) && (position < MANUAL_DATA_MAX_NUMBER_BUFFER_LEN));

	text[MANUAL_DATA_MAX_NUMBER_BUFFER_LEN - 1] = '\0';

	return text;
}

/**
 * Search a node and its children for any filename data associated with
 * a given manual type.
 *
 * \param *node		The node to search down from.
 * \param type		The target output type to search for.
 * \return		True if filename data was found; otherwise false.
 */

bool manual_data_find_filename_data(struct manual_data *node, enum modes_type type)
{
	struct manual_data_mode *resources = NULL;

	if (node == NULL)
		return false;

	while (node != NULL) {
		switch (node->type) {
		case MANUAL_DATA_OBJECT_TYPE_MANUAL:
		case MANUAL_DATA_OBJECT_TYPE_INDEX:
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			resources = modes_find_resources(node->chapter.resources, type);
			if (resources == NULL)
				break;

			/* If there is any filename data, our quest is over. */

			if (resources->filename != NULL || resources->folder != NULL)
				return true;

			/* If there are any child nodes, search them for more sections. */

			if (node->first_child != NULL && manual_data_find_filename_data(node->first_child, type))
				return true;

			break;

		default:
			break;
		}


		node = node->next;
	}

	return false;
}

/**
 * Return a filename for a node, given a default root filename and the
 * target output type.
 *
 * \param *node		The node to return a filename for.
 * \param *root		A default root filename.
 * \param type		The target output type.
 * \return		Pointer to a filename, or NULL on failure.
 */

struct filename *manual_data_get_node_filename(struct manual_data *node, struct filename *root, enum modes_type type)
{
	struct manual_data_mode *resources = NULL;
	struct filename *filename;

	if (node == NULL)
		return NULL;

	filename = filename_make(NULL, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_NONE);
	if (filename == NULL)
		return NULL;

	while (node != NULL) {
		switch (node->type) {
		case MANUAL_DATA_OBJECT_TYPE_MANUAL:
		case MANUAL_DATA_OBJECT_TYPE_INDEX:
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			resources = modes_find_resources(node->chapter.resources, type);
			if (resources == NULL)
				break;

			/* Collect only the leaf filename. */

			if (resources->filename != NULL && filename_is_empty(filename))
				filename_prepend(filename, resources->filename, 0);

			/* Collect any folders that we find, including the
			 * default filename if another hasn't been found.
			 */

			if (resources->folder != NULL) {
				if (root != NULL && filename_is_empty(filename))
					filename_prepend(filename, root, 0);

				filename_prepend(filename, resources->folder, 0);
			}
			break;

		default:
			break;
		}

		node = node->parent;
	}

	if (root != NULL && filename_is_empty(filename))
		filename_prepend(filename, root, 0);

	return filename;
}

/**
 * Given a node, return a pointer to the first parent node which contains
 * a stylesheet filename for the chosen output type.
 *
 * \param *node		The node to return a filename for.
 * \param type		The target output type.
 * \return		Pointer to a node, or NULL on failure.
 */

struct manual_data *manual_data_get_node_stylesheet(struct manual_data *node, enum modes_type type)
{
	struct manual_data_mode *resources = NULL;

	if (node == NULL)
		return NULL;

	while (node != NULL) {
		switch (node->type) {
		case MANUAL_DATA_OBJECT_TYPE_MANUAL:
		case MANUAL_DATA_OBJECT_TYPE_INDEX:
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			resources = modes_find_resources(node->chapter.resources, type);
			if (resources == NULL)
				break;

			/* Return as soon as we find a stylesheet link. */

			if (resources->stylesheet != NULL)
				return node;
			break;

		default:
			break;
		}

		node = node->parent;
	}

	return NULL;
}

/**
 * Test a pair of nodes, to determine whether or not they are
 * in the same file for a given output mode.
 *
 * \param *node1	The first node to be compared.
 * \param *node2	The second node to be compared.
 * \param type		The target output type.
 * \return		True if the nodes are both in the same file;
 *			otherwise false.
 */

bool manual_data_nodes_share_file(struct manual_data *node1, struct manual_data *node2, enum modes_type type)
{
	struct manual_data_mode *resources = NULL;
	bool found = false;

	if (node1 == NULL || node2 == NULL)
		return false;

	found = false;

	while (node1 != NULL && !found) {
		switch (node1->type) {
		case MANUAL_DATA_OBJECT_TYPE_MANUAL:
		case MANUAL_DATA_OBJECT_TYPE_INDEX:
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			resources = modes_find_resources(node1->chapter.resources, type);
			if (resources != NULL && (resources->filename != NULL || resources->folder != NULL))
				found = true;
			break;

		default:
			break;
		}

		node1 = node1->parent;
	}

	found = false;

	while (node2 != NULL && !found) {
		switch (node2->type) {
		case MANUAL_DATA_OBJECT_TYPE_MANUAL:
		case MANUAL_DATA_OBJECT_TYPE_INDEX:
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			resources = modes_find_resources(node2->chapter.resources, type);
			if (resources != NULL && (resources->filename != NULL || resources->folder != NULL))
				found = true;
			break;

		default:
			break;
		}

		node2 = node2->parent;
	}

	/* Are the nodes the same? If no filenames were passed in either
	 * case, both pointers will be NULL, which is a valid outcome: it
	 * just means that both files are the root file for the manual.
	 */

	return (node1 == node2) ? true : false;
}

/**
 * Given a callout node, return an appropriate callout name in
 * the form of a set of node chunks.
 * 
 * \param *callout	Pointer to the callout node.
 * \return		Pointer to the callout name.
 */

struct manual_data *manual_data_get_callout_name(struct manual_data* callout)
{
	char *title;

	if (callout == NULL || callout->type != MANUAL_DATA_OBJECT_TYPE_CALLOUT)
		return NULL;

	switch (callout->chunk.flags & MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE) {
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_ATTENTION:
		title = "Attention";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_CAUTION:
		title = "Caution";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_DANGER:
		title = "Danger";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_ERROR:
		title = "Error";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_HINT:
		title = "Hint";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_IMPORTANT:
		title = "Important";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_NOTE:
		title = "Note";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_SEEALSO:
		title = "See Also";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_TIP:
		title = "Tip";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_WARNING:
		title = "Warning";
		break;
	default:
		title = "";
		break;
	}

	manual_data_chunk_list.type = MANUAL_DATA_OBJECT_TYPE_TITLE;
	manual_data_chunk_list.index = 0;
	manual_data_chunk_list.title = NULL;
	manual_data_chunk_list.first_child = &manual_data_chunk_text;
	manual_data_chunk_list.parent = callout;
	manual_data_chunk_list.previous = NULL;
	manual_data_chunk_list.next = NULL;
	manual_data_chunk_list.chunk.flags = MANUAL_DATA_OBJECT_FLAGS_NONE;
	manual_data_chunk_list.chunk.text = NULL;

	manual_data_chunk_text.type = MANUAL_DATA_OBJECT_TYPE_TEXT;
	manual_data_chunk_text.index = 0;
	manual_data_chunk_text.title = NULL;
	manual_data_chunk_text.first_child = NULL;
	manual_data_chunk_text.parent = &manual_data_chunk_list;
	manual_data_chunk_text.previous = NULL;
	manual_data_chunk_text.next = NULL;
	manual_data_chunk_text.chunk.flags = MANUAL_DATA_OBJECT_FLAGS_NONE;
	manual_data_chunk_text.chunk.text = title;

	return &manual_data_chunk_list;
}
