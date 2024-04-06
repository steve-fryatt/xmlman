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
#include "list_numbers.h"
#include "manual_data.h"
#include "manual_ids.h"
#include "manual_queue.h"
#include "modes.h"
#include "msg.h"
#include "output_text_line.h"

/* Static constants. */

/**
 * The number of characters to indent each new block by.
 */

#define OUTPUT_TEXT_BLOCK_INDENT 2

/**
 * No indent from the parent block.
 */

#define OUTPUT_TEXT_NO_INDENT 0

/**
 * The maximum indent that can be applied to sections (effectively
 * limiting the depth to which they can be nested.
 */

#define OUTPUT_TEXT_MAX_SECTION_LEVEL 5

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

/**
 * The bullets that we will use for unordered lists.
 */

static char *output_text_unordered_list_bullets[] = { "*", "+", ">", NULL };

/* Static Function Prototypes. */

static bool output_text_write_manual(struct manual_data *chapter, struct filename *folder);
static bool output_text_write_file(struct manual_data *object, struct filename *folder, bool single_file);
static bool output_text_write_object(struct manual_data *object, bool root, int level);
static bool output_text_write_head(struct manual_data *manual);
static bool output_text_write_foot(struct manual_data *manual);
static bool output_text_write_heading(struct manual_data *node, int column);
static bool output_text_write_chapter_list(struct manual_data *object, int level);
static bool output_text_write_block_collection_object(struct manual_data *object, int column, int level);
static bool output_text_write_footnote(struct manual_data *object, int column);
static bool output_text_write_list(struct manual_data *object, int column, int level);
static bool output_text_write_table(struct manual_data *object, int target_column);
static bool output_text_write_code_block(struct manual_data *object, int column);
static bool output_text_write_paragraph(struct manual_data *object, int column, bool last_item);
static bool output_text_write_reference(struct manual_data *target);
static bool output_text_write_text(int column, enum manual_data_object_type type, struct manual_data *text);
static bool output_text_write_inline_link(int column, struct manual_data *link);
static bool output_text_write_inline_reference(int column, struct manual_data *reference);
static bool output_text_write_title(int column, struct manual_data *node, bool include_name, bool include_title);
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
	bool single_file = false;

	if (manual == NULL || folder == NULL)
		return true;

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

		if (!output_text_write_file(object, folder, single_file))
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

