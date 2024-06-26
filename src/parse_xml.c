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
 * \file parse_xml.c
 *
 * XML Chunk Parser, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "manual_entity.h"
#include "msg.h"
#include "parse_element.h"
#include "parse_xml.h"

/**
 * The maximum tag or entity name length. */

#define PARSE_XML_MAX_NAME_LEN 64

/**
 * The maximum number of attributes allowed in a tag.
 */

#define PARSE_XML_MAX_ATTRIBUTES 10

/**
 * The maximum length of an attribute value which we process
 * in the parser (eg. boolean values.)
 */

#define PARSE_XML_MAX_ATTRIBUTE_VAL_LEN 64

/**
 * Structure to hold details of an attribute.
 */

struct parse_xml_attribute {
	char name[PARSE_XML_MAX_NAME_LEN];
	long start;
	long length;
	struct parse_xml_block *parser;
};

/**
 * Structure holding an instance of the parser.
 */

struct parse_xml_block {
	/**
	 * The handle of the file, or NULL.
	 */
	FILE *file;

	/**
	 * The current parser mode.
	 */
	enum parse_xml_result current_mode;

	/**
	 * The current file pointer.
	 */
	long file_pointer;

	/**
	 * The file pointer for the most recent line count, to avoid double
	 * counting new lines.
	 */
	long line_count_file_pointer;

	/**
	 * The end of file character for the instance.
	 */
	int eof;

	/**
	 * A count of the lines processed.
	 */
	int line_count;

	/**
	 * Buffer for the current element or entity name.
	 */
	char object_name[PARSE_XML_MAX_NAME_LEN];

	/**
	 * File pointer to the start of the current text block.
	 */
	long text_block_start;

	/**
	 * Size of the current text block.
	 */
	size_t text_block_length;

	/**
	 * The number of attributes in the current element.
	 */
	int attribute_count;

	/**
	 * The attributes for the current element.
	 */
	struct parse_xml_attribute attributes[PARSE_XML_MAX_ATTRIBUTES];
};

/* Static Function Prototypes. */

static struct parse_xml_block *parse_xml_initialise(void);
static struct parse_xml_attribute *parse_xml_find_attribute(struct parse_xml_block *instance, const char *name);
static size_t parse_xml_copy_text_to_buffer(struct parse_xml_block *instance, long start, size_t length, char *buffer, size_t size);
static void parse_xml_read_text(struct parse_xml_block *instance, int c);
static void parse_xml_read_markup(struct parse_xml_block *instance, int c);
static void parse_xml_read_comment(struct parse_xml_block *instance);
static void parse_xml_read_element(struct parse_xml_block *instance, int c);
static void parse_xml_read_element_attributes(struct parse_xml_block *instance, int c);
static void parse_xml_read_entity(struct parse_xml_block *instance, int c);
static bool parse_xml_match_ahead(struct parse_xml_block *instance, const char *text);
static int parse_xml_getc(struct parse_xml_block *instance);

/* Type tests. */

#define parse_xml_isspace(x) ((x) == '\t' || (x) == '\n' || (x) == '\r' || (x) == ' ')
#define parse_xml_isname_start(x) ((x) == ':' || (x) == '_' || ((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z'))
#define parse_xml_isname(x) (parse_xml_isname_start(x) || (x) == '-' || (x) == '.' || ((x) >= '0' && (x) <= '9'))

/**
 * Initialise the file parser for use.
 */

static struct parse_xml_block *parse_xml_initialise(void)
{
	struct parse_xml_block *new = NULL;

	new = malloc(sizeof(struct parse_xml_block));
	if (new == NULL)
		return NULL;

	new->current_mode = PARSE_XML_RESULT_ERROR;

	new->file_pointer = 0;
	new->line_count_file_pointer = 0;
	new->eof = EOF;
	new->line_count = 1;

	new->text_block_start = 0;
	new->text_block_length = 0;

