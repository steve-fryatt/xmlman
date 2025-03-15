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

#include "xmlman.h"
#include "output_html.h"

#include "encoding.h"
#include "filename.h"
#include "manual_data.h"
#include "manual_defines.h"
#include "manual_ids.h"
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

/**
 * The default stylesheet, which is embedded into the HTML file
 * if no external sheet is specified.
 */

static char *output_html_default_stylesheet[] = {
	"BODY { font-family: arial, helvetica, sans-serif; background-color: #FFFFFF; margin: 0; padding: 0; color: #000000; }",
	"DIV#head { margin: 0; padding: 0; border-bottom: 1px solid #000000; }",
	"DIV#head-liner { max-width: 1000px; margin: 0 auto; padding: 1em; }",
	"DIV#head DIV.title-flex { display: flex; margin: 0 0 1em; flex-direction: row; flex-wrap: nowrap; justify-content: flex-start; align-items: center; }",
	"DIV#head DIV.title-flex H1 { margin: 0; padding: 0; flex: 1 1 auto; text-align: left; vertical-align: middle; }",
	"DIV#head DIV.title.flex DIV.title-right { margin: 0.5em; padding: 0; flex: 0 0 auto; vertical-align: middle; }",
	"DIV#head DIV.date { margin-top: 0.3em; font-weight: bold; text-align: right; } ",
	"DIV#head DIV.version { font-weight: bold; text-align: right; }",
	"DIV#head DIV.strapline { margin-top: 0.5em; font-style: italic; font-weight: bold; }",
	"DIV#head DIV.credit { margin-top: 0.5em; }",
	"DIV#body { margin: 0; padding: 0 1em; }",
	"DIV#body-liner { max-width: 1000px; margin: 1em auto; }",
	"H1 { text-align: left; font-size: 2em; font-weight: bold; margin-top: 1em; margin-bottom: 1em; }",
	"H2 { text-align: left; font-size: 1.5em; font-weight: bold; margin-top: 1.5em; margin-bottom: 1em; }",
	"H3 { text-align: left; font-size: 1em; font-weight: bold; font-style: italic; margin-top: 0.5em; margin-bottom:0; }",
	"H4 { text-align: left; font-size: 1em; font-style: italic; margin-top: 0.5em; margin-bottom: 0; }",
	"H5 { text-align: left; font-size: 1em; font-style: italic; margin-top: 0.5em; margin-bottom: 0; }",
	"H6 { text-align: left; font-size: 1em; font-style: italic; margin-top: 0.5em; margin-bottom: 0; }",
	"DIV.callout { margin: 0.5em 4em; padding: 0; }",
	"DIV.callout DIV.heading { margin: 0; padding: 5px; font-weight: bold; }",
	"DIV.callout DIV.content { margin: 0; padding: 5px; }",
	"DIV.callout DIV.content :first-child { margin-top: 0; }",
	"DIV.callout DIV.content :last-child { margin-bottom: 0; }",
	"DIV.danger, DIV.error { background-color: #DFDFFF; border: 1px solid #6297e7; }",
	"DIV.danger DIV.heading, DIV.error DIV.heading { color: #ffffff; background-color: #6297e7; }",
	"DIV.attention, DIV.caution, DIV.warning { background-color: #f5bdac; border: 1px solid #f35424; }",
	"DIV.attention DIV.heading, DIV.caution DIV.heading, DIV.warning DIV.heading { color: #ffffff; background-color: #f35424; }",
	"DIV.note, DIV.important { background-color: #DFDFFF; border: 1px solid #6297e7; }",
	"DIV.note DIV.heading, DIV.important DIV.heading { color: #ffffff; background-color: #6297e7; }",
	"DIV.hint, DIV.seealso, DIV.tip { background-color: #cff8c9; border: 1px solid #aaf3a1; }",
	"DIV.hint DIV.heading, DIV.seealso DIV.heading, DIV.tip DIV.heading { color: #000000; background-color: #aaf3a1; }",
	"DIV.codeblock { padding: 0 2em; text-align: left; font-family: monospace; font-weight: normal; }",
	"UL.contents-list { padding: 0; list-style-type: none; }",
	"DL.footnotes { margin-left: 0; margin-right: 0; padding-left: 0; padding-right: 0; }",
	"DL.footnotes DT { margin-left: 0; margin-right: 0; font-size: 1em; font-weight: bold; font-style: italic; }",
	"DL.footnotes DD { margin-left: 1em; margin-right: 0; }",
	"TABLE { width: 100%; border-collapse: collapse; border-spacing: 2px; }",
	"TH { font-weight: bold; text-align: left; }",
	"TD { text-align: left; }",
	"TR { vertical-align: middle; border-bottom: 1px solid #DFDFFF; }",
	"CODE, SPAN.code { font-family: monospace; font-weight: bold; }",
	"DIV.caption { width: 100%; margin: 0.5em 0 0; padding: 0 2em; font-style: italic; text-align: center; }"
	"STRONG { font-weight: bold; }",
	"EM { font-style: italic; }",
	"SPAN.command { font-weight: bold; }",
	"SPAN.entry { font-family: monospace; }",
	"SPAN.filename { font-weight: bold; font-style: italic; }",
	"SPAN.icon, SPAN.introduction, SPAN.maths, SPAN.menu { font-style: italic; }",
	"SPAN.key, SPAN.mouse { font-style: unset; }",
	"SPAN.keyword, SPAN.name, SPAN.window, SPAN.variable { font-weight: bold; }",
	NULL
};