static bool output_text_write_file(struct manual_data *object, struct filename *folder, bool single_file)
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
		filename = manual_data_get_node_filename(object, output_text_root_filename, MODES_TYPE_TEXT);
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

	if (!output_text_line_open(filename, output_text_page_width)) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Set up a default column on the top level line. */

	if (!output_text_line_add_column(0, OUTPUT_TEXT_LINE_FULL_WIDTH)) {
		output_text_line_close();
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

	if (!output_text_write_object(object, true, 0)) {
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
 * \param root		True if the object is at the root of a file.
 * \param level		The level to write the section at, starting from 0.
 * \return		True if successful; False on error.
 */

static bool output_text_write_object(struct manual_data *object, bool root, int level)
{
	struct manual_data	*block;
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

	resources = modes_find_resources(object->chapter.resources, MODES_TYPE_TEXT);

	/* Check that the nesting depth is OK and sort out the indents. */

	if (level > OUTPUT_TEXT_MAX_SECTION_LEVEL) {
		msg_report(MSG_TOO_DEEP, level);
		return false;
	}

	/* If this isn't the root level, indent everything now, before
	 * the object heading is written.
	 */

	if (!output_text_line_pop())
		return false;

	if (!output_text_line_push(level * OUTPUT_TEXT_BLOCK_INDENT))
		return false;

	if (!output_text_line_add_column(0, OUTPUT_TEXT_LINE_FULL_WIDTH))
			return false;

	/* Write out the object heading.
	 *
	 * Don't do this for a manual, as the file will already have a heading
	 * above it with the title. For sub-files, we do, as the chapter/section
	 * heading won't be the same.
	 */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_MANUAL && object->title != NULL) {
		if (!output_text_line_write_newline())
			return false;

		if (!output_text_write_heading(object, 0))
			return false;
	}

	/* If this is the root level, indent everything now, after the
	 * object heading has been written.
	 */

	if (level == 0) {
		if (!output_text_line_pop())
			return false;

		if (!output_text_line_push(level * OUTPUT_TEXT_BLOCK_INDENT))
			return false;

		if (!output_text_line_add_column(0, OUTPUT_TEXT_LINE_FULL_WIDTH))
			return false;
	}

	/* If this is a separate file, queue it for writing later. Otherwise,
	 * write the objects which fall within it.
	 */

	if (resources != NULL && !root && (resources->filename != NULL || resources->folder != NULL)) {
		if (object->chapter.resources->summary != NULL &&
				!output_text_write_paragraph(object->chapter.resources->summary, 0, true))
			return false;

		if (!output_text_line_write_newline() || !output_text_write_reference(object))
			return false;

		manual_queue_add_node(object);

		return true;
	} else {
		block = object->first_child;

		while (block != NULL) {
			switch (block->type) {
			case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
			case MANUAL_DATA_OBJECT_TYPE_INDEX:
			case MANUAL_DATA_OBJECT_TYPE_SECTION:
				if (!output_text_write_object(block, false, (block->type == MANUAL_DATA_OBJECT_TYPE_SECTION) ? level + 1 : level))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_CONTENTS:
				if (object->type == MANUAL_DATA_OBJECT_TYPE_MANUAL) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				/* The chapter list is treated like a section, so we always bump the level. */

				if (!output_text_write_chapter_list(block, level + 1))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_text_write_paragraph(block, 0, true))
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

				if (!output_text_write_list(block, 0, 0))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_TABLE:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_text_write_table(block, 0))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_text_write_code_block(block, 0))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_text_write_footnote(block, 0))
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
 * \param column		The column to write the heading into.
 * \return			True if successful; False on error.
 */

static bool output_text_write_heading(struct manual_data *node, int column)
{
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

	if (!output_text_line_reset())
		return false;

	if (!output_text_write_title(column, node, false, true))
		return false;

	if (!output_text_line_write(false, true))
		return false;

	return true;
}

/**
 * Write a chapter list. The list will be for the chain of objects at the
 * list object's parent level (so if it appears in a chapter, the list
 * will be for the whole manual).
 *
 * Note that this means that we will list the section (or chapter) in
 * which we appear, assuming that it isn't an index and has a title.
 *
 * \param *object	The chapter list object to be written.
 * \param level		The level to write the section at, starting from 0.
*/

static bool output_text_write_chapter_list(struct manual_data *object, int level)
{
	struct manual_data *entry = NULL;
	bool first = true;

	/* The parent object is in the chain to be listed, so we need to
	 * go up again to its parent and then down to the first child in
	 * order to get the whole list.
	 */

	if (object == NULL || object->parent == NULL || object->parent->parent == NULL)
		return false;

	/* The list is treated as a pseudo-section, so we do a section indent
	 * here to make it line up with any siblings.
	 */

	if (!output_text_line_push(level * OUTPUT_TEXT_BLOCK_INDENT))
		return false;

	if (!output_text_line_add_column(0, OUTPUT_TEXT_LINE_FULL_WIDTH))
		return false;

	/* Output the list. */

	entry = object->parent->parent->first_child;

	while (entry != NULL) {
		switch (entry->type) {
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (entry->title != NULL) {
				if (first == true) {
					if (!output_text_line_write_newline())
						return false;

					first = false;
				}

				if (!output_text_line_reset())
					return false;

				if (!output_text_write_title(0, entry, false, true))
					return false;

				if (!output_text_line_write(false, false))
					return false;
			}
			break;

		default:
			break;
		}

		entry = entry->next;
	}

	/* Pop the line indent, and we're done. */

	if (!output_text_line_pop())
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
 * \param column		The column to write the object into.
 * \param level			The list nesting level, when writing lists.
 * \return			True if successful; False on error.
 */

static bool output_text_write_block_collection_object(struct manual_data *object, int column, int level)
{
	struct manual_data *block;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Confirm that this is a suitable object. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_LIST_ITEM:
	case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
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
		/* The line for the first block should come pre-configured from
		 * the caller, with content set up. Otherwise, reset the line
		 * and put a newline out to separate the blocks in the collection.
		 */

		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			if (!output_text_write_paragraph(block, column, true))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
		case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
			if (!output_text_write_list(block, column, level + 1))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_TABLE:
			if (!output_text_write_table(block, column))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
			if (!output_text_write_code_block(block, column))
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
 * Write a footnote to the output.
 *
 * \param *object		The object to be written.
 * \param column		The column to align the object with.
 * \return			True if successful; False on failure.
 */

static bool output_text_write_footnote(struct manual_data *object, int column)
{
	char *number = NULL;

	if (object == NULL)
		return false;

	/* Confirm that this is an index, chapter or section. */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_FOOTNOTE) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_FOOTNOTE),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* If the current output line has content, we can't add to it. */

	if (output_text_line_has_content()) {
		msg_report(MSG_TEXT_LINE_NOT_EMPTY, manual_data_find_object_name(object->type));
		return false;
	}

	/* Write a heading above the block. */

	if (!output_text_line_write_newline())
		return false;

	if (!output_text_line_reset())
		return false;

	number = manual_data_get_node_number(object, true);
	if (number == NULL)
		return false;

	if (!output_text_line_add_text(column, number)) {
		free(number);
		return false;
	}

	free(number);

	if (!output_text_line_write(false, false))
		return false;

	/* Create an indented line for output. */

	if (!output_text_line_push_to_column(column, OUTPUT_TEXT_BLOCK_INDENT))
		return false;

	if (!output_text_line_add_column(0, OUTPUT_TEXT_LINE_FULL_WIDTH))
		return false;

	if (!output_text_line_reset())
		return false;

	/* Output the block. */

	if (!output_text_write_block_collection_object(object, 0, 0))
		return false;

	if (!output_text_line_pop())
		return false;

	return true;
}

/**
 * Write the contents of a list to the output.
 *
 * \param *object		The object to process.
 * \param column		The column to align the object with.
 * \param level			The list nesting level.
 * \return			True if successful; False on error.
 */

static bool output_text_write_list(struct manual_data *object, int column, int level)
{
	struct manual_data *item;
	struct list_numbers *numbers = NULL;
	int entries = 0;

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

	/* If the current output line has content, we can't add to it. */

	if (output_text_line_has_content()) {
		msg_report(MSG_TEXT_LINE_NOT_EMPTY, manual_data_find_object_name(object->type));
		return false;
	}

	/* Set the list numbers or bullets up. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
		item = object->first_child;

		while (item != NULL) {
			entries++;
			item = item->next;
		}

		numbers = list_numbers_create_ordered(entries, level);
		break;

	case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
		numbers = list_numbers_create_unordered(output_text_unordered_list_bullets, level);
		break;

	default:
		break;
	}

	if (numbers == NULL) {
		msg_report(MSG_BAD_LIST_NUMBERS);
		return false;
	}

	/* Output the list. */

	if (!output_text_line_push_to_column(column, OUTPUT_TEXT_NO_INDENT)) {
		list_numbers_destroy(numbers);
		return false;
	}

	if (!output_text_line_add_column(0, list_numbers_get_max_length(numbers))) {
		list_numbers_destroy(numbers);
		return false;
	}

	if (!output_text_line_add_column(1, OUTPUT_TEXT_LINE_FULL_WIDTH)) {
		list_numbers_destroy(numbers);
		return false;
	}

	/* If the list isn't nested in a list item, output a blank line
	 * above it.
	 */

//	if (object->parent != NULL &&
//			object->parent->type != MANUAL_DATA_OBJECT_TYPE_LIST_ITEM &&
//			!output_text_line_write_newline())
//		return false;

	if (!output_text_line_write_newline()) {
		list_numbers_destroy(numbers);
		return false;
	}

	item = object->first_child;

	while (item != NULL) {
		switch (item->type) {
		case MANUAL_DATA_OBJECT_TYPE_LIST_ITEM:
			if (!output_text_line_reset()) {
				list_numbers_destroy(numbers);
				return false;
			}

			if (!output_text_line_add_text(0, list_numbers_get_next_entry(numbers))) {
				list_numbers_destroy(numbers);
				return false;
			}

			if (!output_text_write_block_collection_object(item, 1, level)) {
				list_numbers_destroy(numbers);
				return false;
			}
			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(item->type),
					manual_data_find_object_name(object->type));
			break;
		}

		item = item->next;
	}

	list_numbers_destroy(numbers);

	if (!output_text_line_pop())
		return false;

	return true;
}

