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
 * \file output_html.c
 *
 * HTML Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "output_html.h"

#include "encoding.h"
#include "filename.h"
#include "manual_data.h"
#include "manual_queue.h"
#include "modes.h"
#include "msg.h"
#include "output_html_file.h"

/* Static constants. */

/**
 * The base level for section nesting.
 */

#define OUTPUT_HTML_BASE_LEVEL 1

/**
 * The maximum depth that sections can be nested.
 */

#define OUTPUT_HTML_MAX_NEST_DEPTH 6

/**
 * The length of buffer required to build <title> and <h?> tags
 * (allowing space for full-length ints, if ever required).
 */

#define OUTPUT_HTML_TITLE_TAG_BLOCK_LEN 20

/**
 * The root filename used when writing into an empty folder.
 */

#define OUTPUT_HTML_ROOT_FILENAME "index.html"

/* Global Variables. */

/**
 * The root filename used when writing into an empty folder.
 */

static struct filename *output_html_root_filename;


/* Static Function Prototypes. */

static bool output_html_write_manual(struct manual_data *manual, struct filename *folder);
static bool output_html_write_file(struct manual_data *object, struct filename *folder, bool single_file);
static bool output_html_write_section_object(struct manual_data *object, int level, bool root);
static bool output_html_write_head(struct manual_data *manual);
static bool output_html_write_foot(struct manual_data *manual);
static bool output_html_write_heading(struct manual_data *node, int level);
static bool output_html_write_block_collection_object(struct manual_data *object);
static bool output_html_write_list(struct manual_data *object);
static bool output_html_write_table(struct manual_data *object);
static bool output_html_write_code_block(struct manual_data *object);
static bool output_html_write_paragraph(struct manual_data *object);
static bool output_html_write_reference(struct manual_data *source, struct manual_data *target, char *text);
static bool output_html_write_text(enum manual_data_object_type type, struct manual_data *text);
static bool output_html_write_span_tag(enum manual_data_object_type type, char *tag, struct manual_data *text);
static bool output_html_write_span_style(enum manual_data_object_type type, char *style, struct manual_data *text);
static bool output_html_write_inline_link(struct manual_data *link);
static bool output_html_write_inline_reference(struct manual_data *reference);
static bool output_html_write_title(struct manual_data *node);
static const char *output_html_convert_entity(enum manual_entity_type entity);

/**
 * Output a manual in HTML form.
 *
 * \param *document	The manual to be output.
 * \param *folder	The folder to write the manual to.
 * \param encoding	The encoding to use for output.
 * \param line_end	The line ending to use for output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_html(struct manual *document, struct filename *folder, enum encoding_target encoding, enum encoding_line_end line_end)
{
	bool result;

	if (document == NULL || document->manual == NULL)
		return false;

	msg_report(MSG_START_MODE, "HTML");

	/* Output encoding defaults to UTF8. */

	encoding_select_table((encoding != ENCODING_TARGET_NONE) ? encoding : ENCODING_TARGET_UTF8);

	/* Output line endings default to LF. */

	encoding_select_line_end((line_end != ENCODING_LINE_END_NONE) ? line_end : ENCODING_LINE_END_LF);

	/* Write the manual file content. */

	output_html_root_filename = filename_make(OUTPUT_HTML_ROOT_FILENAME, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LINUX);

	result = output_html_write_manual(document->manual, folder);

	filename_destroy(output_html_root_filename);

	return result;
}

/**
 * Write an HTML manual body block out.
 *
 * \param *manual	The manual to process.
 * \param *folder	The folder into which to write the manual.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_manual(struct manual_data *manual, struct filename *folder)
{
	struct manual_data *object;
	bool single_file = false;

	if (manual == NULL || folder == NULL)
		return false;

	/* Confirm that this is a manual. */

	if (manual->type != MANUAL_DATA_OBJECT_TYPE_MANUAL) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_MANUAL),
				manual_data_find_object_name(manual->type));
		return false;
	}

	/* Identify whether output is destined for a single file. */

	single_file = !manual_data_find_filename_data(manual, MODES_TYPE_TEXT);

	/* Initialise the manual queue. */

	manual_queue_initialise();

	/* Process the files, starting with the root node. */

	manual_queue_add_node(manual);

	do {
		object = manual_queue_remove_node();
		if (object == NULL)
			continue;

		if (!output_html_write_file(object, folder, single_file))
			return false;
	} while (object != NULL);

	return true;
}


