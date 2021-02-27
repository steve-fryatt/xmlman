/* Copyright 2018-2021, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "output_text.h"

#include "encoding.h"
#include "filename.h"
#include "manual_data.h"
#include "manual_queue.h"
#include "modes.h"
#include "msg.h"
#include "output_text_line.h"

/* Static constants. */

/**
 * The base indent for section nesting.
 */

#define OUTPUT_TEXT_BASE_INDENT 0

/**
 * The number of characters to indent each new block by.
 */
 
#define OUTPUT_TEXT_BLOCK_INDENT 2

/**
 * The maximum indent that can be applied to sections (effectively
 * limiting the depth to which they can be nested.
 */

#define OUTPUT_TEXT_MAX_SECTION_INDENT 10

/**
 * The root filename used when writing into an empty folder.
 */

#define OUTPUT_TEXT_ROOT_FILENAME "ReadMe"

/* Global Variables. */

/**
 * The width of a page, in characters.
 */
static int output_text_page_width = 77;

/**
 * The root filename used when writing into an empty folder.
 */

static struct filename *output_text_root_filename;

/* Static Function Prototypes. */

static bool output_text_write_manual(struct manual_data *chapter, struct filename *folder);
static bool output_text_write_file(struct manual_data *object, struct filename *folder);
static bool output_text_write_object(struct manual_data *object, int indent);
static bool output_text_write_head(struct manual_data *manual);
static bool output_text_write_foot(struct manual_data *manual);
static bool output_text_write_heading(struct manual_data *node, int indent);
static bool output_text_write_paragraph(struct manual_data *object, struct output_text_line *paragraph_line);
static bool output_text_write_text(struct output_text_line *line, int column, enum manual_data_object_type type, struct manual_data *text);
static const char *output_text_convert_entity(enum manual_entity_type entity);

/**
 * Output a manual in text form.
 *
 * \param *document	The manual to be output.
 * \param *filename	The filename to use to write to.
 * \param encoding	The encoding to use for output.
 * \param line_end	The line ending to use for output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_text(struct manual *document, struct filename *filename, enum encoding_target encoding, enum encoding_line_end line_end)
{
	bool result;

	if (document == NULL || document->manual == NULL)
		return false;

	msg_report(MSG_START_MODE, "Text");

	/* Output encoding defaults to UTF8. */

	encoding_select_table((encoding != ENCODING_TARGET_NONE) ? encoding : ENCODING_TARGET_UTF8);

	/* Output line endings default to LF. */

	encoding_select_line_end((line_end != ENCODING_LINE_END_NONE) ? line_end : ENCODING_LINE_END_LF);

	/* Write the manual file content. */

	output_text_root_filename = filename_make(OUTPUT_TEXT_ROOT_FILENAME, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LINUX);

	result = output_text_write_manual(document->manual, filename);

	filename_destroy(output_text_root_filename);

	return result;
}

/**
 * Process the contents of a chapter block and write it out.
 *
 * \param *manual	The manual to process.
 * \param *folder	The folder into which to write the manual.
 * \return		True if successful; False on error.
 */

static bool output_text_write_manual(struct manual_data *manual, struct filename *folder)
{
	struct manual_data *object;

	if (manual == NULL || folder == NULL)
		return true;

	/* Confirm that this is a manual. */

	if (manual->type != MANUAL_DATA_OBJECT_TYPE_MANUAL) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_MANUAL),
				manual_data_find_object_name(manual->type));
		return false;
	}

	/* Initialise the manual queue. */

	manual_queue_initialise();

	/* Process the files, starting with the root node. */

	manual_queue_add_node(manual);

	do {
		object = manual_queue_remove_node();
		if (object == NULL)
			continue;

		if (!output_text_write_file(object, folder))
			return false;
	} while (object != NULL);

	return true;
}