/**
 * Write the contents of a table to the output.
 *
 * \param *object		The object to process.
 * \param target_column		The column to align the object with.
 * \return			True if successful; False on error.
 */

static bool output_text_write_table(struct manual_data *object, int target_column)
{
	struct manual_data *column_set, *column, *row;
	int c;

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

	/* If the current output line has content, we can't add to it. */

	if (output_text_line_has_content()) {
		msg_report(MSG_TEXT_LINE_NOT_EMPTY, manual_data_find_object_name(object->type));
		return false;
	}

	/* Write a newline above the table. */

	if (!output_text_line_write_newline())
		return false;

	/* Create columns for the table. */

	column_set = object->chapter.columns;
	if (column_set->type != MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET),
				manual_data_find_object_name(object->type));
		return false;
	}

	if (!output_text_line_push_to_column(target_column, OUTPUT_TEXT_NO_INDENT))
		return false;

	column = column_set->first_child;

	while (column != NULL) {
		switch (column->type) {
		case MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_DEFINITION:
			if (!output_text_line_add_column((column->previous == NULL) ? 0 : 1, OUTPUT_TEXT_LINE_FULL_WIDTH))
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

	/* Write the table headings. */

	if (!output_text_line_reset())
		return false;

	column = column_set->first_child;
	c = 0;

	while (column != NULL) {
		switch (column->type) {
		case MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_DEFINITION:
			if (!output_text_write_text(c++, column->type, column))
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

	if (!output_text_line_write(false, false))
		return false;

	if (!output_text_line_write_ruleoff('-'))
		return false;

	/* Write the table rows. */

	row = object->first_child;

	while (row != NULL) {
		switch (row->type) {
		case MANUAL_DATA_OBJECT_TYPE_TABLE_ROW:
			if (!output_text_line_reset())
				return false;

			column = row->first_child;
			c = 0;

			while (column != NULL) {
				switch (column->type) {
				case MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN:
					if (!output_text_write_text(c++, column->type, column))
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
			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(row->type),
					manual_data_find_object_name(object->type));
			break;
		}

		if (!output_text_line_write(false, false))
			return false;

		row = row->next;
	}

	if (!output_text_line_pop())
		return false;

	if (object->title == NULL)
		return true;

	/* Write a newline above the title. */

	if (!output_text_line_write_newline())
		return false;

	/* Create a single column for the title. */

	if (!output_text_line_push_to_column(target_column, OUTPUT_TEXT_NO_INDENT))
		return false;

	if (!output_text_line_add_column(0, OUTPUT_TEXT_LINE_FULL_WIDTH))
		return false;

	if (!output_text_line_set_column_flags(0, OUTPUT_TEXT_LINE_COLUMN_FLAGS_CENTRE))
		return false;

	if (!output_text_line_reset())
		return false;

	/* Output the title. */

	if (!output_text_write_title(0, object, true, true))
		return false;

	if (!output_text_line_write(true, false))
		return false;

	if (!output_text_line_pop())
		return false;

	return true;
}

/**
 * Write a code block to the output.
 *
 * \param *object		The object to be written.
 * \param column		The column to align the object with.
 * \return			True if successful; False on failure.
 */

static bool output_text_write_code_block(struct manual_data *object, int column)
{
	if (object == NULL)
		return false;

	/* Confirm that this is an index, chapter or section. */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* If the current output line has content, we can't add to it. */

	if (output_text_line_has_content()) {
		msg_report(MSG_TEXT_LINE_NOT_EMPTY, manual_data_find_object_name(object->type));
		return false;
	}

	/* Write a newline above the block. */

	if (!output_text_line_write_newline())
		return false;

	/* Create a paragraph for output. */

	if (!output_text_line_push_to_column(column, OUTPUT_TEXT_BLOCK_INDENT))
		return false;

	if (!output_text_line_add_column(0, OUTPUT_TEXT_LINE_FULL_WIDTH))
		return false;

	if (!output_text_line_reset())
		return false;

	/* Output the block. */

	if (!output_text_write_text(0, object->type, object))
		return false;

	if (!output_text_line_write(true, false))
		return false;

	if (object->title == NULL) {
		if (!output_text_line_pop())
			return false;

		return true;
	}

	/* Write a newline above the title. */

	if (!output_text_line_write_newline())
		return false;

	/* Centre the title. */

	if (!output_text_line_set_column_flags(0, OUTPUT_TEXT_LINE_COLUMN_FLAGS_CENTRE))
		return false;

	if (!output_text_line_reset())
		return false;

	/* Output the title. */

	if (!output_text_write_title(0, object, true, true))
		return false;

	if (!output_text_line_write(true, false))
		return false;

	if (!output_text_line_pop())
		return false;

	return true;
}

/**
 * Write a paragraph block to the output.
 *
 * \param *object		The object to be written.
 * \param column		The column to write the object into.
 * \param last_item		Should the line be written to the output when done?
 * \return			True if successful; False on failure.
 */

static bool output_text_write_paragraph(struct manual_data *object, int column, bool last_item)
{
	if (object == NULL)
		return false;

	/* Confirm that this is an index, chapter or section. */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_PARAGRAPH && object->type != MANUAL_DATA_OBJECT_TYPE_SUMMARY) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_PARAGRAPH),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* If the current output line is clear, reset it and output the
	 * pre-paragraph line space. Otherwise, we assume that we're writing
	 * to a column that's part of something else which has already been
	 * partly set up.
	 */

	if (!output_text_line_has_content()) {
		if (!output_text_line_reset())
			return false;

		if (!output_text_line_write_newline())
			return false;
	}

	/* Output the paragraph. */

	if (!output_text_write_text(column, object->type, object))
		return false;

	if (last_item && !output_text_line_write(false, false))
		return false;

	return true;
}


/**
 * Write an internal reference (a link to another page) to the output.
 *
 * \param *target		The node to be the target of the link.
 * \return			True if successful; False on failure.
 */

static bool output_text_write_reference(struct manual_data *target)
{
	struct filename *filename = NULL;
	char *link = NULL;

	if (target == NULL)
		return false;

	filename = manual_data_get_node_filename(target, output_text_root_filename, MODES_TYPE_TEXT);
	if (filename == NULL)
		return false;

	link = filename_convert(filename, FILENAME_PLATFORM_RISCOS, 0);
	filename_destroy(filename);

	if (link == NULL)
		return false;

	if (!output_text_line_reset()) {
		free(link);
		return false;
	}

	if (!output_text_line_add_text(0, ">>> ")) {
		free(link);
		return false;
	}

	if (!output_text_line_add_text(0, link)) {
		free(link);
		return false;
	}

	free(link);

	if (!output_text_line_write(false, false))
		return false;

	return true;
}


/**
 * Write a block of text to a column in the current output line.
 *
 * \param column		The column in the line to write to.
 * \param type			The type of block which is expected.
 * \param *text			The block of text to be written.
 * \return			True if successful; False on error.
 */

static bool output_text_write_text(int column, enum manual_data_object_type type, struct manual_data *text)
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
			output_text_line_add_text(column, "/");
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, chunk);
			output_text_line_add_text(column, "/");
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			output_text_line_add_text(column, "*");
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, chunk);
			output_text_line_add_text(column, "*");
			break;
		case MANUAL_DATA_OBJECT_TYPE_CITATION:
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_CITATION, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_CODE:
			output_text_line_add_text(column, "\"");
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_CODE, chunk);
			output_text_line_add_text(column, "\"");
			break;
		case MANUAL_DATA_OBJECT_TYPE_USER_ENTRY:
			output_text_line_add_text(column, "\"");
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_USER_ENTRY, chunk);
			output_text_line_add_text(column, "\"");
			break;
		case MANUAL_DATA_OBJECT_TYPE_FILENAME:
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_FILENAME, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ICON:
			output_text_line_add_text(column, "'");
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_ICON, chunk);
			output_text_line_add_text(column, "'");
			break;
		case MANUAL_DATA_OBJECT_TYPE_KEY:
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_KEY, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_LINK:
			output_text_write_inline_link(column, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_MOUSE:
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_MOUSE, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_REFERENCE:
			output_text_write_inline_reference(column, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_VARIABLE:
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_VARIABLE, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_WINDOW:
			output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_WINDOW, chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_LINE_BREAK:
			output_text_line_add_text(column, "\n");
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			output_text_line_add_text(column, chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			output_text_line_add_text(column, (char *) output_text_convert_entity(chunk->chunk.entity));
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
 * Write an inline link to a column in the current output line.
 *
 * \param column		The column in the line to write to.
 * \param *link			The link to be written.
 * \return			True if successful; False on error.
 */

static bool output_text_write_inline_link(int column, struct manual_data *link)
{
	if (link == NULL)
		return false;

	/* Confirm that this is a link. */

	if (link->type != MANUAL_DATA_OBJECT_TYPE_LINK) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_LINK),
				manual_data_find_object_name(link->type));
		return false;
	}

	/* Write the link text. */

	if (link->first_child != NULL && !output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_LINK, link))
		return false;

	/* If there was link text, and flatten was applied, don't output the link itself. */

	if (link->first_child != NULL && (link->chunk.flags & MANUAL_DATA_OBJECT_FLAGS_LINK_FLATTEN))
		return true;

	/* Write the link information. */

	if (link->first_child != NULL && !output_text_line_add_text(column, " ["))
		return false;

	if (link->chunk.link != NULL && !output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_SINGLE_LEVEL_ATTRIBUTE, link->chunk.link))
		return false;

	if (link->first_child != NULL && !output_text_line_add_text(column, "]"))
		return false;

	return true;
}


