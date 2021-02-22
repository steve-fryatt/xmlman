/* Copyright 2018-2020, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "msg.h"
#include "parse_element.h"
#include "parse_link.h"
#include "parse_stack.h"
#include "parse_xml.h"

/**
 * The maximum length of a file leafname.
 */

#define PARSE_MAX_LEAFNAME 128

/* Static Function Prototypes. */

static bool parse_file(struct filename *filename, struct manual_data **manual, struct manual_data **chapter);

static void parse_manual(struct parse_xml_block *parser, struct manual_data **manual, struct manual_data **chapter);
static struct manual_data *parse_placeholder_chapter(struct parse_xml_block *parser, struct manual_data *parent);
static struct manual_data *parse_chapter(struct parse_xml_block *parser, struct manual_data *parent, struct manual_data *chapter);
static struct manual_data *parse_block_object(struct parse_xml_block *parser, struct manual_data *parent);

static void parse_unknown(struct parse_xml_block *parser);
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

	parse_file(document_base, &manual, &chapter);
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

					parse_file(document_base, &manual, &chapter);
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
 * \param **chapter	Pointer a pointer to the current chapter. The
 *			chapter pointer can be  NULL if this is the root
 *			file.
 */

static bool parse_file(struct filename *filename, struct manual_data **manual, struct manual_data **chapter)
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
			msg_report(MSG_UNEXPECTED_XML, result, "Top Level");
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
 * \param **chapter	Pointer a pointer to the current chapter. The
 *			chapter pointer can be  NULL if this is the root
 *			file.
 */

static void parse_manual(struct parse_xml_block *parser, struct manual_data **manual, struct manual_data **chapter)
{
	bool done = false;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data *tail = NULL, *item = NULL;
	/* Create a new manual if this is the root file. */

	if (*manual == NULL)
		*manual = manual_data_create(MANUAL_DATA_OBJECT_TYPE_MANUAL);
	
	if (*manual == NULL) {
		parse_xml_set_error(parser);
		return;
	}

	type = parse_xml_get_element(parser);

	printf("$ Push Manual (%s)\n", parse_element_find_tag(type));

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TAG_START:
			element = parse_xml_get_element(parser);

			switch (element) {
			case PARSE_ELEMENT_TITLE:
				if ((*manual)->title != NULL) {
					msg_report(MSG_DUPLICATE_TAG, "title", "manual");
					parse_xml_set_error(parser);
				} else {
					(*manual)->title = parse_block_object(parser, *manual);
				}
				break;
			case PARSE_ELEMENT_CHAPTER:
			case PARSE_ELEMENT_INDEX:
				item = parse_chapter(parser, *manual, *chapter);

				if (item != NULL) {
					item->previous = tail;

					if (tail != NULL)
						tail->next = item;
					else
						(*manual)->first_child = item;
					
					tail = item;
				}
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
				if (item != NULL) {
					item->previous = tail;

					if (tail != NULL)
						tail->next = item;
					else
						(*manual)->first_child = item;
					
					tail = item;
				}
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
			else
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
	
	printf("$ Pop Manual (%s)\n", parse_element_find_tag(type));
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
	bool done = false;
	struct parse_xml_block *attribute;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	char filename[PARSE_MAX_LEAFNAME];
	struct manual_data *new_chapter = NULL, *tail = NULL, *text = NULL;

	type = parse_xml_get_element(parser);

	printf("$ Create Placeholder Chapter Object (%s)\n", parse_element_find_tag(type));

	/* Read the supplied filename. */

	if (!parse_xml_get_attribute_text(parser, "file", filename, PARSE_MAX_LEAFNAME)) {
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
 * \param *parent	Pointer to the parent data structure.
 * \param *chapter	Pointer to a placeholder chapter to use, or NULL to
 *			create a new one.
 * \return		Pointer to the new data structure.
 */

static struct manual_data *parse_chapter(struct parse_xml_block *parser, struct manual_data *parent, struct manual_data *chapter)
{
	bool done = false;
	struct parse_xml_block *attribute;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data *new_chapter = NULL, *tail = NULL, *text = NULL;

	type = parse_xml_get_element(parser);

	printf("$ Push Chapter Object (%s)\n", parse_element_find_tag(type));

	/* Read the chapter id. */

//	if (!parse_xml_get_attribute_text(parser, "file", filename, PARSE_MAX_LEAFNAME)) {
//		msg_report(MSG_MISSING_ATTRIBUTE, "file");
//		parse_xml_set_error(parser);
//		return NULL;
//	}

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

	/* Link the chapter object to its parent, if it is newly allocated. */

	if (chapter == NULL)
		new_chapter->parent = parent;

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
				if (new_chapter->title != NULL) {
					msg_report(MSG_DUPLICATE_TAG, "title", parse_element_find_tag(type));
					parse_xml_set_error(parser);
				} else {
					new_chapter->title = parse_block_object(parser, new_chapter);
				}
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), parse_element_find_tag(type));
				parse_unknown(parser);
				break;
			}
			break;
		case PARSE_XML_RESULT_WHITESPACE:
		case PARSE_XML_RESULT_COMMENT:
			break;
		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF);

	printf("$ Pop Chapter Object (%s)\n", parse_element_find_tag(type));

	return new_chapter;
}