/* Static Function Prototypes. */

static bool output_html_write_manual(struct manual_data *manual, struct filename *folder);
static bool output_html_write_file(struct manual_data *object, struct filename *folder, bool single_file);
static bool output_html_write_section_object(struct manual_data *object, int level, bool root);
static bool output_html_write_file_head(struct manual_data *manual);
static bool output_html_write_page_head(struct manual_data *manual, int level);
static bool output_html_write_stylesheet_link(struct manual_data *manual);
static bool output_html_write_page_foot(struct manual_data *manual);
static bool output_html_write_file_foot(struct manual_data *manual);
static bool output_html_write_heading(struct manual_data *node, int level);
static bool output_html_write_chapter_list(struct manual_data *object, int level);
static bool output_html_write_block_collection_object(struct manual_data *object);
static bool output_html_write_footnote(struct manual_data *object);
static bool output_html_write_callout(struct manual_data *object);
static bool output_html_write_list(struct manual_data *object);
static bool output_html_write_table(struct manual_data *object);
static bool output_html_write_code_block(struct manual_data *object);
static bool output_html_write_paragraph(struct manual_data *object);
static bool output_html_write_reference(struct manual_data *source, struct manual_data *target, char *text);
static bool output_html_write_text(enum manual_data_object_type type, struct manual_data *text);
static bool output_html_write_span_tag(enum manual_data_object_type type, char *tag, struct manual_data *text);
static bool output_html_write_span_style(enum manual_data_object_type type, char *style, struct manual_data *text);
static bool output_html_write_inline_defined_text(struct manual_data *defined_text);
static bool output_html_write_inline_link(struct manual_data *link);
static bool output_html_write_inline_reference(struct manual_data *reference);
static bool output_html_write_local_anchor(struct manual_data *source, struct manual_data *target);
static bool output_html_write_title(struct manual_data *node, bool include_name, bool include_title);
static bool output_html_write_id(struct manual_data *node);
static bool output_html_write_entity(enum manual_entity_type entity);

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

	single_file = !manual_data_find_filename_data(manual, MODES_TYPE_HTML);

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

	filename_destroy(foldername);

	if (!output_html_file_open(filename)) {
		filename_destroy(filename);
		return false;
	}

	/* Write the file header. */

	if (!output_html_write_file_head(object)) {
		output_html_file_close();
		filename_destroy(filename);
		return false;
	}

	/* Output the object. */

	if (!output_html_write_section_object(object, OUTPUT_HTML_BASE_LEVEL, true)) {
		output_html_file_close();
		filename_destroy(filename);
		return false;
	}

	/* Output the file footer. */

	if (!output_html_write_file_foot(object)) {
		output_html_file_close();
		filename_destroy(filename);
		return false;
	}

	/* Close the file and set its type. */

	output_html_file_close();

	if (!filename_set_type(filename, FILENAME_FILETYPE_HTML)) {
		filename_destroy(filename);
		return false;
	}

	filename_destroy(filename);

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

	/* Write out the object heading. At the top of the file, this is
	 * the full page heading; lower down, it's just a heading line.
	 */

	if (root == true) {
		if (!output_html_write_page_head(object, level))
			return false;
	} else if (object->title != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_write_heading(object, level))
			return false;

		if (!output_html_file_write_newline())
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
				if (!output_html_write_section_object(block, manual_data_get_nesting_level(block, level), false))
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

				if (!output_html_write_chapter_list(block, manual_data_get_nesting_level(block, level)))
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

			case MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST:
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

			case MANUAL_DATA_OBJECT_TYPE_CALLOUT:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_html_write_callout(block))
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

			case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
				if (object->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
					msg_report(MSG_UNEXPECTED_CHUNK,
							manual_data_find_object_name(block->type),
							manual_data_find_object_name(object->type));
					break;
				}

				if (!output_html_write_footnote(block))
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

	/* If this is the file root, write the page footer out. */

	if (root == true && !output_html_write_page_foot(object))
		return false;

	return true;
}


