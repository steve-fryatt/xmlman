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
 * \file output_strong.c
 *
 * StrongHelp Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "output_strong.h"

#include "encoding.h"
#include "filename.h"
#include "manual_data.h"
#include "manual_queue.h"
#include "msg.h"
#include "output_strong_file.h"

/* Static constants. */

/**
 * The base level for section nesting.
 */

#define OUTPUT_STRONG_BASE_LEVEL 1

/**
 * The maximum depth that sections can be nested.
 */

#define OUTPUT_STRONG_MAX_NEST_DEPTH 6

/**
 * The root filename used when writing into an empty folder.
 */

#define OUTPUT_STRONG_ROOT_FILENAME "!Root"

/**
 * The filetype used for page files.
 */

#define OUTPUT_STRONG_PAGE_FILETYPE 0xfff

/* Global Variables. */

/**
 * The root filename used when writing into an empty folder.
 */

static struct filename *output_strong_root_filename;

/* Static Function Prototypes. */

static bool output_strong_write_manual(struct manual_data *manual);
static bool output_strong_write_file(struct manual_data *object);
static bool output_strong_write_object(struct manual_data *object, int level);
static bool output_strong_write_head(struct manual_data *manual);
static bool output_strong_write_foot(struct manual_data *manual);
static bool output_strong_write_heading(struct manual_data *node, int level);
static bool output_strong_write_paragraph(struct manual_data *object);
static bool output_strong_write_text(enum manual_data_object_type type, struct manual_data *text);
static const char *output_strong_convert_entity(enum manual_entity_type entity);

/**
 * Output a manual in StrongHelp form.
 *
 * \param *document	The manual to be output.
 * \param *filename	The filename to use to write to.
 * \param encoding	The encoding to use for output.
 * \param line_end	The line ending to use for output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_strong(struct manual *document, struct filename *filename, enum encoding_target encoding, enum encoding_line_end line_end)
{
	bool result;

	if (document == NULL || document->manual == NULL)
		return false;

	/* Output encoding defaults to Acorn Latin1. */

	encoding_select_table((encoding != ENCODING_TARGET_NONE) ? encoding : ENCODING_TARGET_ACORN_LATIN1);

	/* Output line endings default to LF. */

	encoding_select_line_end((line_end != ENCODING_LINE_END_NONE) ? line_end : ENCODING_LINE_END_LF);

	/* Find and open the output file. */

	if (!output_strong_file_open(filename))
		return false;

	/* Write the manual content. */

	output_strong_root_filename = filename_make(OUTPUT_STRONG_ROOT_FILENAME, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LINUX);

	result = output_strong_write_manual(document->manual);

	filename_destroy(output_strong_root_filename);

	output_strong_file_close();

	return result;
}

/**
 * Write a StrongHelp manual body block out.
 *
 * \param *manual		The manual to process.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_manual(struct manual_data *manual)
{
	struct manual_data *object;

	if (manual == NULL)
		return false;

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
		printf("De-queued node 0x%x to process...\n", object);
		if (object == NULL)
			continue;

		if (!output_strong_write_file(object))
			return false;
	} while (object != NULL);

	return true;
}


/**
 * Write a node and its descendents as a self-contained file.
 *
 * \param *object	The object to process.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_strong_write_file(struct manual_data *object)
{
	struct filename *filename = NULL;

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

	/* Find the file name and open the file. */

	filename = manual_data_get_node_filename(object, output_strong_root_filename, FILENAME_PLATFORM_RISCOS, MODES_TYPE_STRONGHELP);
	if (filename == NULL)
		return false;

	if (!output_strong_file_sub_open(filename, OUTPUT_STRONG_PAGE_FILETYPE))
		return false;

	/* Write the file header. */

	if (!output_strong_write_head(object)) {
		output_strong_file_sub_close();
		return false;
	}

	if (!output_strong_file_write_newline()) {
		output_strong_file_sub_close();
		return false;
	}

	/* Output the object. */

	if (!output_strong_write_object(object, OUTPUT_STRONG_BASE_LEVEL)) {
		output_strong_file_sub_close();
		filename_destroy(filename);
		return false;
	}

	/* Output the file footer. */

	if (!output_strong_write_foot(object)) {
		output_strong_file_sub_close();
		return false;
	}

	if (!output_strong_file_sub_close())
		return false;

	return true;
}