/**
 * Process a block object (P, TITLE, SUMMARY), returning a pointer to the root
 * of the new data structure.
 *
 * \param *parser	Pointer to the parser to use.
 * \param *parent	Pointer to the parent data structure.
 * \return		Pointer to the new data structure.
 */

static struct manual_data *parse_block_object(struct parse_xml_block *parser, struct manual_data *parent)
{
	bool done = false;
	enum parse_xml_result result;
	enum parse_element_type type, element;
	struct manual_data *new_block = NULL, *tail = NULL, *text = NULL;

	type = parse_xml_get_element(parser);

	printf("$ Push Block Object (%s)\n", parse_element_find_tag(type));

	/* Create the block object. */

	switch (type) {
	case PARSE_ELEMENT_TITLE:
		new_block = manual_data_create(MANUAL_DATA_OBJECT_TYPE_TITLE);
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

	/* Link the block object to its parent. */

	new_block->parent = parent;

	/* Process the content within the new object. */

	do {
		result = parse_xml_read_next_chunk(parser);

		switch (result) {
		case PARSE_XML_RESULT_TEXT:
		case PARSE_XML_RESULT_WHITESPACE:
			text = manual_data_create(MANUAL_DATA_OBJECT_TYPE_TEXT);
			if (text == NULL) {
				result = parse_xml_set_error(parser);
				msg_report(MSG_DATA_MALLOC_FAIL);
				continue;
			}
			text->chunk.text = parse_xml_get_text(parser, false);
			text->parent = new_block;
			text->previous = tail;
			text->chunk.text = parse_xml_get_text(parser, false);

			if (tail != NULL)
				tail->next = text;
			else
				new_block->first_child = text;
			
			tail = text;
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
				new_block->first_child = parse_block_object(parser, new_block);
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
			else
				msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;

		case PARSE_XML_RESULT_COMMENT:
			break;

		default:
			msg_report(MSG_UNEXPECTED_XML, result, parse_element_find_tag(type));
			break;
		}
	} while (result != PARSE_XML_RESULT_ERROR && result != PARSE_XML_RESULT_EOF && !done);
	
	printf("$ Pop Block Object (%s)\n", parse_element_find_tag(type));

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

	type = parse_xml_get_element(parser);

	printf("$ Push Unknown Object (%s)\n", parse_element_find_tag(type));

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
			else
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
	
	printf("$ Pop Unknown Object (%s)\n", parse_element_find_tag(type));
}


static void parse_link_item(struct manual_data **previous, struct manual_data *parent, struct manual_data *item)
{
	if (item == NULL)
		return;

	item->previous = *previous;
	item->parent = parent;

	if (previous != NULL && *previous == NULL)
		(*previous)->next = item;
	else if (parent != NULL)
		parent->first_child = item;

	if (previous != NULL)
		*previous = item;
}

#if 0
/**
 * Process an XML node, passing it on to an appropriate part of the
 * parser based on the current position in the tree.
 *
 * \param reader	The XML Reader to read the node from.
 * \param **manual	Pointer to a pointer to the current manual. The
 *			manual pointer can be NULL if this is the root
 *			file.
 * \param **chapter	Pointer a pointer to the current chapter. The
 *			chapter pointer can be  NULL if this is the root
 *			file.
 */

static void parse_process_node(xmlTextReaderPtr reader, struct manual_data **manual, struct manual_data **chapter)
{
	struct parse_stack_entry	*stack;

	stack = parse_stack_peek(PARSE_STACK_TOP);

	if (stack == NULL) {
		msg_report(MSG_STACK_ERROR);
		return;
	}

	if (stack->content == PARSE_STACK_CONTENT_NONE)
		parse_process_outer_node(reader, manual);
	else
		parse_process_inner_node(reader, chapter);
}


/**
 * Process nodes when we're outside the whole document structure.
 *
 * \param reader	The XML Reader to read the node from.
 * \param **manual	Pointer to a pointer to the current manual. The
 *			manual pointer can be NULL if this is the root
 *			file.
 */

static void parse_process_outer_node(xmlTextReaderPtr reader, struct manual_data **manual)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack, *new_stack;