	new->attribute_count = 0;
	new->file = NULL;

	return new;
}

/**
 * Open a new file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct parse_xml_block *parse_xml_open_file(char *filename)
{
	struct parse_xml_block *instance = NULL, *parser = NULL;
	int i;

	msg_set_location(NULL);

	instance = parse_xml_initialise();
	if (instance == NULL)
		return NULL;

	/* Open the file. */

	instance->file = fopen(filename, "rb");
	if (instance->file == NULL) {
		free(instance);
		return NULL;
	}

	msg_set_location(filename);

	/* Create parsers for the attributes. */

	for (i = 0; i < PARSE_XML_MAX_ATTRIBUTES; i++) {
		parser = parse_xml_initialise();
		if (parser != NULL) {
			parser->file = instance->file;
		}
		instance->attributes[i].parser = parser;
	}

	instance->current_mode = PARSE_XML_RESULT_START;

	return instance;
}


/**
 * Close a file in the XML parser.
 *
 * \param *instance	Pointer to the instance to be closed.
 * \return		True if successful; else False.
 */

void parse_xml_close_file(struct parse_xml_block *instance)
{
	int i;

	if (instance == NULL)
		return;
	
	/* Close the file. */

	if (instance->file != NULL)
		fclose(instance->file);

	/* Free the attribute parser instances. */

	for (i = 0; i < PARSE_XML_MAX_ATTRIBUTES; i++) {
		if (instance->attributes[i].parser != NULL)
			free(instance->attributes[i].parser);
	}

	free(instance);
}


/**
 * Set the parser state to error.
 *
 * \param *instance	Pointer to the instance to update.
 * \return		An error state.
 */

enum parse_xml_result parse_xml_set_error(struct parse_xml_block *instance)
{
	if (instance != NULL)
		instance->current_mode = PARSE_XML_RESULT_ERROR;

	msg_report(MSG_PARSER_SET_ERROR);

	return PARSE_XML_RESULT_ERROR;
}


/**
 * Parse the next chunk from the specified file.
 * 
 * \param *instance	Pointer to the instance to be read.
 * \return		Details of the chunk.
 */

enum parse_xml_result parse_xml_read_next_chunk(struct parse_xml_block *instance)
{
	int c;

	if (instance == NULL || instance->current_mode == PARSE_XML_RESULT_ERROR)
		return PARSE_XML_RESULT_ERROR;

	/* Start with the presumption of failure. */

	instance->current_mode = PARSE_XML_RESULT_ERROR;
	instance->attribute_count = 0;

	/* Exit on error or EOF. */

	if (instance->file == NULL)
		return PARSE_XML_RESULT_ERROR;

	fseek(instance->file, instance->file_pointer, SEEK_SET);

	if (feof(instance->file))
		return PARSE_XML_RESULT_EOF;

	/* Decide what to do based on the next character in the file. */

	c = parse_xml_getc(instance);

	if (c == EOF || c == instance->eof) {
		instance->current_mode = PARSE_XML_RESULT_EOF;
	} else {
		switch (c) {
		case '<':
			parse_xml_read_markup(instance, c);
			break;

		case '&':
			parse_xml_read_entity(instance, c);
			break;

		default:
			parse_xml_read_text(instance, c);
			break;
		}
	}

	instance->file_pointer = ftell(instance->file);

	return instance->current_mode;
}


/**
 * Return a copy of the current text block parsed from
 * the file.
 * 
 * \param *instance		Pointer to the instance to be used.
 * \return			Pointer to a copy of the block, or NULL.
 */

char *parse_xml_get_text(struct parse_xml_block *instance)
{
	char *text;

	if (instance == NULL || instance->file == NULL)
		return NULL;

	if (instance->current_mode != PARSE_XML_RESULT_TEXT &&
			instance->current_mode != PARSE_XML_RESULT_WHITESPACE)
		return NULL;

	text = malloc(instance->text_block_length + 1);
	if (text == NULL)
		return NULL;

	parse_xml_copy_text_to_buffer(instance, instance->text_block_start, instance->text_block_length,
			text, instance->text_block_length + 1);

	return text;
}


