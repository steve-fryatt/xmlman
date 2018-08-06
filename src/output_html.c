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
 * \file output_html.c
 *
 * HTML Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

#include "output_html.h"

#include "encoding.h"
#include "manual_data.h"
#include "msg.h"
#include "output_html_file.h"

/* Static constants. */

/**
 * The length of buffer required to build <title> and <h?> tags.
 */

#define OUTPUT_HTML_TITLE_TAG_BLOCK_LEN 6

/* Static Function Prototypes. */

static bool output_html_write_head(struct manual_data *manual);
static bool output_html_write_body(struct manual_data *manual);
static bool output_html_write_chapter(struct manual_data *chapter, int level);
static bool output_html_write_section(struct manual_data *section, int level);
static bool output_html_write_heading(struct manual_data *title, int level);
static bool output_html_write_text(enum manual_data_object_type type, struct manual_data *text);
static const char *output_html_convert_entity(enum manual_entity_type entity);

/**
 * Output a manual in text form.
 *
 * \param *manual	The manual to be output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_html(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	encoding_select_table(ENCODING_TARGET_UTF8);
	encoding_select_line_end(ENCODING_LINE_END_LF);

	if (!output_html_file_open("test.html"))
		return false;

	if (!output_html_file_write_plain("<!DOCTYPE html>") || !output_html_file_write_newline()) {
		output_html_file_close();
		return false;
	}

	if (!output_html_file_write_plain("<html>") || !output_html_file_write_newline()) {
		output_html_file_close();
		return false;
	}

	if (!output_html_write_head(manual)) {
		output_html_file_close();
		return false;
	}

	if (!output_html_write_body(manual)) {
		output_html_file_close();
		return false;
	}

	if (!output_html_file_write_plain("</html>") || !output_html_file_write_newline()) {
		output_html_file_close();
		return false;
	}

	output_html_file_close();

	return true;
}

/**
 * Write an HTML file head block out.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_head(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("<head>") || !output_html_file_write_newline())
		return false;

	output_html_write_heading(manual->title, 0);

	if (!output_html_file_write_plain("</head>") || !output_html_file_write_newline())
		return false;

	return true;
}

/**
 * Write an HTML file body block out.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_body(struct manual_data *manual)
{
	struct manual_data *chapter;
	int base_level = 1;

	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("<body>") || !output_html_file_write_newline())
		return false;

	/* Output the chapter details. */

	chapter = manual->first_child;

	while (chapter != NULL) {
		if (!output_html_write_chapter(chapter, base_level))
			return false;

		chapter = chapter->next;
	}

	if (!output_html_file_write_plain("</body>") || !output_html_file_write_newline())
		return false;

	return true;
}

/**
 * Process the contents of a chapter block and write it out.
 *
 * \param *chapter		The chapter to process.
 * \param level			The level to write the chapter at.
 * \return			True if successful; False on error.
 */

static bool output_html_write_chapter(struct manual_data *chapter, int level)
{
	struct manual_data *section;

	if (chapter == NULL || chapter->first_child == NULL)
		return true;

	if (!output_html_file_write_newline())
		return false;

	if (chapter->title != NULL) {
		if (!output_html_write_heading(chapter->title, level))
			return false;
	}

	section = chapter->first_child;

	while (section != NULL) {
		if (!output_html_write_section(section, level + 1))
			return false;

		section = section->next;
	}

	return true;
}

/**
 * Process the contents of a section block and write it out.
 *
 * \param *section		The section to process.
 * \param level			The level to write the section at.
 * \return			True if successful; False on error.
 */

static bool output_html_write_section(struct manual_data *section, int level)
{
	struct manual_data	*block;

	if (section == NULL || section->first_child == NULL)
		return true;

	if (section->title != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_write_heading(section->title, level))
			return false;
	}

	block = section->first_child;

	while (block != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_file_write_plain("<p>"))
			return false;

		if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_PARAGRAPH, block)) 
			return false;

		if (!output_html_file_write_plain("</p>"))
			return false;

		block = block->next;
	}

	return true;
}

/**
 * Write a block of text as a section title.
 *
 * \param *title		The block of text to be written.
 * \param level			The level to write the title at.
 * \return			True if successful; False on error.
 */

static bool output_html_write_heading(struct manual_data *title, int level)
{
	char buffer[OUTPUT_HTML_TITLE_TAG_BLOCK_LEN];

	if (title == NULL)
		return true;

	if (level < 0 || level > 6)
		return false;

	if (level == 0)
		snprintf(buffer, OUTPUT_HTML_TITLE_TAG_BLOCK_LEN, "title");
	else
		snprintf(buffer, OUTPUT_HTML_TITLE_TAG_BLOCK_LEN, "h%d", level);

	buffer[OUTPUT_HTML_TITLE_TAG_BLOCK_LEN - 1] = '\0';

	if (!output_html_file_write_plain("<%s>", buffer))
		return false;

	if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, title))
		return false;

	if (!output_html_file_write_plain("</%s>", buffer))
		return false;

	if (!output_html_file_write_newline())
		return false;

	return true;
}

/**
 * Write a block of text to the output file.
 *
 * \param type			The type of block which is expected.
 * \param *text			The block of text to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_text(enum manual_data_object_type type, struct manual_data *text)
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
			output_html_file_write_plain("<em>");
			output_html_write_text(MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, chunk);
			output_html_file_write_plain("</em>");
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			output_html_file_write_plain("<strong>");
			output_html_write_text(MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, chunk);
			output_html_file_write_plain("</strong>");
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			output_html_file_write_text(chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			output_html_file_write_text((xmlChar *) output_html_convert_entity(chunk->chunk.entity));
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
 * Convert an entity into an HTML representation.
 *
 * \param entity		The entity to convert.
 * \return			Pointer to the HTML representation.
 */

static const char *output_html_convert_entity(enum manual_entity_type entity)
{
	switch (entity) {
	case MANUAL_ENTITY_NBSP:
		return "&nbsp;";
	case MANUAL_ENTITY_AMP:
		return "&amp;";
	case MANUAL_ENTITY_LSQUO:
		return "&lsquo;";
	case MANUAL_ENTITY_RSQUO:
		return "&rsquo;";
	case MANUAL_ENTITY_QUOT:
		return "&quot;";
	case MANUAL_ENTITY_LDQUO:
		return "&ldquo;";
	case MANUAL_ENTITY_RDQUO:
		return "&rdquo;";
	case MANUAL_ENTITY_MINUS:
		return "&minus";
	case MANUAL_ENTITY_NDASH:
		return "&ndash;";
	case MANUAL_ENTITY_MDASH:
		return "&mdash";
	case MANUAL_ENTITY_TIMES:
		return "&times;";
	default:
		msg_report(MSG_ENTITY_NO_MAP, manual_entity_find_name(entity));
		return "?";
	}
}