/**
 * Write a node and its descendents as a self-contained file.
 *
 * \param *object	The object to process.
 * \param *folder	The folder into which to write the manual.
 * \param single_file	TRUE if the output is intended to go into a single file.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_file(struct manual_data *object, struct filename *folder, bool single_file)
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

	/* Find the file and folder names. If the output is destined for a single file,
	 * we just start with an empty filename and prepend the supplied path; otherwise
	 * we find a leaf from the manual data.
	 */

	if (single_file == true)
		filename = filename_make(NULL, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_NONE);
	else
		filename = manual_data_get_node_filename(object, output_html_root_filename, MODES_TYPE_HTML);
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

	if (!output_html_file_open(filename)) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Write the file header. */

	if (!output_html_write_head(object)) {
		output_html_file_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Output the object. */

	if (!output_html_write_section_object(object, OUTPUT_HTML_BASE_LEVEL, true)) {
		output_html_file_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Output the file footer. */

	if (!output_html_write_foot(object)) {
		output_html_file_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	output_html_file_close();

	return true;
}


/**
 * Process the contents of an index, chapter or section block and write it out.
 *
 * \param *object		The object to process.
 * \param level			The level to write the section at.
 * \param root			True if the object is at the root of a file.
 * \return			True if successful; False on error.
 */

static bool output_html_write_section_object(struct manual_data *object, int level, bool root)
{
	struct manual_data *block;
	struct manual_data_mode *resources = NULL;

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

	resources = modes_find_resources(object->chapter.resources, MODES_TYPE_HTML);

	/* Check that the nesting depth is OK. */

	if (level > OUTPUT_HTML_MAX_NEST_DEPTH) {
		msg_report(MSG_TOO_DEEP, level);
		return false;
	}

	/* Write out the object heading. */

	if (object->title != NULL) {
		if (!root && !output_html_file_write_newline())
			return false;

		if (!output_html_write_heading(object, level))
			return false;
	}

	/* If this is a separate file, queue it for writing later. Otherwise,
	 * write the objects which fall within it.
	 */

	if (resources != NULL && !root && (resources->filename != NULL || resources->folder != NULL)) {
		if (object->chapter.resources->summary != NULL &&
				!output_html_write_paragraph(object->chapter.resources->summary))
			return false;

		if (!output_html_file_write_newline())
			return false;

		if (!output_html_file_write_plain("<p>"))
			return false;

		if (!output_html_write_reference(object->parent, object, "This is a link to an external file..."))
			return false;

		if (!output_html_file_write_plain("</p>"))
			return false;

		if (!output_html_file_write_newline())
			return false;

		manual_queue_add_node(object);
	} else {
		block = object->first_child;

		/* If changing this switch, note the analogous list in
		 * output_html_write_block_collection_object() which
		 * covers similar block level objects.
		 */

		while (block != NULL) {
			switch (block->type) {
			case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
			case MANUAL_DATA_OBJECT_TYPE_INDEX:
			case MANUAL_DATA_OBJECT_TYPE_SECTION:
				if (!output_html_write_section_object(block, level + 1, false))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_html_write_paragraph(block))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
			case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_html_write_list(block))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_TABLE:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_html_write_table(block))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_html_write_code_block(block))
					return false;
				break;

			default:
				msg_report(MSG_UNEXPECTED_CHUNK,
						manual_data_find_object_name(block->type),
						manual_data_find_object_name(object->type));
				break;
			}

			block = block->next;
		}
	}

	return true;
}


/**
 * Write an HTML file head block out. This starts with the doctype and
 * continues until we've written the opening <body>.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_head(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("<!DOCTYPE html>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<html>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<head>") || !output_html_file_write_newline())
		return false;

	if (!output_html_write_heading(manual, 0))
		return false;

	if (!output_html_file_write_plain("</head>") || !output_html_file_write_newline() || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<body>") || !output_html_file_write_newline())
		return false;

	return true;
}

/**
 * Write an HTML file foot block out. This starts with the closing
 * </body> and runs to the end of the file.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_foot(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("</body>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("</html>") || !output_html_file_write_newline()) {
		output_html_file_close();
		return false;
	}

	return true;
}

/**
 * Write a node title.
 *
 * \param *node			The node for which to write the title.
 * \param level			The level to write the title at.
 * \return			True if successful; False on error.
 */