	old_stack = parse_stack_peek(PARSE_STACK_TOP);
	if (old_stack == NULL || old_stack->content != PARSE_STACK_CONTENT_NONE) {
		msg_report(MSG_STACK_ERROR);
		return;
	}

	type = xmlTextReaderNodeType(reader);
	if (type != XML_READER_TYPE_ELEMENT)
		return;

	element = parse_element_find_type(reader);
	switch (element) {
	case PARSE_ELEMENT_MANUAL:
		new_stack = parse_stack_push(PARSE_STACK_CONTENT_MANUAL, element);
		if (new_stack == NULL)
			return;

		/* Create a new manual if this is the root file. */

		if (*manual == NULL)
			*manual = manual_data_create(MANUAL_DATA_OBJECT_TYPE_MANUAL);

		/* Record the current manual on the stack. */

		new_stack->object = *manual;
		break;
	default:
		msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Outer");
		break;
	}
}


/**
 * Process nodes within the <manual> tag.
 *
 * \param reader	The XML Reader to read the node from.
 * \param **chapter	Pointer a pointer to the current chapter. The
 *			chapter pointer can be NULL if this is the root
 *			file.
 */

static void parse_process_inner_node(xmlTextReaderPtr reader, struct manual_data **chapter)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack, *parent_stack;

	old_stack = parse_stack_peek(PARSE_STACK_TOP);

	type = xmlTextReaderNodeType(reader);
	switch (type) {
	case XML_READER_TYPE_ELEMENT:
		element = parse_element_find_type(reader);

		switch (old_stack->content) {
		case PARSE_STACK_CONTENT_MANUAL:
			switch (element) {
			case PARSE_ELEMENT_TITLE:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_TITLE, element);
				break;
			case PARSE_ELEMENT_INDEX:
				parse_process_add_chapter(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_INDEX, element, *chapter);
				break;
			case PARSE_ELEMENT_CHAPTER:
				if (xmlTextReaderIsEmptyElement(reader))
					parse_process_add_placeholder_chapter(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_CHAPTER);
				else
					parse_process_add_chapter(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_CHAPTER, element, *chapter);
				break;
			case PARSE_ELEMENT_SECTION:
				parse_process_add_section(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SECTION, element);
				break;
			case PARSE_ELEMENT_RESOURCES:
				parse_process_add_resources(reader, old_stack, element);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Manual");
				break;
			}
			break;

		case PARSE_STACK_CONTENT_CHAPTER:
			switch (element) {
			case PARSE_ELEMENT_TITLE:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_TITLE, element);
				break;
			case PARSE_ELEMENT_SUMMARY:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SUMMARY, element);
				break;
			case PARSE_ELEMENT_SECTION:
				parse_process_add_section(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SECTION, element);
				break;
			case PARSE_ELEMENT_RESOURCES:
				parse_process_add_resources(reader, old_stack, element);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Chapter");
				break;
			}
			break;

		case PARSE_STACK_CONTENT_SECTION:
			switch (element) {
			case PARSE_ELEMENT_TITLE:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_TITLE, element);
				break;
			case PARSE_ELEMENT_SUMMARY:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SUMMARY, element);
				break;
			case PARSE_ELEMENT_SECTION:
				parse_process_add_section(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SECTION, element);
				break;
			case PARSE_ELEMENT_RESOURCES:
				parse_process_add_resources(reader, old_stack, element);
				break;
			case PARSE_ELEMENT_PARAGRAPH:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_PARAGRAPH, element);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Section");
				break;
			}
			break;

		case PARSE_STACK_CONTENT_RESOURCES:
			switch (element) {
			case PARSE_ELEMENT_DOWNLOADS:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_RESOURCE_DOWNLOAD, element);
				break;
			case PARSE_ELEMENT_IMAGES:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_RESOURCE_IMAGE, element);
				break;
			case PARSE_ELEMENT_MODE:
				parse_process_add_resources(reader, old_stack, element);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Resources");
				break;
			}
			break;

		case PARSE_STACK_CONTENT_MODE_TEXT:
		case PARSE_STACK_CONTENT_MODE_HTML:
		case PARSE_STACK_CONTENT_MODE_STRONG:
			switch (element) {
			case PARSE_ELEMENT_FILENAME:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_RESOURCE_FILE, element);
				break;
			case PARSE_ELEMENT_FOLDER:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_RESOURCE_FOLDER, element);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Resources");
				break;
			}
			break;

		case PARSE_STACK_CONTENT_BLOCK:
			switch (element) {
			case PARSE_ELEMENT_CITE:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_CITATION, element);
				break;
			case PARSE_ELEMENT_CODE:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_CODE, element);
				break;
			case PARSE_ELEMENT_ENTRY:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_USER_ENTRY, element);
				break;
			case PARSE_ELEMENT_EMPHASIS:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS, element);
				break;
			case PARSE_ELEMENT_FILE:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_FILENAME, element);
				break;
			case PARSE_ELEMENT_ICON:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_ICON, element);
				break;
			case PARSE_ELEMENT_KEY:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_KEY, element);
				break;
			case PARSE_ELEMENT_MOUSE:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_MOUSE, element);
				break;
			case PARSE_ELEMENT_STRONG:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS, element);
				break;
			case PARSE_ELEMENT_WINDOW:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_WINDOW, element);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Block");
				break;
			}
			break;

		case PARSE_STACK_CONTENT_NONE:
			msg_report(MSG_UNEXPECTED_STACK);
			break;
		}
		break;

	case XML_READER_TYPE_TEXT:
	case XML_READER_TYPE_CDATA:
	case XML_READER_TYPE_ENTITY:
	case XML_READER_TYPE_ENTITY_REFERENCE:
	case XML_READER_TYPE_WHITESPACE:
	case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
		parse_process_add_content(reader, old_stack);
		break;

	case XML_READER_TYPE_END_ELEMENT:
	case XML_READER_TYPE_END_ENTITY:
		element = parse_element_find_type(reader);

		if (element != old_stack->closing_element) {
			msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
			break;
		}

		parent_stack = parse_stack_peek(PARSE_STACK_PARENT);

		if (parent_stack != NULL) {
			switch (parent_stack->content) {
			case PARSE_STACK_CONTENT_RESOURCES:
				switch (element) {
				case PARSE_ELEMENT_IMAGES:
				case PARSE_ELEMENT_DOWNLOADS:
					parse_process_collect_resources(old_stack, parent_stack->content);
					break;
				case PARSE_ELEMENT_MODE:
					break;
				default:
					msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
					break;
				}
				break;
			case PARSE_STACK_CONTENT_MODE_TEXT:
			case PARSE_STACK_CONTENT_MODE_STRONG:
			case PARSE_STACK_CONTENT_MODE_HTML:
				switch (element) {
				case PARSE_ELEMENT_FILENAME:
				case PARSE_ELEMENT_FOLDER:
					parse_process_collect_resources(old_stack, parent_stack->content);
					break;
				default:
					msg_report(MSG_UNEXPECTED_CLOSE, parse_element_find_tag(element));
					break;
				}
				break;
			default:
				break;
			}
		}

		switch (old_stack->content) {
		case PARSE_STACK_CONTENT_RESOURCES:
		case PARSE_STACK_CONTENT_MODE_TEXT:
		case PARSE_STACK_CONTENT_MODE_STRONG:
		case PARSE_STACK_CONTENT_MODE_HTML:
			if (old_stack->object != NULL)
				free(old_stack->object);
			break;
		default:
			break;
		}

		parse_stack_pop();
		break;

	case XML_READER_TYPE_COMMENT:
		break;

	default:
		msg_report(MSG_UNEXPECTED_XML, type, "Inner Node");
		break;
	}
}