/**
 * Copy the current text block parsed from the file into a buffer
 * 
 * \param *instance		Pointer to the instance to be used.
 * \param *buffer		Pointer to a buffer to hold the value.
 * \param length		The size of the supplied buffer.
 * \return			The number of bytes copued into the buffer.
 */

size_t parse_xml_copy_text(struct parse_xml_block *instance, char *buffer, size_t length)
{
	/* Check that there's a return buffer. */

	if (buffer == NULL || length == 0)
		return 0;

	buffer[0] = '\0';

	if (instance == NULL || instance->file == NULL)
		return 0;

	if (instance->current_mode != PARSE_XML_RESULT_TEXT &&
			instance->current_mode != PARSE_XML_RESULT_WHITESPACE)
		return 0;

	return parse_xml_copy_text_to_buffer(instance, instance->text_block_start, instance->text_block_length, buffer, length);
}


/**
 * Read the details of the current element parsed from
 * the file.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \return		The element token, or PARSE_ELEMENT_NONE.
 */

enum parse_element_type parse_xml_get_element(struct parse_xml_block *instance)
{
	if (instance == NULL)
		return PARSE_ELEMENT_NONE;

	if (instance->current_mode != PARSE_XML_RESULT_TAG_START &&
			instance->current_mode != PARSE_XML_RESULT_TAG_EMPTY &&
			instance->current_mode != PARSE_XML_RESULT_TAG_END)
		return PARSE_ELEMENT_NONE;

	return parse_element_find_type(instance->object_name);
}


/**
 * Find a parser for a named attribute.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \return		A parser for the attribute value, or NULL.
 */

struct parse_xml_block *parse_xml_get_attribute_parser(struct parse_xml_block *instance, const char *name)
{
	struct parse_xml_attribute *attribute;

	attribute = parse_xml_find_attribute(instance, name);
	if (attribute == NULL)
		return NULL;

	return attribute->parser;
}


/**
 * Return a copy of the text from an attribute, without considering
 * the validity of any of the characters within.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \return		Pointer to a copy of the text, or NULL.
 */

char *parse_xml_get_attribute_text(struct parse_xml_block *instance, const char *name)
{
	struct parse_xml_attribute *attribute;
	char *text;

	if (instance == NULL || instance->file == NULL)
		return NULL;

	attribute = parse_xml_find_attribute(instance, name);
	if (attribute == NULL)
		return NULL;

	/* Allocate a buffer and copy the text. */

	text = malloc(attribute->length + 1);
	if (text == NULL)
		return NULL;

	parse_xml_copy_text_to_buffer(instance, attribute->start, attribute->length, text, attribute->length + 1);

	return text;
}


/**
 * Copy the text from an attribute into a buffer, without
 * considering the validity of any characters within.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \param *buffer	Pointer to a buffer to hold the value.
 * \param length	The size of the supplied buffer.
 * \return		The number of bytes copied into the buffer.
 */

size_t parse_xml_copy_attribute_text(struct parse_xml_block *instance, const char *name, char *buffer, size_t length)
{
	struct parse_xml_attribute *attribute;

	/* Check that there's a return buffer. */

	if (buffer == NULL || length == 0)
		return 0;

	buffer[0] = '\0';

	if (instance == NULL || instance->file == NULL)
		return 0;

	attribute = parse_xml_find_attribute(instance, name);
	if (attribute == NULL)
		return 0;

	return parse_xml_copy_text_to_buffer(instance, attribute->start, attribute->length, buffer, length);
}

/**
 * Parse an attribute as if it is a boolean value; not present is false.
 * 
 * Errors result in PARSE_XML_RESULT_ERROR being set.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \param *value_true	The value which is considered true.
 * \param *value_false	The value which is considered false.
 * \return		TRUE or FALSE.
 */