static bool output_html_write_heading(struct manual_data *node, int level)
{
	char	buffer[OUTPUT_HTML_TITLE_TAG_BLOCK_LEN];

	if (node == NULL || node->title == NULL)
		return true;

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_MANUAL:
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		break;
	default:
		return false;
	}

	if (level < 0 || level > 6)
		return false;

	/* Create and write the opening tag. */

	if (level == 0)
		snprintf(buffer, OUTPUT_HTML_TITLE_TAG_BLOCK_LEN, "title");
	else
		snprintf(buffer, OUTPUT_HTML_TITLE_TAG_BLOCK_LEN, "h%d", level);

	buffer[OUTPUT_HTML_TITLE_TAG_BLOCK_LEN - 1] = '\0';

	if (!output_html_file_write_plain("<%s", buffer))
		return false;

	/* Include the ID in the opening tag, if required. */

	if (level > 0 && node->chapter.id != NULL) {
		if (!output_html_file_write_plain(" id=\""))
			return false;

		if (!output_html_file_write_text(node->chapter.id))
			return false;

		if (!output_html_file_write_plain("\""))
			return false;
	}

	if (!output_html_file_write_plain(">"))
		return false;

	/* Write the title text. */

	if (!output_html_write_title(node))
		return false;

	/* Write the closing tag. */

	if (!output_html_file_write_plain("</%s>", buffer))
		return false;

	if (!output_html_file_write_newline())
		return false;

	return true;
}

/**
 * Process the contents of a block collection and write it out.
 * A block collection must be nested within a parent block object
 * which can take its content directly if there is only one chunk
 * within.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_html_write_block_collection_object(struct manual_data *object)
{
	struct manual_data *block;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Confirm that this is a suitable object. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_LIST_ITEM:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_LIST_ITEM),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Write out the block contents. */

	block = object->first_child;

	/* If changing this switch, note the analogous list in
		* output_html_write_section_object() which covers similar
		* block level objects.
		*/

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			/* If this is the only item in the collection, write it directly
			 * within the parent tags instead of making a paragraph out of it. */
			if (block->previous == NULL && block->next == NULL) {
				if (!output_html_write_text(block->type, block))
					return false;
			} else if (block->previous == NULL) {
				if (!output_html_file_write_plain("<p style=\"margin-top: 0;\">"))
					return false;
				if (!output_html_write_text(block->type, block))
					return false;
				if (!output_html_file_write_plain("</p>"))
					return false;
			} else if (block->next == NULL) {
				if (!output_html_file_write_plain("<p style=\"margin-bottom: 0;\">"))
					return false;
				if (!output_html_write_text(block->type, block))
					return false;
				if (!output_html_file_write_plain("</p>"))
					return false;
			} else {
				if (!output_html_write_paragraph(block))
					return false;
			}
			break;

		case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
		case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
			if (!output_html_write_list(block))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_TABLE:
			if (!output_html_write_table(block))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
			if (!output_html_write_code_block(block))
				return false;
			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(block->type),
					manual_data_find_object_name(object->type));
			break;
		}

		block = block->next;
	}

	return true;
}

/**
 * Process the contents of a list and write it out.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_html_write_list(struct manual_data *object)
{
	struct manual_data *item;

	if (object == NULL)
		return false;

	/* Confirm that this is a list. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
	case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Output the list. */

	if (!output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain((object->type == MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST) ? "<ol>" : "<ul>"))
		return false;


	item = object->first_child;

	while (item != NULL) {
		switch (item->type) {
		case MANUAL_DATA_OBJECT_TYPE_LIST_ITEM:
			if (!output_html_file_write_newline())
				return false;

			if (!output_html_file_write_plain("<li>"))
				return false;

			if (!output_html_write_block_collection_object(item))
				return false;

			if (!output_html_file_write_plain("</li>"))
				return false;
			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(item->type),
					manual_data_find_object_name(object->type));
			break;
		}

		item = item->next;
	}

	if (!output_html_file_write_plain((object->type == MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST) ? "</ol>" : "</ul>"))
		return false;

	return true;
}

