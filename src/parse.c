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
 * \file parse.c
 *
 * XML Parser, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include <libxml/xmlreader.h>

#include "manual_data.h"
#include "parse_element.h"
#include "parse_stack.h"

/* Static Function Prototypes. */

static void parse_process_node(xmlTextReaderPtr reader);
static void parse_process_outer_node(xmlTextReaderPtr reader);
static void parse_process_manual_node(xmlTextReaderPtr reader);
static void parse_process_chapter_node(xmlTextReaderPtr reader);
static void parse_process_title_node(xmlTextReaderPtr reader);
static void parse_error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);


/**
 * Parse an XML file and its descendents.
 *
 * \param *filename	The name of the file to parse.
 */

struct parse_manual *parse_file(char *filename)
{
	xmlTextReaderPtr		reader;
	int				ret;

	parse_stack_reset();

	reader = xmlReaderForFile(filename, NULL, XML_PARSE_DTDATTR | XML_PARSE_DTDVALID);

	if (reader == NULL) {
		fprintf(stderr, "Unable to open %s\n", filename);
		return;
	}

	xmlTextReaderSetErrorHandler(reader, parse_error_handler, NULL);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		parse_process_node(reader);
		ret = xmlTextReaderRead(reader);
	}

	if (xmlTextReaderIsValid(reader) != 1)
		fprintf(stderr, "Document %s does not validate\n", filename);

	xmlFreeTextReader(reader);

	if (ret != 0)
		fprintf(stderr, "%s : failed to parse\n", filename);
}


/**
 * Process an XML node, passing it on to an appropriate part of the
 * parser based on the current position in the tree.
 *
 * \param reader	The XML Reader to read the node from.
 */

static void parse_process_node(xmlTextReaderPtr reader)
{
	struct parse_stack_entry	*stack;

	stack = parse_stack_peek(PARSE_STACK_TOP);

	if (stack == NULL) {
		printf("Stack error in node.\n");
		return;
	}

	switch (stack->content) {
	case PARSE_STACK_CONTENT_NONE:
		parse_process_outer_node(reader);
		break;
	case PARSE_STACK_CONTENT_MANUAL:
		parse_process_manual_node(reader);
		break;
	case PARSE_STACK_CONTENT_CHAPTER:
		parse_process_chapter_node(reader);
		break;
	case PARSE_STACK_CONTENT_TITLE:
		parse_process_title_node(reader);
		break;
	}
}


/**
 * Process nodes when we're outside the whole document structure.
 *
 * \param reader	The XML Reader to read the node from.
 */

static void parse_process_outer_node(xmlTextReaderPtr reader)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack, *new_stack;

	old_stack = parse_stack_peek(PARSE_STACK_TOP);
	if (old_stack->content != PARSE_STACK_CONTENT_NONE) {
		fprintf(stderr, "Unexpected stack state!\n");
		return;
	}

	type = xmlTextReaderNodeType(reader);
	if (type != XML_READER_TYPE_ELEMENT)
		return;

	element = parse_find_element_type(reader);
	switch (element) {
	case PARSE_ELEMENT_MANUAL:
		printf("Found manual\n");
		new_stack = parse_stack_push(PARSE_STACK_CONTENT_MANUAL, element);
		if (new_stack == NULL) {
			fprintf(stderr, "Failed to allocate stack.\n");
			return NULL;
		}

		new_stack->data.manual.manual = manual_data_create();
		new_stack->data.manual.current_chapter = NULL;
		break;
	}
}


/**
 * Process nodes within the <manual> tag.
 *
 * \param reader	The XML Reader to read the node from.
 */

static void parse_process_manual_node(xmlTextReaderPtr reader)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack, *new_stack;

	old_stack = parse_stack_peek(PARSE_STACK_TOP);
	if (old_stack->content != PARSE_STACK_CONTENT_MANUAL) {
		fprintf(stderr, "Unexpected stack state!\n");
		return;
	}

	type = xmlTextReaderNodeType(reader);
	switch (type) {
	case XML_READER_TYPE_ELEMENT:
		element = parse_find_element_type(reader);

		switch (element) {
		case PARSE_ELEMENT_TITLE:
			printf("Found title\n");
			new_stack = parse_stack_push(PARSE_STACK_CONTENT_TITLE, element);
			if (new_stack == NULL) {
				fprintf(stderr, "Failed to allocate stack.\n");
				break;
			}

			break;
		case PARSE_ELEMENT_INDEX:
			printf("Found index\n");
			new_stack = parse_stack_push(PARSE_STACK_CONTENT_CHAPTER, element);
			if (new_stack == NULL) {
				fprintf(stderr, "Failed to allocate stack.\n");
				break;
			}

			new_stack->data.chapter.chapter = manual_data_chapter_create(MANUAL_DATA_OBJECT_TYPE_INDEX);
			new_stack->data.chapter.current_section = NULL;
			break;
		case PARSE_ELEMENT_CHAPTER:
			if (xmlTextReaderIsEmptyElement(reader)) {
				printf ("Found chapter pointer ->\n");
			} else {
				printf("Found chapter\n");
				new_stack = parse_stack_push(PARSE_STACK_CONTENT_CHAPTER, element);
				if (new_stack == NULL) {
					fprintf(stderr, "Failed to allocate stack.\n");
					break;
				}

				new_stack->data.chapter.chapter = manual_data_chapter_create(MANUAL_DATA_OBJECT_TYPE_CHAPTER);
				new_stack->data.chapter.current_section = NULL;
			}
			break;
		}
		break;

	case XML_READER_TYPE_END_ELEMENT:
		element = parse_find_element_type(reader);

		if (element != old_stack->closing_element) {
			printf("Unexpected closing element\n");
			break;
		}

		printf("Closed element type %d\n", element);
		parse_stack_pop();
		break;
	}
}