/**
 * Write a node and its descendents as a self-contained file.
 *
 * \param *object	The object to process.
 * \param *folder	The folder into which to write the manual.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_text_write_file(struct manual_data *object, struct filename *folder)
{
	struct filename *filename = NULL, *foldername = NULL;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Confirm that this is a suitable top-level object for a file. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_MANUAL:
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_SECTION),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Find the file and folder names. */

	filename = manual_data_get_node_filename(object, output_text_root_filename, FILENAME_PLATFORM_LOCAL, MODES_TYPE_TEXT);
	if (filename == NULL)
		return false;

	if (!filename_prepend(filename, folder, 0)) {
		filename_destroy(filename);
		return false;
	}

	foldername = filename_up(filename, 1);
	if (foldername == NULL) {
		filename_destroy(filename);
		return false;
	}

	/* Create the folder and open the file. */

	if (!filename_mkdir(foldername, true)) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (!output_text_line_open(filename)) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Write the file header. */

	if (!output_text_write_head(object)) {
		output_text_line_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (!output_text_line_write_newline()) {
		output_text_line_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Output the object. */

	if (!output_text_write_object(object, OUTPUT_TEXT_BASE_INDENT)) {
		output_text_line_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Output the file footer. */

	if (!output_text_write_foot(object)) {
		output_text_line_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	output_text_line_close();

	return true;
}

/**
 * Process the contents of an index, chapter or section block and write it out.
 *
 * \param *object	The object to process.
 * \param indent	The indent to write the section at.
 * \param *folder	The folder being written within.
 * \param in_line	True if the section is being written inline; otherwise False.
 * \return		True if successful; False on error.
 */

static bool output_text_write_object(struct manual_data *object, int indent)
{
	struct manual_data	*block;
	struct manual_data_mode *resources = NULL;
	struct output_text_line *paragraph_line;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Confirm that this is a suitable object. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_MANUAL:
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_SECTION),
				manual_data_find_object_name(object->type));
		return false;
	}

	resources = modes_find_resources(object->chapter.resources, MODES_TYPE_TEXT);

	/* Check that the nesting depth is OK. */

	if (indent > OUTPUT_TEXT_MAX_SECTION_INDENT) {
		msg_report(MSG_TOO_DEEP, indent);
		return false;
	}

	/* Write out the object heading. */

	if (object->title != NULL) {
		if (!output_text_line_write_newline())
			return false;

		if (!output_text_write_heading(object, indent))
			return false;
	}

	/* Create a paragraph for output. */

	paragraph_line = output_text_line_create();
	if (paragraph_line == NULL)
		return false;

	if (!output_text_line_add_column(paragraph_line, indent, output_text_page_width - indent)) {
		output_text_line_destroy(paragraph_line);
		return false;
	}

	/* If this is a separate file, queue it for writing later. */

	if (resources != NULL && (indent > OUTPUT_TEXT_BASE_INDENT) &&
			(resources->filename != NULL || resources->folder != NULL)) {
		if (object->chapter.resources->summary != NULL &&
				!output_text_write_paragraph(object->chapter.resources->summary, paragraph_line)) {
			output_text_line_destroy(paragraph_line);
			return false;
		}

	//	if (!output_text("\nThis is a link to an external file...\n"))
	//		return false;

		manual_queue_add_node(object);

		return true;
	} else {
		block = object->first_child;

		while (block != NULL) {
			switch (block->type) {
			case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
			case MANUAL_DATA_OBJECT_TYPE_INDEX:
			case MANUAL_DATA_OBJECT_TYPE_SECTION:
				if (!output_text_write_object(block, indent + OUTPUT_TEXT_BLOCK_INDENT)) {
					output_text_line_destroy(paragraph_line);
					return false;
				}
				break;

			case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(block->type));
					break;
				}

				if (!output_text_write_paragraph(block, paragraph_line)) {
					output_text_line_destroy(paragraph_line);
					return false;
				}
				break;

			default:
				msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(block->type));
				break;
			}

			block = block->next;
		}
	}

	output_text_line_destroy(paragraph_line);

	return true;
}

