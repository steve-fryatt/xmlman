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
 * \file parse_xml.c
 *
 * XML Chunk Parser, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "manual_entity.h"
#include "msg.h"
#include "parse_element.h"
#include "parse_xml.h"

#define PARSE_XML_DEBUG

/**
 * The maximum tag or entity name length. */

#define PARSE_XML_MAX_NAME_LEN 64

/**
 * The maximum number of attributes allowed in a tag.
 */

#define PARSE_XML_MAX_ATTRIBUTES 10

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
	 * The end of file character for the instance.
	 */
	int eof;

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
static void parse_copy_text_to_buffer(struct parse_xml_block *instance, long start, size_t length, char *buffer, size_t size);
static void parse_xml_read_text(struct parse_xml_block *instance, char c);
static void parse_xml_read_markup(struct parse_xml_block *instance, char c);
static void parse_xml_read_comment(struct parse_xml_block *instance);
static void parse_xml_read_element(struct parse_xml_block *instance, char c);
static void parse_xml_read_element_attributes(struct parse_xml_block *instance, char c);
static void parse_xml_read_entity(struct parse_xml_block *instance, char c);
static bool parse_xml_match_ahead(struct parse_xml_block *instance, const char *text);

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
	new->eof = EOF;

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

	instance = parse_xml_initialise();
	if (instance == NULL)
		return NULL;

	/* Open the file. */

	instance->file = fopen(filename, "rb");
	if (instance->file == NULL) {
		free(instance);
		return NULL;
	}

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

	printf("# Set Error!\n");

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

	c = fgetc(instance->file);

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
 * \param retain_whitespace	True to retain all whitespace characters.
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

	parse_copy_text_to_buffer(instance, instance->text_block_start, instance->text_block_length,
			text, instance->text_block_length + 1);

	return text;
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
 * Read the details of the current element parsed from
 * the file.
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
 * Copy the text from an attribute into a buffer, without
 * considering the validity of any characters within.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \param *buffer	Pointer to a buffer to hold the value.
 * \param length	The size of the supplied buffer.
 * \return		True if successful; otherwise false.
 */

