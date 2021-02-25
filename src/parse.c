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
 * \file parse.c
 *
 * XML Parser, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "encoding.h"
#include "filename.h"
#include "manual.h"
#include "manual_data.h"
#include "manual_entity.h"
#include "modes.h"
#include "msg.h"
#include "parse_element.h"
#include "parse_link.h"
#include "parse_xml.h"

/**
 * The maximum length of a file leafname.
 */

#define PARSE_MAX_LEAFNAME 128

/* Static Function Prototypes. */

static bool parse_file(struct filename *filename, struct manual_data **manual, struct manual_data *chapter);

static void parse_manual(struct parse_xml_block *parser, struct manual_data **manual, struct manual_data *chapter);
static struct manual_data *parse_placeholder_chapter(struct parse_xml_block *parser, struct manual_data *parent);
static struct manual_data *parse_chapter(struct parse_xml_block *parser, struct manual_data *chapter);
static struct manual_data *parse_section(struct parse_xml_block *parser);
static struct manual_data *parse_block_object(struct parse_xml_block *parser);

static void parse_unknown(struct parse_xml_block *parser);
static void parse_resources(struct parse_xml_block *parser, struct manual_data_resources *resources);
static void parse_mode_resources(struct parse_xml_block *parser, struct manual_data_mode *resources);
static bool parse_fetch_single_chunk(struct parse_xml_block *parser, char *buffer, size_t length);
static void parse_link_item(struct manual_data **previous, struct manual_data *parent, struct manual_data *item);

/**
 * Parse an XML file and its descendents.
 *
 * \param *filename	The name of the root file to parse.
 * \return		Pointer to the resulting manual structure.
 */

struct manual *parse_document(char *filename)
{
	struct manual		*document = NULL;
	struct manual_data	*manual = NULL, *chapter = NULL;
	struct filename		*document_root = NULL, *document_base = NULL;

	document_base = filename_make(filename, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LOCAL);
	if (document_base == NULL)
		return NULL;

	document_root = filename_up(document_base, 1);
	if (document_root == NULL)
		return NULL;

	/* Parse the root file. */

	parse_file(document_base, &manual, chapter);
	filename_destroy(document_base);

	if (manual == NULL)
		return NULL;

	if (manual->type != MANUAL_DATA_OBJECT_TYPE_MANUAL) {
		msg_report(MSG_BAD_TYPE);
		return NULL;
	}

	/* Parse any non-inlined chapter files. */

	chapter = manual->first_child;

	while (chapter != NULL) {
		if (chapter->type != MANUAL_DATA_OBJECT_TYPE_SECTION) {
			if (chapter->type != MANUAL_DATA_OBJECT_TYPE_CHAPTER && chapter->type != MANUAL_DATA_OBJECT_TYPE_INDEX) {
				msg_report(MSG_BAD_TYPE);
				return NULL;
			}

			if (!chapter->chapter.processed) {
				document_base = filename_up(document_root, 0);

				if (filename_add(document_base, chapter->chapter.filename, 0)) {
					filename_destroy(chapter->chapter.filename);
					chapter->chapter.filename = NULL;

					parse_file(document_base, &manual, chapter);
				}

				filename_destroy(document_base);
			}
		}

		chapter = chapter->next;
	}

	/* Link the document. */

	document = manual_create(manual);
	if (document == NULL)
		return NULL;

	document->id_index = parse_link(manual);
	manual_ids_dump(document->id_index);

	return document;
} 

/**
 * Parse an XML file.
 *
 * \param *filename	The name of the file to be parsed.
 * \param **manual	Pointer to a pointer to the current manual. The
 *			manual pointer can be NULL if this is the root
 *			file.
 * \param **chapter	Pointer to the current chapter. The chapter pointer
 * 			can be NULL if this is the root file.
 */