/**
 * Process the contents of an index, chapter or section block and write it out.
 *
 * \param *object		The object to process.
 * \param level			The level to write the section at.
 * \param *folder		The image file folder being written within.
 * \param in_line		True if the section is being written inline; otherwise False.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_object(struct manual_data *object, int level)
{
	struct manual_data	*block;
	struct filename *foldername = NULL, *filename = NULL;
	bool self_contained = false;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Check that the nesting depth is OK. */

	if (level > OUTPUT_STRONG_MAX_NEST_DEPTH) {
		msg_report(MSG_TOO_DEEP, level);
		return false;
	}

	/* Write out the object heading. */

	if (object->title != NULL) {
		if (!output_strong_file_write_newline())
			return false;

		if (!output_strong_write_heading(object, level))
			return false;
	}

	/* Output the blocks within the object. */

	block = object->first_child;

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_INDEX:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (!output_strong_write_object(block, level + 1))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
				msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(block->type));
				break;
			}

			if (!output_strong_write_paragraph(block))
				return false;
			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(block->type));
			break;
		}

		block = block->next;
	}

	return true;
}










#if 0
/**
 * Process the contents of an index, chapter or section block and write it out.
 *
 * \param *object		The object to process.
 * \param level			The level to write the section at.
 * \param *folder		The image file folder being written within.
 * \param in_line		True if the section is being written inline; otherwise False.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_object(struct manual_data *object, int level, struct filename *folder, bool in_line)
{
	struct manual_data	*block;
	struct filename *foldername = NULL, *filename = NULL;
	bool self_contained = false;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Confirm that this is an index, chapter or section. */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_CHAPTER && object->type != MANUAL_DATA_OBJECT_TYPE_INDEX && object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_SECTION),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Check that the nesting depth is OK. */

	if (level > OUTPUT_STRONG_MAX_NEST_DEPTH) {
		msg_report(MSG_TOO_DEEP, level);
		return false;
	}

	/* Check for an inlined or self-contained section. */

	self_contained = output_strong_find_folders(folder, object->chapter.resources, &foldername, &filename);

	if ((self_contained && in_line) || (!self_contained && !in_line)) {
		if (in_line) {
			if (!output_strong_file_write_newline() || !output_strong_write_heading(object, level)) {
				filename_destroy(foldername);
				filename_destroy(filename);
				return false;
			}

			if (object->chapter.resources != NULL && object->chapter.resources->summary != NULL &&
					!output_strong_write_paragraph(object->chapter.resources->summary)) {
				filename_destroy(foldername);
				filename_destroy(filename);
				return false;
			}

			output_strong_file_write_plain("\nThis is a link to an external file...\n");
		}

		filename_destroy(foldername);
		filename_destroy(filename);
		return true;
	}

	if (self_contained && !output_strong_file_sub_open(filename, OUTPUT_STRONG_PAGE_FILETYPE)) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (self_contained && !output_strong_write_head(object)) {
		output_strong_file_sub_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (!output_strong_file_write_newline()) {
		if (self_contained)
			output_strong_file_sub_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Write out the section heading. */

	if (object->title != NULL) {
		if (!self_contained && !output_strong_file_write_newline()) {
			filename_destroy(foldername);
			filename_destroy(filename);
			return false;
		}

		if (!output_strong_write_heading(object, level)) {
			if (self_contained)
				output_strong_file_sub_close();
			filename_destroy(foldername);
			filename_destroy(filename);
			return false;
		}
	}

	/* Output the blocks within the object. */

	block = object->first_child;

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (!output_strong_write_object(block, level + 1, foldername, true)) {
				if (self_contained)
					output_strong_file_sub_close();
				filename_destroy(foldername);
				filename_destroy(filename);
				return false;
			}
			break;

		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
				msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(block->type));
				break;
			}

			if (!output_strong_write_paragraph(block)) {
				if (self_contained)
					output_strong_file_sub_close();
				filename_destroy(foldername);
				filename_destroy(filename);
				return false;
			}
			break;

		default:
			msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(block->type));
			break;
		}

		block = block->next;
	}

	if (self_contained && !output_strong_write_foot(object)) {
		output_strong_file_sub_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (self_contained && !output_strong_file_sub_close()) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Output the stand-alone sections in their own files. */

	block = object->first_child;

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (!output_strong_write_object(block, OUTPUT_STRONG_BASE_LEVEL, foldername, false)) {
				filename_destroy(foldername);
				filename_destroy(filename);
				return false;
			}
			break;

		default:
			break;
		}

		block = block->next;
	}

	/* Free the filenames. */

	filename_destroy(foldername);
	filename_destroy(filename);

	return true;
}
#endif