/**
 * Add a new placeholder chapter to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new chapter will be added.
 * \param type			The type of chapter object (Chapter or
 *				Index) to add.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_placeholder_chapter(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack, enum manual_data_object_type type)
{
	struct manual_data	*new_chapter;
	char*		filename = NULL;

	/* Create a new chapter structure. */

	new_chapter = manual_data_create(type);
	if (new_chapter == NULL)
		return false;

	/* Process any entities which are found. */

	filename = xmlTextReaderGetAttribute(reader, (const char *) "file");

	new_chapter->chapter.filename = filename_make(filename, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LOCAL);

	free(filename);

	/* Link the new item in to the document structure. */

	parse_link_to_chain(old_stack, new_chapter);

	return true;
}

/**
 * Add a new chapter to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new chapter will be added.
 * \param type			The type of chapter object (Chapter or
 *				Index) to add.
 * \param element		The XML element to close the object.
 * \param **chapter		Pointer to the current chapter, which can be
 *				 NULL if this is the root file
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_chapter(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element, struct manual_data *chapter)
{
	struct manual_data		*new_chapter;

	new_chapter = parse_create_new_stacked_object(PARSE_STACK_CONTENT_CHAPTER, element, type, chapter);
	if (new_chapter == NULL)
		return false;

	/* We've now processed the actual chapter data. */

	new_chapter->chapter.processed = true;

	/* Process any entities which are found. */

	new_chapter->id = xmlTextReaderGetAttribute(reader, (const char *) "id");

	/* If this isn't a pre-existing chapter, link the new item in to
	 * the document structure.
	 */

	if (chapter == NULL)
		parse_link_to_chain(old_stack, new_chapter);

	return true;
}