static bool parse_file(struct filename *filename, struct manual_data **manual, struct manual_data *chapter)
{
	struct parse_xml_block	*parser;
	enum parse_xml_result	result;
	enum parse_element_type	element;
	char			*file = NULL;

	file = filename_convert(filename, FILENAME_PLATFORM_LOCAL, 0);

	if (file == NULL) {
		msg_report(MSG_FILE_MISSING);
		return false;
	}

	/* Construct a parser and open the XML file. */

	parser = parse_xml_open_file(file);

	if (parser == NULL) {
		msg_report(MSG_OPEN_FAIL, file);
		free(file);
		return false;
	}

	/* Parse the file contents. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_MANUAL:
				parse_manual(parser, manual, chapter);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Outer");
				parse_unknown(parser);
				break;
			}
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, "Outer");
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF);

	/* Report any errors. */

	if (result == PARSE_XML_RESULT_ERROR)
		msg_report(MSG_XML_FAIL, file);

	/* Close the parser and file. */

	parse_xml_close_file(parser);

	/* Free the file name storage. */

	free(file);

	return true;
}


/**
 * Process a manual object
 *
 * \param *parser	Pointer to the parser to use.
 * \param **manual	Pointer to a pointer to the current manual. The
 *			manual pointer can be NULL if this is the root
 *			file.
 * \param *chapter	Pointer the current chapter. The chapter pointer can be
 * 			NULL if this is the root file.
 */

static void parse_manual(struct parse_xml_block *parser, struct manual_data **manual, struct manual_data *chapter)
{
	bool done = false;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data *tail = NULL, *item = NULL;
	struct manual_data_resources *resources;

	/* Create a new manual if this is the root file. */

	if (*manual == NULL)
		*manual = manual_data_create(MANUAL_DATA_OBJECT_TYPE_MANUAL);
	
	if (*manual == NULL) {
		parse_xml_set_error(parser);
		return;
	}

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Manual", parse_element_find_tag(type));

	/* Process the manual contents. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_TITLE:
				if ((*manual)->title == NULL) {
					(*manual)->title = parse_block_object(parser);
				} else {
					msg_report(MSG_DUPLICATE_TAG, "title", "manual");
					parse_xml_set_error(parser);
				}
				break;
			case PARSE_ELEMENT_SUMMARY:
				resources = manual_data_get_resources(*manual);
				if (resources != NULL) {
					if (resources->summary == NULL) {
						resources->summary = parse_block_object(parser);
					} else {
						msg_report(MSG_DUPLICATE_TAG, parse_element_find_tag(element), parse_element_find_tag(type));
						parse_xml_set_error(parser);
					}
				} else {
					parse_xml_set_error(parser);
				}
				break;
			case PARSE_ELEMENT_RESOURCES:
				resources = manual_data_get_resources(*manual);
				if (resources != NULL)
					parse_resources(parser, resources);
				else
					parse_xml_set_error(parser);
				break;
			case PARSE_ELEMENT_CHAPTER:
			case PARSE_ELEMENT_INDEX:
				item = parse_chapter(parser, chapter);
				if (chapter == NULL)
					parse_link_item(&tail, *manual, item);
				break;
			case PARSE_ELEMENT_SECTION:
				item = parse_section(parser);
				parse_link_item(&tail, *manual, item);
				break;
			case PARSE_ELEMENT_NONE:
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Manual");
				parse_unknown(parser);
				break;
			}
			break;
		case PARSE_XML_RESULT_TAG_EMPTY:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_CHAPTER:
			case PARSE_ELEMENT_INDEX:
				item = parse_placeholder_chapter(parser, *manual);
				parse_link_item(&tail, *manual, item);
				break;
			case PARSE_ELEMENT_NONE:
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
				break;
			}
			break;
		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == PARSE_ELEMENT_MANUAL)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);
	
	msg_report(MSG_PARSE_POP, "Manual", parse_element_find_tag(type));
}



/**
 * Process a placeholder chapter object (CHAPTER, INDEX), returning a pointer
 * to the the new data structure.
 *
 * \param *parser	Pointer to the parser to use.
 * \param *parent	Pointer to the parent data structure.
 * \return		Pointer to the new data structure.
 */

static struct manual_data *parse_placeholder_chapter(struct parse_xml_block *parser, struct manual_data *parent)
{
	enum parse_element_type type;
	char filename[PARSE_MAX_LEAFNAME];
	struct manual_data *new_chapter = NULL;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Placeholder Chapter", parse_element_find_tag(type));

	/* Read the supplied filename. */

	if (parse_xml_copy_attribute_text(parser, "file", filename, PARSE_MAX_LEAFNAME) == 0) {
		msg_report(MSG_MISSING_ATTRIBUTE, "file");
		parse_xml_set_error(parser);
		return NULL;
	}

	/* Create the new chapter object. */

	switch (type) {
	case PARSE_ELEMENT_CHAPTER:
		new_chapter = manual_data_create(MANUAL_DATA_OBJECT_TYPE_CHAPTER);
		break;
	case PARSE_ELEMENT_INDEX:
		new_chapter = manual_data_create(MANUAL_DATA_OBJECT_TYPE_INDEX);
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK_ADD, parse_element_find_tag(type));
		parse_xml_set_error(parser);
		return NULL;
	}

	if (new_chapter == NULL) {
		parse_xml_set_error(parser);
		return NULL;
	}

	/* Link the chapter object to its parent. */

	new_chapter->parent = parent;
	new_chapter->chapter.filename = filename_make(filename, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LOCAL);

	return new_chapter;
}