/**
 * Write a StrongHelp file head block out.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_strong_write_head(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

//	if (!output_strong_file_write_plain("<head>") || !output_strong_file_write_newline())
//		return false;

	if (!output_strong_write_heading(manual, 0))
		return false;

//	if (!output_strong_file_write_plain("</head>") || !output_strong_file_write_newline())
//		return false;

	return true;
}

/**
 * Write a StrongHelp file foot block out.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_strong_write_foot(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	return true;
}

/**
 * Write a node title.
 *
 * \param *node			The node for which to write the title.
 * \param level			The level to write the title at.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_heading(struct manual_data *node, int level)
{
	char *number;

	if (node == NULL || node->title == NULL)
		return true;

	if (level < 0 || level > 6)
		return false;

	number = manual_data_get_node_number(node);

	if ((level > 0) && !output_strong_file_write_plain("{fh%d:", level)) {
		free(number);
		return false;
	}

	if (number != NULL) {
		if (!output_strong_file_write_text(number)) {
			free(number);
			return false;
		}

		if (!output_strong_file_write_text(" ")) {
			free(number);
			return false;
		}

		free(number);
	}

	if (!output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, node->title))
		return false;

	if ((level > 0) && !output_strong_file_write_plain("}"))
		return false;

	if (!output_strong_file_write_newline())
		return false;

	return true;
}

/**
 * Write a paragraph block to the output.
 *
 * \param *object		The object to be written.
 * \return			True if successful; False on failure.
 */

static bool output_strong_write_paragraph(struct manual_data *object)
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

	if (!output_strong_file_write_newline())
		return false;

	if (!output_strong_write_text(object->type, object))
				return false;

	if ((object->next != NULL) && !output_strong_file_write_newline())
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

static bool output_strong_write_text(enum manual_data_object_type type, struct manual_data *text)
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
			output_strong_file_write_plain("{f/:");
			output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, chunk);
			output_strong_file_write_plain("}");
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			output_strong_file_write_plain("<strong>");
			output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, chunk);
			output_strong_file_write_plain("</strong>");
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			output_strong_file_write_text(chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			output_strong_file_write_text(output_strong_convert_entity(chunk->chunk.entity));
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
 * Convert an entity into an StrongHelp representation.
 *
 * \param entity		The entity to convert.
 * \return			Pointer to the HTML representation.
 */

static const char *output_strong_convert_entity(enum manual_entity_type entity)
{
	switch (entity) {
	case MANUAL_ENTITY_NBSP:
		return ENCODING_UTF8_NBSP;
	case MANUAL_ENTITY_AMP:
		return "&";
	case MANUAL_ENTITY_LSQUO:
		return ENCODING_UTF8_LSQUO;
	case MANUAL_ENTITY_RSQUO:
		return ENCODING_UTF8_RSQUO;
	case MANUAL_ENTITY_QUOT:
		return "\"";
	case MANUAL_ENTITY_LDQUO:
		return ENCODING_UTF8_LDQUO;
	case MANUAL_ENTITY_RDQUO:
		return ENCODING_UTF8_RDQUO;
	case MANUAL_ENTITY_MINUS:
		return ENCODING_UTF8_MINUS;
	case MANUAL_ENTITY_NDASH:
		return ENCODING_UTF8_NDASH;
	case MANUAL_ENTITY_MDASH:
		return ENCODING_UTF8_MDASH;
	case MANUAL_ENTITY_TIMES:
		return ENCODING_UTF8_TIMES;
	default:
		msg_report(MSG_ENTITY_NO_MAP, manual_entity_find_name(entity));
		return "?";
	}
}
