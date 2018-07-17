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
#include "msg.h"
#include "parse_element.h"
#include "parse_stack.h"



/* Static Function Prototypes. */

static bool parse_file(char *filename, struct manual_data **manual, struct manual_data_chapter **chapter);
static void parse_process_node(xmlTextReaderPtr reader, struct manual_data **manual, struct manual_data_chapter **chapter);
static void parse_process_outer_node(xmlTextReaderPtr reader, struct manual_data **manual);
static void parse_process_manual_node(xmlTextReaderPtr reader, struct manual_data_chapter **chapter);
static void parse_process_chapter_node(xmlTextReaderPtr reader);
static bool parse_process_add_chapter(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element, struct manual_data_chapter *chapter);
static bool parse_process_add_placeholder_chapter(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack, enum manual_data_object_type type);
static void parse_process_section_node(xmlTextReaderPtr reader);
static void parse_process_title_node(xmlTextReaderPtr reader);
static void parse_error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);


/**
 * Parse an XML file and its descendents.
 *
 * \param *filename	The name of the root file to parse.
 * \return		Pointer to the resulting manual structure.
 */

struct manual_data *parse_document(char *filename)
{
	struct manual_data		*manual = NULL;
	struct manual_data_chapter	*chapter = NULL;

	msg_initialise();

	/* Parse the root file. */

	parse_file(filename, &manual, &chapter);

	/* Parse any non-inlined chapter files. */

	chapter = manual->first_chapter;

	while (chapter != NULL) {
		if (!chapter->processed) {
			parse_file(chapter->filename, &manual, &chapter);
			chapter->processed = true;
		}

		chapter = chapter->next_chapter;
	}

	return manual;
} 

/**
 * Parse an XML file.
 *
 * \param *filename	The name of the file to parse.
 * \param **manual	Pointer to a pointer to the current manual. The
 *			manual pointer can be NULL if this is the root
 *			file.
 * \param **chapter	Pointer a pointer to the current chapter. The
 *			chapter pointer can be  NULL if this is the root
 *			file.
 */

static bool parse_file(char *filename, struct manual_data **manual, struct manual_data_chapter **chapter)
{
	xmlTextReaderPtr		reader;
	int				ret;

	if (filename == NULL) {
		msg_report(MSG_FILE_MISSING);
		return false;
	}

	parse_stack_reset();

	reader = xmlReaderForFile(filename, NULL, XML_PARSE_DTDATTR | XML_PARSE_DTDVALID);

	if (reader == NULL) {
		msg_report(MSG_OPEN_FAIL, filename);
		return false;
	}

	xmlTextReaderSetErrorHandler(reader, parse_error_handler, NULL);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		parse_process_node(reader, manual, chapter);
		ret = xmlTextReaderRead(reader);
	}

	if (xmlTextReaderIsValid(reader) != 1)
		msg_report(MSG_INVALID, filename);

	xmlFreeTextReader(reader);

	if (ret != 0)
		fprintf(stderr, "%s : failed to parse\n", filename);

	return true;
}


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