/**
 * Process a chapter object (CHAPTER, INDEX), returning a pointer to the root
 * of the new data structure.
 *
 * \param *parser	Pointer to the parser to use.
 * \param *chapter	Pointer to a placeholder chapter to use, or NULL to
 *			create a new one.
 * \return		Pointer to the new data structure.
 */

static struct manual_data *parse_chapter(struct parse_xml_block *parser, struct manual_data *chapter)
{
	bool done = false;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data *new_chapter = NULL, *tail = NULL, *item = NULL;
	struct manual_data_resources *resources;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Chapter", parse_element_find_tag(type));

	/* Create the new chapter object. */

	if (chapter == NULL) {
		switch (type) {
		case PARSE_ELEMENT_CHAPTER:
			new_chapter = manual_data_create(MANUAL_DATA_OBJECT_TYPE_CHAPTER);
			break;
		case PARSE_ELEMENT_INDEX:
			new_chapter = manual_data_create(MANUAL_DATA_OBJECT_TYPE_INDEX);
			break;
		default:
			msg_report(MSG_UNEXPECTED_BLOCK_ADD, parse_element_find_tag(type));
			parse_xml_set_error(parser);
			return NULL;
		}
	} else {
		new_chapter = chapter;
	}

	if (new_chapter == NULL) {
		parse_xml_set_error(parser);
		return NULL;
	}

	/* Read the chapter id. */

	new_chapter->id = parse_xml_get_attribute_text(parser, "id");

	/* We've now processed the actual chapter data. */

	new_chapter->chapter.processed = true;

	/* Parse the chapter contents. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_TITLE:
				if (new_chapter->title == NULL) {
					new_chapter->title = parse_block_object(parser);
				} else {
					msg_report(MSG_DUPLICATE_TAG, parse_element_find_tag(element), parse_element_find_tag(type));
					parse_xml_set_error(parser);
				}
				break;
			case PARSE_ELEMENT_SUMMARY:
				resources = manual_data_get_resources(new_chapter);
				if (resources != NULL) {
					if (resources->summary == NULL) {
						resources->summary = parse_block_object(parser);
					} else {
						msg_report(MSG_DUPLICATE_TAG, parse_element_find_tag(element), parse_element_find_tag(type));
						parse_xml_set_error(parser);
					}
				} else {
					parse_xml_set_error(parser);
				}
				break;
			case PARSE_ELEMENT_RESOURCES:
				resources = manual_data_get_resources(new_chapter);
				if (resources != NULL)
					parse_resources(parser, resources);
				else
					parse_xml_set_error(parser);
				break;
			case PARSE_ELEMENT_SECTION:
				item = parse_section(parser);
				parse_link_item(&tail, new_chapter, item);
				break;
			case PARSE_ELEMENT_NONE:
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
				parse_unknown(parser);
				break;
			}
			break;
		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == type)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);

	msg_report(MSG_PARSE_POP, "Chapter", parse_element_find_tag(type));

	return new_chapter;
}


/**
 * Process a section object (SECTION), returning a pointer to the root
 * of the new data structure.
 *
 * \param *parser	Pointer to the parser to use.
 * \return		Pointer to the new data structure.
 */

static struct manual_data *parse_section(struct parse_xml_block *parser)
{
	bool done = false;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data *new_section = NULL, *tail = NULL, *item = NULL;
	struct manual_data_resources *resources;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Section", parse_element_find_tag(type));