/**
 * Write an inline reference to a column in the current output line.
 *
 * \param column		The column in the line to write to.
 * \param *reference		The reference to be written.
 * \return			True if successful; False on error.
 */

static bool output_text_write_inline_reference(int column, struct manual_data *reference)
{
	struct manual_data *target = NULL;
	struct filename *filename = NULL;
	char *link = NULL, *number = NULL;
	bool include_title = false;

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

	/* Write the reference text. */

	if (reference->first_child != NULL && !output_text_write_text(column, MANUAL_DATA_OBJECT_TYPE_REFERENCE, reference))
		return false;

	if (target == NULL)
		return false;

	/* Write the reference information. */

	switch (target->type) {
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		include_title = true;

	case MANUAL_DATA_OBJECT_TYPE_TABLE:
	case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
		if (reference->first_child != NULL && !output_text_line_add_text(column, " (see "))
			return false;

		if (!output_text_write_title(column, target, true, include_title))
			return false;

		if (manual_data_nodes_share_file(reference, target, MODES_TYPE_TEXT) == false) {
			if (!output_text_line_add_text(column, " in "))
				return false;

			filename = manual_data_get_node_filename(target, output_text_root_filename, MODES_TYPE_TEXT);
			if (filename == NULL)
				return false;

			link = filename_convert(filename, FILENAME_PLATFORM_RISCOS, 0);
			filename_destroy(filename);

			if (link == NULL)
				return false;

			if (!output_text_line_add_text(column, link)) {
				free(link);
				return false;
			}

			free(link);
		}

		if (reference->first_child != NULL && !output_text_line_add_text(column, ")"))
			return false;
		break;

	case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
		if (!output_text_line_add_text(column, "["))
			return false;

		number = manual_data_get_node_number(target, false);
		if (number == NULL)
			return false;

		if (!output_text_line_add_text(column, number)) {
			free(number);
			return false;
		}

		free(number);

		if (!output_text_line_add_text(column, "]"))
			return false;
		break;

	default:
		break;
	}

	return true;
}