bool parse_xml_copy_attribute_text(struct parse_xml_block *instance, const char *name, char *buffer, size_t length)
{
	struct parse_xml_attribute *attribute;

	/* Check that there's a return buffer. */

	if (buffer == NULL || length == 0)
		return false;

	buffer[0] = '\0';

	if (instance == NULL || instance->file == NULL)
		return false;

	attribute = parse_xml_find_attribute(instance, name);
	if (attribute == NULL)
		return false;

	parse_copy_text_to_buffer(instance, attribute->start, attribute->length, buffer, length);

	return true;
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
		printf("Compare '%s' to '%s'\n", instance->attributes[i].name, name);
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
 */

static void parse_copy_text_to_buffer(struct parse_xml_block *instance, long start, size_t length, char *buffer, size_t size)
{
	long i, j;
	int c;
	bool last_cr = false;

	if (buffer == NULL || size == 0)
		return;

	/* Ensure a terminated buffer, if nothing else. */

	buffer[0] = '\0';

	if (instance == NULL)
		return;

	/* Find the start of the text to copy. */

	fseek(instance->file, start, SEEK_SET);

	/* Copy the text from the file to the buffer, converting \r and \r\n into \n. */

	for (i = 0, j = 0; i < length && j < (size - 1); i++) {
		c = fgetc(instance->file);

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
}


/**
 * Process a text block from the file.
 *
 * \param *instance	The parser instance to use.
 * \param c		The first character of the markup sequence.
 */

static void parse_xml_read_text(struct parse_xml_block *instance, char c)
{
	bool whitespace = true;

	if (instance == NULL || instance->file == NULL) {
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

		c = fgetc(instance->file);
	}

	/* Return the last character to the file. */

	if (c != EOF)
		fseek(instance->file, -1, SEEK_CUR);

	instance->current_mode = (whitespace == true) ? PARSE_XML_RESULT_WHITESPACE : PARSE_XML_RESULT_TEXT;

#ifdef PARSE_XML_DEBUG
	switch (instance->current_mode) {
	case PARSE_XML_RESULT_TEXT:
		printf("# Text\n");
		break;
	case PARSE_XML_RESULT_WHITESPACE:
		printf("# Whitespace\n");
		break;
	default:
		break;
	}
#endif
}


/**
 * Process a markup block from the file.
 *
 * \param *instance	The parser instance to use.
 * \param c		The first character of the markup sequence.
 */

static void parse_xml_read_markup(struct parse_xml_block *instance, char c)
{
	/* Tags must start with a <; we shouldn't be here otherwise. */

	if (c != '<' || instance == NULL || instance->file == NULL) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Check for special characters at the start of the tag. There's no
	 * need to test for EOF, as we'll fall through to the open tag handler
	 * later on.
	 */

	c = fgetc(instance->file);

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

static void parse_xml_read_element(struct parse_xml_block *instance, char c)
{
	int len = 0;

	if (instance == NULL || instance->file == NULL) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Assume an opening tag until we learn otherwise. */

	instance->current_mode = PARSE_XML_RESULT_TAG_START;

	/* If the tag starts with a /, it's a closing tag. */

	if (c == '/') {
		instance->current_mode = PARSE_XML_RESULT_TAG_END;
		c = fgetc(instance->file);
	}

	/* Copy the tag name until there's whitespace or a /. */

	if (c != instance->eof && parse_xml_isname_start(c)) {
		instance->object_name[len++] = c;

		c = fgetc(instance->file);

		while (c != instance->eof && parse_xml_isname(c)) {
			if (c != instance->eof && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				instance->object_name[len++] = c;

			c = fgetc(instance->file);
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
	c = fgetc(instance->file);

	if (c == '/') {
		if (instance->current_mode == PARSE_XML_RESULT_TAG_START) {
			instance->current_mode = PARSE_XML_RESULT_TAG_EMPTY;
		} else {
			instance->current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_TAG_CLOSE_CONFLICT, instance->object_name);
		}
	}

	/* We should be looking at a > now. */

	c = fgetc(instance->file);

	if (c != '>') {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_TAG_END_NOT_FOUND, c, instance->object_name);
		return;
	}

#ifdef PARSE_XML_DEBUG
	{
		int i;

		switch (instance->current_mode) {
		case PARSE_XML_RESULT_TAG_START:
			printf("# Opening Tag: %s\n", instance->object_name);
			break;
		case PARSE_XML_RESULT_TAG_EMPTY:
			printf("# Self-Closing Tag: %s\n", instance->object_name);
			break;
		case PARSE_XML_RESULT_TAG_END:
			printf("# Closing Tag: %s\n", instance->object_name);
			break;
		default:
			break;
		}

		for (i = 0; i < instance->attribute_count; i++)
			printf("  Attribute: %s\n", instance->attributes[i].name);
	}
#endif
}


/**
 * Process any attributes attached to the current elememt.
 *
 * \param *instance	The parser instance to use.
 * \param c		The first character of the tag sequence.
 */

static void parse_xml_read_element_attributes(struct parse_xml_block *instance, char c)
{
	int len = 0;
	long start = -1, length = 0;
	char name[PARSE_XML_MAX_NAME_LEN], quote = '\0';

	if (instance == NULL || instance->file == NULL) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Process the file until we reach the closing > or EOF. */

	while (c != instance->eof && c != '>') {
		len = 0;

		/* Look for the start of an attribute name. */

		while (c != instance->eof && c != '>' && !parse_xml_isname_start(c))
			c = fgetc(instance->file);

		if (c == instance->eof || c == '>')
			continue;

		/* We've found an attribute name, so copy it into the temp buffer. */

		name[len++] = c;

		c = fgetc(instance->file);

		do {
			if (c != instance->eof && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				name[len++] = c;

			c = fgetc(instance->file);
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
			c = fgetc(instance->file);

		/* There's a value with the attribute. */

		if (c == '=') {
			c = fgetc(instance->file);

			/* Skip any whitespace after the = sign. */

			while (c != instance->eof && isspace(c))
				c = fgetc(instance->file);

			/* What follows must be quoted in " or '. */

			if (c == '\'' || c == '"') {
				quote = c;
				start = ftell(instance->file);

				/* Step through the data, and find the length. */

				c = fgetc(instance->file);

				while (c != instance->eof && c != quote)
					c = fgetc(instance->file);

				length = ftell(instance->file) - (start + 1);

				if (c != quote) {
					instance->current_mode = PARSE_XML_RESULT_ERROR;
					msg_report(MSG_PARSE_UNTERMINATED_ATTRIBUTE, name);
					return;
				}

				c = fgetc(instance->file);
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
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	do {
		c = fgetc(instance->file);

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

#ifdef PARSE_XML_DEBUG
	printf("# Skip Comment\n");
#endif
}


/**
 * Process an entity from the file.
 * 
 * \param *instance	The parser instance to use.
 * \param c		The first character of the entity sequence.
 */

static void parse_xml_read_entity(struct parse_xml_block *instance, char c)
{
	int len = 0;

	/* Entities must start with &; we shouldn't be here otherwise! */

	if (c != '&' || instance == NULL || instance->file == NULL) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	c = fgetc(instance->file);

	/* Copy the entity name until it's terminated or there's whitespace. */

	if (c != instance->eof && parse_xml_isname_start(c)) {
		instance->object_name[len++] = c;

		c = fgetc(instance->file);

		while (c != instance->eof && parse_xml_isname(c)) {
			if (c != instance->eof && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				instance->object_name[len++] = c;

			c = fgetc(instance->file);
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

#ifdef PARSE_XML_DEBUG
	printf("# Entity: %s\n", instance->object_name);
#endif
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
		c = fgetc(instance->file);

		if (c == instance->eof || c != *text)
			break;

		text++;
	}

	/* If there wasn't a match, reset the file pointer. */

	if (*text != '\0')
		fseek(instance->file, start, SEEK_SET);

	return (*text == '\0') ? true : false;
}

/* When parsing text (& attribute values), store start ptr and length. Then use
 * generic copy routine to copy into new malloc block for the client.
 * 
 * tag and entity names can go into a fixed buffer, as we know in advance how long
 * the longest is. Look up, and return enum.
 * 
 * Return open, close, complete tag, entity, text, whitespace, eof, error codes
 * 
 * Parsting tags, we know how many entities there may be, so pre-process into
 * a fixed, static array and allow each to be called up and flagged as used. The
 * client can then trigger an "unused" scan after it has processed what it needs.
 * 
 * Flattening Whitespace appears to be handed by the encoding module?
 * 
 * We can then do a parse using recursive functions, and lose the stack?
 */