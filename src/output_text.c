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
#include "filename.h"
#include "manual_data.h"
#include "msg.h"
#include "output_text_line.h"

/* Static constants. */

/**
 * The number of characters to indent each new block by.
 */
 
#define OUTPUT_TEXT_BLOCK_INDENT 2

/**
 * The maximum indent that can be applied to sections (effectively
 * limiting the depth to which they can be nested.
 */

#define OUTPUT_TEXT_MAX_SECTION_INDENT 10

/* Global Variables. */

/**
 * The width of a page, in characters.
 */
static int output_text_page_width = 77;

/* Static Function Prototypes. */

static bool output_text_write_chapter(struct manual_data *chapter, int indent);
static bool output_text_write_section(struct manual_data *section, int indent);
static bool output_text_write_heading(struct manual_data *node, int indent);
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
	struct manual_data	*chapter;
	int			base_indent = 0;

	if (document == NULL || document->manual == NULL)
		return false;

	/* Output encoding defaults to UTF8. */

	encoding_select_table((encoding != ENCODING_TARGET_NONE) ? encoding : ENCODING_TARGET_UTF8);

	/* Output line endings default to LF. */

	encoding_select_line_end((line_end != ENCODING_LINE_END_NONE) ? line_end : ENCODING_LINE_END_LF);

	/* Find and open the output file. */

	if (!output_text_line_open(filename))
		return false;

	if (!output_text_write_heading(document->manual, base_indent)) {
		output_text_line_close();
		return false;
	}

	/* Output the chapter details. */

	chapter = document->manual->first_child;

	while (chapter != NULL) {
		if (!output_text_write_chapter(chapter, base_indent)) {
			output_text_line_close();
			return false;
		}

		chapter = chapter->next;
	}

	output_text_line_close();

	return true;
}

/**
 * Process the contents of a chapter block and write it out.
 *
 * \param *chapter		The chapter to process.
 * \param indent		The indent to write the chapter at.
 * \return			True if successful; False on error.
 */

static bool output_text_write_chapter(struct manual_data *chapter, int indent)
{
	struct manual_data *section;

	if (chapter == NULL || chapter->first_child == NULL)
		return true;

	/* Confirm that this is a chapter. */

//	if (chapter->type != MANUAL_DATA_OBJECT_TYPE_CHAPTER) {
//		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_CHAPTER),
//				manual_data_find_object_name(chapter->type));
//		return false;
//	}

	if (!output_text_line_write_newline() || !output_text_line_write_newline())
		return false;

	if (chapter->title != NULL) {
		if (!output_text_write_heading(chapter, indent))
			return false;
	}

	section = chapter->first_child;

	while (section != NULL) {
		if (!output_text_write_section(section, indent + OUTPUT_TEXT_BLOCK_INDENT))
			return false;

		section = section->next;
	}

	return true;
}

/**
 * Process the contents of a section block and write it out.
 *
 * \param *section		The section to process.
 * \param indent		The indent to write the section at.
 * \return			True if successful; False on error.
 */

static bool output_text_write_section(struct manual_data *section, int indent)
{
	struct manual_data	*block;
	struct output_text_line *paragraph_line;

	if (section == NULL || section->first_child == NULL)
		return true;

	/* Confirm that this is a section. */

	if (section->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_SECTION),
				manual_data_find_object_name(section->type));
		return false;
	}

	/* Check that the nesting depth is OK. */

	if (indent > OUTPUT_TEXT_MAX_SECTION_INDENT) {
		msg_report(MSG_TOO_DEEP, indent);
		return false;
	}

	/* Write out the section heading. */

	if (section->title != NULL) {
		if (!output_text_line_write_newline())
			return false;

		if (!output_text_write_heading(section, indent))
			return false;
	}

	paragraph_line = output_text_line_create();
	if (paragraph_line == NULL)
		return false;

	if (!output_text_line_add_column(paragraph_line, indent, output_text_page_width - indent)) {
		output_text_line_destroy(paragraph_line);
		return false;
	}

	block = section->first_child;

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			output_text_write_section(block, indent + OUTPUT_TEXT_BLOCK_INDENT);
			break;

		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			if (!output_text_line_reset(paragraph_line)) {
				output_text_line_destroy(paragraph_line);
				return false;
			}

			if (!output_text_line_write_newline()) {
				output_text_line_destroy(paragraph_line);
				return false;
			}

			if (!output_text_write_text(paragraph_line, 0, MANUAL_DATA_OBJECT_TYPE_PARAGRAPH, block)) {
				output_text_line_destroy(paragraph_line);
				return false;
			}

			if (!output_text_line_write(paragraph_line, false)) {
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
	xmlChar			*number;

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

		if (!output_text_line_add_text(line, 0, (xmlChar *) " ")) {
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
			output_text_line_add_text(line, column, (xmlChar *) "/");
			output_text_write_text(line, column, MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, chunk);
			output_text_line_add_text(line, column, (xmlChar *) "/");
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			output_text_line_add_text(line, column, (xmlChar *) "*");
			output_text_write_text(line, column, MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, chunk);
			output_text_line_add_text(line, column, (xmlChar *) "*");
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			output_text_line_add_text(line, column, chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			output_text_line_add_text(line, column, (xmlChar *) output_text_convert_entity(chunk->chunk.entity));
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