/**
 * Write the title of a node to a column in the current output line.
 *
 * \param column		The column in the line to write to.
 * \param *node			The node whose title is to be written.
 * \param include_name		Should we prefix the number with the object name?
 * \param include_title		Should we include the object title?
 * \return			True if successful; False on error.
 */

static bool output_text_write_title(int column, struct manual_data *node, bool include_name, bool include_title)
{
	char *number;

	if (node == NULL || node->title == NULL)
		return false;

	number = manual_data_get_node_number(node, include_name);

	if (number != NULL) {
		if (!output_text_line_add_text(0, number)) {
			free(number);
			return false;
		}

		free(number);

		if (!output_text_line_add_text(0, " "))
			return false;
	}

	return output_text_write_text(0, MANUAL_DATA_OBJECT_TYPE_TITLE, node->title);
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
	case MANUAL_ENTITY_LT:
		return "<";
	case MANUAL_ENTITY_GT:
		return ">";
	case MANUAL_ENTITY_LE:
		return "<=";
	case MANUAL_ENTITY_GE:
		return ">=";
	case MANUAL_ENTITY_MINUS:
		return "-";
	case MANUAL_ENTITY_NDASH:
		return ENCODING_UTF8_NBHY ENCODING_UTF8_NBHY;
	case MANUAL_ENTITY_MDASH:
		return ENCODING_UTF8_NBHY ENCODING_UTF8_NBHY ENCODING_UTF8_NBHY;
	case MANUAL_ENTITY_TIMES:
		return "x";
	case MANUAL_ENTITY_SMILE:
		return ":-)";
	case MANUAL_ENTITY_SAD:
		return ":-(";
	default:
		msg_report(MSG_ENTITY_NO_MAP, manual_entity_find_name(entity));
		return "?";
	}
}
