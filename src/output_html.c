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
 * The length of buffer required to build <title> and <h?> tags.
 */

#define OUTPUT_HTML_TITLE_TAG_BLOCK_LEN 6

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
static bool output_html_write_file(struct manual_data *object, struct filename *folder);
static bool output_html_write_object(struct manual_data *object, int level);
static bool output_html_write_head(struct manual_data *manual);
static bool output_html_write_foot(struct manual_data *manual);
static bool output_html_write_heading(struct manual_data *node, int level);
static bool output_html_write_paragraph(struct manual_data *object);
static bool output_html_write_text(enum manual_data_object_type type, struct manual_data *text);
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

	if (manual == NULL || folder == NULL)
		return false;
printf("Writing HTML manual...\n");
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

		if (!output_html_write_file(object, folder))
			return false;
	} while (object != NULL);

	return true;
}


/**
 * Write a node and its descendents as a self-contained file.
 *
 * \param *object		The object to process.
 * \param *folder	The folder into which to write the manual.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_file(struct manual_data *object, struct filename *folder)
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

	filename = manual_data_get_node_filename(object, output_html_root_filename, FILENAME_PLATFORM_LOCAL, MODES_TYPE_HTML);
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

	if (!output_html_file_write_newline()) {
		output_html_file_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Output the object. */

	if (!output_html_write_object(object, 1)) {
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
 * \param *folder		The folder being written within.
 * \param in_line		True if the section is being written inline; otherwise False.
 * \return			True if successful; False on error.
 */

static bool output_html_write_object(struct manual_data *object, int level)
{
	struct manual_data *block;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Check that the nesting depth is OK. */

	if (level > OUTPUT_HTML_MAX_NEST_DEPTH) {
		msg_report(MSG_TOO_DEEP, level);
		return false;
	}

	/* Write out the object heading. */

	if (object->title != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_write_heading(object, level))
			return false;
	}

	/* Write out the blocks iwithin the object. */

	block = object->first_child;

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_INDEX:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (!output_html_write_object(block, level + 1))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
				msg_report(MSG_UNEXPECTED_CHUNK, manual_data_find_object_name(block->type));
				break;
			}

			if (!output_html_write_paragraph(block))
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



/**
 * Process the contents of an index, chapter or section block and write it out.
 *
 * \param *object		The object to process.
 * \param level			The level to write the section at.
 * \param *folder		The folder being written within.
 * \param in_line		True if the section is being written inline; otherwise False.
 * \return			True if successful; False on error.
 */
#if 0
static bool output_html_write_object(struct manual_data *object, int level, struct filename *folder, bool in_line)
{
	struct manual_data *block;
	struct filename *foldername = NULL, *filename = NULL;
	bool self_contained = false;

	struct filename *nodename = NULL;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Confirm that this is an index, chapter or section. */

	if (object->type != MANUAL_DATA_OBJECT_TYPE_CHAPTER && object->type != MANUAL_DATA_OBJECT_TYPE_INDEX && object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_SECTION),
				manual_data_find_object_name(object->type));
		return false;
	}

	nodename = manual_data_get_node_filename(object, output_html_root_filename, MODES_TYPE_HTML);
	if (nodename != NULL) {
		filename_dump(nodename, "Node Filename");
		free(nodename);
	}

	/* Check that the nesting depth is OK. */

	if (level > OUTPUT_HTML_MAX_NEST_DEPTH) {
		msg_report(MSG_TOO_DEEP, level);
		return false;
	}

	/* Check for an inlined or self-contained section. */

	self_contained = output_html_find_folders(folder, object->chapter.resources, &foldername, &filename);

	if ((self_contained && in_line) || (!self_contained && !in_line)) {
		if (in_line) {
			if (!output_html_write_heading(object, level)) {
				filename_destroy(foldername);
				filename_destroy(filename);
				return false;
			}

			if (object->chapter.resources != NULL && object->chapter.resources->summary != NULL &&
					!output_html_write_paragraph(object->chapter.resources->summary)) {
				filename_destroy(foldername);
				filename_destroy(filename);
				return false;
			}

			output_html_file_write_plain("<p>This is a link to an external file...</p>\n");
		}

		filename_destroy(foldername);
		filename_destroy(filename);
		return true;
	}

	if (self_contained && !filename_mkdir(foldername, true)) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (self_contained && !output_html_file_open(filename)) {
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (self_contained && !output_html_write_head(object)) {
		output_html_file_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (!output_html_file_write_newline()) {
		if (self_contained)
			output_html_file_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	/* Write out the section heading. */

	if (object->title != NULL) {
		if (!self_contained && !output_html_file_write_newline()) {
			filename_destroy(foldername);
			filename_destroy(filename);
			return false;
		}

		if (!output_html_write_heading(object, level)) {
			if (self_contained)
				output_html_file_close();
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
			if (!output_html_write_object(block, level + 1, foldername, true)) {
				if (self_contained)
					output_html_file_close();
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

			if (!output_html_write_paragraph(block)) {
				if (self_contained)
					output_html_file_close();
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

	if (self_contained && !output_html_write_foot(object)) {
		output_html_file_close();
		filename_destroy(foldername);
		filename_destroy(filename);
		return false;
	}

	if (self_contained)
		output_html_file_close();

	/* Output the stand-alone sections in their own files. */

	block = object->first_child;

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (!output_html_write_object(block, OUTPUT_HTML_BASE_LEVEL, foldername, false)) {
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

	if (!output_html_file_write_plain("</head>") || !output_html_file_write_newline())
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
	char	*number;

	if (node == NULL || node->title == NULL)
		return true;

	if (level < 0 || level > 6)
		return false;

	number = manual_data_get_node_number(node);

	if (level == 0)
		snprintf(buffer, OUTPUT_HTML_TITLE_TAG_BLOCK_LEN, "title");
	else
		snprintf(buffer, OUTPUT_HTML_TITLE_TAG_BLOCK_LEN, "h%d", level);

	buffer[OUTPUT_HTML_TITLE_TAG_BLOCK_LEN - 1] = '\0';

	if (!output_html_file_write_plain("<%s>", buffer)) {
		free(number);
		return false;
	}

	if (number != NULL) {
		if (!output_html_file_write_text(number)) {
			free(number);
			return false;
		}

		if (!output_html_file_write_text(" ")) {
			free(number);
			return false;
		}

		free(number);
	}

	if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, node->title))
		return false;

	if (!output_html_file_write_plain("</%s>", buffer))
		return false;

	if (!output_html_file_write_newline())
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

	/* Confirm that this is an index, chapter or section. */

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
			output_html_file_write_text(output_html_convert_entity(chunk->chunk.entity));
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
		return "&minus;";
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