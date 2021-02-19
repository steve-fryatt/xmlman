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
	 * Buffer for the current element or entity name.
	 */
	char object_name[PARSE_XML_MAX_NAME_LEN];

	/**
	 * The number of attributes in the current element.
	 */
	int attribute_count;

	/**
	 * The attributes for the current element.
	 */
	struct parse_xml_attribute attributes[PARSE_XML_MAX_ATTRIBUTES];
};

/**
 * The top-level parser instance.
 */
static struct parse_xml_block *parse_xml_instance = NULL;

/* Static Function Prototypes. */

static void parse_xml_read_markup(struct parse_xml_block *instance, char c);
static void parse_xml_read_comment(struct parse_xml_block *instance);
static void parse_xml_read_element(struct parse_xml_block *instance, char c);
static bool parse_xml_read_element_attributes(struct parse_xml_block *instance, char c);
static void parse_xml_read_entity(struct parse_xml_block *instance, char c);
static bool parse_xml_match_ahead(struct parse_xml_block *instance, const char *text);

/* Type tests. */

#define parse_xml_isname_start(x) ((x) == ':' || (x) == '_' || ((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z'))
#define parse_xml_isname(x) (parse_xml_isname_start(x) || (x) == '-' || (x) == '.' || ((x) >= '0' && (x) <= '9'))

/**
 * Initialise the file parser for use.
 */

bool parse_xml_initialise(void)
{
	parse_xml_instance = malloc(sizeof(struct parse_xml_block));
	if (parse_xml_instance == NULL)
		return false;

	parse_xml_instance->current_mode = PARSE_XML_RESULT_ERROR;
	parse_xml_instance->attribute_count = 0;
	parse_xml_instance->file = NULL;

	return true;
}

/**
 * Open a new file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		True if successful; else False.
 */

bool parse_xml_open_file(char *filename)
{
	if (parse_xml_instance == NULL || parse_xml_instance->file != NULL) {
		msg_report(MSG_PARSE_IN_PROGRESS);
		return false;
	}

	parse_xml_instance->current_mode = PARSE_XML_RESULT_ERROR;
	parse_xml_instance->attribute_count = 0;

	parse_xml_instance->file = fopen(filename, "rb");
	if (parse_xml_instance->file == NULL)
		return false;

	return true;
}


/**
 * Close a file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		True if successful; else False.
 */

void parse_xml_close_file(void)
{
	if (parse_xml_instance != NULL && parse_xml_instance->file != NULL)
		fclose(parse_xml_instance->file);

	parse_xml_instance->file = NULL;
}


/**
 * Parse the next chunk from the current file.
 * 
 * \return		Details of the chunk.
 */

enum parse_xml_result parse_xml_read_next_chunk(void)
{
	int c;

	if (parse_xml_instance == NULL)
		return PARSE_XML_RESULT_ERROR;

	/* Start with the presumption of failure. */

	parse_xml_instance->current_mode = PARSE_XML_RESULT_ERROR;
	parse_xml_instance->attribute_count = 0;

	/* Exit on error or EOF. */

	if (parse_xml_instance->file == NULL)
		return PARSE_XML_RESULT_ERROR;

	if (feof(parse_xml_instance->file))
		return PARSE_XML_RESULT_EOF;

	/* Decide what to do based on the next character in the file. */

	c = fgetc(parse_xml_instance->file);

	switch (c) {
	case EOF:
		parse_xml_instance->current_mode = PARSE_XML_RESULT_EOF;
		break;

	case '<':
		parse_xml_read_markup(parse_xml_instance, c);
		break;

	case '&':
		parse_xml_read_entity(parse_xml_instance, c);
		break;

	default:
		printf("Text: ");
		do {
			printf("%c", c);

			c = fgetc(parse_xml_instance->file);
		} while (c != '<' && c != '&' && c != EOF);

		if (c != EOF)
			fseek(parse_xml_instance->file, -1, SEEK_CUR);

		printf("\n");

		parse_xml_instance->current_mode = PARSE_XML_RESULT_TEXT;
		break;
	}

	return parse_xml_instance->current_mode;
}


/**
 * Read the details of the current element parsed from
 * the file.
 * 
 * \return		The element token, or PARSE_ELEMENT_NONE.
 */

enum parse_element_type parse_xml_get_element(void)
{
	if (parse_xml_instance == NULL)
		return PARSE_XML_RESULT_ERROR;

	if (parse_xml_instance->current_mode != PARSE_XML_RESULT_TAG_START &&
			parse_xml_instance->current_mode != PARSE_XML_RESULT_TAG_EMPTY &&
			parse_xml_instance->current_mode != PARSE_XML_RESULT_TAG_END)
		return PARSE_ELEMENT_NONE;
	
	return parse_element_find_type(parse_xml_instance->object_name);
}


/**
 * Read the details of the current entity parsed from
 * the file.
 * 
 * \return		The entity token, or MANUAL_ENTITY_NONE.
 */

enum manual_entity_type parse_xml_get_entity(void)
{
	if (parse_xml_instance == NULL)
		return PARSE_XML_RESULT_ERROR;

	if (parse_xml_instance->current_mode != PARSE_XML_RESULT_TAG_ENTITY)
		return MANUAL_ENTITY_NONE;
	
