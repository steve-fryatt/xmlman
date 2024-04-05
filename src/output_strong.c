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
#include "modes.h"
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
static bool output_strong_write_object(struct manual_data *object, int level, bool include_id);
static bool output_strong_write_head(struct manual_data *manual);
static bool output_strong_write_foot(struct manual_data *manual);
static bool output_strong_write_heading(struct manual_data *node, int level, bool root);
static bool output_strong_write_block_collection_object(struct manual_data *object);
static bool output_strong_write_footnote(struct manual_data *object);
static bool output_strong_write_code_block(struct manual_data *object);
static bool output_strong_write_paragraph(struct manual_data *object);
static bool output_strong_write_reference(struct manual_data *target, char *text);
static bool output_strong_write_text(enum manual_data_object_type type, struct manual_data *text);
static bool output_strong_write_inline_link(struct manual_data *link);
static bool output_strong_write_inline_reference(struct manual_data *reference);
static bool output_strong_write_title(struct manual_data *node);
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

	msg_report(MSG_START_MODE, "StrongHelp");

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

	filename = manual_data_get_node_filename(object, output_strong_root_filename, MODES_TYPE_STRONGHELP);
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

	if (!output_strong_write_object(object, OUTPUT_STRONG_BASE_LEVEL, true)) {
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
 * \param root			True if the object is at the root of a file.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_object(struct manual_data *object, int level, bool root)
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

	resources = modes_find_resources(object->chapter.resources, MODES_TYPE_STRONGHELP);

	/* Check that the nesting depth is OK. */

	if (level > OUTPUT_STRONG_MAX_NEST_DEPTH) {
		msg_report(MSG_TOO_DEEP, level);
		return false;
	}

	/* Write out the object heading. */

	if (object->title != NULL) {
		if (!output_strong_file_write_newline())
			return false;

		if (!output_strong_write_heading(object, level, !root))
			return false;
	}

	/* If this is a separate file, queue it for writing later. Otherwise,
	 * write the objects which fall within it.
	 */

	if (resources != NULL && !root && (resources->filename != NULL || resources->folder != NULL)) {
		if (object->chapter.resources->summary != NULL &&
				!output_strong_write_paragraph(object->chapter.resources->summary))
			return false;

		if (!output_strong_file_write_newline())
			return false;

		if (!output_strong_file_write_newline())
			return false;

		if (!output_strong_write_reference(object, "This is a link to an external file..."))
			return false;

		if (!output_strong_file_write_newline())
			return false;

		if (!output_strong_file_write_newline())
			return false;

		manual_queue_add_node(object);
	} else {
		block = object->first_child;

		/* If changing this switch, note the analogous list in
		 * output_strong_write_block_collection_object() which
		 * covers similar block level objects.
		 */

		while (block != NULL) {
			switch (block->type) {
			case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
			case MANUAL_DATA_OBJECT_TYPE_INDEX:
			case MANUAL_DATA_OBJECT_TYPE_SECTION:
				if (!output_strong_write_object(block, level + 1, false))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_strong_write_paragraph(block))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_strong_write_code_block(block))
					return false;
				break;

			case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_strong_write_footnote(block))
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

	if (!output_strong_write_heading(manual, 0, false))
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
 * \param include_id		True to include the object ID, if specified.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_heading(struct manual_data *node, int level, bool include_id)
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

	if (level < 0 || level > 6)
		return false;

	/* Include a tag, if required. */

	if (include_id && node->chapter.id != NULL) {
		if (!output_strong_file_write_plain("#TAG "))
			return false;

		if (!output_strong_file_write_text(node->chapter.id))
			return false;

		if (!output_strong_file_write_newline())
			return false;
	}

	/* Write the heading. */

	if ((level > 0) && !output_strong_file_write_plain("{fh%d:", level))
		return false;

	if (!output_strong_write_title(node))
		return false;

	if ((level > 0) && !output_strong_file_write_plain("}"))
		return false;

	if (!output_strong_file_write_newline())
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

static bool output_strong_write_block_collection_object(struct manual_data *object)
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
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			if (!output_strong_write_paragraph(block))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
		case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
	//		if (!output_html_write_list(block))
	//			return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_TABLE:
	//		if (!output_html_write_table(block))
	//			return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
			if (!output_strong_write_code_block(block))
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
 * Process the contents of a footnote and write it out.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_footnote(struct manual_data *object)
{
	char *number = NULL;

	if (object == NULL)
		return false;

	/* Confirm that this is a code block. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_FOOTNOTE),
				manual_data_find_object_name(object->type));
		return false;
	}

	/* Output the footnote block. */

	if (!output_strong_file_write_newline())
		return false;

	/* Include a tag, if required. */

	if (object->chapter.id != NULL) {
		if (!output_strong_file_write_plain("#TAG "))
			return false;

		if (!output_strong_file_write_text(object->chapter.id))
			return false;

		if (!output_strong_file_write_newline())
			return false;
	}

	/* Output the note heading. */

	if (!output_strong_file_write_plain("{f*:"))
		return false;

	if (!output_strong_file_write_text("Note "))
		return false;

	number = manual_data_get_node_number(object);
	if (number == NULL)
		return false;

	if (!output_strong_file_write_text(number)) {
		free(number);
		return false;
	}

	free(number);

	if (!output_strong_file_write_plain("}"))
		return false;

	/* Output the note body. */

	if (!output_strong_write_block_collection_object(object))
		return false;

	return true;
}

/**
 * Process a code block to the output.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_code_block(struct manual_data *object)
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

	if (!output_strong_file_write_newline())
		return false;

	if (!output_strong_file_write_plain("#Indent +2"))
		return false;

	if (!output_strong_file_write_newline())
		return false;

	if (!output_strong_file_write_plain("#fCode"))
		return false;

	if (!output_strong_file_write_newline())
		return false;

	if (!output_strong_write_text(object->type, object))
		return false;

	if (!output_strong_file_write_newline())
		return false;

	if (!output_strong_file_write_plain("#f"))
		return false;

	if (!output_strong_file_write_newline())
		return false;

	if (object->title != NULL) {
		if (!output_strong_file_write_newline())
			return false;

		if (!output_strong_write_title(object))
			return false;

		if (!output_strong_file_write_newline())
			return false;
	}

	if (!output_strong_file_write_plain("#Indent"))
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
 * Write an internal reference (a link to another page) to the output.
 * 
 * \param *target		The node to be the target of the link.
 * \param *text			Text to use for the link, or NULL for none.
 * \return			True if successful; False on failure.
 */

static bool output_strong_write_reference(struct manual_data *target, char *text)
{
	struct filename *filename = NULL;
	char *link = NULL;

	if (target == NULL)
		return false;

	filename = manual_data_get_node_filename(target, output_strong_root_filename, MODES_TYPE_STRONGHELP);
	if (filename == NULL)
		return false;

	link = filename_convert(filename, FILENAME_PLATFORM_STRONGHELP, 0);
	filename_destroy(filename);

	if (link == NULL)
		return false;

	if (!output_strong_file_write_plain("<")) {
		free(link);
		return false;
	}

	if (text != NULL && !output_strong_file_write_text(text)) {
		free(link);
		return false;
	}

	if (text != NULL && !output_strong_file_write_plain("=>")) {
		free(link);
		return false;
	}

	if (!output_strong_file_write_text(link)) {
		free(link);
		return false;
	}

	free(link);

	if (!output_strong_file_write_plain(">"))
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
			output_strong_file_write_plain("{f*:");
			output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, chunk);
			output_strong_file_write_plain("}");
			break;
		case MANUAL_DATA_OBJECT_TYPE_CITATION:
			output_strong_file_write_plain("{f/:");
			output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_CITATION, chunk);
			output_strong_file_write_plain("}");
			break;
		case MANUAL_DATA_OBJECT_TYPE_FILENAME:
			output_strong_file_write_plain("{f*:");
			output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_FILENAME, chunk);
			output_strong_file_write_plain("}");
			break;
		case MANUAL_DATA_OBJECT_TYPE_LINK:
			output_strong_write_inline_link(chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_REFERENCE:
			output_strong_write_inline_reference(chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			output_strong_file_write_text(chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			output_strong_file_write_text((char *) output_strong_convert_entity(chunk->chunk.entity));
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
 * Write an inline link out to the file.
 *
 * \param *link			The link to be written.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_inline_link(struct manual_data *link)
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

	if (link->chunk.link != NULL && !output_strong_file_write_plain("<"))
		return false;

	/* Output the link body. */

	if (link->first_child != NULL) {
		if (!output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_LINK, link))
			return false;
	} else if (link->chunk.link != NULL) {
		if (!output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_SINGLE_LEVEL_ATTRIBUTE, link->chunk.link))
			return false;
	}
	
	/* Output the closing link tag. */

	if (link->chunk.link != NULL && !output_strong_file_write_plain("=>#URL %s>", link->chunk.link))
		return false;

	return true;
}