static void parse_process_node(xmlTextReaderPtr reader, struct manual_data **manual, struct manual_data_chapter **chapter)
{
	struct parse_stack_entry	*stack;

	stack = parse_stack_peek(PARSE_STACK_TOP);

	if (stack == NULL) {
		printf("Stack error in node.\n");
		return;
	}

	switch (stack->content) {
	case PARSE_STACK_CONTENT_NONE:
		parse_process_outer_node(reader, manual);
		break;
	case PARSE_STACK_CONTENT_MANUAL:
		parse_process_manual_node(reader, chapter);
		break;
	case PARSE_STACK_CONTENT_CHAPTER:
		parse_process_chapter_node(reader);
		break;
	case PARSE_STACK_CONTENT_SECTION:
		parse_process_section_node(reader);
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
	if (old_stack->content != PARSE_STACK_CONTENT_NONE) {
		fprintf(stderr, "Unexpected stack state!\n");
		return;
	}

	type = xmlTextReaderNodeType(reader);
	if (type != XML_READER_TYPE_ELEMENT)
		return;

	element = parse_element_find_type(reader);
	switch (element) {
	case PARSE_ELEMENT_MANUAL:
		printf("Found manual\n");
		new_stack = parse_stack_push(PARSE_STACK_CONTENT_MANUAL, element);
		if (new_stack == NULL) {
			fprintf(stderr, "Failed to allocate stack.\n");
			return NULL;
		}

		/* Create a new manual if this is the root file. */

		if (*manual == NULL)
			*manual = manual_data_create();

		/* Record the current manual on the stack. */

		new_stack->data.manual.manual = *manual;
		break;
	}
}


/**
 * Process nodes within the <manual> tag.
 *
 * \param reader	The XML Reader to read the node from.
 * \param **chapter	Pointer a pointer to the current chapter. The
 *			chapter pointer can be  NULL if this is the root
 *			file.
 */

static void parse_process_manual_node(xmlTextReaderPtr reader, struct manual_data_chapter **chapter)
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
		element = parse_element_find_type(reader);

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
			parse_process_add_chapter(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_INDEX, element, *chapter);
			break;
		case PARSE_ELEMENT_CHAPTER:
			if (xmlTextReaderIsEmptyElement(reader)) {
				printf("Found chapter placeholder ->\n");
				parse_process_add_placeholder_chapter(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_CHAPTER);
			} else {
				printf("Found chapter\n");
				parse_process_add_chapter(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_CHAPTER, element, *chapter);
			}
			break;
		}
		break;

	case XML_READER_TYPE_END_ELEMENT:
		element = parse_element_find_type(reader);

		if (element != old_stack->closing_element) {
			printf("Unexpected closing element '<%s>' in manual.\n", parse_element_find_tag(element));
			break;
		}

		printf("Closed element type <%s>\n", parse_element_find_tag(element));
		parse_stack_pop();
		break;
	}
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
		enum manual_data_object_type type, enum parse_element_type element, struct manual_data_chapter *chapter)
{
	struct manual_data_chapter	*new_chapter;
	struct parse_stack_entry	*new_stack;

	if (old_stack->content != PARSE_STACK_CONTENT_MANUAL) {
		fprintf(stderr, "Can only create new chapters in manuals.\n");
		return false;
	}

	/* Push the chapter on to the stack ready to be processed. */

	new_stack = parse_stack_push(PARSE_STACK_CONTENT_CHAPTER, element);
	if (new_stack == NULL) {
		fprintf(stderr, "Failed to allocate stack.\n");
		return false;
	}

	/* If this is a complete chapter that we're going to process from
	 * scratch, create a new chapter structure. Otherwise, just
	 * reference the pre-existing chapter from the place-holder.
	 */

	if (chapter == NULL) {
		new_chapter = manual_data_chapter_create(type);
		if (new_chapter == NULL) {
			fprintf(stderr, "Failed to create new chapter data.\n");
			return false;
		}

		new_chapter->processed = true;
	} else {
		new_chapter = chapter;
	}

	/* Process any entities which are found. */

	new_chapter->id = xmlTextReaderGetAttribute(reader, (const xmlChar *) "id");

	/* Store the chapter details on the stack. */

	new_stack->data.chapter.chapter = new_chapter;

	/* If this isn't a pre-existing chapter, link the new item in to
	 * the document structure.
	 */

	if (chapter == NULL) {
		if (old_stack->data.manual.current_chapter == NULL)
			old_stack->data.manual.manual->first_chapter = new_chapter;
		else
			old_stack->data.manual.current_chapter->next_chapter = new_chapter;

		old_stack->data.manual.current_chapter = new_chapter;
	}

	return true;
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
	struct manual_data_chapter	*new_chapter;

	if (old_stack->content != PARSE_STACK_CONTENT_MANUAL) {
		fprintf(stderr, "Can only create new chapters in manuals.\n");
		return false;
	}

	/* Create a new chapter structure. */

	new_chapter = manual_data_chapter_create(type);
	if (new_chapter == NULL) {
		fprintf(stderr, "Failed to create new chapter data.\n");
		return false;
	}

	/* Process any entities which are found. */

	new_chapter->filename = xmlTextReaderGetAttribute(reader, (const xmlChar *) "file");

	/* Link the new item in to the document structure. */

	if (old_stack->data.manual.current_chapter == NULL)
		old_stack->data.manual.manual->first_chapter = new_chapter;
	else
		old_stack->data.manual.current_chapter->next_chapter = new_chapter;

	old_stack->data.manual.current_chapter = new_chapter;