	/* Create the new section object. */

	switch (type) {
	case PARSE_ELEMENT_SECTION:
		new_section = manual_data_create(MANUAL_DATA_OBJECT_TYPE_SECTION);
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK_ADD, parse_element_find_tag(type));
		parse_xml_set_error(parser);
		return NULL;
	}

	if (new_section == NULL) {
		parse_xml_set_error(parser);
		return NULL;
	}

	/* Read the chapter id. */

	new_section->id = parse_xml_get_attribute_text(parser, "id");

	/* Parse the section contents. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_TITLE:
				if (new_section->title == NULL) {
					new_section->title = parse_block_object(parser);
				} else {
					msg_report(MSG_DUPLICATE_TAG, parse_element_find_tag(element), parse_element_find_tag(type));
					parse_xml_set_error(parser);
				}
				break;
			case PARSE_ELEMENT_SUMMARY:
				resources = manual_data_get_resources(new_section);
				if (resources != NULL) {
					if (resources->summary == NULL) {
						resources->summary = parse_block_object(parser);
					} else {
						msg_report(MSG_DUPLICATE_TAG, parse_element_find_tag(element), parse_element_find_tag(type));
						parse_xml_set_error(parser);
					}
				} else {
					parse_xml_set_error(parser);
				}
				break;
			case PARSE_ELEMENT_RESOURCES:
				resources = manual_data_get_resources(new_section);
				if (resources != NULL)
					parse_resources(parser, resources);
				else
					parse_xml_set_error(parser);
				break;
			case PARSE_ELEMENT_SECTION:
				item = parse_section(parser);
				parse_link_item(&tail, new_section, item);
				break;
			case PARSE_ELEMENT_PARAGRAPH:
				item = parse_block_object(parser);
				parse_link_item(&tail, new_section, item);
				break;
			case PARSE_ELEMENT_NONE:
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
				parse_unknown(parser);
				break;
			}
			break;
		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == type)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);

	msg_report(MSG_PARSE_POP, "Section", parse_element_find_tag(type));

	return new_section;
}


/**
 * Process a block object (P, TITLE, SUMMARY), returning a pointer to the root
 * of the new data structure.
 *
 * \param *parser	Pointer to the parser to use.
 * \return		Pointer to the new data structure.
 */

static struct manual_data *parse_block_object(struct parse_xml_block *parser)
{
	bool done = false;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data *new_block = NULL, *tail = NULL, *item = NULL;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Block", parse_element_find_tag(type));

	/* Create the block object. */

	switch (type) {
	case PARSE_ELEMENT_PARAGRAPH:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_PARAGRAPH);
		break;
	case PARSE_ELEMENT_TITLE:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_TITLE);
		break;
	case PARSE_ELEMENT_SUMMARY:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_SUMMARY);
		break;
	case PARSE_ELEMENT_CITE:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_CITATION);
		break;
	case PARSE_ELEMENT_CODE:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_CODE);
		break;
	case PARSE_ELEMENT_ENTRY:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_USER_ENTRY);
		break;
	case PARSE_ELEMENT_EMPHASIS:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS);
		break;
	case PARSE_ELEMENT_FILE:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_FILENAME);
		break;
	case PARSE_ELEMENT_ICON:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_ICON);
		break;
	case PARSE_ELEMENT_KEY:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_KEY);
		break;
	case PARSE_ELEMENT_MOUSE:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_MOUSE);
		break;
	case PARSE_ELEMENT_STRONG:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS);
		break;
	case PARSE_ELEMENT_WINDOW:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_WINDOW);
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK_ADD, parse_element_find_tag(type));
		parse_xml_set_error(parser);
		return NULL;
	}

	if (new_block == NULL) {
		parse_xml_set_error(parser);
		return NULL;
	}

	/* Process the content within the new object. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TEXT:
		case PARSE_XML_RESULT_WHITESPACE:
			item = manual_data_create(MANUAL_DATA_OBJECT_TYPE_TEXT);
			if (item == NULL) {
				result = parse_xml_set_error(parser);
				msg_report(MSG_DATA_MALLOC_FAIL);
				continue;
			}
			item->chunk.text = parse_xml_get_text(parser);
			encoding_flatten_whitespace(item->chunk.text);
			parse_link_item(&tail, new_block, item);
			break;

		case PARSE_XML_RESULT_TAG_ENTITY:
			item = manual_data_create(MANUAL_DATA_OBJECT_TYPE_ENTITY);
			if (item == NULL) {
				result = parse_xml_set_error(parser);
				msg_report(MSG_DATA_MALLOC_FAIL);
				continue;
			}
			item->chunk.entity = parse_xml_get_entity(parser);
			parse_link_item(&tail, new_block, item);
			break;

		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_CITE:
			case PARSE_ELEMENT_CODE:
			case PARSE_ELEMENT_ENTRY:
			case PARSE_ELEMENT_EMPHASIS:
			case PARSE_ELEMENT_FILE:
			case PARSE_ELEMENT_ICON:
			case PARSE_ELEMENT_KEY:
			case PARSE_ELEMENT_MOUSE:
			case PARSE_ELEMENT_STRONG:
			case PARSE_ELEMENT_WINDOW:
				item = parse_block_object(parser);
				parse_link_item(&tail, new_block, item);
				break;
			case PARSE_ELEMENT_NONE:
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
				parse_unknown(parser);
				break;
			}
			break;

		case PARSE_XML_RESULT_TAG_EMPTY:
			break;

		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == type)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;

		case PARSE_XML_RESULT_COMMENT:
			break;

		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);
	
	msg_report(MSG_PARSE_POP, "Block", parse_element_find_tag(type));

	return new_block;
}


/**
 * Process an unknown object, simply disposing of it and all of its descendents.
 *
 * \param *parser	Pointer to the parser to use.
 */