/**
 * Process nodes within an <index> or <chapter> tag.
 *
 * \param reader	The XML Reader to read the node from.
 */

static void parse_process_chapter_node(xmlTextReaderPtr reader)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack, *new_stack;

	old_stack = parse_stack_peek(PARSE_STACK_TOP);
	if (old_stack->content != PARSE_STACK_CONTENT_CHAPTER) {
		fprintf(stderr, "Unexpected stack state!\n");
		return;
	}

	type = xmlTextReaderNodeType(reader);
	switch (type) {
	case XML_READER_TYPE_ELEMENT:
		element = parse_find_element_type(reader);

		switch (element) {
		case PARSE_ELEMENT_TITLE:
			printf("Found title\n");
			new_stack = parse_stack_push(PARSE_STACK_CONTENT_TITLE, element);
			if (new_stack == NULL) {
				fprintf(stderr, "Failed to allocate stack.\n");
				break;
			}

			break;
		}
		break;

	case XML_READER_TYPE_END_ELEMENT:
		element = parse_find_element_type(reader);

		if (element != old_stack->closing_element) {
			printf("Unexpected closing element\n");
			break;
		}

		printf("Closed element type %d\n", element);
		parse_stack_pop();
		break;
	}
}

/**
 * Process nodes within a <title> tag.
 *
 * \param reader	The XML Reader to read the node from.
 */

static void parse_process_title_node(xmlTextReaderPtr reader)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack, *parent_stack;

	old_stack = parse_stack_peek(PARSE_STACK_TOP);
	if (old_stack->content != PARSE_STACK_CONTENT_TITLE) {
		fprintf(stderr, "Unexpected stack state!\n");
		return;
	}

	type = xmlTextReaderNodeType(reader);
	switch (type) {
	case XML_READER_TYPE_TEXT:
	case XML_READER_TYPE_CDATA:
		parent_stack = parse_stack_peek(PARSE_STACK_PARENT);
		if (parent_stack == NULL) {
			fprintf(stderr, "Title parent not found.\n");
			break;
		}

		switch (parent_stack->content) {
		case PARSE_STACK_CONTENT_MANUAL:
			parent_stack->data.manual.manual->title = "Manual Title";
			printf("Setting manual title\n");
			break;
		case PARSE_STACK_CONTENT_CHAPTER:
			parent_stack->data.chapter.chapter->title = "Chapter Title";
			printf("Setting chapter title\n");
			break;
		default:
			fprintf(stderr, "Unexpected <title> tag found.\n");
		}
		break;
	case XML_READER_TYPE_END_ELEMENT:
		element = parse_find_element_type(reader);

		if (element != old_stack->closing_element) {
			printf("Unexpected closing element\n");
			break;
		}

		printf("Closed element type %d\n", element);
		parse_stack_pop();
		break;
	default:
		fprintf(stderr, "Unexpected data in <title>\n");
		break;
	}
}



/*
	const		xmlChar *name, *value;

	name = xmlTextReaderConstName(reader);
	if (name == NULL)
		name = BAD_CAST "--";


	value = xmlTextReaderConstValue(reader);


	printf("%d %d %s %d %d", 
		xmlTextReaderDepth(reader),
		type,
		name,
		xmlTextReaderIsEmptyElement(reader),
		xmlTextReaderHasValue(reader));

	if (value == NULL) {
		printf("\n");
	} else {
		if (xmlStrlen(value) > 40)
			printf(" %.40s...\n", value);
		else
			printf(" %s\n", value);
	}
*/



static void parse_error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator)
{
	int	line;

	line = xmlTextReaderLocatorLineNumber(locator);

	printf("Error at line %d: %s", line, msg);
}