/**
 * Write an HTML file head block out. This starts with the doctype and
 * continues until we've written the opening <body>.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_file_head(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("<!DOCTYPE html>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<html>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<head>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<meta charset=\"%s\">", encoding_get_current_label()) || !output_html_file_write_newline())
		return false;

	if (manual->title != NULL && !output_html_write_heading(manual, 0))
		return false;

	if (!output_html_write_stylesheet_link(manual))
		return false;

	if (!output_html_file_write_plain("</head>") || !output_html_file_write_newline() || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<body>") || !output_html_file_write_newline())
		return false;

	return true;
}


/**
 * Write an HTML page head block out. This follows the body, and
 * contains the <div class="head">... down to the </div></div class"body">.
 *
 * \param *manual	The manual to base the block on.
 * \param level		The level to write the heading at.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_page_head(struct manual_data *manual, int level)
{
	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("<div id=\"head\"><div id=\"head-liner\">") || !output_html_file_write_newline())
		return false;

	if (manual->type == MANUAL_DATA_OBJECT_TYPE_MANUAL && manual->chapter.resources != NULL) {
		if (!output_html_file_write_plain("<div class=\"title-flex\">"))
			return false;
	}

	if (manual->title != NULL && !output_html_write_heading(manual, level))
		return false;

	if (manual->type == MANUAL_DATA_OBJECT_TYPE_MANUAL && manual->chapter.resources != NULL) {
		if (!output_html_file_write_plain("<div class=\"title-side\">") || !output_html_file_write_newline())
			return false;

		/* Write the date and version here. */

		if (manual->chapter.resources->version != NULL) {
			if (!output_html_file_write_plain("<div class=\"version\">"))
				return false;

			if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_VERSION, manual->chapter.resources->version))
				return false;

			if (!output_html_file_write_plain("</div>") || !output_html_file_write_newline())
				return false;
		}

		if (manual->chapter.resources->date != NULL) {
			if (!output_html_file_write_plain("<div class=\"date\">"))
				return false;

			if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_DATE, manual->chapter.resources->date))
				return false;

			if (!output_html_file_write_plain("</div>") || !output_html_file_write_newline())
				return false;
		}

		if (!output_html_file_write_plain("</div></div>") || !output_html_file_write_newline())
			return false;

		/* Write the strapline. */

		if (manual->chapter.resources->strapline != NULL) {
			if (!output_html_file_write_plain("<div class=\"strapline\">"))
				return false;

			if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_STRAPLINE, manual->chapter.resources->strapline))
				return false;

			if (!output_html_file_write_plain("</div>") || !output_html_file_write_newline())
				return false;
		}

		/* Write the credit line. */

		if (manual->chapter.resources->credit != NULL) {
			if (!output_html_file_write_plain("<div class=\"credit\">"))
				return false;

			if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_CREDIT, manual->chapter.resources->credit))
				return false;

			if (!output_html_file_write_plain("</div>") || !output_html_file_write_newline())
				return false;
		}
	}

	if (!output_html_file_write_plain("</div></div>") || !output_html_file_write_newline() || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<div id=\"body\"><div id=\"body-liner\">") || !output_html_file_write_newline())
		return false;

	return true;
}