/**
 * Write a text file head block out.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_text_write_head(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

//	if (!output_strong_file_write_plain("<head>") || !output_strong_file_write_newline())
//		return false;

	if (!output_text_write_heading(manual, 0))
		return false;

//	if (!output_strong_file_write_plain("</head>") || !output_strong_file_write_newline())
//		return false;

	return true;
}

/**
 * Write a text file foot block out.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_text_write_foot(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	return true;
}

/**
 * Write a node title.
 *
 * \param *node			The node for which to write the title.
 * \param indent		The indent to write the title at.
 * \return			True if successful; False on error.
 */

static bool output_text_write_heading(struct manual_data *node, int indent)
{
	struct output_text_line	*line;
	char			*number;

	if (node == NULL || node->title == NULL)
		return true;

	number = manual_data_get_node_number(node);

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

	if (number != NULL) {
		if (!output_text_line_add_text(line, 0, number)) {
			output_text_line_destroy(line);
			return false;
		}

		if (!output_text_line_add_text(line, 0, " ")) {
			output_text_line_destroy(line);
			return false;
		}

		free(number);
	}

	if (!output_text_write_text(line, 0, MANUAL_DATA_OBJECT_TYPE_TITLE, node->title)) {
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

/**
 * Write a paragraph block to the output.
 *
 * \param *object		The object to be written.
 * \param *paragraph_line	The paragraph line instance to use.
 * \return			True if successful; False on failure.
 */

static bool output_text_write_paragraph(struct manual_data *object, struct output_text_line *paragraph_line)
{
	if (object == NULL)
		return false;

	/* Confirm that this is an index, chapter or section. */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_PARAGRAPH && object->type != MANUAL_DATA_OBJECT_TYPE_SUMMARY) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_PARAGRAPH),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Output the paragraph. */

	if (!output_text_line_reset(paragraph_line))
		return false;

	if (!output_text_line_write_newline()) 
		return false;

	if (!output_text_write_text(paragraph_line, 0, object->type, object))
		return false;

	if (!output_text_line_write(paragraph_line, false))
		return false;

	return true;
}

/**
 * Write a block of text to a column in an output line.
 *
 * \param *line			The line to write to.
 * \param column		The column in the line to write to.
 * \param type			The type of block which is expected.
 * \param *text			The block of text to be written.
 * \return			True if successful; False on error.
 */

static bool output_text_write_text(struct output_text_line *line, int column, enum manual_data_object_type type, struct manual_data *text)
{
	struct manual_data *chunk;

	/* An empty block doesn't require any output. */

	if (text == NULL)
		return true;

	if (text->type != type) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(type), manual_data_find_object_name(text->type));
		return false;
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
			output_text_line_add_text(line, column, output_text_convert_entity(chunk->chunk.entity));
			break;
		default:
			msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(chunk->type));
			break;
		}

		chunk = chunk->next;
	}

	return true;
}

/**
 * Convert an entity into a textual representation.
 *
 * \param entity		The entity to convert.
 * \return			Pointer to the textual representation.
 */

static const char *output_text_convert_entity(enum manual_entity_type entity)
{
	switch (entity) {
	case MANUAL_ENTITY_NBSP:
		return ENCODING_UTF8_NBSP;
	case MANUAL_ENTITY_AMP:
		return "&";
	case MANUAL_ENTITY_LSQUO:
	case MANUAL_ENTITY_RSQUO:
		return "'";
	case MANUAL_ENTITY_QUOT:
	case MANUAL_ENTITY_LDQUO:
	case MANUAL_ENTITY_RDQUO:
		return "\"";
	case MANUAL_ENTITY_MINUS:
		return "-";
	case MANUAL_ENTITY_NDASH:
		return ENCODING_UTF8_NBHY ENCODING_UTF8_NBHY;
	case MANUAL_ENTITY_MDASH:
		return ENCODING_UTF8_NBHY ENCODING_UTF8_NBHY ENCODING_UTF8_NBHY;
	case MANUAL_ENTITY_TIMES:
		return "x";
	default:
		msg_report(MSG_ENTITY_NO_MAP, manual_entity_find_name(entity));
		return "?";
	}
}