bool parse_xml_test_boolean_attribute(struct parse_xml_block *instance, const char *name, char *value_true, char *value_false)
{
	struct parse_xml_attribute *attribute;
	char buffer[PARSE_XML_MAX_ATTRIBUTE_VAL_LEN];

	if (instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return false;
	}

	attribute = parse_xml_find_attribute(instance, name);
	if (attribute == NULL)
		return false;

	parse_xml_copy_text_to_buffer(instance, attribute->start, attribute->length, buffer, PARSE_XML_MAX_ATTRIBUTE_VAL_LEN);

	if (strcmp(buffer, value_true) == 0)
		return true;
	else if (strcmp(buffer, value_false) == 0)
		return false;

	msg_report(MSG_BAD_ATTRIBUTE_VALUE, buffer, attribute->name);
	instance->current_mode = PARSE_XML_RESULT_ERROR;

	return false;
}

/**
 * Parse an attribute as if it is an integer value; not present will return
 * the default value.
 * 
 * Errors result in PARSE_XML_RESULT_ERROR being set.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \param deflt		The value which should be returned by default.
 * \param minimum	The minimum acceptable value.
 * \param maximum	The maximum acceptable value.
 * \return		The value read.
 */

int parse_xml_read_integer_attribute(struct parse_xml_block *instance, const char *name, int deflt, int minimum, int maxumum)
{
	char *endptr = NULL;
	long value;

	struct parse_xml_attribute *attribute;
	char buffer[PARSE_XML_MAX_ATTRIBUTE_VAL_LEN];

	if (instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return deflt;
	}

	attribute = parse_xml_find_attribute(instance, name);
	if (attribute == NULL)
		return deflt;

	parse_xml_copy_text_to_buffer(instance, attribute->start, attribute->length, buffer, PARSE_XML_MAX_ATTRIBUTE_VAL_LEN);

	value = strtol(buffer, &endptr, 10);

	/* Did the value parse OK? */

	if (*endptr != '\0') {
		msg_report(MSG_BAD_ATTRIBUTE_VALUE, buffer, attribute->name);
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return deflt;
	}

	/* Is the value in bounds? */

	if (value < minimum || value > maxumum) {
		msg_report(MSG_BAD_ATTRIBUTE_VALUE, buffer, attribute->name);
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return deflt;
	}

	return value & 0xffffffff;
}

/**
 * Parse an attribute for one of a set of possible values, returning
 * the index into the set, or -1 if not present.
 * 
 * Errors and invalid values result in PARSE_XML_RESULT_ERROR being set.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \param count		The number of possible values supplied.
 * \param ...		The possible values as pointers to strings.
 * \return		The value read.
 */

int parse_xml_read_option_attribute(struct parse_xml_block *instance, const char *name, int count, ...)
{
	va_list ap;
	int i, index = -1;
	struct parse_xml_attribute *attribute;
	char buffer[PARSE_XML_MAX_ATTRIBUTE_VAL_LEN], *pattern;

	if (instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return -1;
	}

	attribute = parse_xml_find_attribute(instance, name);
	if (attribute == NULL)
		return -1;

	parse_xml_copy_text_to_buffer(instance, attribute->start, attribute->length, buffer, PARSE_XML_MAX_ATTRIBUTE_VAL_LEN);

	va_start(ap, count);

	for (i = 0; i < count && index == -1; i++) {
		pattern = va_arg(ap, char *);

		if (pattern != NULL && strcmp(pattern, buffer) == 0)
			index = i;
	}

	va_end(ap);

	/* We need to find a match if attribute is present. */

	if (index == -1) {
		msg_report(MSG_BAD_ATTRIBUTE_VALUE, buffer, attribute->name);
		instance->current_mode = PARSE_XML_RESULT_ERROR;
	}

	return index;
}