/**
 * Process the contents of a table and write it out.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_html_write_table(struct manual_data *object)
{
	struct manual_data *column_set, *column, *row;

	if (object == NULL)
		return false;

	/* Confirm that this is a table. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_TABLE:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_TABLE),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Output the table. */

	if (!output_html_file_write_newline() || !output_html_file_write_plain("<div class=\"table\"><table>"))
		return false;

	/* Write the table headings. */

	column_set = object->chapter.columns;
	if (column_set->type != MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET),
				manual_data_find_object_name(object->type));
		return false;
	}

	if (!output_html_file_write_newline() || !output_html_file_write_plain("<tr>"))
		return false;

	column = column_set->first_child;

	while (column != NULL) {
		switch (column->type) {
		case MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_DEFINITION:
			if (!output_html_file_write_plain("<th>"))
				return false;

			if (!output_html_write_text(column->type, column))
				return false;

			if (!output_html_file_write_plain("</th>"))
				return false;

			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(column->type),
					manual_data_find_object_name(column_set->type));
			break;
		}

		column = column->next;
	}

	if (!output_html_file_write_plain("</tr>"))
		return false;

	/* Write the table rows. */

	row = object->first_child;

	while (row != NULL) {
		switch (row->type) {
		case MANUAL_DATA_OBJECT_TYPE_TABLE_ROW:
			column = row->first_child;

			if (!output_html_file_write_newline() || !output_html_file_write_plain("<tr>"))
				return false;

			while (column != NULL) {
				switch (column->type) {
				case MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN:
					if (!output_html_file_write_plain("<td>"))
						return false;

					if (!output_html_write_text(column->type, column))
						return false;

					if (!output_html_file_write_plain("</td>"))
						return false;

					break;

				default:
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(column->type),
							manual_data_find_object_name(row->type));
					break;
				}

				column = column->next;
			}

			if (!output_html_file_write_plain("</tr>"))
				return false;
			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(row->type),
					manual_data_find_object_name(object->type));
			break;
		}

		row = row->next;
	}

	/* Close the table. */

	if (!output_html_file_write_newline() || !output_html_file_write_plain("</table>"))
		return false;

	/* Write the title. */

	if (object->title != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_file_write_plain("<div class=\"caption\">"))
			return false;

		if (!output_html_write_title(object))
			return false;

		if (!output_html_file_write_plain("</div>"))
			return false;
	}

	/* Close the outer DIV. */

	if (!output_html_file_write_plain("</div>"))
		return false;

	return true;
}

/**
 * Process the contents of a code block and write it out.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_html_write_code_block(struct manual_data *object)
{
	if (object == NULL)
		return false;

	/* Confirm that this is a code block. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Output the code block. */

	if (!output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<div class=\"codeblock\"><pre>"))
		return false;

	if (!output_html_write_text(object->type, object))
		return false;

	if (!output_html_file_write_plain("</pre>"))
		return false;

	if (object->title != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_file_write_plain("<div class=\"caption\">"))
			return false;

		if (!output_html_write_title(object))
			return false;

		if (!output_html_file_write_plain("</div>"))
			return false;
	}

	if (!output_html_file_write_plain("</div>"))
		return false;

	return true;
}

/**
 * Write a paragraph block to the output.
 *
 * \param *object		The object to be written.
 * \return			True if successful; False on failure.
 */

static bool output_html_write_paragraph(struct manual_data *object)
{
	if (object == NULL)
		return false;

	/* Confirm that this is a paragraph or summary . */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_PARAGRAPH && object->type != MANUAL_DATA_OBJECT_TYPE_SUMMARY) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_PARAGRAPH),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Output the paragraph. */

	if (!output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<p>"))
		return false;

	if (!output_html_write_text(object->type, object))
				return false;

	if (!output_html_file_write_plain("</p>"))
		return false;

	return true;
}


/**
 * Write an internal reference (a link to another page) to the output.
 * 
 * \param *source		The node to be the source of the link.
 * \param *target		The node to be the target of the link.
 * \param *text			Text to use for the link, or NULL for none.
 * \return			True if successful; False on failure.
 */