	return manual_entity_find_type(parse_xml_instance->object_name);
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
	bool previous_was_slash = false;

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

	if (c != EOF && parse_xml_isname_start(c)) {
		instance->object_name[len++] = c;

		c = fgetc(instance->file);

		while (c != EOF && parse_xml_isname(c)) {
			if (c != EOF && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				instance->object_name[len++] = c;

			c = fgetc(instance->file);
		}
	}

	instance->object_name[PARSE_XML_MAX_NAME_LEN - 1] = '\0';

	/* The tag wasn't terminated. */

	if (c == EOF) {
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

	previous_was_slash = parse_xml_read_element_attributes(instance, c);
	if (instance->current_mode == PARSE_XML_RESULT_ERROR)
		return;

	/* The tag wasn't terminated. */

	if (c == EOF) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_TAG, instance->object_name);
		return;
	}

	/* If the tag ended with a /, it was a self-closing tag. */

	if (previous_was_slash) {
		if (instance->current_mode == PARSE_XML_RESULT_TAG_START) {
			instance->current_mode = PARSE_XML_RESULT_TAG_EMPTY;
		} else {
			instance->current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_TAG_CLOSE_CONFLICT, instance->object_name);
		}
	}
}


/**
 * Process any attributes attached to the current elememt.
 *
 * \param *instance	The parser instance to use.
 * \param c		The first character of the tag sequence.
 * \return		True if the final character before the closing > was a /.
 */

static bool parse_xml_read_element_attributes(struct parse_xml_block *instance, char c)
{
	int len = 0;
	long start = -1, length = 0;
	bool previous_was_slash = false;
	char name[PARSE_XML_MAX_NAME_LEN], quote;

	if (instance == NULL || instance->file == NULL) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	previous_was_slash = (c == '/') ? true : false;

	/* Process the file until we reach the closing > or EOF. */

	while (c != EOF && c != '>') {
		len = 0;

		/* Look for the start of an attribute name. */

		while (c != EOF && !parse_xml_isname_start(c)) {
			c = fgetc(instance->file);

			if (c != '>')
				previous_was_slash = (c == '/') ? true : false;
		}

		if (c == EOF)
			continue;

		/* We've found an attribute name, so copy it into the temp buffer. */

		name[len++] = c;

		c = fgetc(instance->file);

		do {
			if (c != EOF && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
				name[len++] = c;

			c = fgetc(instance->file);
		} while (c != EOF && parse_xml_isname(c));

		name[PARSE_XML_MAX_NAME_LEN - 1] = '\0';

		/* The name is too long. */

		if (len >= PARSE_XML_MAX_NAME_LEN) {
			instance->current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_ATTRIBUTE_TOO_LONG, name);
			return previous_was_slash;
		}

		name[len] = '\0';

		/* Skip any whitespace after the name. */

		while (c != EOF && isspace(c)) {
			c = fgetc(instance->file);

			if (c != '>')
				previous_was_slash = (c == '/') ? true : false;
		}

		/* There's a value with the attribute. */

		if (c == '=') {
			c = fgetc(instance->file);

			/* Skip any whitespace after the = sign. */

			while (c != EOF && isspace(c)) {
				c = fgetc(instance->file);

				if (c != '>')
					previous_was_slash = (c == '/') ? true : false;
			}

			/* What follows must be quoted in " or '. */

			if (c == '\'' || c == '"') {
				quote = c;
				start = ftell(instance->file);

				/* Step through the data, and find the length. */

				c = fgetc(instance->file);

				while (c != EOF && c != quote)
					c = fgetc(instance->file);

				length = ftell(instance->file) - (start + 1);

				if (c != quote) {
					instance->current_mode = PARSE_XML_RESULT_ERROR;
					msg_report(MSG_PARSE_UNTERMINATED_ATTRIBUTE, name);
					return previous_was_slash;
				}

				c = fgetc(instance->file);

				if (c != '>')
					previous_was_slash = (c == '/') ? true : false;
			}
		} else {
			start = -1;
			length = 0;
		}

		/* Check that there's room for the attribute. */

		if (instance->attribute_count > PARSE_XML_MAX_ATTRIBUTES) {
			instance->current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_TOO_MANY_ATTRIBUTES);
			return previous_was_slash;
		}

		strncpy(instance->attributes[instance->attribute_count].name, name, PARSE_XML_MAX_NAME_LEN);
		instance->attributes[instance->attribute_count].start = start;
		instance->attributes[instance->attribute_count].length = length;

		instance->attribute_count++;

		printf("Found attribute %d: %s, offset %ld, length %ld\n", instance->attribute_count, name, start, length);
	}

	return previous_was_slash;
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
	} while (c != EOF);

	if (c == EOF) {
		instance->current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_COMMENT);
		return;
	}

	instance->current_mode = PARSE_XML_RESULT_COMMENT;
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

	if (c != EOF && parse_xml_isname_start(c)) {
		instance->object_name[len++] = c;

		c = fgetc(instance->file);

		while (c != EOF && parse_xml_isname(c)) {
			if (c != EOF && parse_xml_isname(c) && len < PARSE_XML_MAX_NAME_LEN)
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

		if (c == EOF || c != *text)
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