/**
 * Locate an attribute for the current entity.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the required entity.
 * \return		Pointer to the attribute block, or NULL.
 */

static struct parse_xml_attribute *parse_xml_find_attribute(struct parse_xml_block *instance, const char *name)
{
	int i;

	if (instance == NULL)
		return NULL;

	if (instance->current_mode != PARSE_XML_RESULT_TAG_START &&
			instance->current_mode != PARSE_XML_RESULT_TAG_EMPTY)
		return NULL;

	for (i = 0; i < instance->attribute_count; i++) {
		if (strcmp(instance->attributes[i].name, name) == 0)
			return instance->attributes + i;
	}

	return NULL;
}


/**
 * Read the details of the current entity parsed from
 * the file.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \return		The entity token, or MANUAL_ENTITY_NONE.
 */

enum manual_entity_type parse_xml_get_entity(struct parse_xml_block *instance)
{
	if (instance == NULL)
		return MANUAL_ENTITY_NONE;

	if (instance->current_mode != PARSE_XML_RESULT_TAG_ENTITY)
		return MANUAL_ENTITY_NONE;
	
	return manual_entity_find_type(instance->object_name);
}


/**
 * Copy a chunk of text from the file into a buffer, converting line
 * endings according to the XML spec as we go.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param start		The pointer to where the text starts in the file.
 * \param length	The number of characters to read.
 * \param *buffer	Pointer to a buffer to take the text.
 * \param size		The size of the supplied buffer.
 * \return		The number of bytes copied into the output buffer.
 */

static size_t parse_xml_copy_text_to_buffer(struct parse_xml_block *instance, long start, size_t length, char *buffer, size_t size)
{
	long i, j;
	int c;
	bool last_cr = false;

	if (buffer == NULL || size == 0)
		return 0;

	/* Ensure a terminated buffer, if nothing else. */

	buffer[0] = '\0';

	if (instance == NULL)
		return 0;

	/* Find the start of the text to copy. */

	fseek(instance->file, start, SEEK_SET);

	/* Copy the text from the file to the buffer, converting \r and \r\n into \n. */

	for (i = 0, j = 0; i < length && j < (size - 1); i++) {
		c = parse_xml_getc(instance);

		if (c == instance->eof)
			break;
		
		if (c == '\r') {
			buffer[j++] = '\n';
			last_cr = true;
		} else if (c == '\n') {
			if (!last_cr)
				buffer[j++] = '\n';
			last_cr = false;
		} else {
			buffer[j++] = c;
			last_cr = false;
		}
	}

	buffer[j] = '\0';

	/* Restore the file pointer to where it came from. */

	fseek(instance->file, instance->file_pointer, SEEK_SET);

	return j;
}


/**
 * Process a text block from the file.
 *
 * \param *instance	The parser instance to use.
 * \param c		The first character of the markup sequence.
 */