static void parse_unknown(struct parse_xml_block *parser)
{
	bool done = false;
	enum parse_xml_result result;
	enum parse_element_type type, element;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Unknown", parse_element_find_tag(type));

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			parse_unknown(parser);
			break;
		case PARSE_XML_RESULT_TAG_EMPTY:
			break;
		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == type)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, "Unknown");
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);
	
	msg_report(MSG_PARSE_POP, "Unknown", parse_element_find_tag(type));
}


/**
 * Process a resource object (RESOURCES), storing the information contained
 * within inside the supplied resources object.
 *
 * \param *parser	Pointer to the parser to use.
 * \param *resources	Pointer to the resources object to fill.
 */

static void parse_resources(struct parse_xml_block *parser, struct manual_data_resources *resources)
{
	bool done = false;
	char buffer[PARSE_MAX_LEAFNAME];
	enum modes_type mode = MODES_TYPE_NONE;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data_mode *mode_resources = NULL;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Resources", parse_element_find_tag(type));

	if (type != PARSE_ELEMENT_RESOURCES) {
		msg_report(MSG_UNEXPECTED_BLOCK_ADD, parse_element_find_tag(type));
		parse_xml_set_error(parser);
		return;
	}

	/* Check that the client supplied a resources block to fill. */

	if (resources == NULL) {
		parse_xml_set_error(parser);
		return;
	}

	/* Parse the resources data contents. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_MODE:
				if (parse_xml_copy_attribute_text(parser, "type", buffer, PARSE_MAX_LEAFNAME) > 0) {
					mode = modes_find_type(buffer);
					if (mode != MODES_TYPE_NONE) {
						mode_resources = modes_find_resources(resources, mode);
						parse_mode_resources(parser, mode_resources);
					} else {
						msg_report(MSG_UNKNOWN_MODE, buffer);
						parse_xml_set_error(parser);
					}
				} else {
					msg_report(MSG_MISSING_ATTRIBUTE, "type");
					parse_xml_set_error(parser);
				}
				break;
			case PARSE_ELEMENT_DOWNLOADS:
				if (parse_fetch_single_chunk(parser, buffer, PARSE_MAX_LEAFNAME))
					resources->downloads = filename_make(buffer, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
				break;
			case PARSE_ELEMENT_IMAGES:
				if (parse_fetch_single_chunk(parser, buffer, PARSE_MAX_LEAFNAME))
					resources->images = filename_make(buffer, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
				break;
			case PARSE_ELEMENT_NONE:
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
				parse_unknown(parser);
				break;
			}
			break;
		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == type)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);

	msg_report(MSG_PARSE_POP, "Resources", parse_element_find_tag(type));
}


/**
 * Process a mode resource object (MODE), storing the information contained
 * within inside the supplied resources object.
 *
 * \param *parser	Pointer to the parser to use.
 * \param *resources	Pointer to the resources object to fill.
 */

static void parse_mode_resources(struct parse_xml_block *parser, struct manual_data_mode *resources)
{
	bool done = false;
	char buffer[PARSE_MAX_LEAFNAME];
	enum parse_xml_result result;
	enum parse_element_type type, element;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Mode Resources", parse_element_find_tag(type));

	if (type != PARSE_ELEMENT_MODE) {
		msg_report(MSG_UNEXPECTED_BLOCK_ADD, parse_element_find_tag(type));
		parse_xml_set_error(parser);
		return;
	}

	/* Check that the client supplied a resources block to fill. */

	if (resources == NULL) {
		parse_xml_set_error(parser);
		return;
	}

	/* Parse the resources data contents. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_FILENAME:
				if (parse_fetch_single_chunk(parser, buffer, PARSE_MAX_LEAFNAME))
					resources->filename = filename_make(buffer, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LINUX);
				break;
			case PARSE_ELEMENT_FOLDER:
				if (parse_fetch_single_chunk(parser, buffer, PARSE_MAX_LEAFNAME))
					resources->folder = filename_make(buffer, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
				break;
			case PARSE_ELEMENT_NONE:
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
				parse_unknown(parser);
				break;
			}
			break;
		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == type)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);

	msg_report(MSG_PARSE_POP, "Mode Resources", parse_element_find_tag(type));
}


/**
 * Fetch a single block from the XML, up to the end of a non-nested tag pair.
 * There must be no tags within the block, and only the default entities will
 * be processed.
 *
 * \param *parser	Pointer to the parser to use.
 * \param *buffer	Pointer to a buffer to take the text.
 * \param length	The length of the supplied buffer.
 * \return		True if successful; false on failure.
 */

static bool parse_fetch_single_chunk(struct parse_xml_block *parser, char *buffer, size_t length)
{
	bool done = false;
	size_t end = 0;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	enum manual_entity_type entity;

	/* Identify the tag which got us here. */

	type = parse_xml_get_element(parser);

	msg_report(MSG_PARSE_PUSH, "Single Chunk", parse_element_find_tag(type));

	/* Parse the resources data contents. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);
			msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
			parse_unknown(parser);
			break;
		case PARSE_XML_RESULT_TAG_END:
			element = parse_xml_get_element(parser);

			if (element == type)
				done = true;
			else if (element != PARSE_ELEMENT_NONE)
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		case PARSE_XML_RESULT_TEXT:
		case PARSE_XML_RESULT_WHITESPACE:
			end += parse_xml_copy_text(parser, buffer + end, length - end);
			break;
		case PARSE_XML_RESULT_TAG_ENTITY:
			if (end >= (length - 2))
				break;

			entity = parse_xml_get_entity(parser);

			switch (entity) {
			case MANUAL_ENTITY_AMP:
				buffer[end++] = '&';
				break;
			case MANUAL_ENTITY_QUOT:
				buffer[end++] = '"';
				break;
			case MANUAL_ENTITY_APOS:
				buffer[end++] = '\'';
				break;
			case MANUAL_ENTITY_LT:
				buffer[end++] = '<';
				break;
			case MANUAL_ENTITY_GT:
				buffer[end++] = '>';
				break;
			default:
				buffer[end++] = '?';
				break;
			}
			buffer[end] = '\0';
			break;
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);

	msg_report(MSG_PARSE_POP, "Single Chunk", parse_element_find_tag(type));

	return (end > 0) ? true : false;
}


/**
 * Link a new manual data block on to the end of a chain.
 *
 * \param **previous	Pointer to the pointer to the chain tail.
 * \param *parent	Pointer to the item's parent.
 * \param *item		Pointer to the item.
 */

static void parse_link_item(struct manual_data **previous, struct manual_data *parent, struct manual_data *item)
{
	if (item == NULL)
		return;

	if (previous != NULL)
		item->previous = *previous;

	item->parent = parent;

	if (previous != NULL && *previous != NULL)
		(*previous)->next = item;
	else if (parent != NULL)
		parent->first_child = item;

	if (previous != NULL)
		*previous = item;
}
