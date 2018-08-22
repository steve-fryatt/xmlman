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

#include "encoding.h"
#include "filename.h"
#include "manual.h"
#include "msg.h"
#include "parse_element.h"
#include "parse_link.h"
#include "parse_stack.h"



/* Static Function Prototypes. */

static bool parse_file(struct filename *filename, struct manual_data **manual, struct manual_data **chapter);
static void parse_process_node(xmlTextReaderPtr reader, struct manual_data **manual, struct manual_data **chapter);
static void parse_process_outer_node(xmlTextReaderPtr reader, struct manual_data **manual);
static void parse_process_inner_node(xmlTextReaderPtr reader, struct manual_data **chapter);
static bool parse_process_add_placeholder_chapter(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack, enum manual_data_object_type type);
static bool parse_process_add_chapter(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element, struct manual_data *chapter);
static bool parse_process_add_section(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element);
static bool parse_process_add_resources(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element);
static bool parse_process_add_block(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element);
static bool parse_process_add_content(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack);
static struct manual_data *parse_create_new_stacked_object(enum parse_stack_content content,
		enum parse_element_type element, enum manual_data_object_type type,
		struct manual_data *object);
static void parse_link_to_chain(struct parse_stack_entry *parent, struct manual_data *object);
static void parse_error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);

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

	document_base = filename_make((xmlChar *) filename, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LOCAL);
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
		if (chapter->type != MANUAL_DATA_OBJECT_TYPE_CHAPTER && chapter->type != MANUAL_DATA_OBJECT_TYPE_INDEX) {
			msg_report(MSG_BAD_TYPE);
			return NULL;
		}

		if (!chapter->chapter.processed) {
			document_base = filename_up(document_root, 0);

			if (filename_add(document_base, chapter->chapter.filename, 0)); {
				filename_destroy(chapter->chapter.filename);
				chapter->chapter.filename = NULL;

				parse_file(document_base, &manual, &chapter);
			}

			filename_destroy(document_base);
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
	xmlTextReaderPtr	reader;
	char			*file = NULL;
	int			ret;

	file = filename_convert(filename, FILENAME_PLATFORM_LOCAL);

	if (file == NULL) {
		msg_report(MSG_FILE_MISSING);
		return false;
	}

	parse_stack_reset();

	reader = xmlReaderForFile(file, NULL, XML_PARSE_DTDATTR | XML_PARSE_DTDVALID);

	if (reader == NULL) {
		msg_report(MSG_OPEN_FAIL, file);
		free(file);
		return false;
	}

	xmlTextReaderSetErrorHandler(reader, parse_error_handler, NULL);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		parse_process_node(reader, manual, chapter);
		ret = xmlTextReaderRead(reader);
	}

	if (xmlTextReaderIsValid(reader) != 1)
		msg_report(MSG_INVALID, file);

	xmlFreeTextReader(reader);
	free(file);

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
 *			chapter pointer can be  NULL if this is the root
 *			file.
 */

static void parse_process_inner_node(xmlTextReaderPtr reader, struct manual_data **chapter)
{
	xmlReaderTypes			type;
	enum parse_element_type		element;
	struct parse_stack_entry	*old_stack;

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
			case PARSE_ELEMENT_RESOURCES:
				parse_process_add_resources(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_MANUAL, element);
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
			case PARSE_ELEMENT_SECTION:
				parse_process_add_section(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SECTION, element);
				break;
			case PARSE_ELEMENT_RESOURCES:
				parse_process_add_resources(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_CHAPTER, element);
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
			case PARSE_ELEMENT_SECTION:
				parse_process_add_section(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SECTION, element);
				break;
			case PARSE_ELEMENT_RESOURCES:
				parse_process_add_resources(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_SECTION, element);
				break;
			case PARSE_ELEMENT_PARAGRAPH:
				parse_process_add_block(reader, old_stack, MANUAL_DATA_OBJECT_TYPE_PARAGRAPH, element);
				break;
			default:
				msg_report(MSG_UNEXPECTED_NODE, parse_element_find_tag(element), "Section");
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
			printf("Unexpected closing element '<%s>' in block.\n", parse_element_find_tag(element));
			break;
		}

		parse_stack_pop();
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
	xmlChar*		filename = NULL;

	/* Create a new chapter structure. */

	new_chapter = manual_data_create(type);
	if (new_chapter == NULL) {
		fprintf(stderr, "Failed to create new chapter data.\n");
		return false;
	}

	/* Process any entities which are found. */

	filename = xmlTextReaderGetAttribute(reader, (const xmlChar *) "file");

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

	new_chapter->id = xmlTextReaderGetAttribute(reader, (const xmlChar *) "id");

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

	new_section->id = xmlTextReaderGetAttribute(reader, (const xmlChar *) "id");

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
 * \param type			The type of section object (Section) which
 *				will contain the resources structure..
 * \param element		The XML element to close the object.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_resources(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element)
{
	if (parse_create_new_stacked_object(PARSE_STACK_CONTENT_RESOURCES, element, type, old_stack->object) == NULL)
		return false;

	return true;
}

/**
 * Add a new block to the document structure.
 *
 * \param reader		The XML Reader to read the node from.
 * \param *old_stack		The parent stack entry, within which the
 *				new block will be added.
 * \param type			The type of block object (Paragraph or
 *				Title) to add.
 * \param element		The XML element to close the object.
 * \return			TRUE on success, or FALSE on failure.
 */

static bool parse_process_add_block(xmlTextReaderPtr reader, struct parse_stack_entry *old_stack,
		enum manual_data_object_type type, enum parse_element_type element)
{
	struct manual_data		*new_block;

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
	const xmlChar			*value;
	xmlReaderTypes			node_type;

	/* Content can only be stored within block objects. */

	if (old_stack->content != PARSE_STACK_CONTENT_BLOCK) {
		fprintf(stderr, "Unexpected content.\n");
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
		fprintf(stderr, "Unhandled object type\n");
		return false;
	}

	new_object = manual_data_create(object_type);
	if (new_object == NULL) {
		fprintf(stderr, "Failed to create new object data.\n");
		return false;
	}

	parse_link_to_chain(old_stack, new_object);

	switch (node_type) {
	case XML_READER_TYPE_TEXT:
	case XML_READER_TYPE_CDATA:
	case XML_READER_TYPE_WHITESPACE:
	case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
		value = xmlTextReaderConstValue(reader);
		if (value != NULL) {
			new_object->chunk.text = xmlStrdup(value);
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
		fprintf(stderr, "Mismatched pre-existing object type.\n");
		return NULL;
	}

	/* Push an entry on to the stack ready to be processed. */

	stack = parse_stack_push(content, element);
	if (stack == NULL)
		return NULL;

	/* Create the new object structure, if one wasn't supplied. */

	if (object == NULL) {
		object = manual_data_create(type);
		if (object == NULL) {
			fprintf(stderr, "Failed to create new object data.\n");
			return NULL;
		}
	}

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