/**
 * Write a stylesheet link for a file associated with a given node.
 *
 * \param *manual	The node at the root of the file.
 * \return		TRUE if successful, otherwise FALSE.
 * 
 */

static bool output_html_write_stylesheet_link(struct manual_data *manual)
{
	struct manual_data *sheet_node = NULL;
	struct filename *sheetname = NULL, *sourcename = NULL, *targetname = NULL, *filename = NULL;
	char *link = NULL;
	int line = 0;

	if (manual == NULL)
		return false;

	/* Find the nearest stylesheet details. If there isn't one, write the default
	 * sheet and exit. 
	 */

	sheet_node = manual_data_get_node_stylesheet(manual, MODES_TYPE_HTML);
	if (sheet_node == NULL) {
		if (!output_html_file_write_plain("<style>") || !output_html_file_write_newline())
			return false;

		for (line = 0; output_html_default_stylesheet[line] != NULL; line++) {
			if (!output_html_file_write_plain("  %s", output_html_default_stylesheet[line]) || !output_html_file_write_newline())
				return false;
		}

		if (!output_html_file_write_plain("</style>") || !output_html_file_write_newline())
			return false;

		return true;
	}

	sheetname = filename_up(sheet_node->chapter.resources->html.stylesheet, 0);
	if (sheetname == NULL)
		return false;

	/* If the two nodes are not in the same file, get a relative filename. */

	if (manual_data_nodes_share_file(manual, sheet_node, MODES_TYPE_HTML) == false) {
		sourcename = manual_data_get_node_filename(manual, output_html_root_filename, MODES_TYPE_HTML);
		if (sourcename == NULL)
			return false;

		targetname = manual_data_get_node_filename(sheet_node, output_html_root_filename, MODES_TYPE_HTML);
		if (targetname == NULL) {
			filename_destroy(sourcename);
			return false;
		}

		filename = filename_get_relative(sourcename, targetname);
		filename_destroy(sourcename);
		filename_destroy(targetname);
		if (filename == NULL)
			return false;

		if (!filename_prepend(sheetname, filename, 0)) {
			filename_destroy(filename);
			filename_destroy(sheetname);
			return false;
		}
	}

	link = filename_convert(sheetname, FILENAME_PLATFORM_LINUX, 0);
	filename_destroy(sheetname);

	if (link == NULL)
		return false;

	if (!output_html_file_write_plain("<link rel=\"stylesheet\" type=\"text/css\" href=\"")) {
		free(link);
		return false;
	}

	if (!output_html_file_write_text(link)) {
		free(link);
		return false;
	}

	free(link);

	if (!output_html_file_write_plain("\">") || !output_html_file_write_newline())
		return false;

	return true;
}


/**
 * Write an HTML page foot block out. This ends the <div id="body">
 * section and runs down to just before the </body> tag.
 *
 * \param *manual	The manual to base the block on.
 * \return		TRUE if successful, otherwise FALSE.
 */

static bool output_html_write_page_foot(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("</div></div>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<div id=\"foot\"><div id=\"foot-liner\">") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("</div></div>") || !output_html_file_write_newline())
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