static bool output_html_write_reference(struct manual_data *source, struct manual_data *target, char *text)
{
	struct filename *sourcename = NULL, *targetname = NULL, *filename = NULL;
	char *link = NULL;

	if (source == NULL || target == NULL)
		return false;

	sourcename = manual_data_get_node_filename(source, output_html_root_filename, MODES_TYPE_HTML);
	if (sourcename == NULL)
		return false;

	targetname = manual_data_get_node_filename(target, output_html_root_filename, MODES_TYPE_HTML);
	if (targetname == NULL) {
		filename_destroy(sourcename);
		return false;
	}

	filename = filename_get_relative(sourcename, targetname);
	filename_destroy(sourcename);
	filename_destroy(targetname);
	if (filename == NULL)
		return false;

	link = filename_convert(filename, FILENAME_PLATFORM_LINUX, 0);
	filename_destroy(filename);

	if (link == NULL)
		return false;

	if (!output_html_file_write_plain("<a href=\"")) {
		free(link);
		return false;
	}

	if (text != NULL && !output_html_file_write_text(link)) {
		free(link);
		return false;
	}

	free(link);

	if (text != NULL && !output_html_file_write_plain("\">"))
		return false;

	if (!output_html_file_write_text(text))
		return false;

	if (!output_html_file_write_plain("</a>"))
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
			output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, "em", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, "strong", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_CITATION:
			output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_CITATION, "cite", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_CODE:
			output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_CODE, "code", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_FILENAME:
			output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_FILENAME, "filename", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ICON:
			output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_ICON, "icon", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_KEY:
			output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_KEY, "key", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_LINK:
			output_html_write_inline_link(chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_MOUSE:
			output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_MOUSE, "mouse", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_REFERENCE:
			output_html_write_inline_reference(chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			output_html_file_write_text(chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_USER_ENTRY:
			output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_USER_ENTRY, "entry", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_VARIABLE:
			output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_VARIABLE, "variable", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_WINDOW:
			output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_WINDOW, "window", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			output_html_file_write_text((char *) output_html_convert_entity(chunk->chunk.entity));
			break;
		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(chunk->type),
					manual_data_find_object_name(text->type));
			break;
		}

		chunk = chunk->next;
	}

	return true;
}

/**
 * Write out a section of text wrapped in HTML tags.
 * 
 * \param type			The type of block which is expected.
 * \param *tag			The HTML tag to use.
 * \param *text			The block of text to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_span_tag(enum manual_data_object_type type, char *tag, struct manual_data *text)
{
	if (text == NULL || tag == NULL)
		return false;

	if (output_html_file_write_plain("<%s>", tag) == false)
		return false;

	if (output_html_write_text(type, text) == false)
		return false;

	if (output_html_file_write_plain("</%s>", tag) == false)
		return false;

	return true;
}

/**
 * Write out a section of text wrapped in HTML <span> tags.
 * 
 * \param type			The type of block which is expected.
 * \param *style		The CSS class to use in the span.
 * \param *text			The block of text to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_span_style(enum manual_data_object_type type, char *style, struct manual_data *text)
{
	if (text == NULL || style == NULL)
		return false;

	if (output_html_file_write_plain("<span class=\"%s\">", style) == false)
		return false;

	if (output_html_write_text(type, text) == false)
		return false;

	if (output_html_file_write_plain("</span>") == false)
		return false;

	return true;
}


/**
 * Write out an inline link.
 *
 * \param *link			The link to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_inline_link(struct manual_data *link)
{
	if (link == NULL)
		return false;

	/* Confirm that this is a link. */

	if (link->type != MANUAL_DATA_OBJECT_TYPE_LINK) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_LINK),
				manual_data_find_object_name(link->type));
		return false;
	}

	/* Output the opening link tag. */

	if (link->chunk.link != NULL && !output_html_file_write_plain("<a href=\""))
		return false;

	if (link->chunk.link != NULL && !output_html_write_text(MANUAL_DATA_OBJECT_TYPE_SINGLE_LEVEL_ATTRIBUTE, link->chunk.link))
		return false;

	if (link->chunk.link != NULL && !output_html_file_write_plain("\""))
		return false;

	if ((link->chunk.flags & MANUAL_DATA_OBJECT_FLAGS_LINK_EXTERNAL) && !output_html_file_write_plain(" class=\"external\""))
		return false;

	if (link->chunk.link != NULL && !output_html_file_write_plain(">"))
		return false;

	/* Output the link body. */

	if (link->first_child != NULL) {
		if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_LINK, link))
			return false;
	} else if (link->chunk.link != NULL) {
		if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_SINGLE_LEVEL_ATTRIBUTE, link->chunk.link))
			return false;
	}
	
	/* Output the closing link tag. */

	if (link->chunk.link != NULL && !output_html_file_write_plain("</a>"))
		return false;

	return true;
}