static void parse_xml_read_text(struct parse_xml_block *instance, int c)
{
	bool whitespace = true;

	if (instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Count the size of the text block. */

	instance->text_block_start = ftell(instance->file) - 1;
	instance->text_block_length = 0;

	while (c != instance->eof && c != '<' && c != '&') {
		instance->text_block_length++;
		if (!parse_xml_isspace(c))
			whitespace = false;

		c = parse_xml_getc(instance);
	}

	/* Return the last character to the file. */

	if (c != EOF)
		fseek(instance->file, -1, SEEK_CUR);

	/* Update the status. */

	if (whitespace == true) {
		instance->current_mode = PARSE_XML_RESULT_WHITESPACE;
		msg_report(MSG_PARSER_FOUND_WHITESPACE);
	} else {
		instance->current_mode = PARSE_XML_RESULT_TEXT;
		msg_report(MSG_PARSER_FOUND_TEXT);
	}
}


/**
 * Process a markup block from the file.
 *
 * \param *instance	The parser instance to use.
 * \param c		The first character of the markup sequence.
 */

static void parse_xml_read_markup(struct parse_xml_block *instance, int c)
{
	/* Tags must start with a <; we shouldn't be here otherwise. */

	if (c != '<' || instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Check for special characters at the start of the tag. There's no
	 * need to test for EOF, as we'll fall through to the open tag handler
	 * later on.
	 */

	c = parse_xml_getc(instance);

	if (c == '!') {
		if (parse_xml_match_ahead(instance, "--")) {
			parse_xml_read_comment(instance);
			return;
		} else {
			/* TODO CDATA, DOCTYPE, etc. */

			instance->current_mode = PARSE_XML_RESULT_OTHER;
		}
	} else if (c == '?') {
		/* TODO Processing Instruction */
		instance->current_mode = PARSE_XML_RESULT_OTHER;
	} else {
		parse_xml_read_element(instance, c);
	}
}


/**
 * Process an element block from the file.
 * 
 * \param *instance	The parser instance to use.
 * \param c		The first character of the tag sequence.
 */

static void parse_xml_read_element(struct parse_xml_block *instance, int c)
{
	int len = 0;

	if (instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Assume an opening tag until we learn otherwise. */

	instance->current_mode = PARSE_XML_RESULT_TAG_START;

	/* If the tag starts with a /, it's a closing tag. */

	if (c == '/') {
		instance->current_mode = PARSE_XML_RESULT_TAG_END;
		c = parse_xml_getc(instance);
	}

	/* Copy the tag name until there's whitespace or a /. */

	if (c != instance->eof && parse_xml_isname_start(c)) {
		instance->object_name[len++] = c;

		c = parse_xml_getc(instance);

		while (c != instance->eof && parse_xml_isname(c)) {
			if (c != instance->eof && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				instance->object_name[len++] = c;

			c = parse_xml_getc(instance);
		}
	}

	instance->object_name[PARSE_XML_MAX_NAME_LEN - 1] = '\0';

	/* The tag wasn't terminated. */

	if (c == instance->eof) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_TAG, instance->object_name);
		return;
	}

	/* The name is too long. */

	if (len >= PARSE_XML_MAX_NAME_LEN) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_TAG_TOO_LONG, instance->object_name);
		return;
	}

	/* Terminate the valid name. */

	instance->object_name[len] = '\0';

	/* Read the attributes. */

	parse_xml_read_element_attributes(instance, c);
	if (instance->current_mode == PARSE_XML_RESULT_ERROR)
		return;

	/* The tag wasn't terminated. */

	if (c == instance->eof) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_TAG, instance->object_name);
		return;
	}

	/* If the tag ended with a /, it was a self-closing tag. */

	fseek(instance->file, -2, SEEK_CUR);
	c = parse_xml_getc(instance);

	if (c == '/') {
		if (instance->current_mode == PARSE_XML_RESULT_TAG_START) {
			instance->current_mode = PARSE_XML_RESULT_TAG_EMPTY;
		} else {
			instance->current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_TAG_CLOSE_CONFLICT, instance->object_name);
		}
	}

	/* We should be looking at a > now. */

	c = parse_xml_getc(instance);

	if (c != '>') {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_TAG_END_NOT_FOUND, c, instance->object_name);
		return;
	}

	/* Log what we found. */

	switch (instance->current_mode) {
	case PARSE_XML_RESULT_TAG_START:
		msg_report(MSG_PARSER_FOUND_OPENING_TAG, instance->object_name);
		break;
	case PARSE_XML_RESULT_TAG_EMPTY:
		msg_report(MSG_PARSER_FOUND_SELF_CLOSING_TAG, instance->object_name);
		break;
	case PARSE_XML_RESULT_TAG_END:
		msg_report(MSG_PARSER_FOUND_CLOSING_TAG, instance->object_name);
		break;
	default:
		break;
	}
}