/**
 * Add a new section to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new section will be added.
 * \param type			The type of section object (Section) to add.
 * \param element		The XML element to close the object.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_section(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element)
{
	struct manual_data		*new_section;

	new_section = parse_create_new_stacked_object(PARSE_STACK_CONTENT_SECTION, element, type, NULL);
	if (new_section == NULL)
		return false;

	/* Process any entities which are found. */

	new_section->id = xmlTextReaderGetAttribute(reader, (const char *) "id");

	/* Link the new item in to the document structure. */

	parse_link_to_chain(old_stack, new_section);

	return true;
}

/**
 * Add a new resources block to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new section will be added.
 * \param element		The XML element to close the object.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_resources(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum parse_element_type element)
{
	enum parse_stack_content	content = PARSE_STACK_CONTENT_NONE;
	char				*type;

	switch (element) {
	case PARSE_ELEMENT_RESOURCES:
		content = PARSE_STACK_CONTENT_RESOURCES;
		break;
	case PARSE_ELEMENT_MODE:
		type = xmlTextReaderGetAttribute(reader, (const char *) "type");
		if (type == NULL) {
			msg_report(MSG_MISSING_ATTRIBUTE, "type");
			return false;
		}

		if (xmlStrcmp((const char *) "text", type) == 0) {
			content = PARSE_STACK_CONTENT_MODE_TEXT;
		} else if (xmlStrcmp((const char *) "html", type) == 0) {
			content = PARSE_STACK_CONTENT_MODE_HTML;
		} else if (xmlStrcmp((const char *) "strong", type) == 0) {
			content = PARSE_STACK_CONTENT_MODE_STRONG;
		} else {
			msg_report(MSG_UNKNOWN_MODE, type);
		}

		free(type);
		break;
	default:
		msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Resources");
		break;
	}

	if (content == PARSE_STACK_CONTENT_NONE)
		return false;

	parse_create_new_stacked_object(content, element, MANUAL_DATA_OBJECT_TYPE_NONE, NULL);

	/* The new object isn't linked to the chain, as we don't wish
	 * to include it in the document. At the end, we'll pick the
	 * stand-alone chain up and process it.
	 */

	return true;
}