/**
 * Write out an inline reference.
 *
 * \param *reference		The reference to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_inline_reference(struct manual_data *reference)
{
	struct manual_data *target = NULL;
	struct filename *sourcename = NULL, *targetname = NULL, *filename = NULL;
	char *link = NULL;

	if (reference == NULL)
		return false;

	/* Confirm that this is a reference. */

	if (reference->type != MANUAL_DATA_OBJECT_TYPE_REFERENCE) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_REFERENCE),
				manual_data_find_object_name(reference->type));
		return false;
	}

	/* Find the target object. */

	target = manual_ids_find_node(reference);

	/* Establish the relative link, if external. */

	if (manual_data_nodes_share_file(reference, target, MODES_TYPE_HTML) == false) {
		sourcename = manual_data_get_node_filename(reference, output_html_root_filename, MODES_TYPE_HTML);
		if (sourcename == NULL)
			return false;

		targetname = manual_data_get_node_filename(target, output_html_root_filename, MODES_TYPE_HTML);
		if (targetname == NULL) {
			filename_destroy(sourcename);
			return false;
		}

		filename = filename_get_relative(sourcename, targetname);
		filename_destroy(sourcename);
		filename_destroy(targetname);
		if (filename == NULL)
			return false;

		link = filename_convert(filename, FILENAME_PLATFORM_LINUX, 0);
		filename_destroy(filename);
	}

	/* Output the opening link tag. */

	if (target != NULL && !output_html_file_write_plain("<a href=\"%s#%s\">", (link == NULL) ? "" : link, (target->chapter.id == NULL) ? "" : target->chapter.id))
		return false;

	/* Output the link body. */

	if (reference->first_child != NULL) {
		if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_REFERENCE, reference))
			return false;
	} else {
		if (!output_html_write_title(target))
			return false;
	}
	
	/* Output the closing link tag. */

	if (target != NULL && !output_html_file_write_plain("</a>"))
		return false;

	return true;
}


/**
 * Write out the title of a node.
 *
 * \param *node			The node whose title is to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_title(struct manual_data *node)
{
	char *number;

	if (node == NULL || node->title == NULL)
		return false;

	number = manual_data_get_node_number(node);

	if (number != NULL) {
		if (!output_html_file_write_text(number)) {
			free(number);
			return false;
		}

		free(number);

		if (!output_html_file_write_text(" "))
			return false;
	}

	return output_html_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, node->title);
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
	case MANUAL_ENTITY_LT:
		return "&lt;";
	case MANUAL_ENTITY_GT:
		return "&gt";
	case MANUAL_ENTITY_LE:
		return "&le;";
	case MANUAL_ENTITY_GE:
		return "&ge;";
	case MANUAL_ENTITY_MINUS:
		return "&minus;";
	case MANUAL_ENTITY_NDASH:
		return "&ndash;";
	case MANUAL_ENTITY_MDASH:
		return "&mdash";
	case MANUAL_ENTITY_TIMES:
		return "&times;";
	case MANUAL_ENTITY_SMILE:
		return "&#128578;";
	case MANUAL_ENTITY_SAD:
		return "&#128577;";
	default:
		msg_report(MSG_ENTITY_NO_MAP, manual_entity_find_name(entity));
		return "?";
	}
}

/**
 * Taking a base folder and an object resources, work out the object's
 * base folder and filename.
 * 
 * \param *base		The base folder.
 * \param *resources	The object's resources.
 * \param **folder	Pointer to a filename pointer in which to return
 *			the object's base folder.
 * \param **file	Pointer to a filename pointer in which to return
 *			the object's filename.
 * \return		True if the object should be in a self-contained
 *			file; False if it should appear inline in its parent.
 */
#if 0
static bool output_html_find_folders(struct filename *base, struct manual_data_resources *resources, struct filename **folder, struct filename **file)
{
	if (folder != NULL)
		*folder = NULL;

	if (file != NULL)
		*file = NULL;

	/* If there are no resources, then this must be an inline block. */

	if (resources == NULL || (resources->html.filename == NULL && resources->html.folder == NULL)) {
		if (base != NULL && folder != NULL)
			*folder = filename_up(base, 0);
		return false;
	}

	/* If there's no base filename, there's no point working out the folders. */

	if (base == NULL)
		return true;

	/* Get the root folder. */

	*folder = filename_join(base, resources->html.folder);

	/* Work out the filename. */

	if (file != NULL) {
		if (resources->html.filename == NULL)
			*file = filename_join(*folder, output_html_root_filename);
		else
			*file = filename_join(*folder, resources->html.filename);
	}

	/* This is a self-contained file. */

	return true;
}
#endif