/**
 * Process any attributes attached to the current elememt.
 *
 * \param *instance	The parser instance to use.
 * \param c		The first character of the tag sequence.
 */

static void parse_xml_read_element_attributes(struct parse_xml_block *instance, int c)
{
	int len = 0;
	long start = -1, length = 0;
	char name[PARSE_XML_MAX_NAME_LEN], quote = '\0';

	if (instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Process the file until we reach the closing > or EOF. */

	while (c != instance->eof && c != '>') {
		len = 0;

		/* Look for the start of an attribute name. */

		while (c != instance->eof && c != '>' && !parse_xml_isname_start(c))
			c = parse_xml_getc(instance);

		if (c == instance->eof || c == '>')
			continue;

		/* We've found an attribute name, so copy it into the temp buffer. */

		name[len++] = c;

		c = parse_xml_getc(instance);

		do {
			if (c != instance->eof && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				name[len++] = c;

			c = parse_xml_getc(instance);
		} while (c != instance->eof && parse_xml_isname(c));

		name[PARSE_XML_MAX_NAME_LEN - 1] = '\0';

		/* The name is too long. */

		if (len >= PARSE_XML_MAX_NAME_LEN) {
			instance->current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_ATTRIBUTE_TOO_LONG, name);
			return;
		}

		name[len] = '\0';

		/* Skip any whitespace after the name. */

		while (c != instance->eof && isspace(c))
			c = parse_xml_getc(instance);

		/* There's a value with the attribute. */

		if (c == '=') {
			c = parse_xml_getc(instance);

			/* Skip any whitespace after the = sign. */

			while (c != instance->eof && isspace(c))
				c = parse_xml_getc(instance);

			/* What follows must be quoted in " or '. */

			if (c == '\'' || c == '"') {
				quote = c;
				start = ftell(instance->file);

				/* Step through the data, and find the length. */

				c = parse_xml_getc(instance);

				while (c != instance->eof && c != quote)
					c = parse_xml_getc(instance);

				length = ftell(instance->file) - (start + 1);

				if (c != quote) {
					instance->current_mode = PARSE_XML_RESULT_ERROR;
					msg_report(MSG_PARSE_UNTERMINATED_ATTRIBUTE, name);
					return;
				}

				c = parse_xml_getc(instance);
			}
		} else {
			start = -1;
			length = 0;
		}

		/* Check that there's room for the attribute. */

		if (instance->attribute_count > PARSE_XML_MAX_ATTRIBUTES) {
			instance->current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_TOO_MANY_ATTRIBUTES);
			return;
		}

		strncpy(instance->attributes[instance->attribute_count].name, name, PARSE_XML_MAX_NAME_LEN);
		instance->attributes[instance->attribute_count].start = start;
		instance->attributes[instance->attribute_count].length = length;
		instance->attributes[instance->attribute_count].parser->current_mode = PARSE_XML_RESULT_START;
		instance->attributes[instance->attribute_count].parser->file_pointer = start;
		instance->attributes[instance->attribute_count].parser->eof = quote;

		instance->attribute_count++;
	}

	return;
}

/**
 * Process a comment sequence from the file.
 *
 * \param *instance	The parser instance to use.
 */

static void parse_xml_read_comment(struct parse_xml_block *instance)
{
	int c, dashes = 0;

	if (instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	do {
		c = parse_xml_getc(instance);

		if (c == '-')
			dashes++;
		else if (dashes == 2 && c == '>')
			break;
		else
			dashes = 0;
	} while (c != instance->eof);

	if (c == instance->eof) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_COMMENT);
		return;
	}

	instance->current_mode = PARSE_XML_RESULT_COMMENT;

	msg_report(MSG_PARSER_FOUND_COMMENT);
}


/**
 * Process an entity from the file.
 * 
 * \param *instance	The parser instance to use.
 * \param c		The first character of the entity sequence.
 */