	return true;
}
static bool parse_process_add_section(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element);

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
		element = parse_element_find_type(reader);

		switch (element) {
		case PARSE_ELEMENT_TITLE:
			printf("Found title\n");
			new_stack = parse_stack_push(PARSE_STACK_CONTENT_TITLE, element);
			if (new_stack == NULL) {
				fprintf(stderr, "Failed to allocate stack.\n");
				break;
			}
			break;

		case PARSE_ELEMENT_SECTION:
			printf("Found section\n");
			parse_process_add_section(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SECTION, element);
			break;
		}
		break;

	case XML_READER_TYPE_END_ELEMENT:
		element = parse_element_find_type(reader);

		if (element != old_stack->closing_element) {
			printf("Unexpected closing element '<%s>' in chapter.\n", parse_element_find_tag(element));
			break;
		}

		printf("Closed element type <%s>\n", parse_element_find_tag(element));
		parse_stack_pop();
		break;
	}
}

/**
 * Add a new section to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new chapter will be added.
 * \param type			The type of chapter object (Chapter or
 *				Index) to add.
 * \param element		The XML element to close the object.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_section(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element)
{
	struct manual_data_section	*new_section;
	struct parse_stack_entry	*new_stack;

	if (old_stack->content != PARSE_STACK_CONTENT_CHAPTER) {
		fprintf(stderr, "Can only create new sections in chapters.\n");
		return false;
	}

	/* Push the section on to the stack ready to be processed. */

	new_stack = parse_stack_push(PARSE_STACK_CONTENT_SECTION, element);
	if (new_stack == NULL) {
		fprintf(stderr, "Failed to allocate stack.\n");
		return false;
	}

	/* Create the new section structure. */

	new_section = manual_data_section_create(type);
	if (new_section == NULL) {
		fprintf(stderr, "Failed to create new section data.\n");
		return false;
	}

	/* Process any entities which are found. */

	new_section->id = xmlTextReaderGetAttribute(reader, (const xmlChar *) "id");

	/* Store the section details on the stack. */

	new_stack->data.section.section = new_section;

	/* Link the new item in to the document structure. */

	if (old_stack->data.chapter.current_section == NULL)
		old_stack->data.chapter.chapter->first_section = new_section;
	else
		old_stack->data.chapter.current_section->next_section = new_section;

	old_stack->data.chapter.current_section = new_section;

	return true;
}

/**
 * Process nodes within a <section> tag.
 *
 * \param reader	The XML Reader to read the node from.
 */

static void parse_process_section_node(xmlTextReaderPtr reader)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack, *new_stack;

	old_stack = parse_stack_peek(PARSE_STACK_TOP);
	if (old_stack->content != PARSE_STACK_CONTENT_SECTION) {
		fprintf(stderr, "Unexpected stack state!\n");
		return;
	}

	type = xmlTextReaderNodeType(reader);
	switch (type) {
	case XML_READER_TYPE_ELEMENT:
		element = parse_element_find_type(reader);

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
		element = parse_element_find_type(reader);

		if (element != old_stack->closing_element) {
			printf("Unexpected closing element '<%s>' in section.\n", parse_element_find_tag(element));
			break;
		}

		printf("Closed element type <%s>\n", parse_element_find_tag(element));
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
	const xmlChar			*value;

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

		value = xmlTextReaderConstValue(reader);
		if (value == NULL)
			break;

		switch (parent_stack->content) {
		case PARSE_STACK_CONTENT_MANUAL:
			parent_stack->data.manual.manual->title = xmlStrdup(value);
			printf("Setting manual title\n");
			break;
		case PARSE_STACK_CONTENT_CHAPTER:
			parent_stack->data.chapter.chapter->title = xmlStrdup(value);
			printf("Setting chapter title\n");
			break;
		case PARSE_STACK_CONTENT_SECTION:
			parent_stack->data.section.section->title = xmlStrdup(value);
			printf("Setting section title\n");
			break;
		default:
			fprintf(stderr, "Unexpected <title> tag found.\n");
		}
		break;
	case XML_READER_TYPE_END_ELEMENT:
		element = parse_element_find_type(reader);

		if (element != old_stack->closing_element) {
			printf("Unexpected closing element '<%s>' in title.\n", parse_element_find_tag(element));
			break;
		}

		printf("Closed element type <%s>\n", parse_element_find_tag(element));
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