/**
 * Write an inline reference to the output file.
 *
 * \param *reference		The reference to be written.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_inline_reference(struct manual_data *reference)
{
	struct manual_data *target = NULL;
	struct filename *filename = NULL;
	char *link = NULL, *number = NULL;

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

	/* If the target is a footnote, write the body text and
	 * opening square bracket out now. */

	if (target != NULL && target->type == MANUAL_DATA_OBJECT_TYPE_FOOTNOTE) {
		if (reference->first_child != NULL && !output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_REFERENCE, reference))
			return false;

		if (!output_strong_file_write_plain("["))
			return false;
	}

	/* Output the opening link tag. */

	if (target != NULL && !output_strong_file_write_plain("<"))
		return false;

	/* Output the link body. */

	if (target != NULL && target->type == MANUAL_DATA_OBJECT_TYPE_FOOTNOTE) {
		number = manual_data_get_node_number(target);
		if (number == NULL)
			return false;

		if (!output_strong_file_write_text(number)) {
			free(number);
			return false;
		}

		free(number);
	} else {
		if (reference->first_child != NULL) {
			if (!output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_REFERENCE, reference))
				return false;
		} else {
			if (!output_strong_write_title(target))
				return false;
		}
	}

	/* Establish the relative link, if external. */

	if (target != NULL) {
		if (manual_data_nodes_share_file(reference, target, MODES_TYPE_STRONGHELP) == false) {
			filename = manual_data_get_node_filename(target, output_strong_root_filename, MODES_TYPE_STRONGHELP);
			if (filename == NULL)
				return false;

			link = filename_convert(filename, FILENAME_PLATFORM_STRONGHELP, 0);
			filename_destroy(filename);

			if (link == NULL)
				return false;

			if (!output_strong_file_write_plain("=>%s", link)) {
				free(link);
				return false;
			}

			free(link);

			if (target->chapter.id != NULL && !output_strong_file_write_plain("#"))
				return false;
		} else if (target->chapter.id != NULL) {
			if (!output_strong_file_write_plain("=>#TAG "))
				return false;
		}

		if (target->chapter.id != NULL && !output_strong_file_write_plain("%s", target->chapter.id))
			return false;

		/* Output the closing link tag. */

		if (!output_strong_file_write_plain(">"))
			return false;

		/* Close the square brackets if this is a footnote. */

		if (target->type == MANUAL_DATA_OBJECT_TYPE_FOOTNOTE && !output_strong_file_write_plain("]"))
			return false;
	}

	return true;
}

/**
 * Write the title of a node to the output file.
 *
 * \param *node			The node whose title is to be written.
 * \return			True if successful; False on error.
 */

static bool output_strong_write_title(struct manual_data *node)
{
	char *number;

	if (node == NULL || node->title == NULL)
		return false;

	number = manual_data_get_node_number(node);

	if (number != NULL) {
		if (!output_strong_file_write_text(number)) {
			free(number);
			return false;
		}

		free(number);

		if (!output_strong_file_write_plain(" "))
			return false;
	}

	return output_strong_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, node->title);
}

/**
 * Convert an entity into a StrongHelp representation.
 *
 * \param entity		The entity to convert.
 * \return			Pointer to the StrongHelp representation.
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
	case MANUAL_ENTITY_SMILE:
		return ":-)";
	case MANUAL_ENTITY_SAD:
		return ":-(";
	default:
		msg_report(MSG_ENTITY_NO_MAP, manual_entity_find_name(entity));
		return "?";
	}
}