static bool output_html_write_file_foot(struct manual_data *manual)
{
	if (manual == NULL)
		return false;

	if (!output_html_file_write_plain("</body>") || !output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("</html>") || !output_html_file_write_newline())
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

static bool output_html_write_heading(struct manual_data *node, int level)
{
	char buffer[OUTPUT_HTML_TITLE_TAG_BLOCK_LEN];

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

	if (level > 0 && !output_html_write_id(node))
		return false;

	if (!output_html_file_write_plain(">"))
		return false;

	/* Write the title text. */

	if (!output_html_write_title(node, false, true))
		return false;

	/* Write the closing tag. */

	if (!output_html_file_write_plain("</%s>", buffer))
		return false;

	if (!output_html_file_write_newline())
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

static bool output_html_write_chapter_list(struct manual_data *object, int level)
{
	struct manual_data *entry = NULL;
	bool first = true;

	/* The parent object is in the chain to be listed, so we need to
	 * go up again to its parent and then down to the first child in
	 * order to get the whole list.
	 */

	if (object == NULL || object->parent == NULL || object->parent->parent == NULL)
		return false;

	/* Output the list. */

	entry = object->parent->parent->first_child;

	while (entry != NULL) {
		switch (entry->type) {
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (entry->title != NULL) {
				if (first == true) {
					if (!output_html_file_write_newline())
						return false;
					
					if (!output_html_file_write_plain("<ul class=\"contents-list\">") || !output_html_file_write_newline())
						return false;

					first = false;
				}

				if (!output_html_file_write_plain("<li>"))
					return false;

				if (entry->chapter.id != NULL && !output_html_write_local_anchor(object, entry))
					return false;

				if (!output_html_write_title(entry, false, true))
					return false;

				if (entry->chapter.id != NULL && !output_html_file_write_plain("</a>"))
					return false;

				if (!output_html_file_write_plain("</li>") || !output_html_file_write_newline())
					return false;
			}
			break;

		default:
			break;
		}

		entry = entry->next;
	}

	/* Close the list if we need to, and we're done. */

	if (first == false && (!output_html_file_write_plain("</ul>") || !output_html_file_write_newline()))
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

		case MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST:
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
 * Process the contents of a footnote and write it out.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_html_write_footnote(struct manual_data *object)
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

	if (!output_html_file_write_newline())
		return false;

	if (object->previous == NULL || object->previous->type != MANUAL_DATA_OBJECT_TYPE_FOOTNOTE) {
		if (!output_html_file_write_plain("<dl class=\"footnotes\">") || !output_html_file_write_newline())
			return false;
	}

	/* Output the node heading. */

	if (!output_html_file_write_plain("<dt"))
		return false;

	if (!output_html_write_id(object))
		return false;

	if (!output_html_file_write_plain(">"))
		return false;

	number = manual_data_get_node_number(object, true);
	if (number == NULL)
		return false;

	if (!output_html_file_write_text(number)) {
		free(number);
		return false;
	}

	free(number);

	if (!output_html_file_write_plain("</dt>"))
		return false;

	/* Output the note body. */

	if (!output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<dd>"))
		return false;

	if (!output_html_write_block_collection_object(object))
		return false;

	if (!output_html_file_write_plain("</dd>"))
		return false;

	if (!output_html_file_write_newline())
		return false;

	/* Close the footnote block. */

	if ((object->next == NULL || object->next->type != MANUAL_DATA_OBJECT_TYPE_FOOTNOTE)) {
		if (!output_html_file_write_plain("</dl>") || !output_html_file_write_newline())
			return false;
	}

	return true;
}

/**
 * Process the contents of a callout and write it out.
 *
 * \param *object		The object to process.
 * \return			True if successful; False on error.
 */

static bool output_html_write_callout(struct manual_data *object)
{
	struct manual_data *block, *title;
	char* type_style = NULL;

	if (object == NULL || object->first_child == NULL)
		return true;

	/* Confirm that this is a suitable object. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_CALLOUT:
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_CALLOUT),
				manual_data_find_object_name(object->type));
		return false;
	}

	switch (object->chunk.flags & MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE) {
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_ATTENTION:
		type_style = " attention";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_CAUTION:
		type_style = " caution";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_DANGER:
		type_style = " danger";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_ERROR:
		type_style = " error";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_HINT:
		type_style = " hint";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_IMPORTANT:
		type_style = " important";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_NOTE:
		type_style = " note";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_SEEALSO:
		type_style = " seealso";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_TIP:
		type_style = " tip";
		break;
	case MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_WARNING:
		type_style = " warning";
		break;
	default:
		type_style = "";
		break;
	}

	/* Write out the box heading, */

	if (!output_html_file_write_newline())
		return false;

	if (!output_html_file_write_plain("<div class=\"callout%s\">", type_style))
		return false;

	if (object->title == NULL) {
		title = manual_data_get_callout_name(object);
	} else {
		title = object->title;
	}

	if (title != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_file_write_plain("<div class=\"heading\">"))
			return false;

		if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, title))
			return false;

		if (!output_html_file_write_plain("</div>") || !output_html_file_write_newline())
			return false;

		if (!output_html_file_write_newline())
			return false;
	}

	if (!output_html_file_write_plain("<div class=\"content\">"))
		return false;

	/* Write the contents. */

	block = object->first_child;

	/* If changing this switch, note the analogous list in
	 * output_html_write_block_collection_object() which
	 * covers similar block level objects.
	 */

	while (block != NULL) {
		switch (block->type) {
		case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
			if (!output_html_write_paragraph(block))
				return false;
			break;

		case MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST:
		case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
		case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
			if (!output_html_write_list(block))
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

	/* Close the DIV. */

	if (!output_html_file_write_plain("</div></div>") || !output_html_file_write_newline())
		return false;

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
	case MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST:
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

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST:
		if (!output_html_file_write_plain("<dl>"))
			return false;
		break;
	case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
		if (!output_html_file_write_plain("<ol>"))
			return false;
		break;
	case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
		if (!output_html_file_write_plain("<ul>"))
			return false;
		break;
	default:
		break;
	}

	if (!output_html_file_write_newline())
		return false;


	item = object->first_child;

	while (item != NULL) {
		switch (item->type) {
		case MANUAL_DATA_OBJECT_TYPE_LIST_ITEM:
			if (object->type == MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST && item->title != NULL) {
				if (!output_html_file_write_plain("<dt>"))
					return false;

				if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, item->title))
					return false;

				if (!output_html_file_write_plain("</dt>"))
					return false;

				if (!output_html_file_write_newline())
					return false;
			}

			if (!output_html_file_write_plain((object->type == MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST) ? "<dd>" : "<li>"))
				return false;

			if (!output_html_write_block_collection_object(item))
				return false;

			if (!output_html_file_write_plain((object->type == MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST) ? "</dd>" : "</li>"))
				return false;

			if (!output_html_file_write_newline())
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


	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST:
		if (!output_html_file_write_plain("</dl>"))
			return false;
		break;
	case MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST:
		if (!output_html_file_write_plain("</ol>"))
			return false;
		break;
	case MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST:
		if (!output_html_file_write_plain("</ul>"))
			return false;
		break;
	default:
		break;
	}
	
	if (!output_html_file_write_newline())
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

	if (!output_html_file_write_newline())
		return false;
	
	if (!output_html_file_write_plain("<div class=\"table\""))
		return false;
	
	if (!output_html_write_id(object))
		return false;

	if (!output_html_file_write_plain("><table>") || !output_html_file_write_newline())
		return false;

	/* Write the table headings. */

	column_set = object->chapter.columns;
	if (column_set->type != MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET),
				manual_data_find_object_name(object->type));
		return false;
	}

	if (!output_html_file_write_plain("<tr>"))
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

	if (!output_html_file_write_plain("</tr>") || !output_html_file_write_newline())
		return false;

	/* Write the table rows. */

	row = object->first_child;

	while (row != NULL) {
		switch (row->type) {
		case MANUAL_DATA_OBJECT_TYPE_TABLE_ROW:
			column = row->first_child;

			if (!output_html_file_write_plain("<tr>"))
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

			if (!output_html_file_write_plain("</tr>") || !output_html_file_write_newline())
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

	if (!output_html_file_write_plain("</table>"))
		return false;

	/* Write the title. */

	if (object->title != NULL) {
		if (!output_html_file_write_newline())
			return false;

		if (!output_html_file_write_plain("<div class=\"caption\">"))
			return false;

		if (!output_html_write_title(object, true, true))
			return false;

		if (!output_html_file_write_plain("</div>"))
			return false;
	}

	/* Close the outer DIV. */

	if (!output_html_file_write_plain("</div>") || !output_html_file_write_newline())
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

	if (!output_html_file_write_plain("<div class=\"codeblock\""))
		return false;
	
	if (!output_html_write_id(object))
		return false;

	if (!output_html_file_write_plain("><pre>"))
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

		if (!output_html_write_title(object, true, true))
			return false;

		if (!output_html_file_write_plain("</div>"))
			return false;
	}

	if (!output_html_file_write_plain("</div>") || !output_html_file_write_newline())
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

	if (!output_html_file_write_newline())
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
	bool success = true;

	/* An empty block doesn't require any output. */

	if (text == NULL)
		return true;

	if (text->type != type) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(type), manual_data_find_object_name(text->type));
		return false;
	}

	chunk = text->first_child;

	while (success == true && chunk != NULL) {
		switch (chunk->type) {
		case MANUAL_DATA_OBJECT_TYPE_CITATION:
			success = output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_CITATION, "cite", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_CODE:
			success = output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_CODE, "code", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_COMMAND:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_COMMAND, "command", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_CONSTANT:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_CONSTANT, "code", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_DEFINED_TEXT:
			success = output_html_write_inline_defined_text(chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_EVENT:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_EVENT, "name", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_FILENAME:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_FILENAME, "filename", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_FUNCTION:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_FUNCTION, "code", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ICON:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_ICON, "icon", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_INTRO:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_INTRO, "introduction", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_KEY:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_KEY, "key", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_KEYWORD:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_KEYWORD, "keyword", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS:
			success = output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, "em", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_LINK:
			success = output_html_write_inline_link(chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_MATHS:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_MATHS, "maths", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_MENU:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_MENU, "menu", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_MESSAGE:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_MESSAGE, "name", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_MOUSE:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_MOUSE, "mouse", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_NAME:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_NAME, "name", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_REFERENCE:
			success = output_html_write_inline_reference(chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
			success = output_html_write_span_tag(MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, "strong", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_SWI:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_SWI, "name", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_TYPE:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_TYPE, "name", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_USER_ENTRY:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_USER_ENTRY, "entry", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_VARIABLE:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_VARIABLE, "variable", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_WINDOW:
			success = output_html_write_span_style(MANUAL_DATA_OBJECT_TYPE_WINDOW, "window", chunk);
			break;
		case MANUAL_DATA_OBJECT_TYPE_LINE_BREAK:
			success = (output_html_file_write_plain("<br>") && output_html_file_write_newline());
			break;
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			success = output_html_file_write_text(chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			success = output_html_write_entity(chunk->chunk.entity);
			break;
		default:
			msg_report(MSG_UNEXPECTED_CHUNK,
					manual_data_find_object_name(chunk->type),
					manual_data_find_object_name(text->type));
			break;
		}

		chunk = chunk->next;
	}

	return success;
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
 * Write out an inline defined text block.
 *
 * \param *defined_text		The reference to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_inline_defined_text(struct manual_data *defined_text)
{
	char *value = NULL;

	if (defined_text == NULL)
		return false;

	/* Confirm that this is a reference. */

	if (defined_text->type != MANUAL_DATA_OBJECT_TYPE_DEFINED_TEXT) {
		msg_report(MSG_UNEXPECTED_BLOCK, manual_data_find_object_name(MANUAL_DATA_OBJECT_TYPE_DEFINED_TEXT),
				manual_data_find_object_name(defined_text->type));
		return false;
	}

	/* Find the target object. */

	value = manual_defines_find_value(defined_text->chunk.name);

	/* If text was found, write it out. */

	if (value != NULL) {
		if (!output_html_file_write_text(value))
			return false;
	}

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
	char *number = NULL;
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

	/* If the target is a footnote, write the body text out now. */

	if (target != NULL && target->type == MANUAL_DATA_OBJECT_TYPE_FOOTNOTE && reference->first_child != NULL) {
		if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_REFERENCE, reference))
			return false;
	}

	/* Output the opening anchor tag. */

	if (target != NULL && !output_html_write_local_anchor(reference, target))
		return false;

	/* Output the link body. For footnotes, this is the superscript note number;
	 * for other rererences, this is eiether the target title or the reference
	 * body text.
	 */

	if (target != NULL && target->type == MANUAL_DATA_OBJECT_TYPE_FOOTNOTE) {
		if (!output_html_file_write_plain("<sup>"))
			return false;

		number = manual_data_get_node_number(target, false);
		if (number == NULL)
			return false;

		if (!output_html_file_write_text(number)) {
			free(number);
			return false;
		}

		free(number);

		if (!output_html_file_write_plain("</sup>"))
			return false;
	} else if (target != NULL) {
		if (reference->first_child != NULL) {
			if (!output_html_write_text(MANUAL_DATA_OBJECT_TYPE_REFERENCE, reference))
				return false;
		} else {
			include_title = (
					target->type == MANUAL_DATA_OBJECT_TYPE_CHAPTER ||
					target->type == MANUAL_DATA_OBJECT_TYPE_INDEX ||
					target->type == MANUAL_DATA_OBJECT_TYPE_SECTION
					) ? true : false;

			if (!output_html_write_title(target, true, include_title))
				return false;
		}
	}

	/* Output the closing link tag. */

	if (target != NULL && !output_html_file_write_plain("</a>"))
		return false;

	return true;
}


/**
 * Write an opening <a ...> tag for a link from a source node to a
 * target node.
 *
 * \param *source		The source node.
 * \param *target		The target node.
 * \return			True if successful; False on error.
 */

static bool output_html_write_local_anchor(struct manual_data *source, struct manual_data *target)
{
	struct filename *sourcename = NULL, *targetname = NULL, *filename = NULL;
	char *link = NULL;

	if (source == NULL || target == NULL)
		return false;

	/* Establish the relative link, if external. */

	if (manual_data_nodes_share_file(source, target, MODES_TYPE_HTML) == false) {
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
	}

	/* Output the opening link tag. */

	if (!output_html_file_write_plain("<a href=\"%s#%s\">",
			(link == NULL) ? "" : link,
			(target->chapter.id == NULL) ? "" : target->chapter.id)) {
		free(link);
		return false;
	}

	free(link);

	return true;
}

/**
 * Write out the title of a node.
 *
 * \param *node			The node whose title is to be written.
 * \param include_name		Should we prefix the number with the object name?
 * \param include_title		Should we include the object title?
 * \return			True if successful; False on error.
 */

static bool output_html_write_title(struct manual_data *node, bool include_name, bool include_title)
{
	char *number;

	if (node == NULL || (include_title == true && node->title == NULL))
		return false;

	number = manual_data_get_node_number(node, include_name);

	if (number != NULL) {
		if (!output_html_file_write_text(number)) {
			free(number);
			return false;
		}

		free(number);

		if (include_title && !output_html_file_write_text(" "))
			return false;
	}

	return (include_title == false) || output_html_write_text(MANUAL_DATA_OBJECT_TYPE_TITLE, node->title);
}

/**
 * Write an ID attribute for a node.
 *
 * \param *node			The node whose ID is to be written.
 * \return			True if successful; False on error.
 */

static bool output_html_write_id(struct manual_data *node)
{
	if (node == NULL)
		return false;

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_MANUAL:
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
	case MANUAL_DATA_OBJECT_TYPE_TABLE:
	case MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK:
	case MANUAL_DATA_OBJECT_TYPE_FOOTNOTE:
		break;
	default:

		return false;
	}

	/* Return without failing if there isn't an ID to write. */

	if (node->chapter.id == NULL)
		return true;

	if (!output_html_file_write_plain(" id=\""))
		return false;

	if (!output_html_file_write_text(node->chapter.id))
		return false;

	if (!output_html_file_write_plain("\""))
		return false;

	return true;
}

/**
 * Convert an entity into an HTML representation and write
 * it to the current file.
 * 
 * Unless we have a special case, we just pass it to the manual_entity
 * module to turn the entity into unicode for us. This will then get
 * encoded when writen out to the file.
 *
 * \param entity		The entity to convert.
 * \return			True on success; False on failure.
 */

static bool output_html_write_entity(enum manual_entity_type entity)
{
	int codepoint;
	char buffer[ENCODING_CHAR_BUF_LEN], *text = "?";

	switch (entity) {
	case MANUAL_ENTITY_LT:
		text = "&lt;";
		break;
	case MANUAL_ENTITY_GT:
		text = "&gt;";
		break;
	case MANUAL_ENTITY_AMP:
		text = "&amp;";
		break;
	case MANUAL_ENTITY_QUOT:
		text = "&quot;";
		break;
	case MANUAL_ENTITY_SMILEYFACE:
		text = "&#128578;";
		break;
	case MANUAL_ENTITY_SADFACE:
		text = "&#128577;";
		break;
	case MANUAL_ENTITY_MSEP:
		text = ENCODING_UTF8_NDASH;
		break;
	default:
		codepoint = manual_entity_find_codepoint(entity);
		if (codepoint != MANUAL_ENTITY_NO_CODEPOINT) {
			encoding_write_utf8_character(buffer, ENCODING_CHAR_BUF_LEN, codepoint);
			text = buffer;
		} else {
			text = (char *) manual_entity_find_name(entity);
			msg_report(MSG_ENTITY_NO_MAP, (text == NULL) ? "*UNKNOWN*" : text);
			return false;
		}
		break;
	}

	return output_html_file_write_text(text);
}