static void parse_xml_read_entity(struct parse_xml_block *instance, int c)
{
	int len = 0;

	/* Entities must start with &; we shouldn't be here otherwise! */

	if (c != '&' || instance == NULL || instance->file == NULL) {
		if (instance != NULL)
			instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	c = parse_xml_getc(instance);

	/* Copy the entity name until it's terminated or there's whitespace. */

	if (c != instance->eof && parse_xml_isname_start(c)) {
		instance->object_name[len++] = c;

		c = parse_xml_getc(instance);

		while (c != instance->eof && parse_xml_isname(c)) {
			if (c != instance->eof && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				instance->object_name[len++] = c;

			c = parse_xml_getc(instance);
		}
	}

	instance->object_name[PARSE_XML_MAX_NAME_LEN - 1] = '\0';

	/* If the character isn't ;, the entity wasn't terminated. */

	if (c != ';') {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_ENTITY, instance->object_name);
		return;
	}

	/* The name is too long. */

	if (len >= PARSE_XML_MAX_NAME_LEN) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_ENTITY_TOO_LONG, instance->object_name);
		return;
	}

	/* Terminate the valid name. */

	instance->object_name[len] = '\0';

	instance->current_mode = PARSE_XML_RESULT_TAG_ENTITY;

	msg_report(MSG_PARSER_FOUND_ENTITY, instance->object_name);
}


/**
 * Test the next data in the file against a string. Leave the file
 * pointer after a match, or reset it if one isn't found.
 * 
 * \param *instance	The parser instance to use.
 * \param *text		Pointer to the string to match.
 * \return		True if the string matches; else false.
 */

static bool parse_xml_match_ahead(struct parse_xml_block *instance, const char *text)
{
	int c;
	long start;

	if (text == NULL || instance == NULL || instance->file == NULL)
		return false;

	/* Remember where we started. */

	start = ftell(instance->file);

	/* Match through the required string. */

	while (*text != '\0') {
		c = parse_xml_getc(instance);

		if (c == instance->eof || c != *text)
			break;

		text++;
	}

	/* If there wasn't a match, reset the file pointer. */

	if (*text != '\0')
		fseek(instance->file, start, SEEK_SET);

	return (*text == '\0') ? true : false;
}

/**
 * Get the next character from the file, updating the line count if
 * necessary.
 * 
 * \param *instance	The parser instance to use.
 * \return		The next character read from the file.
 */

static int parse_xml_getc(struct parse_xml_block *instance)
{
	int c;
	long fp;

	if (instance == NULL || instance->file == NULL)
		return instance->eof;

	c = fgetc(instance->file);

	fp = ftell(instance->file);

	if (c == '\n' && fp > instance->line_count_file_pointer) {
		msg_set_line(++(instance->line_count));
		instance->line_count_file_pointer = fp;
	}

	return c;
}

/**
 * Given an XML result code, return a human-readbale name.
 * 
 * \param result	The result code to look up.
 * \return		Pointer to a name for the result.
 */

char *parse_xml_get_result_name(enum parse_xml_result result)
{
	switch (result) {
	case PARSE_XML_RESULT_START:
		return "Start";
	case PARSE_XML_RESULT_ERROR:
		return "Error";
	case PARSE_XML_RESULT_EOF:
		return "EOF";
	case PARSE_XML_RESULT_TAG_START:
		return "Tag Start";
	case PARSE_XML_RESULT_TAG_END:
		return "Tag End";
	case PARSE_XML_RESULT_TAG_EMPTY:
		return "Tag Empty";
	case PARSE_XML_RESULT_TAG_ENTITY:
		return "Tag Entity";
	case PARSE_XML_RESULT_TEXT:
		return "Text";
	case PARSE_XML_RESULT_WHITESPACE:
		return "White Space";
	case PARSE_XML_RESULT_COMMENT:
		return "Comment";
	case PARSE_XML_RESULT_OTHER:
		return "Other";
	default:
		return "Unknown Result";
	}
}