/**
 * Collect a resource specifier from a block object, copying its contents
 * into a single string and freeing the components blocks.
 *
 * \param *old_stack		The parent stack entry pointing to the
 *				object to be collected.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_collect_resources(struct parse_stack_entry *old_stack, enum parse_stack_content content)
{
	struct parse_stack_entry	*parent_stack;
	struct manual_data		*object, *chunk, *previous;
	char				*c, *resource, *r;
	size_t				length = 0;
	struct manual_data_resources	*resources = NULL;

	/* Resource data is collected as a block object. */

	if (old_stack == NULL || old_stack->content != PARSE_STACK_CONTENT_BLOCK) {
		msg_report(MSG_BAD_RESOURCE_BLOCK);
		return false;
	}

	/* If there's no block object, we can't do anything. */

	object = old_stack->object;
	if (object == NULL) {
		msg_report(MSG_BAD_RESOURCE_BLOCK);
		return false;
	}

	/* We only wish to claim the following types. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_FILE:
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_FOLDER:
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_IMAGE:
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_DOWNLOAD:
		break;
	default:
		msg_report(MSG_BAD_RESOURCE_BLOCK);
		return false;
	}

	/* Find the combined length of the resource, and claim some memory. */

	chunk = object->first_child;

	while (chunk != NULL) {
		switch (chunk->type) {
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			c = chunk->chunk.text;
			if (c == NULL)
				break;

			while (*c++ != '\0')
				length++;
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			length++;
			break;
		default:
			msg_report(MSG_BAD_TYPE);
			break;
		}

		chunk = chunk->next;
	}

	resource = malloc(length + 1);
	if (resource == NULL)
		return false;

	/* Copy the resource content into the memory buffer, and free
	 * the objects as we go.
	 */

	r = resource;

	chunk = object->first_child;

	while (chunk != NULL) {
		switch (chunk->type) {
		case MANUAL_DATA_OBJECT_TYPE_TEXT:
			c = chunk->chunk.text;
			if (c == NULL)
				break;

			while (*c != '\0')
				*r++ = *c++;

			free(chunk->chunk.text);
			break;
		case MANUAL_DATA_OBJECT_TYPE_ENTITY:
			*r++ = '?';
			break;
		default:
			msg_report(MSG_BAD_TYPE);
			break;
		}

		previous = chunk;
		chunk = chunk->next;

		free(previous);
	}

	*r = '\0';

	/* Find the stack entry above the resources block, where the
	 * resource will be saved, and get the resource block.
	 */

	parent_stack = parse_stack_peek_content(PARSE_STACK_CONTENT_RESOURCES, PARSE_STACK_PARENT);

	if (parent_stack == NULL || parent_stack->object == NULL) {
		free(resource);
		return false;
	}

	resources = manual_data_get_resources(parent_stack->object);
	if (resources == NULL) {
		free(resource);
		return false;
	}

	/* Copy the resource into the block in the required location. */

	switch (object->type) {
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_FILE:
		switch (content) {
		case PARSE_STACK_CONTENT_MODE_TEXT:
			resources->text.filename = filename_make(resource, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LINUX);
			break;
		case PARSE_STACK_CONTENT_MODE_STRONG:
			resources->strong.filename = filename_make(resource, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LINUX);
			break;
		case PARSE_STACK_CONTENT_MODE_HTML:
			resources->html.filename = filename_make(resource, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LINUX);
			break;
		default:
			break;
		}
		break;
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_FOLDER:
		switch (content) {
		case PARSE_STACK_CONTENT_MODE_TEXT:
			resources->text.folder = filename_make(resource, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
			break;
		case PARSE_STACK_CONTENT_MODE_STRONG:
			resources->strong.folder = filename_make(resource, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
			break;
		case PARSE_STACK_CONTENT_MODE_HTML:
			resources->html.folder = filename_make(resource, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
			break;
		default:
			break;
		}
		break;
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_IMAGE:
		resources->images = filename_make(resource, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
		break;
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_DOWNLOAD:
		resources->downloads = filename_make(resource, FILENAME_TYPE_DIRECTORY, FILENAME_PLATFORM_LINUX);
		break;
	default:
		break;
	}

	/* Free the local copy of the name. */

	free(resource);

	return true;
}


/**
 * Add a new block to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new block will be added.
 * \param type			The type of block object to add.
 * \param element		The XML element to close the object.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_block(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element)
{
	struct manual_data		*new_block;
	struct manual_data_resources	*resources = NULL;

	new_block = parse_create_new_stacked_object(PARSE_STACK_CONTENT_BLOCK, element, type, NULL);
	if (new_block == NULL)
		return false;

	/* Link the new item in to the document structure. */

	switch (type) {
	case MANUAL_DATA_OBJECT_TYPE_PARAGRAPH:
	case MANUAL_DATA_OBJECT_TYPE_CITATION:
	case MANUAL_DATA_OBJECT_TYPE_CODE:
	case MANUAL_DATA_OBJECT_TYPE_USER_ENTRY:
	case MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS:
	case MANUAL_DATA_OBJECT_TYPE_FILENAME:
	case MANUAL_DATA_OBJECT_TYPE_ICON:
	case MANUAL_DATA_OBJECT_TYPE_KEY:
	case MANUAL_DATA_OBJECT_TYPE_MOUSE:
	case MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS:
	case MANUAL_DATA_OBJECT_TYPE_WINDOW:
		parse_link_to_chain(old_stack, new_block);
		break;
	case MANUAL_DATA_OBJECT_TYPE_TITLE:
		old_stack->object->title = new_block;
		break;
	case MANUAL_DATA_OBJECT_TYPE_SUMMARY:
		resources = manual_data_get_resources(old_stack->object);
		if (resources == NULL)
			return false;
		resources->summary = new_block;
		break;
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_IMAGE:
	case MANUAL_DATA_OBJECT_TYPE_RESOURCE_DOWNLOAD:
		old_stack->object = new_block;
		break;
	default:
		msg_report(MSG_UNEXPECTED_BLOCK_ADD, manual_data_find_object_name(type));
		break;
	}

	return true;
}

/**
 * Add a content node to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new block will be added.
 * \return			True on success; False on failure.
 */

static bool parse_process_add_content(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack)
{
	struct manual_data		*new_object;
	enum manual_data_object_type	object_type;
	const char			*value;
	xmlReaderTypes			node_type;

	/* Content can only be stored within block objects. */

	if (old_stack->content != PARSE_STACK_CONTENT_BLOCK) {
		msg_report(MSG_UNEXPECTED_CONTENT, old_stack->content);
		return true;
	}

	/* Create the new object. */

	node_type = xmlTextReaderNodeType(reader);

	switch (node_type) {
	case XML_READER_TYPE_TEXT:
	case XML_READER_TYPE_CDATA:
	case XML_READER_TYPE_WHITESPACE:
	case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
		object_type = MANUAL_DATA_OBJECT_TYPE_TEXT;
		break;

	case XML_READER_TYPE_ENTITY:
	case XML_READER_TYPE_ENTITY_REFERENCE:
		object_type = MANUAL_DATA_OBJECT_TYPE_ENTITY;
		break;

	default:
		msg_report(MSG_UNEXPECTED_XML, node_type, "Content Node");
		return false;
	}

	new_object = manual_data_create(object_type);
	if (new_object == NULL)
		return false;

	parse_link_to_chain(old_stack, new_object);

	switch (node_type) {
	case XML_READER_TYPE_TEXT:
	case XML_READER_TYPE_CDATA:
	case XML_READER_TYPE_WHITESPACE:
	case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
		value = xmlTextReaderConstValue(reader);
		if (value != NULL) {
			new_object->chunk.text = strdup(value);
			encoding_flatten_whitespace(new_object->chunk.text);
		}
		break;

	case XML_READER_TYPE_ENTITY:
	case XML_READER_TYPE_ENTITY_REFERENCE:
		new_object->chunk.entity = manual_entity_find_type(reader);
		break;

	default:
		msg_report(MSG_UNEXPECTED_XML, node_type, "Content Node");
		break;
	}

	return true;
}

/**
 * Create a new object which is pushed on to the stack.
 *
 * \param content		The type of stack content to push.
 * \param element		The type of element holding the object.
 * \param type			The required object type.
 * \param *object		Pointer to the object to push, or NULL
 *				to allocate one.
 * \return			Pointer to the newly created object, or
 *				NULL on failure.
 */

static struct manual_data *parse_create_new_stacked_object(enum parse_stack_content content,
		enum parse_element_type element, enum manual_data_object_type type,
		struct manual_data *object)
{
	struct parse_stack_entry	*stack = NULL;

	/* Any pre-existing object must have the correct type. */

	if (object != NULL && object->type != type) {
		msg_report(MSG_UNEXPECTED_PUSH, manual_data_find_object_name(object->type), manual_data_find_object_name(type));
		return NULL;
	}

	/* Push an entry on to the stack ready to be processed. */

	stack = parse_stack_push(content, element);
	if (stack == NULL)
		return NULL;

	/* Create the new object structure, if one wasn't supplied. */

	if (object == NULL && type != MANUAL_DATA_OBJECT_TYPE_NONE)
		object = manual_data_create(type);

	/* Link the object into the stack. */

	stack->object = object;

	return object;
}

/**
 * Link an object on to the end of an object chain held on the stack.
 *
 * \param *parent		The stack entry referencing the chain
 *				parent object.
 * \param *object		The object to be linked.
 */

static void parse_link_to_chain(struct parse_stack_entry *parent, struct manual_data *object)
{
	/* If the items are already linked, there's nothing to do. */

	if (parent->current_child == object)
		return;

	/* Link the new object on to the end of the chain. */

	if (parent->current_child == NULL)
		parent->object->first_child = object;
	else
		parent->current_child->next = object;

	parent->current_child = object;
}

/**
 * Handle errors reported by the XML Reader.
 *
 * \param *arg			Client data.
 * \param *msg			The error message to be reported.
 * \param severity		The severity of the error.
 * \param locator		The location of the error.
 */

static void parse_error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator)
{
	int	line;

	line = xmlTextReaderLocatorLineNumber(locator);

	printf("Error at line %d: %s", line, msg);
}
#endif
